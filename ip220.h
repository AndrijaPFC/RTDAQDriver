//
// description:
//   Header file for the Acromag IP220 Industrial I/O Pack
//   12-Bit Analog Output
//
//   ATTENTION!! HARD references to memory of IP modules
//   IP220 has to be in slot B
//
// copyright:
//   05/10/2001 Alexander Fiedel
//   13/11/2001 Stefan Doehla
//
// filename and version:
//   $Id: ip220.h,v 3.0 2002/04/23 20:58:39 martin Exp $
//
// history:
//   $Log: ip220.h,v $
//   Revision 3.0  2002/04/23 20:58:39  martin
//   *** empty log message ***
//
//   v3.0 no changes, except for this header
//


int ip220_base;

// ip220:W DAC base adress of 1st DAC channel - 16bit channels
#define IP220_DACbase             (ip220_base)

// ip220:W transparent mode select register - activate just with write
#define IP220_TRANSPARENT_MODE    (ip220_base+0x20)

// ip220:W simultaneous mode select register - activate just with write 
// - software trigger necessary
#define IP220_SIMULTANEOUS_MODE   (ip220_base+0x22)

// ip220:W output trigger signal register - activate just with write
#define IP220_OUTPUT_TRIGGER      (ip220_base+0x24)
