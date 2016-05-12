![logo](http://www.khronos.org/assets/images/khronos-group-logo.png)
The Official Khronos KTX Repository
---

|                      |  master  |  incoming  |
|----------------------| :------: | :--------: |
| GNU/Linux, iOS & OSX | [![Build Status](https://travis-ci.org/KhronosGroup/KTX.svg?branch=master)](https://travis-ci.org/KhronosGroup/KTX) | [![Build Status](https://travis-ci.org/KhronosGroup/KTX.svg?branch=incoming)](https://travis-ci.org/KhronosGroup/KTX) |
| Windows | [![Build status](https://ci.appveyor.com/api/projects/status/rj9bg8g2jphg3rc0/branch/master?svg=true)](https://ci.appveyor.com/project/msc-/ktx) | [![Build status](https://ci.appveyor.com/api/projects/status/rj9bg8g2jphg3rc0/branch/incoming?svg=true)](https://ci.appveyor.com/project/msc-/ktx) |

This is the offical home of the source code
for the Khronos KTX library and tools.

Download packages and live documentation can be
found on the [KTX page](http://www.khronos.org/opengles/sdk/tools/KTX/) of
the [OpenGL ES SDK](http://www.khronos.org/opengles/sdk) on
[khronos.org](http://www.khronos.org).

See [CONTRIBUTING](CONTRIBUTING.md) for information about contributing.

See [LICENSE](LICENSE.md) for information about licensing.

See [BUILDING](BUILDING.md) for information about building the code.

If you need help with using KTX please use the [KTX forum](https://forums.khronos.org/forumdisplay.php/103-KTX-file-format-for-OpenGL-OpenGL-ES-and-WebGL-textures). To report problems use the GitHub [issues](https://github.com/KhronosGroup/KTX/issues).

**IMPORTANT:** you **must** install the [Git LFS](https://github.com/github/git-lfs)
command line extension in order to fully checkout this repository after cloning. You
need at least version 1.1.

A few files have `$Date$` keywords. If you care about having the proper
dates shown or will be generating the documentation or preparing
distribution archives, you **must** follow the instructions below.

#### <a id="kwexpansion"></a>$Date$ keyword expansion

$Date$ keywords are expanded via a smudge & clean filter. To install
the filter, issue the following commands in the root of your clone.

On Unix (Linux, Mac OS X, etc.) platforms and Windows using Git for Windows'
Git Bash or Cygwin's bash terminal:

```bash
./install-gitconfig.sh
rm TODO.md include/ktx.h tools/toktx/toktx.cpp
git checkout TODO.md include/ktx.h tools/toktx/toktx.cpp
```

On Windows with the Command Prompt (requires `git.exe` in a directory
on your %PATH%):

```cmd
install-gitconfig.bat
del TODO.md include/ktx.h tools/toktx/toktx.cpp
git checkout TODO.md include/ktx.h tools/toktx/toktx.cpp 
```

The first command adds an [include] of the repo's `.gitconfig` to the
local git config file `.git/config`, i.e. the one in your clone of the repo.
`.gitconfig` contains the config of the "keyworder" filter. The remaining
commands force a new checkout of the affected files to smudge them with the
date. These two are unnecessary if you plan to edit these files.

