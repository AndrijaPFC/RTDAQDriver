/*
 * description:
 *   Realtime driver for Acromag I/O products
 *   This driver is published under the GPL
 *
 * copyright:
 *   (c) 2002 Martin Fischer
 *   (c) 2001-2002 Stefan Doehla
 *   (c) 2004 Jeff Bloemink
 *
 * filename and version:
 *   $Id: acromag.c,v 3.0 2004/05/17 Jeff Bloemink
 *
 * history:
 *   Revision 4.0 Jeff
 *   -Changed parameters intended to be global to 
 *    external vars, and loaded in commons module
 *   $Log: acromag.c,v $
 *   Revision 3.0  2002/04/23 18:55:27  martin
 *   ins_mod will now fail, if errors occur during init_module.
 *   added some debug messages.
 *
 *
 *   changes for V3.0: comments, formatting and debug info
 *   rtce_init_input  returns now an error code
 *   rtce_init_output returns now an error code
 *   init_module      checks now for error codes and returns an error code
 *   Revision < 3.0 by Stefan Doehla
 *
 */



#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/modversions.h>
#include <ctype.h>
#include <rtl_debug.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "apc862x.h"
#include "ip340.h"
#include "ip220.h"
#include "ip1k.h"
#include "rtce_module.h"
#include "acromag.h"


#define __VERBOSE

#ifdef __VERBOSE
#  define PRINTF(fmt, args...)  rtl_printf("ACROMAG: " fmt , ## args)
#else
#  define PRINTF(fmt, args...)
#endif

 #define SLOW_CPU

MODULE_AUTHOR("Stefan Doehla <stefan@power.ele.utoronto.ca>");
MODULE_DESCRIPTION ("Driver for Acromag APC8620, IP340, IP220");
MODULE_LICENSE("GPL");

#define acroinit_successfull  (1)

// all relevant changeable information of the acromag board
static int                busmaster = 0;
static int                irq = 0;
static char *             acromag_base = NULL;  //most important ptr
static int                output_mode = SIMULTANEOUS_OUTPUT;
static int                timer_mode = 0;
static unsigned int       ts = 100;
static unsigned int       in_channels = 4;
static unsigned int       out_channels = 4;
static int                check_fifo = 0;
static int                status = 0;

MODULE_PARM (busmaster, "i");
MODULE_PARM (irq, "i");
MODULE_PARM (ts, "i");
MODULE_PARM (output_mode, "i");
MODULE_PARM (timer_mode, "i");
MODULE_PARM (in_channels, "i");
MODULE_PARM (out_channels, "i");
MODULE_PARM (check_fifo, "i");

/* flags incase not all the boards are present */
static unsigned char valid_ip340, valid_ip220, valid_ip1k;

/* this will include an array called IP1K100, which will contain the FPGA *
 * program. It must come from the hfilegenerator program                  */
#include "fpga_prog.h"


/* Martin wants a pretty picture, Martin gets a pretty picture....*/
/* 36 chars per line (Including \n) */
unsigned char acromag_image[] = 
"~|---------------------------------|";

#define IMAGE_LINE_LENGTH (37)

#define MODULEA_NAME_OFFSET (121)
#define MODULEB_NAME_OFFSET (343)
#define MODULEC_NAME_OFFSET (142) /* This is VERTICLE, needs offset of 36 between chars */

static char ip1k_name_string[] = "IP1K100";
static char ip220_name_string[] = "IP220";
static char ip340_name_string[] = "IP340";


// --------------------------------------------------------
// acromag_lookup_pci
//
// find the pci device and map the memory
// this function is written for kernel 2.4 (no pci_bios!)
// the advantage: should also work on PPC and weired boards
//
// it's possible to set the Acromag board as the busmaster
// but not necessary and therefore not used
// --------------------------------------------------------
static int
acromag_pci_lookup (void)
{
  struct pci_dev *dev_acromag = NULL;
  unsigned long pci_ioaddr = 0;
  u32 acromag_ioaddr = 0;

  if (pci_present())
  {
    if((dev_acromag = pci_find_device(PCI_VENDOR_ID_ACROMAG,
       PCI_DEVICE_ID_ACROMAG_BOARD, dev_acromag)) != NULL)

    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
      pci_read_config_byte(dev_acromag, PCI_INTERRUPT_LINE,
                           &rtce_pci_irq);
      pci_read_config_dword(dev_acromag, PCI_BASE_ADDRESS_2,
                           &acromag_ioaddr);
      PRINTF("PCIBIOS IRQ of Carrier:    [%i]\n", rtce_pci_irq);
#else
      rtce_pci_irq = dev_acromag->irq;
      PRINTF("PCI IRQ of Carrier:        [%i]\n", rtce_pci_irq);
      PRINTF("WARNING: This driver does not support shared IRQs,\n be sure that the acromag card has an unique IRQ\n");
      acromag_ioaddr = (u32)pci_resource_start(dev_acromag, 2);
#endif

      if(busmaster)
        pci_set_master(dev_acromag);

      pci_ioaddr = PCI_BASE_ADDRESS_MEM_MASK & (unsigned long)acromag_ioaddr;
      acromag_base = ioremap_nocache(pci_ioaddr, ACROMAG_IO_EXTENT);
      PRINTF("Carrier board found        [*]\n");
      return acroinit_successfull;
    }
  }
  PRINTF("Carrier board not found    [_]\n");
  return !acroinit_successfull;
}

// --------------------------------------------------------
// the apc8620_init routine
//
// this function is called from the rtce_init_input routine
//
// do here:
//   reset the board
//   turn interrupts on ...
// --------------------------------------------------------
void apc862x_init(void){
  PRINTF("Carrier Init Sequence      \n");
  writew(0x0080, acromag_base);             // RESET
  writew(0x0004, acromag_base);             // interrupts ON
  return;
}


/* general h/w check that will detect which cards are in which slots *
 * this will only find one ip220 and/or one ip340 and/or one ip1k    *
 * if several of the same IP module are installed only the last one  *
 * will be returned in the relevant base pointer                     */
void hw_check(void) {

  char id;

  int i, j;
  char *ptr, *p;

  valid_ip220 = 0;
  valid_ip340 = 0;
  valid_ip1k = 0;


  /* Yes, this is an obscene number of switch statements, sorry...*/

  PRINTF("Searching slots A, B and C......\n");

  id = readb(acromag_base+IP_SLOT_A_ID);
  switch(id) {
  case 0x40:
  case 0x41:
    ip1k_base = IP_SLOT_A_IO_BASE;
    ip1k_model = IP_SLOT_A_ID;
    PRINTF("Found IP1k (id %x) in slot A\n", id);
    valid_ip1k = 1;
    break;
  case 0x22:
  case 0x23:
    ip220_base = IP_SLOT_A_IO_BASE;
    PRINTF("Found IP220 (id %x) in slot A\n", id);
    valid_ip220 = 1;
    break;
  case 0x28:
  case 0x29:
    ip340_base = IP_SLOT_A_IO_BASE;
    PRINTF("Found IP340 (id %x) in slot A\n", id);
    valid_ip340 = 1;
    break;
  default:
    PRINTF("Found nothing/unknown IP module in slot A (id %x)\n", id);
  }

  id = readb(acromag_base+IP_SLOT_B_ID);
  switch(id) {
  case 0x40:
  case 0x41:
    ip1k_base = IP_SLOT_B_IO_BASE;
    ip1k_model = IP_SLOT_B_ID;
    PRINTF("Found IP1k (id %x) in slot B\n", id);
    valid_ip1k = 2;
    break;
  case 0x22:
  case 0x23:
    ip220_base = IP_SLOT_B_IO_BASE;
    PRINTF("Found IP220 (id %x) in slot B\n", id);
    valid_ip220 = 2;
    break;
  case 0x28:
  case 0x29:
    ip340_base = IP_SLOT_B_IO_BASE;
    PRINTF("Found IP340 (id %x) in slot B\n", id);
    valid_ip340 = 2;
    break;
  default:
    PRINTF("Found nothing/unknown IP module in slot B (id %x)\n", id);
  }

  id = readb(acromag_base+IP_SLOT_C_ID);
  switch(id) {
  case 0x40:
  case 0x41:
    ip1k_base = IP_SLOT_C_IO_BASE;
    ip1k_model = IP_SLOT_C_ID;
    PRINTF("Found IP1k (id %x) in slot C\n", id);
    valid_ip1k = 3;
    break;
  case 0x22:
  case 0x23:
    ip220_base = IP_SLOT_C_IO_BASE;
    PRINTF("Found IP220 (id %x) in slot C\n", id);
    valid_ip220 = 3;
    break;
  case 0x28:
  case 0x29:
    ip340_base = IP_SLOT_C_IO_BASE;
    PRINTF("Found IP340 (id %x) in slot C\n", id);
    valid_ip340 = 3;
    break;
  default:
    PRINTF("Found nothing/unknown IP module in slot C (id %x)\n", id);
  }

  /* create the ASCII art image of the board */
  switch(valid_ip220) {
  case 0:
    break;
  case 1:
    ptr = acromag_image+MODULEA_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip220_name_string) - 1 ); i++) *ptr++ = ip220_name_string[i];
    break;
  case 2:
    ptr = acromag_image+MODULEB_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip220_name_string) - 1 ); i++) *ptr++ = ip220_name_string[i];
    break;
  case 3:
    ptr = acromag_image+MODULEC_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip220_name_string) - 1 ); i++) {
      *ptr = ip220_name_string[i];
      ptr += IMAGE_LINE_LENGTH; /* Slot C is a verticle image so the ptr must be offset by the line length */
    }
    break;
  default:
    PRINTF("IP220 module detected in unexpected slot\nThis driver can only use slots A - C\n");
    ptr = acromag_image+MODULEA_NAME_OFFSET;
  }



  switch(valid_ip340) {
  case 0:
    break;
  case 1:
    ptr = acromag_image+MODULEA_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip340_name_string) - 1 ); i++) *ptr++ = ip340_name_string[i];
    break;
  case 2:
    ptr = acromag_image+MODULEB_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip340_name_string) - 1 ); i++) *ptr++ = ip340_name_string[i];
    break;
  case 3:
    ptr = acromag_image+MODULEC_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip340_name_string) - 1 ); i++) {
      *ptr = ip340_name_string[i];
      ptr += IMAGE_LINE_LENGTH; /* Slot C is a verticle image so the ptr must be offset by the line length */
    }
    break;
  default:
    PRINTF("IP220 module detected in unexpected slot\nThis driver can only use slots A - C\n");
    ptr = acromag_image+MODULEA_NAME_OFFSET;
  }


  switch(valid_ip1k) {
  case 0:
    break;
  case 1:
    ptr = acromag_image+MODULEA_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip1k_name_string) - 1 ); i++) *ptr++ = ip1k_name_string[i];
    break;
  case 2:
    ptr = acromag_image+MODULEB_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip1k_name_string) - 1 ); i++) *ptr++ = ip1k_name_string[i];
    break;
  case 3:
    ptr = acromag_image+MODULEC_NAME_OFFSET;
    for(i = 0; i < (sizeof(ip1k_name_string) - 1 ); i++) {
      *ptr = ip1k_name_string[i];
      ptr += IMAGE_LINE_LENGTH; /* Slot C is a verticle image so the ptr must be offset by the line length */
    }
    break;
  default:
    PRINTF("IP220 module detected in unexpected slot\nThis driver can only use slots A - C\n");
    ptr = acromag_image+MODULEA_NAME_OFFSET;
  }

  /* This is necessary since the printk buffer is only 1024 bytes */
  ptr = vmalloc(512);

  for(i = 0; i< sizeof(acromag_image); i += 512) {
    p = ptr;
    for(j = 0; (j < 512)&&(i+j < (sizeof(acromag_image))); j++) *p++ = acromag_image[i+j];
    printk("%s", ptr);
  }

  vfree(ptr);

}

/* functions to access the FPGA registers (This assumes that the FPGA*
 * has been programmed using the base specification from acromag as  *
 * detailed in the IP1K100 manual, fig. 3.3                          */
void acromag_write_reg(int addr, int data) {
  /* force 16 bit */
  addr &= 0xffff;
  data &= 0xffff;

  writew(addr, acromag_base+IP1K_MEM_ADDR);
  writew(data, acromag_base+IP1K_MEM_DATA);
  

}

int acromag_read_reg(int addr) {

  int i;
  int x;

  /*force 16 bit*/
  addr &= 0xffff;

  writew(addr, acromag_base+IP1K_MEM_ADDR);
  /* see what comes back....... */
  for(i = 0; i < 5; i++) {
    x = readw(acromag_base+IP1K_MEM_DATA);
    PRINTF("Read %x\n", x);
  }

  return x;
}

// HELPER FUNCTIONS ....
//
// seperated from the normal functions for better maintenance

// -------------------------------------------------------
// calculate the hardware timer settings
//
// input is the sample rate in us
// output is short
// -------------------------------------------------------

static int
acromag_calc_hw_timer(int us)
{
  int tv;
  tv = us * 8 - 1;
  return tv;
}


// -------------------------------------------------------
// home made implementation of the pow() function
//
// x^y
//
// works only for integers
// okay in this case because the init_module() function
// doesn't support floating point operations for libmath
// -------------------------------------------------------
static int
pow(int x, int y)
{
  int ret;
  int i;
  if(y<0)                            // we don't support frac
    return (-1);
  else if(y==0)                      // x^0 = 1
    return 1;
  else if(y==1)                      // x^1 = x
    return x;
  else
  {
    ret = x;
    for(i=1; i<y; i++)
    {
      ret = ret * x;
    }
  }
  return ret;
}

// HELPER __inline__ functions
//
// are used in every sample run

// -------------------------------------------------------
// convert hex numbers of ADC to float
// factor works only for 12 bit Acromag modules
// -------------------------------------------------------
static float __inline__
adc_hex2dec(signed short in)
{
  float out;
  in>>=4;                  // cut the last 4 bits (always 0) for 12bit
  out=in*0.00488;          // the result should be signed float
  return out;
}


// -------------------------------------------------------
// function to convert double to hex for d/a
//
// factors are only valid for IP220 module
// -------------------------------------------------------
static short __inline__
double2short(double in)
{
  short out;
  out=(int) (in * 204.91803);
  out*=0x10;
  out-=0x8000;
  return out;
}


/* init the ip1k FPGA module */
void rtce_init_ip1k(void) {

  int i, j;
  char id = 0;

  PRINTF("Begining initialisation of IP1K100.....\n");
  PRINTF("Module id now %x\n", readb(acromag_base+IP1K_IPMODEL_REG));
  PRINTF("Control Register now %x\n", readb(acromag_base+IP1K_CONTROL));
  j = 0;

 blob:
  PRINTF("Attempt %d:\n", j);
  PRINTF("Soft resetting...\n", id);
  writeb(0x01, acromag_base);
  writew(0x80, acromag_base+IP1K_CONTROL); /* write to the 15th bit of the control register */
  id = readb(acromag_base+IP1K_IPMODEL_REG);    
  while((id != 0x40)&&(id != 0x41)) id = readb(acromag_base+IP1K_IPMODEL_REG);
  for(i = 0; i < 10000; i++) {
    id = readb(acromag_base+IP1K_IPMODEL_REG);
    if(id == 0x40) {
      PRINTF("Success! (IP Module code: %x)\n", id);
      goto ba;
    }
  }
  PRINTF("Failed\n");

  /* I appreciate that on the face of it doing the next *
   * part having failed the previous part doesnt make   *
   * sense but it seems to kick the thing into action   */

 ba:
  PRINTF("Module id now %x\n", readb(acromag_base+IP1K_IPMODEL_REG));
  PRINTF("Control Register now %x\n", readb(acromag_base+IP1K_CONTROL));

  PRINTF("Entering config mode....(Writing 1 to control register)\n");
  writeb(0x01, acromag_base+IP1K_CONTROL);
  /* usleep() seems to cause a segmentation fault so Im just going *
   * to do a busy waiting loop since it should only take 5us       */
  //while(!(readb(acromag_base+IP1K_CONTROL) & 0x01)) __asm__ __volatile__ ("\tnop");
  /* Since the required state is never occuring currently I have limited the wait time *
   * with this for loop. Otherwise it would obviously wait forever. This may or may not*
   * be lasting 5us but however high it is set it never terminates.                    */
  for(i = 0; i < 1000; i++) {
    id = readb(acromag_base+IP1K_CONTROL);
    if(id & 0x01) {
      PRINTF("Success! (Control register: %x)\n", id);
      goto bah;
    }
  }

  /* ok, this is disgusting but.......             *
   * I found that running this three or four times *
   * made it work sooner or later so I have just   *
   * automated the trial and error                 */

  PRINTF("Failed\n");
  if(j++ < 9) goto blob;
  else goto end;

  bah:

  PRINTF("Module id now %x\n", readb(acromag_base+IP1K_IPMODEL_REG));
  PRINTF("Control Register now %x\n", readb(acromag_base+IP1K_CONTROL));

  PRINTF("Uploading FPGA program......\n");
  
  for(i = 0; i < sizeof(IP1K100); i++) {
    writeb(IP1K100[i],  acromag_base+IP1K_CONFIG_DATA);
  }

  PRINTF("Module id now %x\n", readb(acromag_base+IP1K_IPMODEL_REG));
  PRINTF("Control Register now %x\n", readb(acromag_base+IP1K_CONTROL));

 end:
	return;
}



// --------------------------------------------------------
// the init routine
// returns acroinit_successfull if successfull
// 
// do here:
//   look for the card on PCI, ISA, ...
//   map the memory
//   reset the board

//void
//rtce_init_input(void)
int
rtce_init_input(void)
{
  short hw_tv = 0;
  short in_thr = 0;
  short in_ch = 0;
  int i;
  
  PRINTF("rtce_init_input - begin\n");
  PRINTF("(A/D-Hardware)\n");
   
  //if (ip340_hw_check() != 0) return !acroinit_successfull;
    
  apc862x_init();                                      // init carrier
  hw_tv = acromag_calc_hw_timer(ts);                 // calculate timerval
  PRINTF("Sample-Period: %ius (variable 'ts').\n",ts);
  PRINTF("To 'ts' corresponding Acromag-Value: %x \n", hw_tv);
  
  if(in_channels > 8) in_thr = 7; 
  else in_thr = in_channels - 1;                            // set threshold to
                                                       //  inputs - 1
  
  for (i=0; i<in_channels; i++)                        // turn on channels
    {                                                    // starting from #0
      in_ch = in_ch + pow(2, i);
    }
  PRINTF("Channel Setting for %i channels:  %x \n", 
	 in_channels, in_ch);
  
  writew(0xff00, acromag_base+IP340_CONTROLREGISTER);  // reset
  apc862x_init();
  writew(0xff00, acromag_base+IP340_CONTROLREGISTER);  // reset
  switch(timer_mode)
    {
    case SOFTWARE_TRIGGERED:
      writew(0x0021, acromag_base+IP340_CONTROLREGISTER);  // single conversion 
      //  without triggers
      writew(in_ch , acromag_base+IP340_CHANNELENABLE);    // for outputpins 
      //  -> see manual!!
      writew(0x003f, acromag_base+IP340_CONVTIMER1);       // min is 63dec - 3f
      PRINTF("Software triggered timer \n");
      break;
      
    case HARDWARE_TRIGGERED:
      writew(0x0062, acromag_base+IP340_CONTROLREGISTER);  // continous conversion 
      //  without triggers
      //  but irq-generation
      writew(in_ch , acromag_base+IP340_CHANNELENABLE);    // for outputpins 
      //  -> see manual!!
      writew(in_thr, acromag_base+IP340_FIFOthreshold);    // hard locked to 
                                                           //  8 values/interrupt
      writew(hw_tv , acromag_base+IP340_CONVTIMER1);       // min is 63dec - 3f 
      //  1df is 60us
      //  18f is 50us
      writew(0x0000, acromag_base+IP340_CONVTIMER2);       // 8.000.000dec should 
      //  be about 1s


      /* This should cause channels 8-15 to be converted as soon as 0-8 are finished
	 Hopefully...   -Ramsay */
      if(in_channels > 8) {
	PRINTF("SETTING HIGHBANK TIMER TO 0x3f........\n");
	writew(0x003f, acromag_base+IP340_HIGHBANKTIMER1);
	writeb(0x00, acromag_base+IP340_HIGHBANKTIMER2);
      }
      // this is disabled if the start_input function is used
      // writew(0xffff, acromag_base+IP340_STARTCONVERT);    // start the conversion
      // values need 
      //  to be available
      PRINTF("Hardware triggered timer \n");
      break;
      
    case TURBO_TRIGGERED:
      writew(0x0022, acromag_base+IP340_CONTROLREGISTER);  // continous conversion·
      //  with triggers
      //  no interrupt!!!!
      writew(in_ch , acromag_base+IP340_CHANNELENABLE);    // for outputpins·
      //  -> see manual!!
      writew(in_thr, acromag_base+IP340_FIFOthreshold);    // hard locked to·
      //  8 values/interrupt
      writew(hw_tv , acromag_base+IP340_CONVTIMER1);       // min is 63dec - 3f·
      //  1df is 60us
      //  18f is 50us
      writew(0x0000, acromag_base+IP340_CONVTIMER2);       // 8.000.000dec should·
      //  be about 1s
      // this is disabled if the start_input function is used
      // writew(0xffff, acromag_base+IP340_STARTCONVERT);    // start the conversion
      // values need·
      //  to be available
      PRINTF("Timer turbo triggered :-) \n");
      break;
    default:
      PRINTF("OOPS, no timer mode set");
      PRINTF("rtce_init_input - end      [_]\n");
      return !acroinit_successfull;
    }
  PRINTF("rtce_init_input - end      [*]\n");
  return acroinit_successfull;

}
 

// --------------------------------------------------------
// the init output routine
// returns acroinit_successfull if successfull
//
// do here
//   find card (if not done at init input)
//   set ouput to 0
//   map memory
// --------------------------------------------------------
int
rtce_init_output(void)
{
  if(valid_ip220) {
    PRINTF("rtce_init_output - begin\n");
    PRINTF("(D/A-Hardware)\n");
    
    //if (ip220_hw_check() != 0) return !acroinit_successfull;
    
    switch (output_mode)
      {
      case SIMULTANEOUS_OUTPUT:
	writew(0xffff, acromag_base+IP220_SIMULTANEOUS_MODE);
	PRINTF("output_mode:IP220_SIMULTANEOUS_MODE\n");
	break;
      default:
	writew(0xffff, acromag_base+IP220_TRANSPARENT_MODE);
	PRINTF("output_mode:IP220_TRANSPARENT_MODE\n");
	break;
      }
    rtce_reset_output();
    
    PRINTF("rtce_init_output - end     [*]\n");
  }
    return acroinit_successfull;

}


// --------------------------------------------------------
// start routine for hardware trigger
//
// a additional check for the fifo is done here
//
// if the fifo's not empty then read a fixed amount of data
// the standard value is 32 values ( 1,7 us per read!)
//
// interrupts are only possible if the conversion is started
// this is done by writing to the STARTCONVERT register
// --------------------------------------------------------
void
rtce_start_input(void)
{
  int i, j;

  //j = in_channels * 4;
  j = 512;
  if (readb(acromag_base+IP340_FIFOempty)!=0)          // check fifo_empty
                                                       //  flag
    {
      for (i=0; i<j; i++)                                // if fifo !empty
	readw(acromag_base+IP340_FIFODATA);              //  read from memory
      PRINTF("IP340 FIFO empty ???       [_]\n");
      PRINTF("Reading %i vals to Nirvana [*]\n", i);
    }
  else PRINTF("IP340 FIFO empty ???       [*]\n");


  writew(0xffff, acromag_base+IP340_STARTCONVERT);     // start the conversion
                                                       // values need 8us to 
 

  return;
}


// --------------------------------------------------------
// check fifo
//
// checks the fifo after every measurement if it's empty
// not recommended but a good way to check if we read
// old values
// --------------------------------------------------------
static void
rtce_check_fifo(void)
{
  int i;
  if (readb(acromag_base+IP340_FIFOempty)!=0)          // check fifo_empty
                                                       //  flag
  {
    for (i=0; i<in_channels; i++)                               // if fifo !empty
      readw(acromag_base+IP340_FIFODATA);              //  read from memory
    rtl_printf("Reading %i vals to Nirvana [*]\n", i); // output to kernel log
  }
  return;
}


// RESET ROUTINES
//
// (no unloading - only pause)

// --------------------------------------------------------
// the stop output routine
//
// do here
//   stop sending data
//   reset the output (to 0V for a D/A card)
// --------------------------------------------------------
void
rtce_stop_output(void)
{
  rtce_reset_output();
  return;
}


// --------------------------------------------------------
// the reset output routine
//
// do here
//   reset the output (to 0V for a D/A card)
// --------------------------------------------------------
void
rtce_reset_output(void)
{
  int i;
  if(valid_ip220) {
    PRINTF("rtce_reset_output\n");
    for (i=0; i<out_channels; i++)
      {
	writew(0x8000, acromag_base+IP220_DACbase+0x2*i);
      }
    if(output_mode==SIMULTANEOUS_OUTPUT)
      writew(0xffff, acromag_base+IP220_OUTPUT_TRIGGER);
  }
  return;
}


// --------------------------------------------------------
// the reset input routine
//
// TODO:
//   make a calibration
// --------------------------------------------------------
void
rtce_reset_input(void)
{
  PRINTF("rtce_reset_input\n");
  writew(0xff00, acromag_base+IP340_CONTROLREGISTER);  // Reset!
  return;
}


// UNLOADING FUNCTIONS
//
// unmap pointers ...

// --------------------------------------------------------
// the stop input routine
//
// do here
//   stop reading
//   unmap memory
// --------------------------------------------------------
void
rtce_stop_input(void)
{
  PRINTF("rtce_stop_input\n");
  writew(0xff00, acromag_base+IP340_CONTROLREGISTER);  // Reset!
  iounmap(acromag_base);                               // Unmap Memory
  
  return;
}

// RUN FUNCTIONS ....
//
// maybe the most important part
// needs to be optimized

// --------------------------------------------------------
// the reading data routine
//
// do here
//   read data from the inputs
//   assign the data to the rtce_input_buf e.g. by a pointer
// --------------------------------------------------------
// Changed 4/12/2004 By Jeff to pass pointer, was not seeing it
// ie trying to write to mem location 0x0, no good. This is slower
// so if there is a way around it it should be looked into.
// --------------------------------------------------------
void
rtce_run_input(void)
{

  int i = 0;
  unsigned short hexdata;
  

//  signed short temp;

  switch (timer_mode)
    {
    case SOFTWARE_TRIGGERED:
      writew(0xffff, acromag_base+IP340_STARTCONVERT); // start the conversion
      // values need 8us
      usleep(ACROMAG_READ_DELAY);                      // to be available
      break;

    case HARDWARE_TRIGGERED:
      break;
    case TURBO_TRIGGERED:
      rtce_t_start = gethrtime();
      while (readb(acromag_base+IP340_FIFOempty)!=3)   // fifo not empty
	{                                                // threshold reached
	  // clock_nanosleep (CLOCK_REALTIME, 0, hrt2ts(100), NULL);
	  // wait 100ns
	  udelay(1);                                     // busy wait
#ifdef SLOW_CPU  // needs a timeout ...
	  if(gethrtime()-rtce_t_start > 20000) break;
#endif
	}
    }

  rtce_t_sample = gethrtime();

  for (i=0; i<in_channels; i++)
    {
      hexdata=readw(acromag_base+IP340_FIFODATA);



// Jeff: Was used for debugging - to see if data changing
// with input. Seems to work for hardware triggering mode...
//	if(i==0) {
//          
//		temp = (signed int)hexdata;
//		temp>>=4;
//		PRINTF("%#x, %#x, %#x, %+4d \n", acromag_base, IP340_FIFODATA, acromag_base + IP340_FIFODATA, temp);
//	}



      *(rtce_input_pointer+i) = adc_hex2dec((signed int)hexdata);
    }
 
  if(check_fifo) rtce_check_fifo();
}


// --------------------------------------------------------
// the writing data routine
//
// do here
//   read data from the rtce_output_buf e.g. by a pointer
//   write it to memory/ports ...
// --------------------------------------------------------

void
rtce_run_output(void)
{
  int i;
  if(valid_ip220) {
    for (i=0; i<out_channels; i++)
      writew(double2short(*(rtce_output_pointer+i)),
	     acromag_base+IP220_DACbase+0x2*i);
    if(output_mode==SIMULTANEOUS_OUTPUT)
      writew(0xffff, acromag_base+IP220_OUTPUT_TRIGGER);
  }
  return;
}

// INFO functions
//
// return driver parameter

int
rtce_acromag_is_busmaster(void)
{
  if(busmaster == 0) return FALSE;
  else return TRUE;
}

int
rtce_acromag_get_irq(void)
{
  return irq;
}

int
rtce_acromag_is_fifo_checked(void)
{
  if(check_fifo == 0) return FALSE;
  else return 1;
}

int
rtce_acromag_get_ts(void)
{
  return ts;
}

void rtce_acromag_set_ts(int t) {
  int ts1=0;
  short ts2=0;
  ts = t;
  ts1=acromag_calc_hw_timer(ts);
  if (ts1>65535){
    ts2=ts1>>16;
    ts1=ts1-65535;
  };
  writew(ts1, acromag_base+IP340_CONVTIMER1);       // min is 63dec - 3f·
  writeb(ts2, acromag_base+IP340_CONVTIMER2);
}

int
rtce_acromag_get_in_channels(void)
{
  return in_channels;
}

int
rtce_acromag_get_out_channels(void)
{
  return out_channels;
}

int
rtce_acromag_get_timer_mode(void)
{
  return timer_mode;
}

int
rtce_acromag_get_status(void)
{
  return status;
}

char
*rtce_acromag_get_acromag_base() 
{
  return acromag_base;
}

int
rtce_acromag_get_output_mode(void)
{
  return output_mode;
}


// MODULE FUNCTIONS
//
//

void
cleanup_module(void)
{
  PRINTF("cleanup_module - begin\n");
  if(valid_ip220) rtce_reset_output(); // set to 0
  if(valid_ip340) rtce_stop_input();   // unmap ...
}

#ifdef MODULE
int
init_module(void)
{
  PRINTF("init_module - begin\n");

  PRINTF("  $Id: acromag.c,v 3.0 2002/04/23 18:55:27 martin Exp $\n");
  if (acromag_pci_lookup() != acroinit_successfull) goto InitFailed;

  hw_check();

  /* init_ip1k() MUST be done first since it will cause *
   * a carrier board reset, wiping any programming of   *
   * the other modules                                  */
  //  valid_ip1k = !ip1k_hw_check();
  if(!valid_ip1k) {
    PRINTF("No valid FPGA device found\n");
  } 
  else 
    rtce_init_ip1k();

  //  valid_ip340 = !ip340_hw_check();
  if(!valid_ip340) {
    PRINTF("No valid input device found\n Cannot contiune without ip340 module (%d)\n", valid_ip340);
    goto InitFailed;
  } else rtce_init_input();

  //  valid_ip220 = !ip220_hw_check();
  if(!valid_ip220) {
    PRINTF("No valid output device found\n");
  } 
  else 
    rtce_init_output();

  if(!(valid_ip340 || valid_ip220 || valid_ip1k)) {
    PRINTF("No valid modules found, exiting...\n");
    goto InitFailed;
  } 

  PRINTF("init_module - end          [*]\n\n");
  return 0;
  
InitFailed:
  PRINTF("init_module - end          [_]\n\n");
  cleanup_module();
  return 1;

}

#endif







