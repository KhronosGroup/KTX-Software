#
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on OpenGL ES 3.
#
{
  'includes': [
    'config.gypi',
  ],
  'targets': [
  {
    'target_name': 'libgles3',
    'type': 'none',
    'direct_dependent_settings': {
      'include_dirs': [
        '<(gl_includes_parent_dir)',
      ]
    },
    'conditions': [
      ['OS == "win"', {
        # This is for the ARM MALI emulator whose installation adds
        # OPENGLES_LIBDIR to the environment and adds its value to
        # PATH so the dlls will be found.
        'variables' : {
          #'dlls': [
          #  '$(OPENGLES_LIBDIR)/libEGL.dll',
          #  '$(OPENGLES_LIBDIR)/libGLESv2.dll',
          #  '$(OPENGLES_LIBDIR)/d3dcompiler_46.dll',
          #],
          'lib_dirs': [ '$(OPENGLES_LIBDIR)' ],
          'conditions': [
            ['GENERATOR == "msvs"', {
              'libs': ['-llibGLESv2', '-llibEGL'],
            }, {
              'libs': ['-lGLESv2', '-lEGL'],
            }],
          ],
        }, # variables
        # Configuration dependent copies are not possible, nor are
        # configuration dependent sources. Hence use of $(PlatformName)
        # that is set by the build environment. NOTE: $(PlatformName)
        # may not work with the make generator.
        #'copies': [{
          # Files appearing in 'copies' cause gyp to generate a folder
          # hierarchy in Visual Studio filters reflecting the location
          # of each file. The folders will be empty.
        #  'destination': '<(PRODUCT_DIR)',
        #  'files': [
        #    '<@(dlls)',
        #  ],
        #}],
      }], # OS == "win"
      ['OS == "ios"', {
        'variables': {
          'lib_dirs': [ ],
          'libs': ['$(SDKROOT)/System/Library/Frameworks/OpenGLES.framework'],
        },
      }],
      ['OS == "android"', {
        'variables': {
          'lib_dirs': [ ],
          'libs': ['-lGLESv2', '-lEGL'],
        }
      }],
    ], # conditions
    'link_settings': {
      'libraries': [ '<@(libs)' ],
      'library_dirs': [ '<@(lib_dirs)' ],
    }
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
