//
// description:
//   Header file for the Acromag Carrier Board Series
//   APC8620/8621 Industrial I/O Pack
//
// copyright:
//   05/10/2001 Alexander Fiedel
//   16/10/2001 Stefan Doehla
//
// filename and version:
//   $Id: apc862x.h,v 3.0 2002/04/23 21:01:27 martin Exp $
//
// history:
//   $Log: apc862x.h,v $
//   Revision 3.0  2002/04/23 21:01:27  martin
//   *** empty log message ***
//
//   v3.0 no changes, except for this header
//


#define PCI_VENDOR_ID_ACROMAG        0x10b5  // Vendior ID of Acromag
#define PCI_DEVICE_ID_ACROMAG_BOARD  0x1024  // Device id of APC8620
#define ACROMAG_IO_EXTENT            0x3FF   // 0x400, length of the
                                             //  i/o area in byte
#define CARD_RESET                   0x1     // reset of the pci carrier card
#define IPCARD_OFFSET                0x40    // beginn of ID space
                                             //  of 1st card
#define IPCARD_IDCODE                0x8     // manufactorer code -
                                             //  refers to IPCARD_OFFSET
#define IPCARD_IPMODEL               0xa     // IP model code - refers to
                                             // IPCARD_OFFSET
#define IP_SLOT_A_ID (0x4a)
#define IP_SLOT_B_ID (0x8a)
#define IP_SLOT_C_ID (0xCa)

#define IP_SLOT_A_IO_BASE (0x180)
#define IP_SLOT_B_IO_BASE (0x200)
#define IP_SLOT_C_IO_BASE (0x280)
