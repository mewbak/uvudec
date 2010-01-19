# UVNet Universal Decompiler (uvudec)
# Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
# Licensed under terms of the three clause BSD license, see LICENSE for details

ifeq ($(INCLUDE_LEVEL),)
INCLUDE_LEVEL=.
else
INCLUDE_LEVEL+=/..
endif

ifndef ROOT_DIR
ROOT_DIR=$(INCLUDE_LEVEL)
endif

default: all
	@(true)

# System defaults
include $(ROOT_DIR)/Makefile.defaults
# Optional "./configure" result
$(shell touch $(ROOT_DIR)/Makefile.configure)
include $(ROOT_DIR)/Makefile.configure

BIN_DIR=$(ROOT_DIR)/bin
LIB_DIR=$(BIN_DIR)

ASSEMBLY_DIR=$(ROOT_DIR)/assembly
COMPILER_DIR=$(ROOT_DIR)/compiler
CORE_DIR=$(ROOT_DIR)/core
DATA_DIR=$(ROOT_DIR)/data
ELF_DIR=$(ROOT_DIR)/elf
HASH_DIR=$(ROOT_DIR)/hash
INIT_DIR=$(ROOT_DIR)/init
INTERPRETER_DIR=$(ROOT_DIR)/interpreter
LANGUAGE_DIR=$(ROOT_DIR)/language
RELOCATION_DIR=$(ROOT_DIR)/relocation
UTIL_DIR=$(ROOT_DIR)/util

# version stuff
UVUDEC_VER_MAJOR=0
UVUDEC_VER_MINOR=3
UVUDEC_VER_PATCH=0
# Deprecated
UVUDEC_VER_BUILD=0
UVUDEC_VER=$(UVUDEC_VER_MAJOR).$(UVUDEC_VER_MINOR).$(UVUDEC_VER_PATCH)
UVUDEC_VER_FLAGS=-DUVUDEC_VER_MAJOR=$(UVUDEC_VER_MAJOR) -DUVUDEC_VER_MINOR=$(UVUDEC_VER_MINOR) -DUVUDEC_VER_PATCH=$(UVUDEC_VER_PATCH)

PACKAGE=uvudec

# hmm include are kinda weird, all projects use <dir_name>/<file_name>.h, but we include all invidual dirs
INCLUDES += -I. -I$(ROOT_DIR) 
#for curDir in $(SOURCE_DIRS); do \
#INCLUDES += " -I$${curDir}" ;\
#done;
INCLUDES += -I$(ASSEMBLY_DIR) -I$(COMPILER_DIR) -I$(CORE_DIR) -I$(DATA_DIR) -I$(ELF_DIR) -I$(HASH_DIR) -I$(INIT_DIR) -I$(INTERPRETER_DIR) -I$(LANGUAGE_DIR) -I$(UTIL_DIR) -I$(RELOCATION_DIR)

#OPTIMIZATION_LEVEL=-O3
DEBUG_FLAGS=-g
WARNING_FLAGS=-Wall -Werror
FLAGS_SHARED = -c $(WARNING_FLAGS) $(INCLUDES) $(DEBUG_FLAGS) $(UVUDEC_VER_FLAGS) $(OPTIMIZATION_LEVEL)
CCFLAGS = $(FLAGS_SHARED)
CXXFLAGS = $(FLAGS_SHARED)

#LDFLAGS=

FLAGS_SHARED += -DDEFAULT_DECOMPILE_FILE=$(DEFAULT_DECOMPILE_FILE)

# Name of the uvudec library
LIB_UVUDEC=libuvudec
# Full name of the version we are using (will link against)
#LIB_UVUDEC_FULL=libuvudec
LIB_UVUDEC_DYNAMIC_USED=$(LIB_DIR)/$(LIB_UVUDEC).so.$(UVUDEC_VER_MAJOR).$(UVUDEC_VER_MINOR)
LIB_UVUDEC_STATIC=$(LIB_DIR)/$(LIB_UVUDEC).a

# Experimental way to speed up printing
ifeq ($(USING_ROPE),Y)
FLAGS_SHARED += -DUSING_ROPE
endif

# Lua stuff
ifeq ($(USING_LUA),Y)
FLAGS_SHARED += -DUSING_LUA
LUA_INCLUDE=$(LUA_DIR)/src
LUA_LIB_STATIC=$(LUA_DIR)/src/liblua.a
INCLUDES += -I$(LUA_INCLUDE)
LIBS += $(LUA_LIB_STATIC)
endif

# Python stuff
# This may get more complicated if I can get the APIs working better
ifeq ($(USING_PYTHON),Y)
FLAGS_SHARED += -DUSING_PYTHON
endif

# Javascript support
USING_JAVASCRIPT=N
ifeq ($(USING_SPIDERAPE),Y)
USING_JAVASCRIPT=Y
else
USING_SPIDERAPE=N
endif
ifeq ($(USING_SPIDERMONKEY),Y)
USING_JAVASCRIPT=Y
else
USING_SPIDERMONKEY=N
endif
# Now do actual USING_JAVASCRIPT effects
ifeq ($(USING_JAVASCRIPT),Y)
FLAGS_SHARED += -DUSING_JAVASCRIPT
endif

# SpiderApe stuff (a javascript engine)
SPIDERAPE_STATIC=$(SPIDERAPE_DIR)/src/ape/libSpiderApe.a
JS_STATIC=$(SPIDERAPE_DIR)/src/js/Linux_All_DBG.OBJ/libjs.a
ifeq ($(USING_SPIDERAPE),Y)
FLAGS_SHARED += -DUSING_SPIDERAPE
INCLUDES += -I$(SPIDERAPE_DIR)/include -I$(SPIDERAPE_DIR)/src/js -I$(SPIDERAPE_DIR)/src/ape 
LIBS += $(SPIDERAPE_STATIC) $(JS_STATIC)
# FIXME: this breaks static linkage
# fixed: disable plugins
# LIBS +=  -lltdl
endif
		
OBJS = $(CC_SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)
UVUDEC_EXE = $(BIN_DIR)/uvudec
COFF2PAT_EXE = $(BIN_DIR)/uvcoff2pat
OMF2PAT_EXE = $(BIN_DIR)/uvomf2pat
ELF2PAT_EXE = $(BIN_DIR)/uvelf2pat
PAT2SIG_EXE = $(BIN_DIR)/uvpat2sig
EXES= $(UVUDEC_EXE) $(COFF2PAT_EXE) $(OMF2PAT_EXE) $(ELF2PAT_EXE) $(PAT2SIG_EXE)

ifdef EXE_NAME
include $(ROOT_DIR)/Makefile.exe
endif

ifdef LIB_NAME
include $(ROOT_DIR)/Makefile.lib
endif

# BEGIN TARGETS

all:  $(ALL_TARGETS)
	@(echo 'all: done')
	@(true)

$(BIN_DIR):
	mkdir $(BIN_DIR)
ifneq ($(LIB_DIR),$(BIN_DIR))
$(LIB_DIR):
	mkdir $(LIB_DIR)
endif

.c.o:
	$(CC) $(CCFLAGS) $< -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

cleanLocal:
	$(RM) *.o *~ $(UVUDEC_EXE) $(OBJS) uv_log.txt Makefile.bak core.*

CLEAN_DEPS+=cleanLocal
clean: $(CLEAN_DEPS)
	@(true)

MAKEFILE_DEPEND=Makefile.depend
$(shell touch $(MAKEFILE_DEPEND))
include $(MAKEFILE_DEPEND)

# Silicenced because they started to take up a lot of screen during each build
# Ignore cannot find stdio.h stuff
depend:
	@($(MAKEDEPEND) -f$(MAKEFILE_DEPEND) -Y $(CCFLAGS) $(CC_SRCS) $(CXX_SRCS) 2>/dev/null >/dev/null)
# Remove annoying backup
	@($(RM) $(MAKEFILE_DEPEND).bak)

PHONY += all .c.o .cpp.o clean dist depend info cleanLocal

.PHONY: $(PHONY)

info: $(INFO_TARGETS)
	@(echo "Shared info")
	@(echo "USING_UPX: $(USING_UPX)")
	@(echo "USING_STATIC: $(USING_STATIC)")
	@(echo "USING_PYTHON: $(USING_PYTHON)")
	@(echo "USING_LUA: $(USING_LUA)")
	@(echo "USING_JAVASCRIPT: $(USING_JAVASCRIPT)")
	@(echo "USING_SPIDERAPE: $(USING_SPIDERAPE)")
	@(echo "USING_SPIDERMONKEY: $(USING_SPIDERMONKEY)")
	@(echo "")
	@(echo "ROOT_DIR: $(ROOT_DIR)")
	@(echo "PHONY: $(PHONY)")
	@(echo "LIBS: $(LIBS)")
	@(echo "LDFLAGS: $(LDFLAGS)")
	@(echo "INCLUDES: $(INCLUDES)")
	@(echo "ALL_TARGETS:$(ALL_TARGETS)")
	@(echo "")
	@(echo "OBJS: $(OBJS)")