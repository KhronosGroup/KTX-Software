# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4:

# GNU make file to build libKTX and doxygen documentation

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
# $Revision$ $Date::                            $

depth := ../..
version := 10

# Location of KTX home page
ktxHomePage = $(depth)/../../../sdk/tools/KTX

ktxPubInc := $(depth)/include
ktxLibSrc := $(depth)/lib
ktxTools  := $(depth)/tools
toktxSrc  := $(ktxTools)/toktx
ktxDoxyConfig := ktxDoxy

ktxZipName := ktx$(version).zip
ktxDistZip := ../$(ktxZipName)
ktxZipOut := build/$(ktxZipName)
ktxTgzName := ktx$(version).tgz
ktxDistTgz := ../$(ktxTgzName)
ktxTgzOut := build/$(ktxTgzName)

ktxLibDocOutDir := ../doc

vc9 := ../vc9
toktxWinExe := $(vc9)/Release/toktx.exe
devenv = $(shell cygpath "$(VS90COMNTOOLS)/../IDE/devenv.exe")

distFiles := include/ktx.h $(ktxDoxyConfig) lib tests TODO tools \
	         testimages/*.ppm testimages/{etc1,*reference}.ktx \
			 build/GNUmake/GNUmakefile build/GNUmake/lib/GNUmakefile \
	         build/vc9/*.sln build/vc9/*/*.vcproj build/vc9/*.txt \
             build/vc9/*.vsprops 

sources = $(notdir $(wildcard ${ktxLibSrc}/*.c))
sources += etcdec.cxx
headers = $(notdir $(wildcard ${ktxLibSrc}/*.h $(ktxPubInc)/*.h))

vpath %.h $(ktxPubInc) $(ktxLibSrc)
vpath %.c $(ktxLibSrc)
vpath %.cxx $(ktxLibSrc)

SUBDIRS = lib #toktx gles1_loadtests

.PHONY: clean clobber doc dist help $(ktxDistZip) $(ktxDistTgz) $(SUBDIRS)
.PHONY: updateKtxHome toktxWin

default: $(SUBDIRS)

doc: $(ktxLibDocOutDir)/html

dist: $(ktxDistZip) $(ktxDistTgz)

help:
	@echo "The following targets are available:"
	@echo "    default       - compile the library"
	@echo "    doc           - build the documentation with doxygen"
	@echo "    dist          - build the .zip and .tgz files for distribution"
	@echo "    help          - print this list of targets"
	@echo "    updateKtxHome - runs dist, doc and toktxWin then updates the KTX"
	@echo "                    page in the OpenGL ES SDK. Requires an SVN"
	@echo "	                   workspace for the SDK on the same system"
	@echo "    toktxWin      - build the windows binary of toktx"

updateKtxHome: dist $(ktxLibDocOutDir)/html toktxWin
	cp -r $(ktxLibDocOutDir)/html/. $(ktxHomePage)/doc/libktx
	cp $(ktxDistZip) $(ktxDistTgz) $(ktxHomePage)/downloads
	cp $(toktxWinExe) $(ktxHomePage)/downloads

# doxygen is run in the top-level directory so that ancestors of that
# directory will be removed from paths displayed in the documentation.
$(ktxLibDocOutDir)/html: $(depth)/$(ktxDoxyConfig) $(sources) $(headers)
	cd $(depth); doxygen $(ktxDoxyConfig)

# Having to change directory to run zip means $@ cannot be used.
$(ktxDistZip):
	cd $(depth); zip --quiet -r $(ktxZipOut) $(distFiles) --exclude "*/.svn/*"

$(ktxDistTgz):
	cd $(depth); tar czf $(ktxTgzOut) $(distFiles) --exclude="*/.svn/*"

toktxWin:
#	"$(devenv)" $(vc9)/ktx.sln /build "Release" /project $(vc9)/toktx/toktx.vcproj /projectconfig Release
	"$(devenv)" $(vc9)/ktx.sln /build "Release"

#$(ktxLibDocOutDir):
#	mkdir -P $@

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean clobber: $(SUBDIRS)
