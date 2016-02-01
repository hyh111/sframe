
include ../make/global-config.mk

TARGET_TYPE := lib

SOURCE_DIRS := ./util \
			   ./net \
               ./net/linux \
               ./serv \
               ./conf

ifeq ($(VER),debug)
	TARGET_NAME := ./lib$(MFLAGS)/libsframe-d.a
else
	TARGET_NAME := ./lib$(MFLAGS)/libsframe.a
endif

include ../make/real.make