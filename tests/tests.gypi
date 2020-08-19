# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project files for building KTX tests.
#
{
  'includes': [
     'loadtests/loadtests.gypi',
     'testimages/testimages.gypi',
  ],
  'conditions': [
    ['OS == "mac" or OS == "win" or OS == "linux"', {
      'includes': [
        'gtest/gtest.gypi',
        'texturetests/texturetests.gypi',
        'transcodetests/transcodetests.gypi',
        'unittests/unittests.gypi',
      ]
    }]
  ]
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
