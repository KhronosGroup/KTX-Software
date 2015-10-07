
About the GLEW debug libraries
==============================

Debug-{Win32,x64]/glew32.{dll,obj} are not debug versions of the libraries.
This is because the standard projects compile the debug libraries as
glew32d.{dll,lib}. An app linked with this .lib will look for glew32d.dll
regardless of the name of the .lib file.

It is impossible to have different library names per configuration in GYP
so I have put the non-debug versions in the Debug folders. If if ever
becomes necessary to debug GLEW inside KTX, use the static library
glew32s.lib which *is* a debug library.
