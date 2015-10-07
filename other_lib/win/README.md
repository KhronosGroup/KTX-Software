
About the GLEW debug libraries
==============================
GLEW version 1.13.0

`Debug-{Win32,x64]/glew32.{dll,obj}` are not debug versions of the libraries.
This is because the standard projects compile the debug libraries as
`glew32d.{dll,lib}`. An app linked with this .lib will look for `glew32d.dll`
at run time regardless of the name of the .lib file.

It is impossible to have different library names per configuration in GYP
so I have put the non-debug versions in the Debug folders. If if ever
becomes necessary to debug GLEW inside KTX, use the static library
`Debug*/glew32s.lib` which *is* a debug library.
