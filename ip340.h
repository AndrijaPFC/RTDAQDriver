//
// description:
//   Header file for the Acromag IP340 Industrial I/O Pack
//   12-Bit Analog Input
//
//   ATTENTION!! HARD references to memory of ip modules
//   IP340 has to be in slot A
//
// copyright:
//   05/10/2001 Alexander Fiedel
//   10/11/2001 Stefan Doehla
//
// filename and version:
//   $Id: ip340.h,v 3.0 2002/04/23 21:00:23 martin Exp $
//
// history:
//   $Log: ip340.h,v $
//   Revision 3.0  2002/04/23 21:00:23  martin
//   *** empty log message ***
//
//   v3.0 no changes, except for this header
//

/* This needs to be set to the base address of the *
 * IO space of the slot which contains the IP340   */
int ip340_base;

#define IP340_CONTROLREGISTER   (ip340_base) // contr.reg in i/o space
                                      //  16bit
#define IP340_CHANNELENABLE     (ip340_base+0x2) // channel enable 16bit --
                                      //  -» outputpins see manual!!
#define IP340_CONVTIMER1        (ip340_base+0x4) // conversion timer bits 15-0
#define IP340_CONVTIMER2        (ip340_base+0x6) // conversion timer bits 23-16
                                      //  LAST 8bit
#define IP340_HIGHBANKTIMER1    (ip340_base+0x8) // high bank timer bits 15-0
#define IP340_HIGHBANKTIMER2    (ip340_base+0xa) // high bank timer bits 23-16
                                      //  LAST 8bit
#define IP340_STARTCONVERT      (ip340_base+0x10) // start convert - 1bit
                                      //  first set modes in controllregister!
#define IP340_FIFODATA          (ip340_base+0x12) // fifo channel data port
#define IP340_FIFOPORT          (ip340_base+0x14) // fifo data tag port --
                                      //  read after FIFODATAPORT -- 8bit
#define IP340_REVVOLTAGEADDR    (ip340_base+0x16) // must be accessed before read
                                      //  reference voltage
#define IP340_REVVOLTREAD       (ip340_base+0x8) // reference voltage read data port
#define IP340_FIFOempty         (ip340_base+0x1) // FIFO status
                                      //  0 if empty,
                                      //  3 is not empty
                                      //  + FIFO max not set or reached
#define IP340_FIFOthreshold     (ip340_base+0xc) // FIFO threshold, 9bit

#define IP340_0_CH      0x0000  // no input
#define IP340_1_CH      0x0001  // input on channel  0
#define IP340_2_CH      0x0003  // input on channels 0-1
#define IP340_3_CH      0x0007  // input on channels 0-2
#define IP340_4_CH      0x000f  // input on channels 0-3
#define IP340_5_CH      0x001f  // input on channels 0-4
#define IP340_6_CH      0x003f  // input on channels 0-5
#define IP340_7_CH      0x007f  // input on channels 0-6
#define IP340_8_CH      0x00ff  // input on channels 0-7

// -------------------------------------------------------
// some frequencies for the hardware timer
// -------------------------------------------------------

#define  HT_8US          0x003f  // 125  kHz
#define  HT_10US         0x004f  // 100  kHz
#define  HT_20US         0x009f  // 50   kHz
#define  HT_40US         0x013f  // 25   kHz
#define  HT_50US         0x018f  // 20   kHz
#define  HT_60US         0x01df  // 16.6 kHz
#define  HT_100US        0x031f  // 10   kHz
#define  HT_200US        0x063f  // 5    kHz
