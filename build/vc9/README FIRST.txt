
    Using the VC++ 9 (Visual Studio 2008) Project Files
    ---------------------------------------------------
    
    Finding GL
    ----------

    You need to edit the user macros in the following property files to
    indicate the locations of the include, library and dll files for the
    OpenGL or OpenGL ES implementation you are using.

    - gles1.vsprops
    - gles2.vsprops
    - gl.vsprops

    You can edit files, found in this vc9 folder, or you can edit the macros
    within Visual Studio. Click the Properties tab, expand an appropriate
    configuration (with GLES1, GLES2 or GL in the name as desired) and
    double click the property sheet and in the properties dialog that
    appears, select "User Macros." The property sheet names corresponding
    to the above files are:

    - OpenGL ES 1.1 Properties
    - OpenGL ES 2.0 Properties
    - OpenGL Properties

    The macros you may need to change are:

    - GL{,ES}BinDir
        The directory containing the OpenGL related DLLs to copy to
        the application's directory. For OpenGL this is usally empty.

    - GL{,ES}IncludeParent
        The parent directory of the GL, GLES1, GLES2, EGL & KHR include
        directories.

    - GL{,ES}LibDir
        The directory containing the GL or GLES library with which to link.
        For OpenGL this is usually empty as opengl32.lib can be found with
        the standard Visual C++ include path.

    - EGLLib
        the name of the EGL library & dll. For OpenGL this is usually
        empty because EGL is not used and WGL functions are included
        in opengl32.{lib,dll}.

    - GL{,ES}Lib
        the name of the GL or GLES library library & dll.

    - EGLAuxDll
        the name of any auxiliary dll that may be needed.

    NOTES

    The same property sheet is used in both Debug & Release configurations
    so you only need to edit the macros once.

    GLVer is not currently used and in any case should not need changing.
    
    Configurations
    --------------
    
    libktx can be built in GLES1, GLES2 and GL configurations.
    
    gles1_loadtests, unsurprisingly, can only be built with the GLES1 configurations.
    
    toktx is set to build only in the GL configuration as this toktx can be
    used to create ktx files for any version of GL.

    --
    $Revision$ on $Date::                            $