##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Master for generating project files for building KTX library,
#        tools and tests.
#
# The command for generating msvs project files from this .gyp file is:
#   gyp -f msvs -G msvs_version=<vername> --generator-output=build/msvs --depth=. ktx.gyp
# where <vername> is one of 2005, 2005e, 2008, 2008e, 2010, 2010e, 2012,
# 2012e, 2013 or 2013e. The suffix 'e' generates a project for the express edition.
#
# Other currently available formats are cmake, make, ninja and xcode.
#
# Is there a way to set --generator-output=DIR and --format=FORMATS a.k.a
# -f FORMATS from within the GYP file?  I'd like people to just be able
# to run gyp on this file.
#
{
  'includes': [
     'gyp_include/config.gypi',
     'gyp_include/default.gypi',
     'tests/tests.gypi',
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
