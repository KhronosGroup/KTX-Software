## How to contribute to the KTX library and tools.

1. Make sure you have a GitHub account.
2. Fork the repository on GitHub.
3. Make changes to your clone of the repository.
4. Update or supplement the tests as necessary.
5. Submit a pull request against the _incoming_ branch.

### $Date$ keyword expansion

A few of the source files have $Date$ keywords in them which are expanded
via a smudge filter. If you are generating the documentation or preparing
distribution archives, you must issue the following commands in the root
of your clone so that $Date$ will be expanded and unexpanded. This is
optional if not doing one of the aforementioned tasks.

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

The first command adds an [include] of the repo's `.gitconfig` to the local git config file`.git/config` in your clone of the repo. `.gitconfig` contains the config of the "keyworder" filter. The remaining commands force a new checkout of the affected files to smudge them with the date. These two are unnecessary if you plan to edit these files. All are unecessary if you do not care about having the dates shown.
