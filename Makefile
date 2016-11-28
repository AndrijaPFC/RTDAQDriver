#
# description:
#   Makefile for the realtime part of RTCE
#
# copyright:
#   (c) 2002 Stefan Doehla
#   (c) 2002 Martin Fischer
#

# take care to include the rtl.mk of the current RTLinux Installation,
# that is copy /usr/src/rtlinux/rtl.mk to your RTCE-directory, 
# where you found this Makefile 
include ../../include/rtl.mk

CFLAGS := $(CFLAGS) -I../../include 

# the relative path to the kernel modules:
RTCE_MODULE_DIR= ../../modules

default: acromag
	
acromag:acromag.o
	cp acromag.o            $(RTCE_MODULE_DIR)/rtce_driver.o
	@echo " *  now the acromag-driver will be used for A/D and D/A"

acromag_dummy:acromag_dummy.o
	cp acromag_dummy.o      $(RTCE_MODULE_DIR)/rtce_driver.o
	@echo " *  now the acromag-dummy-driver will be used for A/D and D/A"
	@echo " *  only dummy-driver, no hardware access!"

clean:
	rm *.o
