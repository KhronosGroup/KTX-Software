/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=80: */

/*
 * Copyright 2025 Khronos Group, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file post_ready.js
 * @~English
 *
 * @brief Implements Module.ready Promise pattern for proper async initialization.
 *
 * This file is injected via --post-js to add a Module.ready Promise that resolves
 * when the WebAssembly runtime is fully initialized. This is essential for ES6
 * modules and async initialization patterns used by modern bundlers.
 *
 * Usage:
 * @code{.js}
 * import createKtxModule from './libktx.mjs';
 *
 * const ktx = await createKtxModule({
 *   locateFile: (path) => `/libs/${path}`
 * });
 *
 * await ktx.ready;  // Wait for WASM initialization
 *
 * // Now ready to use
 * const texture = new ktx.texture(data);
 * @endcode
 */

if (!Module.ready) {
  var readyPromiseResolve, readyPromiseReject;
  Module.ready = new Promise(function(resolve, reject) {
    readyPromiseResolve = resolve;
    readyPromiseReject = reject;
  });

  var originalOnRuntimeInitialized = Module.onRuntimeInitialized;
  Module.onRuntimeInitialized = function() {
    try {
      if (originalOnRuntimeInitialized) {
        originalOnRuntimeInitialized.call(Module);
      }
      if (readyPromiseResolve) {
        readyPromiseResolve(Module);
      }
    } catch (err) {
      if (readyPromiseReject) {
        readyPromiseReject(err);
      }
      throw err;
    }
  };
}
