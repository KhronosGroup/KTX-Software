// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab textwidth=80:

//
// Copyright 2019-2024 Khronos Group, Inc.
// SPDX-License-Identifier: Apache-2.0
//

// Provide old module create function name for backward compatibility.
// N.B. --pre-js and --post-js code is run inside the module creation
// function so the variable holding that function is not available.
// This has to be --extern-post-js.

var LIBKTX
if (typeof createKtxReadModule === "function")
  LIBKTX = createKtxReadModule;
else
  LIBKTX = createKtxModule;
