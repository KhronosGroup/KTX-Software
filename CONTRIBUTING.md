## How to contribute to the KTX library and tools.

1. Make sure you have a GitHub account.
2. Fork the repository on GitHub.
3. Make changes to your clone of the repository.
4. Submit a pull request.

### $Date$ keyword expansion

A few of the source files have $Date$ keywords in them. If you are generating the documentation or preparing distribution archives, you must add the following to the .git/config file in your clone of the repository so that $Date$ will be expanded and unexpanded. This is optional if not doing one of the aforementioned tasks.

```
[filter "kwexpander"]
	# The rev-parse command returns the absolute path to the top of the working tree.
	smudge = $(git rev-parse --show-toplevel)/expandkw %f
	clean = sed -e 's/\\$Date.*\\$/$Date$/'
```

If you would like to use $Date$ expansion in other projects then we recommend you copy expandkw to somewhere in your path, such /usr/local/bin and add the above to your ~/.gitconfig file instead, while changing the git rev-parse clause to simply

```
smudge = expandkw %f
```
