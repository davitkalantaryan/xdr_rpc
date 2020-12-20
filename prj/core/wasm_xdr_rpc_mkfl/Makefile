
PROJECT_NAME 		= xdr_rpc
OUTPUT_NAME 		= libxdr_rpc.wa
TARGET_NAME 		= $(PROJECT_NAME)
#LSB_RELEASE_BASE	:= $(shell lsb_release -c | cut -f 2)
LSB_RELEASE			:= wasm
mkfile_path 		:= $(abspath $(lastword $(MAKEFILE_LIST)))
#mkfile_dir 		:= $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))
mkfile_dir			:= $(shell dirname $(mkfile_path))
TARGET_DIR  		= $(mkfile_dir)/../../../sys/$(LSB_RELEASE)/lib
SOURCES_BASE_DIR	= $(mkfile_dir)/../../../src/core


CC = emcc
CPP = em++
LINK = emar r
#CPP = arm-linux-gnueabihf-g++
#LINK = arm-linux-gnueabihf-g++


#LIBS = -ldl
#LFLAGS  = -Wl,-E -pie -shared
#LFLAGS  += -soname,gul.so.1

OBJECT_FILES_DIR = $(mkfile_dir)/../../../sys/$(LSB_RELEASE)/.objects/$(PROJECT_NAME)
TARGET_FILE_PATH = $(TARGET_DIR)/$(OUTPUT_NAME)


#CPPPARAMS = -m32 -I../../../include -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -Wno-write-strings -DIs32Bit
CPARAMS = -O2 -DLINUX -D_REENTRANT -D_GNU_SOURCE -D_BSD -D__USE_GNU -D__USE_UNIX98
CPARAMS += -DMULTITHREADED
CPARAMS += -I$(mkfile_dir)/../../../include -fPIC
CPARAMS += $(WASM_MULTITHREADED_FLAGS)
CPPPARAMS = $(CPARAMS)


# $(OBJECT_FILES_DIR)/shmalloc.o # shared memory routines for windows
# $(OBJECT_FILES_DIR)/syslib.o   # <execinfo.h> is not known
# $(OBJECT_FILES_DIR)/tine32.o   # TINE Win32 DLL API routines:

# $(OBJECT_FILES_DIR)/clnt_per.o
# $(OBJECT_FILES_DIR)/clnt_per_singlethreaded.o
# $(OBJECT_FILES_DIR)/mini_xdr_rpc_missing_functions.o

OBJECTS		+=	\
	$(OBJECT_FILES_DIR)/emscripten_threading.o				\
	$(OBJECT_FILES_DIR)/auth_non.o							\
	$(OBJECT_FILES_DIR)/auth_uni.o							\
	$(OBJECT_FILES_DIR)/clnt_gen.o							\
	$(OBJECT_FILES_DIR)/clnt_per_singlethreaded.o			\
	$(OBJECT_FILES_DIR)/clnt_tcp.o							\
	$(OBJECT_FILES_DIR)/clnt_udp.o							\
	$(OBJECT_FILES_DIR)/mini_xdr_rpc_entry.o					\
	$(OBJECT_FILES_DIR)/pmap_get.o							\
	$(OBJECT_FILES_DIR)/rpc_prot.o							\
	$(OBJECT_FILES_DIR)/xdr.o									\
	$(OBJECT_FILES_DIR)/xdr_array.o							\
	$(OBJECT_FILES_DIR)/xdr_float.o							\
	$(OBJECT_FILES_DIR)/xdr_mem.o								\
	$(OBJECT_FILES_DIR)/xdr_rec.o								\
	$(OBJECT_FILES_DIR)/xdr_reference.o

default: $(TARGET_NAME)

testVars:
	@echo "mkfile_dir="$(mkfile_dir)


$(OBJECT_FILES_DIR)/%.o: $(SOURCES_BASE_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CPARAMS) -o $@ -c $<
	
$(OBJECT_FILES_DIR)/%.o: $(SOURCES_BASE_DIR)/%.cpp
	mkdir -p $(@D)
	$(CPP) $(CPPPARAMS) -o $@ -c $<


$(TARGET_NAME): $(OBJECTS)
	mkdir -p $(TARGET_DIR)
	#ld $(LDPARAMS) -T $< -o $(TARGET_FILE_PATH) $(OBJECTS)
	#$(LINK) $(OBJECTS) $(LIBS) $(LFLAGS) -o $(TARGET_FILE_PATH)
	$(LINK) $(TARGET_FILE_PATH) $(OBJECTS) $(LIBS) $(LFLAGS)
	rm -rf $(SRC_TMP)

# .PHONY means always out of date
.PHONY: clean

clean:
	#rm -f $(OBJECTS) $(TARGET_FILE_PATH) $(SRC_TMP)/*.cpp $(SRC_TMP)/*.s $(ISO_DIR2)/mykernel.iso
	rm -f $(OBJECTS) $(TARGET_FILE_PATH) 

