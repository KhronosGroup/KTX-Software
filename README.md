<!-- Copyright 2013-2020 Mark Callow -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

<img src="https://www.khronos.org/assets/images/api_logos/khronos.svg" width="300"/>

## The Official Khronos KTX Software Repository


| GNU/Linux, iOS, macOS & wasm | Windows | Android | Mingw |
| :--------------------------: | :-----: | :-----: | :---: |
| ![Build Status](https://app.travis-ci.com/KhronosGroup/KTX-Software.svg?token=4zN6rB3y3dunnF2sbsco&branch=main) | ![Build status](https://github.com/KhronosGroup/KTX-Software/actions/workflows/windows.yml/badge.svg) | ![KTX-Software CI](https://github.com/KhronosGroup/KTX-Software/actions/workflows/android.yml/badge.svg) | ![KTX-Software CI](https://github.com/KhronosGroup/KTX-Software/actions/workflows/mingw.yml/badge.svg) |

This is the official home of the source code for the Khronos KTX library and tools.

KTX (Khronos Texture) is a lightweight container for textures for OpenGL<sup>®</sup>, Vulkan<sup>®</sup> and other GPU APIs. KTX files contain all the parameters needed for texture loading. A single file can contain anything from a simple base-level 2D texture through to a cubemap array texture with mipmaps. Contained textures can be in a Basis Universal format, in any of the block-compressed formats supported by OpenGL family and Vulkan APIs and extensions or in an uncompressed single-plane format. Basis Universal currently encompasses two formats that can be quickly transcoded to any GPU-supported format: LZ/ETC1S, which combines block-compression and supercompression, and UASTC, a block-compressed format. Formats other than LZ/ETC1S can be supercompressed with Zstd and ZLIB.

Download [KTX Software Releases](https://github.com/KhronosGroup/KTX-Software/releases)
to get binary packages of the tools, library and development headers
described below. The [Releases](https://github.com/KhronosGroup/KTX-Software/releases) 
page also has packages with the Javascript wrappers and .wasm binaries.

See the Doxygen generated [live documentation](https://github.khronos.org/KTX-Software/)
for API and tool usage information.

The software consists of: (links are to source folders in the KhronosGroup repo)

- *libktx* - a small library of functions for writing and reading KTX
files, and instantiating OpenGL®, OpenGL ES™️ and Vulkan® textures
from them. [`lib`](https://github.com/KhronosGroup/KTX-Software/tree/main/lib)
- *libktx.{js,wasm}* - Web assembly version of libktx and
Javascript wrapper. [`interface/js_binding`](https://github.com/KhronosGroup/KTX-Software/tree/main/interface/js_binding)
- *msc\_basis\_transcoder.{js,wasm}* - Web assembly transcoder and
Javascript wrapper for Basis Universal formats. For use with KTX parsers written in Javascript. [`interface/js_binding`](https://github.com/KhronosGroup/KTX-Software/tree/main/interface/js_binding)
- *libktx.jar, libktx-jni* - Java wrapper and native interface library.
[`interface/java_binding`](https://github.com/KhronosGroup/KTX-Software/tree/main/interface/java_binding)
- *ktx* - a generic command line tool for managing KTX2 files with subcommands.[`tools/ktx`](https://github.com/KhronosGroup/KTX-Software/tree/main/tools/ktx)
  - *ktx compare* - Compare two KTX2 files
  - *ktx create* - Create a KTX2 file from various input files
  - *ktx deflate* - Deflate a KTX2 file with zstd or ZLIB
  - *ktx extract* - Export selected images from a KTX2 file
  - *ktx encode* - Encode a KTX2 file
  - *ktx transcode* - Transcode a KTX2 file
  - *ktx info* - Prints information about a KTX2 file
  - *ktx validate* - Validate a KTX2 file
  - *ktx help* - Display help information about the ktx tools
- *ktx2check* - a tool for validating KTX Version 2 format files. [`tools/ktx2check`](https://github.com/KhronosGroup/KTX-Software/tree/main/tools/ktx2check)
- *ktx2ktx2* - a tool for converting a KTX Version 1 file to a KTX
Version 2 file. [`tools/ktx2ktx2`](https://github.com/KhronosGroup/KTX-Software/tree/main/tools/ktx2ktx2)
- *ktxinfo* - a tool to display information about a KTX file in
human readable form. [`tools/ktxinfo`](https://github.com/KhronosGroup/KTX-Software/tree/main/tools/ktxinfo)
- *ktxsc* - a tool to supercompress a KTX Version 2 file that
contains uncompressed images.[`tools/ktxsc`](https://github.com/KhronosGroup/KTX-Software/tree/main/tools/ktxsc)
- *pyktx* - Python wrapper
- *toktx* - a tool to create KTX files from PNG, Netpbm or JPEG format images. It supports mipmap generation, encoding to
Basis Universal formats and Zstd supercompression.[`tools/toktx`](https://github.com/KhronosGroup/KTX-Software/tree/main/tools/toktx)

See [CONTRIBUTING](CONTRIBUTING.md) for information about contributing.

See [LICENSE](LICENSE.md) for information about licensing.

See [BUILDING](BUILDING.md) for information about building the code.

<!--
More information about KTX and links to tools that support it can be
found on the
[KTX page](http://www.khronos.org/opengles/sdk/tools/KTX/) of
the [OpenGL ES SDK](http://www.khronos.org/opengles/sdk) on
[khronos.org](http://www.khronos.org).
-->

If you need help with using the KTX library or KTX tools, please use GitHub
[Discussions](https://github.com/KhronosGroup/KTX-Software/discussions).
To report problems use GitHub [issues](https://github.com/KhronosGroup/KTX/issues).

**IMPORTANT:** you **must** install the [Git LFS](https://github.com/github/git-lfs)
command line extension in order to fully checkout this repository after cloning. You
need at least version 1.1. If you did not have Git LFS installed at first checkout
then, after installing it, you **must** run

```bash
git lfs checkout
```

### KTX-Software-CTS - Conformance Test Suite

The tests and test files for the generic command line `ktx` tool can be found in a separate
[CTS Repository](https://github.com/KhronosGroup/KTX-Software-CTS/). To save space and bandwidth this repository
is included with git submodule and by default it is not required for building the libraries or the tools.
For more information about building, running and extending the CTS tests see [BUILDING](BUILDING.md#Conformance-Test-Suite) 
and [CTS README](https://github.com/KhronosGroup/KTX-Software-CTS/blob/main/README.md).

### <a id="kwexpansion"></a>$Date$ keyword expansion

A few files have `$Date$` keywords. If you care about having the proper
dates shown or will be generating the documentation or preparing
distribution archives, you **must** follow the instructions below.

$Date$ keywords are expanded via smudge & clean filters. To install
the filters, issue the following commands in the root of your clone.

On Unix (Linux, Mac OS X, etc.) platforms and Windows using Git for
Windows' Git Bash or Cygwin's bash terminal:

```bash
./install-gitconfig.sh
./scripts/smudge_date.sh

```

On Windows PowerShell (requires `git.exe` in a directory
on your %PATH%):

```ps1
install-gitconfig.ps1
./scripts/smudge_date.ps1
```

The first command adds an [include] of the repo's `.gitconfig` to the
local git config file `.git/config`, i.e. the one in your clone of the repo.
`.gitconfig` contains the config of the "keyworder" filter. The script in
the second command forces a new checkout of the affected files to smudge them
with their last modified date. This is unnecessary if you plan to edit
these files.

### Useful Tools

#### scripts/gk

For finding strings within the KTX-Software source. Type `scripts/gk -h` for help. `gk` avoids looking in any build directories, `.git`, `external` or `tests/cts`.

#### scripts/ktx-compare-git

Wrapper that allows use of `ktx compare` when using `git diff` on KTX2 files.
Together with this, `.gitconfig` now includes a *ktx-compare* diff command.
Those wishing to use this must run, or have run, install-gitconfig.{ps1,sh}
as described above for keyword expansion so that `.gitconfig` is
included by your local `.git/config`.
    
You need to have the `ktx` command installed in a directory on your $PATH.
    
You need to add the line

``` 
*.ktx2 binary diff=ktx-compare
```
  
to your repo clone's `.git/info/attributes`. This is not included in the repo's
`.gitattributes` because not everyone will have the `ktx` command installed nor have `.gitconfig` included by `.git/config`.

*NOTE:* This line in a user-global or system-global Git attributes file will not 
work because those are lower priority than `.gitattributes` so are read first
and `.gitattributes` already has an entry for *.ktx2, indicating binary, which
overrides anything from the global files.

To set up your `tests/cts` submodule to use this, copy the *ktx-compare* diff
command to `.git/modules/tests/cts/config`. Depending on when you set up the
submodule and when you ran `install-gitconfig.sh`, it may already be there.
Add the attribute line to `.git/modules/tests/cts/info/attributes`.

We will be happy to accept a PR to add a .ps1 equivalent script.

