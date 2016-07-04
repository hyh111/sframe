
CC             := gcc
CXX            := g++

ifeq ($(VER),debug)
	VER_FLAGS  := -g -DDEBUG
	OBJS_DIR   := ./obj/debug
else
	VER_FLAGS  := -O3
	OBJS_DIR   := ./obj/release
endif

ifneq ($(MFLAGS),)
	TEMPFILE_MFLAG := .$(MFLAGS)
	COMPILE_MFLAG := -m$(MFLAGS)
endif

SRCEXTS        := .c .cpp
SRCS           := $(foreach d,$(SOURCE_DIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
SRC_CXX        := $(filter-out %.c,$(SRCS))
OBJS           := $(addprefix $(OBJS_DIR)/,$(addsuffix $(TEMPFILE_MFLAG).o,$(basename $(notdir $(SRCS)))))
D_FILES        := $(OBJS:.o=.d)
TARGET_DIR     := $(dir $(TARGET_NAME))

ifeq ($(TARGET_TYPE),lib)
	BUILD_CMD  := $(AR) -cqs $(TARGET_NAME) $(OBJS)
else
	ifeq ($(SRC_CXX),)
		BUILD_CMD  := $(CC) $(COMPILE_MFLAG) $(CFLAGS) $(CPPFLAGS) $(VER_FLAGS) -o $(TARGET_NAME) $(OBJS) $(LIB_PATH) $(LIBS)
	else
		BUILD_CMD  := $(CXX) $(COMPILE_MFLAG) $(CXXFLAGS) $(CPPFLAGS) $(VER_FLAGS) -o $(TARGET_NAME) $(OBJS) $(LIB_PATH) $(LIBS)
	endif
endif

space := $(nullstring) # end of the line
VPATH = $(subst $(space),:,$(SOURCE_DIRS))

all:$(TARGET_DIR) $(TARGET_NAME)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

$(TARGET_NAME):$(OBJS)
	$(BUILD_CMD)

$(OBJS_DIR)/%$(TEMPFILE_MFLAG).d:%.c
	@set -e;mkdir -p $(OBJS_DIR);rm -f $@;$(CC) $(INCLUDE_PATH) -MM $(CFLAGS) $(CPPFLAGS) $(VER_FLAGS) $< > $@.$$$$;sed 's,\($*\)\.o[ :]*,$(OBJS_DIR)/\1$(TEMPFILE_MFLAG).o $@ : ,g' < $@.$$$$ > $@;rm -f $@.$$$$

$(OBJS_DIR)/%$(TEMPFILE_MFLAG).d:%.cpp
	@set -e;mkdir -p $(OBJS_DIR);rm -f $@;$(CXX) $(INCLUDE_PATH) -MM $(CXXFLAGS) $(CPPFLAGS) $(VER_FLAGS) $< > $@.$$$$;sed 's,\($*\)\.o[ :]*,$(OBJS_DIR)/\1$(TEMPFILE_MFLAG).o $@ : ,g' < $@.$$$$ > $@;rm -f $@.$$$$

sinclude $(D_FILES)

$(OBJS_DIR)/%$(TEMPFILE_MFLAG).o:%.c
	$(CC) $(INCLUDE_PATH) $(COMPILE_MFLAG) $(CFLAGS) $(CPPFLAGS) $(VER_FLAGS) -c $< -o $@

$(OBJS_DIR)/%$(TEMPFILE_MFLAG).o:%.cpp
	$(CXX) $(INCLUDE_PATH) $(COMPILE_MFLAG) $(CXXFLAGS) $(CPPFLAGS) $(VER_FLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -f $(TARGET_NAME) $(OBJS) $(D_FILES)