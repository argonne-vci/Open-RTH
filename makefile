#=============================================================================
# This is a makefile that can compile apps for an ARM target
#=============================================================================


#-----------------------------------------------------------------------------
# Name of build directory, executable, extension and target (do not add spaces)
# Extract version number from 'changelog.h', or manually specify a version no.
#-----------------------------------------------------------------------------
BUILD_DIR := build
FILE_NAME := OpenRTH
TARGET	  := EVAchargeSE
EXTENSION := chb
VERSION   := $(shell grep '^#define SW_VERSION' version.h | cut -d'"' -f2)

# Set if we want to strip debugging symbols from binary (on or off)
STRIP_DEBUG := on

#-----------------------------------------------------------------------------
# This is a list of directories that have compilable code in them. If there
# are no subdirectories, this line is must SUBDIRS = .
#-----------------------------------------------------------------------------
SUBDIRS = \
src/app \
src/config \
src/io \
src/J1772 \
src/json \
lib/jsoncpp \
src/mqtt \
src/sdp \
src/tcp \
src/wolfMQTT_cpp \
src/utilities \
src/utilities/open-plc-utils \
src/utilities/open-plc-utils/key \
src/utilities/open-plc-utils/ether \
src/utilities/open-plc-utils/mme \
src/utilities/open-plc-utils/plc \
src/utilities/open-plc-utils/slac \
src/utilities/open-plc-utils/tools


#-----------------------------------------------------------------------------
# Declare location of all included header files, including external libraries
#-----------------------------------------------------------------------------

INCLUDES = \
-Isrc \
-Isrc/app \
-Isrc/config \
-Isrc/io \
-Isrc/J1772 \
-Isrc/json \
-Ilib/jsoncpp \
-Isrc/mqtt \
-Isrc/sdp \
-Isrc/tcp \
-Isrc/wolfMQTT_cpp \
-Isrc/utilities \
-Isrc/utilities/open-plc-utils\
-Isrc/utilities/open-plc-utils/plc \
-Isrc/utilities/open-plc-utils/ether


LIB_INCLUDES = \
-Ilib/wolfssl/include \
-Ilib/wolfmqtt/include


#-----------------------------------------------------------------------------
# Link options for any external static (.a) or shared (.so) libraries
# Each library and its path should be specified with -L and -l flags
# Example: -Lpath/to/lib -lnameoflib
#-----------------------------------------------------------------------------
LINK_FLAGS =  -pthread -lm -lrt \
-Llib/wolfssl/lib -lwolfssl \
-Llib/wolfmqtt/lib -lwolfmqtt


#-----------------------------------------------------------------------------
# Declare any specific compile-time flags
#-----------------------------------------------------------------------------
CFLAGS = \
-O2 -g -Wall \
-c -fmessage-length=0 \
-D_GNU_SOURCE \
-Wno-sign-compare \
-Wno-unused-value \
-Wno-unused-function \
-Wno-psabi
#-----------------------------------------------------------------------------
# Declare special compile time flags for ARM targets
#-----------------------------------------------------------------------------
ARM_FLAGS = 


#-----------------------------------------------------------------------------
# Parameters for remote upload of executable to the target via SCP
#-----------------------------------------------------------------------------
HOSTNAME    := N/A
USERNAME    := N/A
UPLOAD_PATH := N/A


#-----------------------------------------------------------------------------
# These are the settings of the cross compiler based on board type
#-----------------------------------------------------------------------------
ifeq ($(TARGET), EVAchargeSE)
	CC_PATH  = /opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi
	CC       = $(CC_PATH)/bin/arm-none-linux-gnueabi-gcc
	CXX      = $(CC_PATH)/bin/arm-none-linux-gnueabi-g++
	CC_STRIP = ${CC_PATH}/bin/arm-none-linux-gnueabi-strip
	C_STD 	= -std=gnu99
	CPP_STD	= -std=c++0x
endif



##############################################################################
#                         DO NOT EDIT BELOW THIS LINE                        #
##############################################################################

#-----------------------------------------------------------------------------
# This is the base name of the executable file
#-----------------------------------------------------------------------------
EXE_PATH = $(BUILD_DIR)/$(FILE_NAME)_$(VERSION).$(EXTENSION)
EXE_NAME = $(FILE_NAME)_$(VERSION).$(EXTENSION)

#-----------------------------------------------------------------------------
# Declare where the object files get created
#-----------------------------------------------------------------------------
OBJ_DIR := $(BUILD_DIR)/obj


#-----------------------------------------------------------------------------
# We're going to compile every .c and .cpp file in each directory
#-----------------------------------------------------------------------------
C_SRC_FILES   := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))
CPP_SRC_FILES := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.cpp))


#-----------------------------------------------------------------------------
# In the source files, normalize "./filename" to just "filename"
#-----------------------------------------------------------------------------
C_SRC_FILES   := $(subst ./,,$(C_SRC_FILES))
CPP_SRC_FILES := $(subst ./,,$(CPP_SRC_FILES))


#-----------------------------------------------------------------------------
# Create the base-names of the object files
#-----------------------------------------------------------------------------
C_OBJ     := $(C_SRC_FILES:.c=.o)
CPP_OBJ   := $(CPP_SRC_FILES:.cpp=.o)
OBJ_FILES := ${C_OBJ} ${CPP_OBJ}


#-----------------------------------------------------------------------------
# We are going to keep object files in separate sub-directories
#-----------------------------------------------------------------------------
OBJS := $(addprefix $(OBJ_DIR)/,$(OBJ_FILES))


#-----------------------------------------------------------------------------
# Get the current date and time
#-----------------------------------------------------------------------------
NOW = $(shell date +%Y-%m-%d\ %H:%M:%S)


#-----------------------------------------------------------------------------
# Set ANSI escape sequences for common colors to use in text output for clarity
#-----------------------------------------------------------------------------
BLACK        = \033[0;30m
RED          = \033[0;31m
GREEN        = \033[0;32m
YELLOW       = \033[0;33m
BLUE         = \033[0;34m
MAGENTA      = \033[0;35m
CYAN         = \033[0;36m
WHITE        = \033[0;37m
BOLD_BLACK   = \033[1;30m
BOLD_RED     = \033[1;31m
BOLD_GREEN   = \033[1;32m
BOLD_YELLOW  = \033[1;33m
BOLD_BLUE    = \033[1;34m
BOLD_MAGENTA = \033[1;35m
BOLD_CYAN    = \033[1;36m
BOLD_WHITE   = \033[1;37m
NC           = \033[0m


#-----------------------------------------------------------------------------
# This rule tells how to compile a .o object file from a .c and .cpp source
#-----------------------------------------------------------------------------
$(OBJ_DIR)/%.o : %.cpp
	@echo "$(WHITE)Compiling object from $< $(NC)"
	@$(CXX) $(CC_FLAGS) $(CPP_STD) $(INCLUDES) $(LIB_INCLUDES) $(CFLAGS) $(ARM_FLAGS) -c $< -o $@ $(CCZE) 

$(OBJ_DIR)/%.o : %.c
	@echo "$(WHITE)Compiling object from $< $(NC)"
	@$(CC) $(CC_FLAGS) $(C_STD) $(INCLUDES) $(LIB_INCLUDES) $(CFLAGS) $(ARM_FLAGS) -c $< -o $@ $(CCZE)



#-----------------------------------------------------------------------------
# This target makes all neccessary folders for object files
#-----------------------------------------------------------------------------
$(OBJ_DIR):
	@for subdir in $(SUBDIRS); do \
	    mkdir -p -m 777 $(OBJ_DIR)/$$subdir ;\
	done


#-----------------------------------------------------------------------------
# This rule builds the executable from the object files
#-----------------------------------------------------------------------------
$(EXE_PATH) : $(OBJS)
	@echo "$(BOLD_MAGENTA)Building executable $(BOLD_GREEN)$(EXE_NAME) $(WHITE)from object files ... $(NC)"
	@$(CXX) $(ARM_FLAGS) -o $@ $(OBJS) $(LINK_FLAGS)

ifeq ($(STRIP_DEBUG),on)
	@echo "$(BOLD_CYAN)Stripping debugging symbols from binary ... $(NC)"
	@$(CC_STRIP) $(EXE_PATH)
else ifeq ($(STRIP_DEBUG),off)
	@echo "$(BOLD_YELLOW)CAUTION: Not stripping debugging symbols from binary ... $(NC)"
endif

#-----------------------------------------------------------------------------
# If there is no target on the command line, this is the target we use
#-----------------------------------------------------------------------------
.DEFAULT_GOAL := arm


#-----------------------------------------------------------------------------
# The following targets are not associated with actual files
#-----------------------------------------------------------------------------
.PHONY: $(OBJ_DIR) START END upload clean clear tarball depend debug 


#-----------------------------------------------------------------------------
# These targets print out a message before and after the build
#-----------------------------------------------------------------------------
START:
	@echo ".................................................................."
	@echo "$(WHITE)Building $(BOLD_GREEN)$(EXE_NAME) $(WHITE)- $(NOW)"
	@echo "$(WHITE)Using compiler: $(BOLD_WHITE)$(CC)$(NC)"
	@echo ".................................................................."
	@echo

END:
	@echo ".................................................................."
	@echo "$(BOLD_GREEN)Done! $(NC)"
	@echo ".................................................................."


#-----------------------------------------------------------------------------
# This is the main target that builds the executable
#-----------------------------------------------------------------------------
arm: START $(OBJ_DIR) $(EXE_PATH) END


#-----------------------------------------------------------------------------
# This target builds all executables supported by this platform
#-----------------------------------------------------------------------------
all: arm 

#-----------------------------------------------------------------------------
# This target clears the terminal screen
#-----------------------------------------------------------------------------
clear:
	clear

#-----------------------------------------------------------------------------
# This target is for build/upload - used for quick testing only
#-----------------------------------------------------------------------------
go: arm upload 


#-----------------------------------------------------------------------------
# This target is for clean/build/upload - used for quick testing only
#-----------------------------------------------------------------------------
new: clean arm upload 


#-----------------------------------------------------------------------------
# This target is for clean/build/ - used for debugging large changes
#-----------------------------------------------------------------------------
dev: clean arm 


#-----------------------------------------------------------------------------
# This target is for clean/build/ - used for debugging large changes
#-----------------------------------------------------------------------------
again: clear clean arm 


#-----------------------------------------------------------------------------
# This target copies the executable to the target board via SCP
#-----------------------------------------------------------------------------
upload: 
	@echo
	@echo "$(BOLD_BLACK)Uploading $(EXE_PATH) to target $(HOSTNAME) ... $(NC)"
	@echo
	scp $(EXE_PATH) $(USERNAME)@$(HOSTNAME):$(UPLOAD_PATH)


#-----------------------------------------------------------------------------
# This target removes all files that are created at build time
#-----------------------------------------------------------------------------
clean:
	@echo 
	@echo "$(BOLD_BLUE)Cleaning up all build files ... $(NC)"
	@echo
	rm -rf Makefile.bak makefile.bak $(EXE_NAME).tgz $(EXE_PATH)
	rm -rf $(BUILD_DIR)
	rm -rf $(EXE_NAME).tgz


#-----------------------------------------------------------------------------
# This target creates a compressed tarball of the source code
#-----------------------------------------------------------------------------
tarball:	clean
	@echo 
	@echo "$(YELLOW)Creating a compressed tarball of the source code ... $(NC)"
	@echo
	tar --create --exclude-vcs -v -z -f $(EXE_NAME).tgz *


#-----------------------------------------------------------------------------
# This target appends/updates the dependencies list at the end of this file
#-----------------------------------------------------------------------------
depend:
	@echo 
	@echo "$(CYAN)Appending list of dependencies at the end of makefile ... $(NC)"
	@echo
	@makedepend -a -p$(OBJ_DIR)/ $(C_SRC_FILES) $(CPP_SRC_FILES) -Y 2>/dev/null


#-----------------------------------------------------------------------------
# Convenience target for displaying makefile variables 
#-----------------------------------------------------------------------------
debug:
	@echo "SUBDIRS       = ${SUBDIRS}"
	@echo "C_SRC_FILES   = ${C_SRC_FILES}"
	@echo "CPP_SRC_FILES = ${CPP_SRC_FILES}"
	@echo "C_OBJ         = ${C_OBJ}"
	@echo "CPP_OBJ       = ${CPP_OBJ}"
	@echo "OBJ_FILES     = ${OBJ_FILES}"

#-----------------------------------------------------------------------------
