
    Using the VC++ 9 (Visual Studio 2008) Project Files
    ---------------------------------------------------
    
    Finding GL
    ----------

    You need to edit the user macros in the following property files to
    indicate the locations of the include, library and dll files for the
    OpenGL or OpenGL ES implementation you are using.

    - gles1.vsprops
    - gles2.vsprops
    - gles3.vsprops
    - gl.vsprops

    In the case of gles3, the include location is taken from
    gles3.vsprops. This is included by additional properties files
    giving the location of the library and dll files:

    - gles3adreno.vsprops
    - gles3mali.vsprops
    - gles3pvr.vsprops

    See below for notes about the above emulators.
    
    If you are working with another implementation of OpenGL ES 3.0,
    we recommend you create another build configuration for that
    implementation that uses a properties file similar to the three
    above.

    You can edit the files, found in this vc9 folder, or you can edit
    the macros within Visual Studio. Click the Properties Manager tab,
    expand an appropriate configuration (with GLES1, GLES2 or GL in the
    name as desired) and double click the property sheet and in the
    properties dialog that appears, select "User Macros." The property
    sheet names corresponding to the above files are:

    - OpenGL ES 1.1 Properties
    - OpenGL ES 2.0 Properties
    - OpenGL ES 3.0 Properties
    - OpenGL Properties

    The macros you may need to change are:

    - GL{,ES}BinDir
        The directory containing the OpenGL related DLLs to copy to
        the application's directory. For OpenGL this should point
        to the directory containing glew32.dll.

    - GL{,ES}IncludeParent
        The parent directory of the GL, GLES1, GLES2, EGL & KHR include
        directories.

    - GL{,ES}LibDir
        The directory containing the GL or GLES library with which to link.
        For OpenGL this should point to the directory containing glew32.lib.
        opengl32.lib can be found with the standard Visual C++ include path.

    - EGLLib
        the name of the EGL library & dll. For OpenGL this is usually
        empty because EGL is not used and WGL functions are included
        in opengl32.{lib,dll}.

    - GL{,ES}Lib
        the name of the GL or GLES library library & dll.

    - GLEWLib
        the name of the GLEW library to link to (gl.vsprops only).

    NOTES

    The same property sheet is used in both Debug & Release configurations
    so you only need to edit the macros once.

    GLVer is not currently used and in any case should not need changing.
    
    Solution Configurations
    -----------------------
    
    libktx can be built in GLES1, GLES2, GLES3 and GL configurations.
    
    gles1_loadtests, unsurprisingly, can only be built with the GLES1
    configurations.
    
    Likewise gles3_loadtests and gl3 loadtests can only be built with GLES3
    and GL configurations respectively.
    
    toktx is set to build in in the GL configurations as well as the
    undecorated Debug and Release configurations. toktx does not use
    OpenGL itself but can be used to create ktx files for any version
    of OpenGL or OpenGL ES.
    
    OpenGL ES 3.0 Emulator Notes
    ----------------------------
    
    The ARM Mali OpenGL ES Emulator v1.2.0 or later is recommended. Earlier
    versions have a nasty bug that causes a command prompt window to flash
    up (appear and disappear) whenever a shader is compiled or a program
    linked.
    
    The Imagination Technology PowerVR Graphics SDK v3.1 raises a spurious
    GL_INVALID_VALUE error at the second glDeleteProgram() in the code
    extract below from atRelease_01_draw_texture()
    
    	glUseProgram(0);
		glDeleteTextures(1, &pData->gnTexture);
		glDeleteProgram(pData->gnTexProg);
		glDeleteProgram(pData->gnColProg);
		...
		
	which causes an assert to fire when the test switches from one image to
	another. Also the emulator crashes on program exit when running on
    Windows XP.
    
    The Qualcomm Adreno SDK v 3.2.4 emulator crashes in glDrawArrays() when
    run on an AMD host GPU. When run on an NVIDIA host GPU, a shader program
    link error appears saying that 2 attributes are assigned to the same
    index. Reportedly this is because the emulator does not yet properly
    support GLSL ES 3.00 and, in particular, layout qualifiers. If that is
    so, it immediately raises the question "why does this link error not
    appear when running on an AMD host?"
    
    Whatever the reason, the conclusion is avoid the Adreno SDK.

    --
    $Revision$ on $Date::                            $
