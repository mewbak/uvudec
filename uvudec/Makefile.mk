# UVNet Universal Decompiler (uvudec)
# Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
# Licensed under terms of the three clause BSD license, see LICENSE for details

# LINKAGE: set to either dynamic or static for our current target
# USING_DYNAMIC: if we configured to support building dynamic exe
# USING_STATIC: if we configured to support building static exe

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

# FIXME: how do I do an "or"
COMPILING_CODE=
ifdef EXE_NAME
COMPILING_CODE=Y
endif
ifdef LIB_NAME
COMPILING_CODE=Y
endif

# System defaults
include $(ROOT_DIR)/Makefile.defaults
# Optional "./configure" result
#$(shell if [ '!' -f $(ROOT_DIR)/Makefile.configure ] ; then touch $(ROOT_DIR)/Makefile.configure); fi )
include $(ROOT_DIR)/Makefile.configure

BIN_DIR=$(ROOT_DIR)/bin
LIB_DIR=$(BIN_DIR)

ASSEMBLY_DIR=$(ROOT_DIR)/assembly
COMPILER_DIR=$(ROOT_DIR)/compiler
CORE_DIR=$(ROOT_DIR)/core
DATA_DIR=$(ROOT_DIR)/data
ELF_DIR=$(ROOT_DIR)/elf
FLIRT_DIR=$(ROOT_DIR)/flirt
GUI_DIR=$(ROOT_DIR)/GUI
HASH_DIR=$(ROOT_DIR)/hash
INIT_DIR=$(ROOT_DIR)/init
INTERPRETER_DIR=$(ROOT_DIR)/interpreter
LANGUAGE_DIR=$(ROOT_DIR)/language
RELOCATION_DIR=$(ROOT_DIR)/relocation
TESTING_DIR=$(ROOT_DIR)/testing
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
INCLUDES += -I$(ASSEMBLY_DIR) -I$(COMPILER_DIR) -I$(CORE_DIR) -I$(DATA_DIR) -I$(ELF_DIR) -I$(HASH_DIR) -I$(INIT_DIR) -I$(INTERPRETER_DIR) -I$(LANGUAGE_DIR) -I$(UTIL_DIR) -I$(RELOCATION_DIR) -I$(FLIRT_DIR)

#OPTIMIZATION_LEVEL=-O3
DEBUG_FLAGS=-g
WARNING_FLAGS=-Wall -Werror
FLAGS_SHARED += -c $(WARNING_FLAGS) $(INCLUDES) $(DEBUG_FLAGS) $(UVUDEC_VER_FLAGS) $(OPTIMIZATION_LEVEL)
CCFLAGS += $(FLAGS_SHARED)
CXXFLAGS += $(FLAGS_SHARED)

LDFLAGS += -L$(LIB_DIR) -L.

FLAGS_SHARED += -DDEFAULT_DECOMPILE_FILE=$(DEFAULT_DECOMPILE_FILE) -DDEFAULT_CPU_DIR=$(DEFAULT_CPU_DIR) -DDEFAULT_CPU_FILE=$(DEFAULT_CPU_FILE)

# Name of the uvudec library
LIB_UVUDEC=libuvudec
# Full name of the version we are using (will link against)
#LIB_UVUDEC_FULL=libuvudec
# Leave out the patch, gives us a binary compatible version
#LIB_UVUDEC_DYNAMIC_USED=$(LIB_DIR)/$(LIB_UVUDEC).so.$(UVUDEC_VER_MAJOR).$(UVUDEC_VER_MINOR)
LIB_UVUDEC_DYNAMIC_USED=uvudec
LIB_UVUDEC_STATIC=$(LIB_DIR)/$(LIB_UVUDEC).a

# Experimental way to speed up printing
ifeq ($(USING_ROPE),Y)
FLAGS_SHARED += -DUSING_ROPE
endif

# Lua stuff
ifeq ($(USING_LUA),Y)
FLAGS_SHARED += -DUSING_LUA -DUSING_LUA_API
LUA_INCLUDE=$(LUA_DIR)/src
LUA_LIB_STATIC=$(LUA_DIR)/src/liblua.a
INCLUDES += -I$(LUA_INCLUDE)
ifeq ($(LINKAGE),static)
LIBS += $(LUA_LIB_STATIC)
else
LIBS += -llua
endif
endif

ifeq ($(USING_LIBBFD),Y)
USING_LIBZ=Y
endif

# Python stuff
# This may get more complicated if I can get the APIs working better
ifeq ($(USING_PYTHON),Y)
FLAGS_SHARED += -DUSING_PYTHON
ifeq ($(USING_PYTHON_EXEC),Y)
FLAGS_SHARED += -DUSING_PYTHON_EXEC
endif
ifeq ($(USING_PYTHON_API),Y)
FLAGS_SHARED += -DUSING_PYTHON_API
ifeq ($(LINKAGE),static)
else
LIBS += -lpython2.6
LDFLAGS +=
endif
endif
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
FLAGS_SHARED += -DUSING_JAVASCRIPT_API
endif

# SpiderApe stuff (a javascript engine)
# TODO: figure out how to use this more properly
ifeq ($(USING_SPIDERAPE),Y)
FLAGS_SHARED += -DUSING_SPIDERAPE
LIBS += -lSpiderApe -ljs
ifdef SPIDERAPE_PREFIX
LDFLAGS += -L$(SPIDERAPE_PREFIX)/lib
INCLUDES += -I$(SPIDERAPE_PREFIX)/include
endif
endif


ifeq ($(USING_LIBBFD),Y)
FLAGS_SHARED += -DUVD_FLIRT_PATTERN_BFD
ifdef BINUTILS_PREFIX
INCLUDES+=-I$(BINUTILS_PREFIX)/include
LDFLAGS+=-L$(BINUTILS_PREFIX)/lib
endif
LIBBFD_STATIC_LIB=-lbfd
LIBOPCODES_STATIC_LIB=-lopcodes
LIBIBERTY_STATIC_LIB=-liberty
LIBS+=$(LIBBFD_STATIC_LIB) $(LIBOPCODES_STATIC_LIB) $(LIBIBERTY_STATIC_LIB)

# Otherwise use system provided (doesn't build until install)
LIBS += -lbfd -lopcodes -liberty

endif


# hmm why is this hard coded?
LIBZ_STATIC=/usr/lib/libz.a

ifeq ($(USING_LIBZ),Y)
ifeq ($(LINKAGE),static)
LIBS += $(LIBZ_STATIC)
else
LIBS += -lz
endif
endif



# General libc stuff
ifeq ($(LINKAGE),static)

# We only include libs in the final exe for static buids usually
LIBS +=
# And since we don't link until final exe, don't set any LDFLAGS
LDFLAGS +=

else

# We must incrementally link as we go along
LIBS += -lc -lm -lgcc
#LIBS += -lstdc++ -lgcc
LDFLAGS +=

endif

ifeq ($(LINKAGE),static)
OBJECT_LINKAGE_SUFFIX=_s
else
OBJECT_LINKAGE_SUFFIX=_d
endif
OBJS = $(CC_SRCS:.c=$(OBJECT_LINKAGE_SUFFIX).o) $(CXX_SRCS:.cpp=$(OBJECT_LINKAGE_SUFFIX).o)

UVUDEC_EXE = $(BIN_DIR)/uvudec
COFF2PAT_EXE = $(BIN_DIR)/uvcoff2pat
GUI_EXE = $(BIN_DIR)/uvudecgui
OBJ2PAT_EXE = $(BIN_DIR)/uvobj2pat
OMF2PAT_EXE = $(BIN_DIR)/uvomf2pat
ELF2PAT_EXE = $(BIN_DIR)/uvelf2pat
PAT2SIG_EXE = $(BIN_DIR)/uvpat2sig
EXES= $(UVUDEC_EXE) $(COFF2PAT_EXE) $(OMF2PAT_EXE) $(ELF2PAT_EXE) $(PAT2SIG_EXE)
TESTING_EXE = $(BIN_DIR)/uvtest

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

%$(OBJECT_LINKAGE_SUFFIX).o: %.c
	$(CC) $(CCFLAGS) $< -o $@

%$(OBJECT_LINKAGE_SUFFIX).o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

cleanLocal:
	$(RM) *.o *~ $(UVUDEC_EXE) $(OBJS) uv_log.txt Makefile.bak core.* Makefile.depend*

CLEAN_DEPS+=cleanLocal
cleanTarget: $(CLEAN_DEPS)
	@(true)

# Sudsy soap would be proud!
clean:
	$(MAKE) TARGET=static cleanTarget
	$(MAKE) TARGET=dynamic cleanTarget

ifdef COMPILING_CODE
MAKEFILE_DEPEND=Makefile.depend$(OBJECT_LINKAGE_SUFFIX)
$(shell touch $(MAKEFILE_DEPEND))
include $(MAKEFILE_DEPEND)

ifdef MAKEDEPEND
# Silicenced because they started to take up a lot of screen during each build
# Ignore cannot find stdio.h stuff
depend:
#$(MAKEDEPEND) -f$(MAKEFILE_DEPEND) -Y $(CCFLAGS) $(CC_SRCS) $(CXX_SRCS)
	@($(MAKEDEPEND) -f$(MAKEFILE_DEPEND) -Y $(CCFLAGS) $(CC_SRCS) $(CXX_SRCS) 2>/dev/null >/dev/null)
	perl -pi -e 's/[.]o/$(OBJECT_LINKAGE_SUFFIX).o/g' $(MAKEFILE_DEPEND)
#	$(MAKEDEPEND) -f$(MAKEFILE_DEPEND) -Y $(CCFLAGS) $(CC_SRCS) $(CXX_SRCS) 2>/dev/null >/dev/null
# Remove annoying backup
	@($(RM) $(MAKEFILE_DEPEND).bak)
endif
endif

PHONY += all .c.o .cpp.o clean dist depend info cleanLocal

.PHONY: $(PHONY)

info: $(INFO_TARGETS)
	@(echo "Shared info")
	@(echo "USING_UPX: $(USING_UPX)")
	@(echo "LINKAGE: $(LINKAGE)")
	@(echo "USING_STATIC: $(USING_STATIC)")
	@(echo "USING_DYNAMIC: $(USING_DYNAMIC)")
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
	@(echo "CXX:$(CXX)")
	@(echo "")
	@(echo "OBJS: $(OBJS)")
	
