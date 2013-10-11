# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4:

# GNU make file to build libKTX

# ---------------------------------------------------------------
#
# Copyright (c) 2010 The Khronos Group Inc.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Materials.
# 
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
# 
# ---------------------------------------------------------------

# Author: Mark Callow, HI Corporation
# $Id$

depth := ../../..

ktxPubInc := $(depth)/include
ktxLibSrc := $(depth)/lib

LIBX := a
ktxLib := libktx.$(LIBX)

csources = $(notdir $(wildcard ${ktxLibSrc}/*.c))
cxxsources = $(notdir $(wildcard ${ktxLibSrc}/*.cxx))
headers = $(notdir $(wildcard ${ktxLibSrc}/*.h $(ktxPubInc)/*.h))
objects = $(csources:%.c=%.o)
objects += $(cxxsources:%.cxx=%.o)

# Change this to the parent of *your* GL header files
glincdir = $(HI_BSLIB_HOME)/include

vpath %.h $(ktxPubInc) $(ktxLibSrc)
vpath %.c $(ktxLibSrc)
vpath %.cxx $(ktxLibSrc)

CFLAGS = -MMD
ifneq ($(OPTIMIZE),true)
  CFLAGS += -g
endif
cincs = -I$(ktxPubInc) -I$(ktxLibSrc) -I$(glincdir)
CFLAGS += $(cdefs) $(cincs)
subdirs = es1 es2 es3 gl
versions = $(addsuffix ver,${subdirs})
makes = $(addsuffix make,${subdirs})

.PHONY: clean clobber versions $(versions) $(makes)

default: $(versions)

es1ver: cdefs=-DKTX_OPENGL_ES1=1
es2ver: cdefs=-DKTX_OPENGL_ES2=1
es3ver: cdefs=-DKTX_OPENGL_ES3=1
glver: cdefs=-DKTX_OPENGL=1
$(versions): target=$(ktxLib)
$(versions): %ver: %make

makes: $(makes)

$(makes): %make: %
	$(MAKE) -C $< -f ../$(lastword ${MAKEFILE_LIST}) depth=../$(depth) cdefs=$(cdefs) $(target)

$(subdirs):
	+mkdir $@

$(ktxLib): $(objects)
	$(AR) -r $@ $(objects)

# Have to use the Ericsson etcdec.cxx file unchanged so it is not
# renamed. KTX original files that need to be C++ also use .cxx
# for consistency. GNUmake's implicit rules do not recognize cxx
# as a c++ extension. so create our own rule.
%.o: %.cxx ; $(CXX) -c $(CPPFLAGS) $(CFLAGS) $<

clean: target=cleansubdir
clean: makes

clobber: target=clobbersubdir
clobber: makes
	rmdir $(subdirs)

cleansubdir:
	rm -f *.d *.o

clobbersubdir: cleansubdir
	rm -f $(ktxLib)

-include *.d
