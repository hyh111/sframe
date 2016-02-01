
TOP_DIR = ../..

include $(TOP_DIR)/make/global-config.mk

SOURCE_DIRS := ./ \
               ./config \
			   ./gateservice \
               ./ssproto \
               ./workservice

INCLUDE_PATH = -I$(TOP_DIR)/sframe
LIB_PATH = -L$(TOP_DIR)/sframe/lib$(MFLAGS)

ifneq ($(MFLAGS),32)
	EXE_ADD_NAME := $(MFLAGS)
endif

ifeq ($(VER),debug)
	TARGET_NAME := $(TOP_DIR)/example/bin/server/server$(EXE_ADD_NAME)-d
	LIBS = -lsframe-d
else
	TARGET_NAME := $(TOP_DIR)/example/bin/server/server$(EXE_ADD_NAME)
	LIBS = -lsframe
endif

include $(TOP_DIR)/make/real.make