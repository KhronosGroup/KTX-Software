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
# $Revision$ $Date:: 2010-07-07 15:54:52 +0900 #$

depth := ../../..

ktxPubInc := $(depth)/include
ktxLibSrc := $(depth)/lib

LIBX := a
ktxLib := libktx.$(LIBX)

sources = $(notdir $(wildcard ${ktxLibSrc}/*.c))
headers = $(notdir $(wildcard ${ktxLibSrc}/*.h $(ktxPubInc)/*.h))
objects = $(sources:%.c=%.o) etcdec.o

# Change this to the parent of *your* GL header files
glincdir = $(HI_BSLIB_HOME)/include

vpath %.h $(ktxPubInc) $(ktxLibSrc)
vpath %.c $(ktxLibSrc)
vpath %.cxx $(ktxLibSrc)

CFLAGS = -MMD
ifneq ($(OPTIMIZE),true)
  CFLAGS += -g
endif
cdefs = -DKTX_OPENGL_ES1=1
cincs = -I$(ktxPubInc) -I$(ktxLibSrc) -I$(glincdir)
CFLAGS += $(cdefs) $(cincs)

.PHONY: clean clobber

$(ktxLib): $(objects)
	$(AR) -r $@ $(objects)

# Have to use this Ericsson file unchanged so it is not renamed.
# GNUmake's built-in implicit rules do not recognize cxx as a
# c plusplus extension.
etcdec.o: etcdec.cxx
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -x c $<

clean:
	rm *.o

clobber:
	rm $(ktxLib)

-include *.d
