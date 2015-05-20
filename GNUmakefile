#
# run GYP to generate various project files.
#
# Use this because GYP does not have a way to set its options within
# the GYP file.
#
# © 2015 Mark Callow.
#
# TODO: Clean up this makefile and try using gypi files with things like
# {
#   'GYP_DEFINES:' 'OS=ios',
#   'GYP_GENERATORS:' 'msvs',
#   'GYP_GENERATOR_FLAGS:' 'msvs_version=2010'
# }

pname:=ktx
# Formats to generate by default.
formats=msvs xcode # make

builddir:=build

# GYP only updates solution & project files that have changed since
# the last time they were generated. It is impractical to list
# as dependencies herein all the output files of a given GYP generator
# so use a file with the timestamp of the latest generation to ensure
# gyp is only run when necessary.
stampfile:=.${pname}-stamp

msvs_buildd:=$(builddir)/msvs
msvs_platforms:=web win wingl
msvs_vernames:=vs2010 vs2010e vs2008 vs2013
# Build list of names "${platform}/vs{2010,2010e,2008}/.ktx-stamp"
msvs_targets=$(addprefix ${platform}/,$(addsuffix /${stampfile},${msvs_vernames}))
msvs_platform_dirs:=$(addprefix ${msvs_buildd}/,${msvs_platforms})
# Expand the ${platform} in each of name in msvs_targets
msvs_targets:=$(foreach platform,${msvs_platform_dirs},${msvs_targets})

xcode_buildd:=$(builddir)/xcode
xcode_platforms:=ios macgl# mac
xcode_targets=$(addsuffix /${stampfile},$(addprefix ${xcode_buildd}/,${xcode_platforms}))

# Used by msvs recipe to extract version number from the partial target
# name set in $* by the pattern rule. Loops over the list of platform
# names attempting to match and delete it. if findstring is necessary
# because subst returns the full string in the event of no match.
msvs_version=$(strip $(foreach platform,${msvs_platforms},$(if $(findstring ${platform},$*),$(subst ${platform}/vs,,$*))))

gypfiles=ktx.gyp \
		 gyp_include/config.gypi \
		 gyp_include/libgl.gypi \
		 gyp_include/libgles1.gypi \
		 gyp_include/libgles2.gypi \
		 gyp_include/libgles3.gypi \
		 gyp_include/libsdl.gypi \
		 lib/libktx_core.gypi \
		 lib/libktx_gl.gypi \
		 lib/libktx_es3.gypi \
		 tests/tests.gypi \
		 tests/loadtests/appfwSDL/appfwSDL.gypi \
		 tests/loadtests/loadtests.gypi \

# Uncomment these 2 lines if you do not want to install our modified GYP.
# Set gypdir to the directory containing the modified GYP.
# PYTHONPATH is inserted at start of python's search path thus the second
# line ensures the modified GYP is used.
#gypdir=tools/gyp/
#export PYTHONPATH=$(gypdir)pylib

# Normal use
gyp=$(gypdir)gyp# --debug=all

# Use this to start in the Python debugger
#gyp=python -m pdb $(gypdir)gyp_main.py

# Use this if running in a Mac OS X terminal with the current
# directory set to an SVN workarea in Windows that is being
# viewed via a Parallels (or VMWare?) VM. The explicit python
# command is necessary because the gyp shell script does not
# appear have execute permission in this case.
#gyp=python $(gypdir)/gyp_main.py

.PHONY: msvs xcode

default: $(formats)

msvs: $(msvs_targets)

xcode: $(xcode_targets)

#android: $(android_targets)

# Can't use wildcards in target patterns so have to match the whole
# {win+web,wingl}/vs<version> part of the target name. Uses the
# msvs_version macro above to extract the version.
$(msvs_targets): $(msvs_buildd)/%/$(stampfile): $(gypfiles)
	$(gyp) -f msvs -DUSE_GL=$(use_gl) -G msvs_version=$(msvs_version) --generator-output=$(dir $@) --depth=. $(pname).gyp
	@date > $@


$(xcode_targets): $(xcode_buildd)/%/$(stampfile): $(gypfiles)
	$(gyp) -f xcode -DOS=$(patsubst %gl,%,$*) --generator-output=$(dir $@) --depth=. $(pname).gyp
	@date > $@

# vim:ai:ts=4:sts=4:sw=2:textwidth=75

