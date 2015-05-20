##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project to build KTX library for OpenGL ES 3.
#
{
 # As writer.c does not need OpenGL, do not add a dependency on
 # OpenGL{, ES} here.
 'targets': [
  {
    'target_name': 'libktx_es3',
    'includes': [
      'libktx_core.gypi',
    ],
    'variables': {
      # Because these must be specified in two places.
      'defines': [ 'KTX_OPENGL_ES3=1' ],
    },
    'direct_dependent_settings': {
       'defines': [ '<@(defines)' ],
    },
    'defines': [ '<@(defines)' ],
  }], # libktx_es3 target && targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
