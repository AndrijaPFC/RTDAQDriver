//
// description:
//  contains the variables of the types defined in rtce_packets.h
//  and needed by the realtime parts
//  contains the prototypes of the functions
//  that are widespread in all realtime parts
//
// copyright:
//   (c) 2002 Martin Fischer
//   (c) 2001-2002 Stefan Doehla
//
// filename and version:
//   $Id: rtce_module.h,v 3.2beta 2002/06/03 17:04:47 martin Exp martin $
//
// history:
//   $Log: rtce_module.h,v $
//   Revision 3.1  2002/06/03 17:04:47  martin
//   deleted rtce_scalar_param_data, rtce_matrix_param_data.
//   added rtce_set_ok_to_run.
//
//   Revision 3.0  2002/04/23 21:07:31  martin
//   declaration of rtce_init_input and rtce_init_output changed:
//   returns result now: int
//
//
//   changes for V3.0: comments, formatting and debug info
//   changes mf:  rtce_init_input(void)  returns int value now.
//                rtce_init_output(void) returns int value now.
//   Revision < 3.0 by Stefan Doehla
//


#include <rtl.h>
#include <rtl_sched.h>
#include <rtl_time.h>
#include "rtce_packets.h"


#define SAMPLECLOCK gethrtime()



// #######################################################
//  TYPE DECLARATIONS
// #######################################################

// -------------------------------------------------------
// rtce_ok_to_run will be checked in each execution of the
//  control-loop within the run-thread.
//  it is the switch used by rtce_communication.c to
//  turn the controller (the run-thread) off again 
// -------------------------------------------------------
char rtce_ok_to_run;                 
void rtce_set_ok_to_run(char ok_to_run);  


// -------------------------------------------------------
// for communication of the realtime threads
//
// rtce_status:        status registers
// rtce_data_vector:   the output data vector,
//                     sent to the non_realtime tasks
// -------------------------------------------------------
//struct status_struct rtce_status;
extern struct data_struct   rtce_data_vector;


// -------------------------------------------------------
// command packets for the realtime controller
// -------------------------------------------------------
extern struct command_struct rtce_command;

// -------------------------------------------------------
// the ringbuffer struct
// -------------------------------------------------------
extern struct ringbuf_index_struct    rtce_rbuf;
extern struct data_struct            *rb;
extern struct ringbuf_index_struct   *rbi;


// -------------------------------------------------------
// rtce_input_buf
//   holds the input data (size is user-declared)
// *rtce_input_pointer
//   the pointer to the input data
//
// the pointer can only be read INPUT_BUF_SIZE times
// or garbage is received 
// -------------------------------------------------------


// 5/13/2004 - Jeff
// For some reason in Kernel 2.4, the variables defined
// in kernel space - even though they have the same name
// are referenced seperately for each file that declared them.
// For this reason, any variables referenced from multiple
// modules are going to be declared as external, and initialized
// as global variables in a rtce_common module, that will be
// loaded before the acromag, rtce (interface), and rtce control
// driver.

extern float  rtce_input_buf[INPUT_BUF_SIZE];
extern float *rtce_input_pointer;

// -------------------------------------------------------
// rtce_output_buf
//   holds the output data (size is user-declared)
// *rtce_output_pointer
//   the pointer to the output data
//
// the pointer can only be sent OUTPUT_BUF_SIZE times
// or garbage is sent - attention - danger of crash
// -------------------------------------------------------
extern float  rtce_output_buf[OUTPUT_BUF_SIZE];
extern float *rtce_output_pointer;

// -------------------------------------------------------
// rtce_route
//   holds informations which channels are displayed
// -------------------------------------------------------
extern float *rtce_route_table[ROUTE_TABLE_SIZE];
extern int   *rtce_route;

// -------------------------------------------------------
// we stop the time of the threads
// rtce_start_time is the 0-point when START is pressed
// hr_time_t is a hard realtime timer type
// -------------------------------------------------------
hrtime_t rtce_start_time;            // holds the system execution start time


// #######################################################
//  FUNCTION DECLARATIONS
// #######################################################

// -------------------------------------------------------
//   master- and run-thread
// -------------------------------------------------------
void *rtce_master_thread(void *arg);  // the main thread
void *rtce_run_thread(void *arg);     // the loop for I/O, control

// -------------------------------------------------------
// the input/output functions of the driver
//
// anyone should feel free to adapt this part to the
// features of the I/O devices
//
// INIT, RUN, STOP, RESET
// -------------------------------------------------------
int  rtce_init_input(void);      // init INPUT device
void rtce_run_input(void);       // run INPUT
void rtce_stop_input(void);      // stop INPUT
void rtce_reset_input(void);     // reset the INPUT device
void rtce_start_input(void);     // send START convert command

int  rtce_init_output(void);     // init OUTPUT device
void rtce_run_output(void);      // run OUTPUT
void rtce_stop_output(void);     // stop OUTPUT device
void rtce_reset_output(void);    // reset the OUTPUT device
                                 // if D/A => set to 0V

// -------------------------------------------------------
// the prototypes of the control functions
//
// INIT, RUN, STOP, RESET
// -------------------------------------------------------
void rtce_init_control(void);    // init control algorithm
void rtce_run_control(void);     // run control algorithm
void rtce_stop_control(void);    // stop control algorithm
void rtce_reset_control(void);   // cleanup after control algorithm

// -------------------------------------------------------
// the prototypes of the data exchange functions
// -------------------------------------------------------
void rtce_assign_data(void);   // user controller !!!
void rtce_send_data(void);
void rtce_receive_commands(void);


// -------------------------------------------------------
// the prototypes of the fifo handler
// -------------------------------------------------------
int rtce_fifo_handler(unsigned int fifo);

// -------------------------------------------------------
// the prototype of the interrupt handler
// -------------------------------------------------------
unsigned int rtce_irq_handler(unsigned int pci_irq,
                              struct pt_regs *regs);

// #######################################################
//   GLOBAL VARIABLES FOR REALTIME
// #######################################################

extern pthread_t      rtce_thread[2];
extern unsigned char  rtce_pci_irq;

extern int            rtce_ts;  // sample-period in us of the AD-card

extern hrtime_t       rtce_t_sample;
extern hrtime_t       rtce_t_start;
#ifdef RT_TASK_BENCHMARK
	extern hrtime_t     rtce_t_presleep;
	extern hrtime_t     rtce_t_preread;
	extern hrtime_t     rtce_T_max;
	extern hrtime_t     rtce_T_min;
#endif
