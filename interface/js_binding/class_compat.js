// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab textwidth=80:

//
// Copyright 2024 Khronos Group, Inc.
// SPDX-License-Identifier: Apache-2.0
//

// Provide old name for backward compatibility.

Module.onRuntimeInitialized = function() {
  Module['ktxTexture'] = Module.texture;
  Module['ErrorCode'] = Module.error_code;
  Module['TranscodeTarget'] = Module.transcode_fmt;
  Module['TranscodeFlags'] = Module.transcode_flag_bits;
}

