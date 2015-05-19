##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on OpenGL.
#
{
  'includes': [
    'config.gypi',
  ],
  'targets': [
  {
    'target_name': 'libgl',
    'type': 'none',
    'direct_dependent_settings': {
      'include_dirs': [
        '<(gl_includes_parent_dir)',
        #'../other_include',
      ]
    },
    'variables': {
      'conditions': [
        ['OS == "win"', {
          # None of 'copies', 'link_settings' or libraries can appear inside
          # configurations hence use of $(PlatformName), which is used
          # instead of $(Platform) for compatibility with VS2005. 
          'winolib_dir': '<(otherlibroot_dir)/$(PlatformName)',
          'lib_dirs': [ '<(winolib_dir)' ],
          'conditions': [
            ['GENERATOR == "msvs"', {
              'libs': [ '-lopengl32.lib' ],
            }, {
              'libs': [ '-lgl'],
            }],
          ],
        }, 'OS == "android" or OS == "ios"', {
          # OpenGL not supported on these platforms.
          # XXX How to get GYP to print a useful error? An "undefined
          # variable 'libs' in ktx.gyp" error is currently emitted.
        }, 'OS == "mac"', {
          'lib_dirs': [ ],
          'libs': ['$(SDKROOT)/System/Library/Frameworks/OpenGL.framework'],
       }],
      ],
    }, # variables
    'link_settings': {
     'libraries': [ '<@(libs)' ],
     'library_dirs': [ '<@(lib_dirs)' ],
    },
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
