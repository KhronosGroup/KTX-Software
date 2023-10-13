# Copyright 2019-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

out := out

targets := testcreatedfd testinterpretdfd testbidirectionalmapping doc

all: $(addprefix out/,${targets})

CFLAGS=-I.

# Note. Currently we use an unreleased version of vulkan_core.h with enums
# for ASTC 3D. Since this is temporary, the sources still use <angled>
# includes. The -I. causes our local copy to be found while the VULKAN_SDK
# part keeps compilers from warning that the file was not found with
# <angled> include.
$(out)/testcreatedfd: createdfd.c createdfdtest.c printdfd.c vk2dfd.c vk2dfd.inl KHR/khr_df.h dfd.h | out
	gcc createdfdtest.c createdfd.c printdfd.c -I. -o $@ -std=c99 -W -Wall -pedantic -O2 -Wno-strict-aliasing -Wno-unused-parameter

$(out)/testinterpretdfd: createdfd.c interpretdfd.c interpretdfdtest.c printdfd.c KHR/khr_df.h dfd.h | out
	gcc interpretdfd.c createdfd.c interpretdfdtest.c printdfd.c -o $@ -I.  -O -W -Wall -std=c99 -pedantic -Wno-unused-parameter

$(out)/testbidirectionalmapping: testbidirectionalmapping.c interpretdfd.c createdfd.c dfd2vk.c dfd2vk.inl vk2dfd.c vk2dfd.inl KHR/khr_df.h dfd.h | out
	gcc testbidirectionalmapping.c interpretdfd.c createdfd.c -o $@ -I. -g -W -Wall -std=c99 -pedantic -Wno-unused-parameter

$(out)/doc: colourspaces.c createdfd.c createdfdtest.c printdfd.c queries.c KHR/khr_df.h dfd.h | out
	doxygen dfdutils.doxy


# Test build dfd2vk and vk2dfd
vk2dfd.o: $(out)/vk2dfd.o
$(out)/vk2dfd.o: vk2dfd.c | $(out)
	echo @ is $@ '<' is $<
	cc -c -o $@ -I . $<

dfd2vk.o: $(out)/dfd2vk.o
$(out)/dfd2vk.o: dfd2vk.c | $(out)
	cc -c -o $@ -I . $<

build out:
	mkdir -p $@

clean:

clobber: clean
	rm -rf $(addprefix out/,${targets})

doc: $(out)/doc

switches: dfd2vk.inl vk2dfd.inl

dfd2vk.inl: vulkan/vulkan_core.h makedfd2vk.pl
	./makedfd2vk.pl $< $@

vk2dfd.inl: vulkan/vulkan_core.h makevk2dfd.pl
	./makevk2dfd.pl $< $@

# For those who wish to generate a project from the gyp file so
# as to use xcode for debugging.
xcodeproj: build/project.pbxproj
build/project.pbxproj: dfdutils.gyp | build
	gyp -f xcode -DOS=mac --generator-output=build --depth=.

