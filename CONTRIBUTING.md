## How to contribute to the KTX library and tools.

1. Make sure you have a GitHub account.
2. Fork the repository on GitHub.
3. Make changes to your clone of the repository.
4. Submit a pull request.

### $Date$ keyword expansion

A few of the source files have $Date$ keywords in them. If you are generating the documentation or preparing distribution archives, you must add one of the following to the .git/config file in your clone of the repository so that $Date$ will be expanded and unexpanded. This is optional if not doing one of the aforementioned tasks.

```
[filter "kwexpander"]
	# The rev-parse command returns the absolute path to the working tree top.
	smudge = bash expandkw %f
	clean = bash -c \"sed -e 's/\\$Date.*\\$/\\$Date\\$/'\"
```
Assumes bash is in a folder in your PATH environment variable. On Unix/GNULinux/OSX, this is normally the case. On Windows, you will need Cygwin and must either add C:\cygwin\bin to %PATH% or prefix "bash" above with C:/cygwin/bin/.

On Unix/GNULinux/OSX you can change clean to simply

```
clean = sed -e 's/\\$Date.*\\$/\\$Date\\$/'
```

#### Using $Date$ expansion in other projects

Add the above to your ~/.gitconfig file instead. If on Unix/Linux/OSX, you should copy expandkw to somewhere in your path, such as /usr/local/bin and change smudge to simply

```
smudge = expandkw %f
```

If on Windows, you will need to use the full path to "expandkw" in the smudge driver. The driver shown relies on Git setting the current directory to the top of the KTX working tree.
