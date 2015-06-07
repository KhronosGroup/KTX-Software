
Building KTX
============

Status
------

Construction of new build system and portable loader tests is
underway.  At present only the Mac OS X and iOS builds are completed.
Windows will follow soon. This document is about these new items.

Dependencies
------------

The KTX project uses GYP to generate project files. You only need
GYP if you want to re-generate the supplied projects or generate
additional projects.

The KTX loader tests use libSDL 2.0.4. You do not need SDL if you
only wish to build the KTX library. 2.0.4 is necessary for Android. 2.0.3 is acceptable on OS X.


SDL
---

Builds of SDL are provided in the Git repo.

### Mac OS X

To use SDL on OS X, open a shell and enter the following command

```bash
cp -R other_lib/mac/<configuration>/SDL2.framework /Library/Frameworks
```

replacing `<configuration>` with your choice of `Debug` or `Release`. There
are alternative ways of using the provided SDL2.framework. See
`gyp_include/config.gypi` for details. You will have to regenerate the xcode
project if you wish to use one of these alternatives.

NOTE: xcode crashes when using the SDL 2.0.3 binary distributed by
https://libsdl.org. This is due to an issue with the code signature. The
same problem may affect the SDL2.framework included with KTX. The problem
and solution is described at [stackoverflow]
(https://stackoverflow.com/questions/22368202/xcode-5-crashes-when-running-an-app-with-sdl-2).
If the problem occurs, use `codesign` to fix it, as described in the second
answer.

### iOS

Nothing need be done.

### Windows

Nothing need be done.

### Building SDL from source

~If you want to build libSDL 2.0.4 from source, clone the fork of the libsdl.org repo at https://github.com/...~ This fork is not yet up on GitHub. For now clone the repo at libsdl.org using `git-remote-hg` as detailed below.

This repo has the libSDL.org Mercurial repo as a remote. To pull
from that repo, you need to install Mercurial and have the script
`git-remote-hg` in your `$PATH`.

The example below copies `git-remote-hg` to `/usr/local/bin`. Note
this URL is a fork with fixes for compability with Mercurial 3.2+.
The origin is at `s/fingolfin/felipec/`.

```bash
sudo curl -o /usr/local/bin/git-remote-hg https://raw.githubusercontent.com/fingolfin/git-remote-hg/master/git-remote-hg
sudo chmod +x /usr/local/bin/git-remote-hg
```

If on OS X you may need to edit the script and change the first line
```
- #!/usr/bin/env python2
+ #!/usr/bin/env python
```

Copy the results of your build to the appropriate place under the
`other_lib` directory.

GYP
---

All the builds use project and make files generated with a modified version
of [GYP](https://github.com/msc-/gyp). To install this modified version, clone
the repo to your machine and run the following commands in a shell:

```bash
cd <your_gyp_clone>
sudo ./setup.py install
```

On Windows either

* install [Git for Windows](https://msysgit.github.io/) a.k.a `msysgit`
* install [python 2.7.x](https://www.python.org/downloads/release/python) and add
`<python>` and `<python>/Scripts` to the PATH environment in Windows. Replace
`<python>` with your install directory; it defaults to `C:\Python27`.
* copy [make for msysgit](http://repo.or.cz/w/msysgit.git?a=blob;f=bin/make.exe;h=a971ea1266ff40e89137bba068e2c944a382725f;hb=968336eddac1874c56cd934d10783566af5a3e26)
to the `msysgit` `bin` directory; the default location is
`%USERPROFILE%\AppData\Programs\Git\bin`. Note that the download is named
`bin_make.exe`; you must rename this to make.exe. Note also that this is
version 3.79 of GNU make. It works but version 3.81 is preferable, if you
can find it.
* open a `Git Bash` shell to run the GYP setup commands.

or

* install [Cygwin](http://www.cygwin.com/) making sure to include `make` from
the *development* section and `python` from the *shells* section.
* open a Cygwin `bash` shell to run the GYP setup commands.

To generate the projects run the following command in the home directory
of the project:

```bash
make [xcode,msvs]
```

All important configuration options are gathered together in the file `gyp_include/config.gypi`.

Building
========
The KTX source distribution contains project files generated with GYP. At
present only xcode projects for Mac and iOS and MS Visual Studio projects for Windows are included.

Use `build/xcode/ios/ktx.xcodeproject` to build the library and load tests to run
on iOS using OpenGL ES 3.0.

Use `build/xcode/macgl/ktx.xcodeproject` to build the library and load tests to
run under OpenGL 3.3 on OS X.

Use one of `build/msvs/win/vs20{08,10,10e,13,13e}/ktx.sln` to build the library and load tests for Windows Win32 or x64.

{# vim: set ai ts=4 sts=4 sw=2 expandtab textwidth=75:}
