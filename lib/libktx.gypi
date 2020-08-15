# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project to build KTX library for OpenGL.
#
{
  'variables': {
    'sources': [
      # .h files are included so they will appear in IDEs' file lists.
      '../include/ktx.h',
      'basisu/apg_bmp.c',
      'basisu/apg_bmp.h',
      'basisu/basisu_astc_decomp.cpp',
      'basisu/basisu_astc_decomp.h',
      'basisu/basisu_backend.cpp',
      'basisu/basisu_backend.h',
      'basisu/basisu_basis_file.cpp',
      'basisu/basisu_basis_file.h',
      'basisu/basisu_bc7enc.cpp',
      'basisu/basisu_bc7enc.h',
      'basisu/basisu_comp.cpp',
      'basisu/basisu_comp.h',
      'basisu/basisu_enc.cpp',
      'basisu/basisu_enc.h',
      'basisu/basisu_etc.cpp',
      'basisu/basisu_etc.h',
      'basisu/basisu_frontend.cpp',
      'basisu/basisu_frontend.h',
      'basisu/basisu_global_selector_palette_helpers.cpp',
      'basisu/basisu_global_selector_palette_helpers.h',
      'basisu/basisu_gpu_texture.cpp',
      'basisu/basisu_gpu_texture.h',
      'basisu/basisu_pvrtc1_4.cpp',
      'basisu/basisu_pvrtc1_4.h',
      'basisu/basisu_resampler.cpp',
      'basisu/basisu_resampler.h',
      'basisu/basisu_resample_filters.cpp',
      'basisu/basisu_resampler_filters.h',
      'basisu/jpgd.cpp',
      'basisu/jpgd.h',
      'basisu/lodepng.cpp',
      'basisu/lodepng.h',
      'basisu/transcoder/basisu.h',
      'basisu/transcoder/basisu_file_headers.h',
      'basisu/transcoder/basisu_transcoder.cpp',
      'basisu/transcoder/basisu_transcoder.h',
      'basisu/transcoder/basisu_transcoder_internal.h',
      'basisu/transcoder/basisu_transcoder_tables_astc.inc',
      'basisu/transcoder/basisu_transcoder_tables_astc_0_255.inc',
      'basisu/transcoder/basisu_transcoder_tables_atc_55.inc',
      'basisu/transcoder/basisu_transcoder_tables_atc_56.inc',
      'basisu/transcoder/basisu_transcoder_tables_bc7_m5_alpha.inc',
      'basisu/transcoder/basisu_transcoder_tables_bc7_m5_color.inc',
      'basisu/transcoder/basisu_transcoder_tables_dxt1_5.inc',
      'basisu/transcoder/basisu_transcoder_tables_dxt1_6.inc',
      'basisu/transcoder/basisu_transcoder_tables_pvrtc2_45.inc',
      'basisu/transcoder/basisu_transcoder_tables_pvrtc2_alpha_33.inc',
      'basisu/basisu_uastc_enc.cpp',
      'basisu/basisu_uastc_enc.h',
      'basis_sgd.h',
      'basis_encode.cpp',
      'basis_transcode.cpp',
      'checkheader.c',
      'dfdutils/createdfd.c',
      'dfdutils/dfd.h',
      'dfdutils/dfd2vk.c',
      'dfdutils/dfd2vk.inl',
      'dfdutils/interpretdfd.c',
      'dfdutils/printdfd.c',
      'dfdutils/queries.c',
      'dfdutils/vk2dfd.c',
      'dfdutils/vk2dfd.inl',
      'etcdec.cxx',
      'etcunpack.cxx',
      'filestream.c',
      'filestream.h',
      'formatsize.h',
      'gl_format.h',
      'gl_funclist.inl',
      'gl_funcs.c',
      'gl_funcs.h',
      'glloader.c',
      'hashlist.c',
      'info.c',
      'ktxint.h',
      'memstream.c',
      'memstream.h',
      'stream.h',
      'strings.c',
      'swap.c',
      'texture.c',
      'texture.h',
      'texture_funcs.inl',
      'texture1.c',
      'texture1.h',
      'texture2.c',
      'texture2.h',
      'uthash.h',
      'vkformat_enum.h',
      'vkformat_check.c',
      'vkformat_str.c',
      'writer1.c',
      'writer2.c',
    ],
    # Use _files to get the names relativized
    'vksource_files': [
      '../include/ktxvulkan.h',
      'vk_format.h',
      'vkloader.c',
      'vk_funclist.inl',
      'vk_funcs.c',
      'vk_funcs.h'
    ],
    'include_dirs': [
      'dfdutils',
      '../include',
      '../other_include',
    ],
    'version_file': 'version.h',
  }, # variables

  'includes': [
    '../gyp_include/libvulkan.gypi',
    '../gyp_include/libzstd.gypi',
  ],

  'xcode_settings': {
      # These are actually Xcode's defaults shown here for documentation.
      #'DSTROOT': '/tmp/$(PROJECT_NAME).dst',
      #'INSTALL_PATH': '/usr/local/lib',
      # Override DSTROOT to use same place for lib & tools.
      'DSTROOT': '/tmp/ktx.dst',
  },

  'targets': [
    {
      'target_name': 'version.h',
      'type': 'none',
      'actions': [{
        'action_name': 'mkversion',
        'inputs': [
          '../mkversion',
          '../.git'
        ],
        'outputs': [ '<(version_file)' ],
        'msvs_cygwin_shell': 1,
        'action': [ './mkversion', '-o', 'version.h', 'lib' ],
      }],
    },
    {
      'target_name': 'libktx',
      'type': '<(library)',
      # To quiet warnings about the anon structs and unions in Basisu.
      'cflags_cc': [ '-Wno-pedantic' ],
      'defines': [
        'KHRONOS_STATIC=1',
        'LIBKTX=1',
        # To reduce size, don't support transcoding to ancient formats.
        'BASISD_SUPPORT_FXT1=0',
      ],
      'direct_dependent_settings': {
        'include_dirs': [ '<@(include_dirs)' ],
      },
      'include_dirs': [ '<@(include_dirs)' ],
      'mac_bundle': 0,
      'dependencies': [ 'libzstd', 'version.h' ],
      'sources': [
        '<@(sources)',
        '<@(vksource_files)',
      ],
      'link_settings': {
        'conditions': [
          ['OS == "linux"', {
            'libraries': [ '-ldl', '-lpthread' ],
          }],
        ],
      },
      'conditions': [
        ['_type == "shared_library"', {
          'defines!': ['KHRONOS_STATIC=1'],
          'conditions': [
            ['OS == "mac" or OS == "ios"', {
              'direct_dependent_settings': {
                'target_conditions': [
                  ['_type != "none" and _mac_bundle == 1', {
                    'copies': [{
                      'xcode_code_sign': 1,
                      'destination': '<(PRODUCT_DIR)/$(FRAMEWORKS_FOLDER_PATH)',
                      'files': [ '<(PRODUCT_DIR)/<(_target_name)<(SHARED_LIB_SUFFIX)' ],
                    }], # copies
                    'xcode_settings': {
                      # Tell DYLD where to search for this dylib.
                      # "man ld" for more information. Look for -rpath.
                      'LD_RUNPATH_SEARCH_PATHS': [ '@executable_path/../Frameworks' ],
                    },
                  }, {
                    'xcode_settings': {
                      'LD_RUNPATH_SEARCH_PATHS': [
                        '@executable_path',
                        '/usr/local/lib',
                      ],
                    },
                  }], # _mac_bundle == 1
                ], # target_conditions
              }, # direct_dependent_settings
              'xcode_settings': {
                # Set the "install name" to instruct dyld to search a list of
                # paths in order to locate the library. If left at the default
                # of an absolute location (/usr/local/lib), that path will be
                # built into any executables that link with it and dyld will
                # search only that location. For bundles, the path we set above
                # is built into the executable. For non bundle's nothing will be
                # built into the executable and the standard dyld search path
                # will be used.
                'INSTALL_PATH': '@rpath',
              }
            }, 'OS == "win"', {
              'defines': [
                'KTX_APICALL=__declspec(dllexport)',
                'BASISU_NO_ITERATOR_DEBUG_LEVEL',
              ],
              # The msvs generator automatically sets the needed VCLinker
              # option when a .def file is seen in sources.
              'sources': [ 'internalexport.def' ],
            }] # OS == "mac or OS == "ios"
          ], # conditions
        }, {
          'conditions': [
            ['OS == "web"', {
              # The zstd decoder does not use macros with variadic macros
              # correctly and they seem unwilling to fix so turn those
              # off too.
              'cflags_c': [
                '-Wno-gnu-zero-variadic-macro-arguments',
              ],
              'defines': [
                'KTX_OMIT_VULKAN=1',
                # To reduce size, don't support transcoding to formats not
                # supported # by WebGL.
                'BASISD_SUPPORT_BC7=0',
                'BASISD_SUPPORT_ATC=0',
                'BASISD_SUPPORT_PVRTC2=0',
                'BASISD_SUPPORT_FXT1=0',
                'BASISD_SUPPORT_ETC2_EAC_RG11=0',
                # Don't support higher quality mode to avoid 64k table.
                'BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0',
              ],
              'dependencies!': [ 'libzstd' ],
              'sources!': [ '<@(vksource_files)' ],
              'sources': [ 'zstddeclib.c' ],
            }, {
              'export_dependent_settings': [ 'libzstd' ],
            }],
          ]
        }], # _type == "shared_library"
      ], # conditions
      'xcode_settings': {
        # The BasisU transcoder uses anon types and structs. They compile ok in
        # Visual Studio (2015+) and on Linux so quiet the clang warnings.
        'WARNING_CFLAGS': [
          '-Wno-nested-anon-types',
          '-Wno-gnu-anonymous-struct',
        ],
        # This is used by a Copy Headers phase which gyp only allows to be
        # be created for a framework bundle. Remember in case we want to
        # switch the lib to a framework.
        #'PUBLIC_HEADERS_FOLDER_PATH': '/usr/local/include',
      },
    }, # libktx target
  ],
  'conditions': [
    ['OS == "mac" or OS == "win" or OS == "linux"', {
      'targets': [
        {
          'target_name': 'libktx.doc',
          'type': 'none',
          'variables': {
            'variables': { # level 2
              'output_dir': '../build/docs',
            },
            'output_dir': '<(output_dir)',
            'doxyConfig': 'libktx.doxy',
            'timestamp': '<(output_dir)/.libktx_gentimestamp',
          },
          'actions': [
            {
              'action_name': 'buildLibktxDoc',
              'message': 'Generating libktx documentation with Doxygen',
              'inputs': [
                '../<(doxyConfig)',
                '../runDoxygen',
                '../lib/mainpage.md',
                '../LICENSE.md',
                '../TODO.md',
                '<@(sources)',
                '<@(vksource_files)',
              ],
              # If other partial Doxygen outputs are included, e.g.
              # (<(output_dir)/html/libktx), CMake's make generator
              # on Linux (at least), makes timestamp dependent on
              # those other outputs. If those outputs exist, then
              # neither timestamp nor the document is updated.
              'outputs': [ '<(timestamp)' ],
              # doxygen must be run in the top-level project directory
              # so that ancestors of that directory will be removed
              # from paths displayed in the documentation. That is
              # the directory where the .doxy and .gyp files are stored.
              #
              # With Xcode, the current directory during project
              # build is one we need so we're good to go. However
              # we need to spawn another shell with -l so the
              # startup (.bashrc, etc) files will be read.
              #
              # With MSVS the working directory will be the
              # location of the vcxproj file. However when the
              # action is using bash ('msvs_cygwin_shell': '1',
              # the default, is set) no path relativization is
              # performed on any command arguments. If forced, by
              # using variable names such as '*_dir', paths will be
              # made relative to the location of the .gyp file.
              #
              # A setup_env.bat file is run before the command.
              # Apparently that .bat file is expected to be in the
              # same location as the .gyp and to cd to
              # its directory. That makes things work.
              #
              # Note that the same setup_env.bat is run by
              # rules but rules relativize paths to the vcxproj
              # location so cd to the .gyp home breaks rules.
              # Therefore in rules set 'msvs_cygwin_shell': '0.
              #
              # If using cmd.exe ('msvs_cygwin_shell': '0')
              # the MSVS generator will relativize to the vcxproj
              # location *all* command arguments, that do not look
              # like options.
              #
              # With `make`, cmake, etc, like Xcode,  the current
              # directory during project build is the one we need.
              'msvs_cygwin_shell': 1,
              'action': [
                './runDoxygen',
                '-t', '<(timestamp)',
                '-o', '<(output_dir)/html',
                '<(doxyConfig)',
              ],
            }, # buildLibktxDoc action
          ], # actions
          # For 'version.h'.
          'dependencies': [ 'version.h' ],
        }, # libktx.doc
        {
          'target_name': 'mkvkformatfiles',
          'type': 'none',
          'variables': {
            'vkformatfiles_dir': '.',
#  Use local vulkan_core.h until ASTC 3D extension is released.
#            'conditions': [
#              ['GENERATOR == "cmake"', {
#                # FIXME Need to find a way to use $VULKAN_SDK *if* set.
#                'vkinclude_dir': '/usr/include',
#              }, {
#                'vkinclude_dir': '$(VULKAN_SDK)/include',
#              }],
#            ], # conditions
            'vkinclude_dir': 'dfdutils'
          },
          'actions': [
            {
              'action_name': 'run_mkvkformatfiles',
              'message': 'Generating VkFormat-related source files',
              'inputs': [
                '<(vkinclude_dir)/vulkan/vulkan_core.h',
                'mkvkformatfiles',
              ],
              'outputs': [
                'vkformat_enum.h',
                'vkformat_check.c',
                'vkformat_str.c',
              ],
              # The current directory during project is that of
              # the .gyp file. See above. Hence the annoying "lib/"
              'msvs_cygwin_shell': 1,
              'action': [
                'lib/mkvkformatfiles', '<(vkformatfiles_dir)',
              ],
            }, # run mkvkformatfiles action
            {
              'action_name': 'run_makevkswitch',
              'message': 'Generating VkFormat/DFD switch body',
              'inputs': [
                'vkformat_enum.h',
                #'<(vkinclude_dir)/vulkan/vulkan_core.h',
                'dfdutils/makevkswitch.pl',
              ],
              'outputs': [
                'dfdutils/vk2dfd.inl',
              ],
              # The current directory during this action is that of
              # the .gyp file. See above. Hence the annoying "lib/"
              'msvs_cygwin_shell': 1,
              'action': [
                'lib/dfdutils/makevkswitch.pl',
                '<@(_inputs)',
                '<@(_outputs)',
              ],
            }, # run makevkswitch action
           {
            'action_name': 'run_makedfdtovk',
            'message': 'Generating DFD/VkFormat switch body',
            'inputs': [
              'vkformat_enum.h',
              #'<(vkinclude_dir)/vulkan/vulkan_core.h',
              'dfdutils/makedfd2vk.pl',
            ],
            'outputs': [
              'dfdutils/dfd2vk.inl',
            ],
            # The current directory during this action is that of
            # the .gyp file. See above. Hence the annoying "lib/"
            'msvs_cygwin_shell': 1,
            'action': [
              'lib/dfdutils/makedfd2vk.pl',
              '<@(_inputs)',
              '<@(_outputs)',
            ],
          }, # run makevkswitch action
         ], # actions
        }, # mkvkformatfiles
        {
          'target_name': 'install.lib',
          'type': 'none',
          # These variables duplicate those in ktxtools.gyp:install_tools.
          # See there for explanation.
          'variables': {
            'conditions': [
              ['GENERATOR == "msvs"', {
                'staticlib_dir': '<(PRODUCT_DIR)/lib',
              }, {
                'staticlib_dir': '<(PRODUCT_DIR)',
              }],
              ['GENERATOR == "cmake"', {
                'libktx_dir': '<(PRODUCT_DIR)/lib.target',
              }, {
                'libktx_dir': '<(PRODUCT_DIR)',
              }],
            ], # conditions
          }, # variables
          'dependencies': [ 'libktx', 'libktx.doc' ],
          'xcode_settings': {
            'INSTALL_PATH': '/usr/local',
          },
          'copies': [{
            'xcode_code_sign': 1,
            'destination': '<(dstroot)/<(installpath)/lib',
            'conditions': [
              ['OS == "win" or "<(library)" != "shared_library"', {
                'files': [ '<(staticlib_dir)/libktx<(STATIC_LIB_SUFFIX)' ],
              }],
              ['"<(library)" == "shared_library"', {
                'files': [ '<(libktx_dir)/libktx<(SHARED_LIB_SUFFIX)' ],
              }],
            ], # conditions
          }, {
            'destination': '<(dstroot)/<(installpath)/include',
            'files': [ '../include/ktx.h', '../include/ktxvulkan.h' ],
          }, {
            # Windows, unlike macOS, does not do a recursive copy
            # here. Darn! How to copy all these man pages.
            'destination': '<(dstroot)/<(installpath)/share/man',
            'files': [ '../build/docs/man/man3/' ],
          }]
        } # install_lib target
      ], # mac, win or linux targets
    }], # OS == "mac" or OS == "win" or OS == "linux"
  ], # conditions
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
