
About GLEW debug libraries
==========================
GLEW version 2.1.0 downloaded from
    https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0-win32.zip/download

Both Debug & Release builds use `glew32.{dll,obj} from Release-*.
This is because the standard projects compile the debug libraries as
`glew32d.{dll,lib}`. An app linked with this .lib will look for `glew32d.dll`
at run time regardless of the name of the .lib file.

It is impossible to have different library names per configuration in GYP
so we always use the release versions.
