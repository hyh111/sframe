
TOP_DIR = ../..

include $(TOP_DIR)/make/global-config.mk

SOURCE_DIRS := ./ \
               ./config \
			   ./gateservice \
               ./ssproto \
               ./workservice

INCLUDE_PATH = -I$(TOP_DIR)/sframe
LIB_PATH = -L$(TOP_DIR)/sframe/lib$(MFLAGS)

ifeq ($(VER),debug)
	TARGET_NAME := $(TOP_DIR)/example/bin/server/server$(MFLAGS)-d
	LIBS = -lsframe-d
else
	TARGET_NAME := $(TOP_DIR)/example/bin/server/server$(MFLAGS)
	LIBS = -lsframe
endif

include $(TOP_DIR)/make/real.make