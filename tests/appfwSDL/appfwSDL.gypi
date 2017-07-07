##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building the application framework.
#
{
  'includes': [
     '../../gyp_include/libsdl.gypi',
  ],
  'targets': [
    {
      'target_name': 'appfwSDL',
      'type': 'static_library',
      #'toolsets': [target', 'emscripten'],
      'sources': [
        # .h files are included so they will appear in IDEs' file lists.
        'main.cpp',
        'AppBaseSDL.cpp',
        'AppBaseSDL.h',
        'GLAppSDL.cpp',
        'GLAppSDL.h',
      ],
      'dependencies': [ 'libsdl' ],
      'export_dependent_settings': [ 'libsdl' ],
      'link_settings': {
        'conditions': [
          ['OS == "android"', {
            'libraries': [ '-lstlport_static', ],
            'library_dirs': [ '<(cxx-stl)/stlport/libs/$(TARGET_ABI)' ],
          }]
        ]
      }, 

      # Ideally we should create a 'link_settings' here to set
      # "-s NO_EXIT_RUNTIME" here for Emscripten for html5 since the
      # no-loop framework needs the runtime to keep running when main
      # exits, but, as ldflags & VCLinker.AdditionalOptions are
      # strings, any setting made here will overwrite any settings
      # made by the application. Defer to the app for setting this.

    } # target appfwSDL
  ] #targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
