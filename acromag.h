//
// description:
//   Acromag header file -
//   Acromag specific functions used in all realtime parts
//
// copyright:
//   (c) 2001-2002 Stefan Doehla
//
// filename and version:
//   * $Id: acromag.h,v 3.0 2002/04/23 20:56:35 martin Exp $
//
// history:
//   $Log: acromag.h,v $
//   v3.1 Martin  changed "freq" to "ts" in all fcts 
//        Ramsay  added rtce_acromag_set_freq
//        Ramsay  added rtce_acromag_get_acromag_base
//
//   Revision 3.0  2002/04/23 20:56:35  martin
//   no changes except for this header
//
//


#ifndef __ACROMAG_H
#define __ACROMAG_H


// set with timer_mode= 0/1/2  at insmod
#define SOFTWARE_TRIGGERED  0
#define HARDWARE_TRIGGERED  1
#define TURBO_TRIGGERED     2

// set with output_mode= 0/1  at insmod
#define TRANSPARENT_OUTPUT  0
#define SIMULTANEOUS_OUTPUT 1

#define ACROMAG_READ_DELAY (7) // the delay after a start_read call
                               // values need 8 us, we run with 7

/* Extra functions by Ramsay */
char *rtce_acromag_get_acromag_base(void);
void rtce_acromag_set_ts(int t);


int rtce_acromag_is_busmaster(void);
int rtce_acromag_is_fifo_checked(void);
int rtce_acromag_get_irq(void);
int rtce_acromag_get_ts(void);
int rtce_acromag_get_in_channels(void);
int rtce_acromag_get_out_channels(void);
int rtce_acromag_get_timer_mode(void);
int rtce_acromag_get_status(void);
int rtce_acromag_get_output_mode(void);

#endif
