##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# @brief Create variables listing libktx source files.
#
{
  'variables': {
    'sources': [
      # .h files are included so they will appear in IDEs' file lists.
      '../include/ktx.h',
      'checkheader.c',
      'errstr.c',
      'etcdec.cxx',
      'etcunpack.cxx',
      'filestream.c',
      'filestream.h',
      'gl_format.h',
      'gl_funcptrs.h',
      'gles1_funcptrs.h',
      'gles2_funcptrs.h',
      'gles3_funcptrs.h',
      'glloader.c',
      'hashlist.c',
      'hashtable.c',
      'ktxint.h',
      'memstream.c',
      'memstream.h',
      'stream.h',
      'swap.c',
      'texture.c',
      'uthash.h',
      'writer.c',
      'writer_v1.c'
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
  }, # variables
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
