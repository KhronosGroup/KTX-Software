# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# Find Vulkan package
if(APPLE)
    # N.B. FindVulkan needs the VULKAN_SDK environment variable set to find
    # the iOS frameworks and to set Vulkan_SDK_Base, used later in this
    # file. Therefore ensure to make that env. var. available to CMake and
    # Xcode. Special care is needed to ensure it is available to the CMake
    # and Xcode GUIs.
#    set(CMAKE_FIND_DEBUG_MODE TRUE)
    find_package( Vulkan REQUIRED COMPONENTS MoltenVK )
#    set(CMAKE_FIND_DEBUG_MODE FALSE)
    # Derive some other useful variables from those provided by find_package
    if(APPLE_LOCKED_OS)
        set( Vulkan_SHARE_VULKAN ${Vulkan_SDK_Base}/${CMAKE_SYSTEM_NAME}/share/vulkan )
    else()
        # Vulkan_LIBRARIES points to "libvulkan.dylib".
        # Find the name of the actual dylib which includes the version no.
        # readlink -f requires macOS >= 12.3!
        execute_process(COMMAND readlink -f ${Vulkan_LIBRARIES}
                        OUTPUT_VARIABLE Vulkan_LIBRARY_REAL_PATH_NAME
                        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        cmake_path(GET
                  Vulkan_LIBRARY_REAL_PATH_NAME
                  FILENAME
                  Vulkan_LIBRARY_REAL_FILE_NAME
        )
        # Find the name that includes only the major version number.
        execute_process(COMMAND readlink ${Vulkan_LIBRARIES}
                        OUTPUT_VARIABLE Vulkan_LIBRARY_SONAME_FILE_NAME
                        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set( Vulkan_SHARE_VULKAN appfwSDL/VulkanAppSDL/mac/vulkan )
    endif()
else()
    find_package(Vulkan REQUIRED)
endif()

#cmake_print_variables(
#    Vulkan_LIBRARIES
#    Vulkan_LIBRARY_REAL_PATH_NAME
#    Vulkan_LIBRARY_REAL_FILE_NAME
#    Vulkan_LIBRARY_SONAME_FILE_NAME
#)

include(compile_shader.cmake)

set(SHADER_SOURCES "")

compile_shader(shader_textoverlay textoverlay appfwSDL/VulkanAppSDL/shaders shaders )
compile_shader(shader_cube cube vkloadtests/shaders/cube shaders )
compile_shader(shader_cubemap_reflect reflect vkloadtests/shaders/cubemap shaders )
compile_shader(shader_cubemap_skybox skybox vkloadtests/shaders/cubemap shaders )
compile_shader_list(shader_texture vkloadtests/shaders/texture shaders texture.vert texture1d.frag texture2d.frag)
compile_shader(shader_texture3d instancing3d vkloadtests/shaders/texture3d shaders )
compile_shader(shader_texturearray instancing vkloadtests/shaders/texturearray shaders )
compile_shader(shader_texturemipmap instancinglod vkloadtests/shaders/texturemipmap shaders )

add_custom_target(
    spirv_shaders
    DEPENDS
    shader_textoverlay
    shader_cube
    shader_cubemap_reflect
    shader_cubemap_skybox
    shader_texture
    shader_texture3d
    shader_texturearray
    shader_texturemipmap
)

set( VK_TEST_IMAGES
    etc1s_Iron_Bars_001_normal.ktx2
    uastc_Iron_Bars_001_normal.ktx2
    ktx_document_uastc_rdo4_zstd5.ktx2
    color_grid_uastc_zstd.ktx2
    color_grid_zstd.ktx2
    color_grid_uastc.ktx2
    color_grid_basis.ktx2
    kodim17_basis.ktx2
    pattern_02_bc2.ktx2
    ktx_document_basis.ktx2
    rgba-mipmap-reference-basis.ktx2
    3dtex_7_reference_u.ktx2
    arraytex_7_mipmap_reference_u.ktx2
    cubemap_goldengate_uastc_rdo4_zstd5_rd.ktx2
    cubemap_yokohama_basis_rd.ktx2
    skybox_zstd.ktx2
    orient-down-metadata.ktx
    orient-up-metadata.ktx
    rgba-reference.ktx
    etc2-rgb.ktx
    etc2-rgba8.ktx
    etc2-sRGB.ktx
    etc2-sRGBa8.ktx
    pattern_02_bc2.ktx
    rgb-amg-reference.ktx
    metalplate-amg-rgba8.ktx
    not4_rgb888_srgb.ktx
    texturearray_bc3_unorm.ktx
    texturearray_astc_8x8_unorm.ktx
    texturearray_etc2_unorm.ktx
    straight-rgba.ktx2
    premultiplied-rgba.ktx2
)
list( TRANSFORM VK_TEST_IMAGES
    PREPEND "${PROJECT_SOURCE_DIR}/tests/testimages/"
)

set( KTX_RESOURCES ${LOAD_TEST_COMMON_RESOURCE_FILES} ${VK_TEST_IMAGES} )
if(APPLE)
    # Adding this directory to KTX_RESOURCES and ultimately vkloadtests's
    # RESOURCE property causes the install command (later in this file) to
    # raise an error at configuration time: "RESOURCE given directory". Use
    # this instead to cause the files to be added to Resources in the bundle.
    set_source_files_properties( ${Vulkan_SHARE_VULKAN}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
endif()

add_executable( vkloadtests
    ${EXE_FLAG}
    appfwSDL/VulkanAppSDL/VulkanAppSDL.cpp
    appfwSDL/VulkanAppSDL/VulkanAppSDL.h
    appfwSDL/VulkanAppSDL/vulkancheckres.h
    appfwSDL/VulkanAppSDL/VulkanContext.cpp
    appfwSDL/VulkanAppSDL/VulkanContext.h
    appfwSDL/VulkanAppSDL/vulkandebug.cpp
    appfwSDL/VulkanAppSDL/vulkandebug.h
    appfwSDL/VulkanAppSDL/VulkanSwapchain.cpp
    appfwSDL/VulkanAppSDL/VulkanSwapchain.h
    appfwSDL/VulkanAppSDL/vulkantextoverlay.hpp
    appfwSDL/VulkanAppSDL/vulkantools.cpp
    appfwSDL/VulkanAppSDL/vulkantools.h
    common/disable_glm_warnings.h
    common/reenable_warnings.h
    vkloadtests/InstancedSampleBase.cpp
    vkloadtests/InstancedSampleBase.h
    vkloadtests/Texture.cpp
    vkloadtests/Texture.h
    vkloadtests/Texture3d.cpp
    vkloadtests/Texture3d.h
    vkloadtests/TextureArray.cpp
    vkloadtests/TextureArray.h
    vkloadtests/TextureCubemap.cpp
    vkloadtests/TextureCubemap.h
    vkloadtests/TexturedCube.cpp
    vkloadtests/TexturedCube.h
    vkloadtests/TextureMipmap.cpp
    vkloadtests/TextureMipmap.h
    vkloadtests/utils/VulkanMeshLoader.hpp
    vkloadtests/utils/VulkanTextureTranscoder.hpp
    vkloadtests/VulkanLoadTests.cpp
    vkloadtests/VulkanLoadTests.h
    vkloadtests/VulkanLoadTestSample.cpp
    vkloadtests/VulkanLoadTestSample.h
    ${LOAD_TEST_COMMON_RESOURCE_FILES}
    ${Vulkan_SHARE_VULKAN}
    ${SHADER_SOURCES}
    ${VK_TEST_IMAGES}
    vkloadtests.cmake
)

set_code_sign(vkloadtests)

# If VulkanAppSDL is ever made into its own target change the target here.
target_compile_features(vkloadtests
PRIVATE
    cxx_std_14
)

target_include_directories(vkloadtests
PRIVATE
    SDL3::Headers
    $<TARGET_PROPERTY:appfwSDL,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    appfwSDL/VulkanAppSDL
    vkloadtests
    vkloadtests/utils
)

target_include_directories(vkloadtests
  SYSTEM PRIVATE
      ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(vkloadtests
    ktx
    objUtil
    appfwSDL
)

set_target_properties(vkloadtests PROPERTIES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
)

target_link_libraries(
    vkloadtests
    Vulkan::Vulkan
)

if(APPLE)
    if(IOS)
        set( INFO_PLIST_IN "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/ios/Info.plist.in" )
        set( icon_launch_assets
            ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
            vkloadtests/resources/ios/LaunchImages.xcassets
            vkloadtests/resources/ios/LaunchScreen.storyboard
        )
        target_sources( vkloadtests
            PRIVATE
                ${icon_launch_assets}
        )
        # Add to resources so they'll be copied to the bundle.
        list( APPEND KTX_RESOURCES ${icon_launch_assets} )
        target_link_libraries(
            vkloadtests
            ${AudioToolbox_LIBRARY}
            ${AVFoundation_LIBRARY}
            ${CoreAudio_LIBRARY}
            ${CoreBluetooth_LIBRARY}
            ${CoreGraphics_LIBRARY}
            ${CoreMotion_LIBRARY}
            ${CoreHaptics_LIBRARY}
            ${Foundation_LIBRARY}
            ${GameController_LIBRARY}
            ${IOSurface_LIBRARY}
            ${Metal_LIBRARY}
            ${MOLTENVK_FRAMEWORK}
            ${OpenGLES_LIBRARY}
            ${QuartzCore_LIBRARY}
            ${UIKit_LIBRARY}
        )
    else()
        set( INFO_PLIST_IN "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/mac/Info.plist.in" )
    endif()
elseif(WIN32)
    ensure_runtime_dependencies_windows(vkloadtests)
elseif(LINUX)
        target_sources(
            vkloadtests
        PRIVATE
            vkloadtests/resources/linux/vkloadtests.desktop
        )

endif()

target_link_libraries( vkloadtests ${LOAD_TEST_COMMON_LIBS} )

target_compile_definitions(
    vkloadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

set_target_properties( vkloadtests PROPERTIES RESOURCE "${KTX_RESOURCES};${SHADER_SOURCES}" )

if(APPLE)
    set( product_name vkloadtests )
    set( bundle_identifier org.khronos.ktx.${product_name} )
    # This property must be set to avoid an Xcode warning.
    set_target_properties( vkloadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${bundle_identifier} )
    # The file identified by MACOSX_BUNDLE_INFO_PLIST is subject to an
    # implicit configure_file() step by CMake. Since this target has a custom
    # Info.plist this is not strictly necessary but the writer does not know
    # how to prevent it. Furthermore the BUNDLE_NAME, EXECUTABLE_NAME and
    # GUI_IDENTIFIER properties could all be set from Xcode build settings
    # but using those in the custom Info.plist would not be portable to other
    # generators. Since configure_file() is happening use the standard
    # property names for consistency with the standard Info.plist template.
    set_target_properties( vkloadtests PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${product_name}
        MACOSX_BUNDLE_EXECUTABLE_NAME ${product_name}
        MACOSX_BUNDLE_COPYRIGHT "© 2024 Khronos Group, Inc."
        MACOSX_BUNDLE_GUI_IDENTIFIER ${bundle_identifier}
        MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST_IN}
        MACOSX_BUNDLE_INFO_STRING "View KTX textures; display via Vulkan."
        MACOSX_BUNDLE_ICON_FILE ${KTX_APP_ICON}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING  ${PROJECT_VERSION}
        # Because libassimp is built with bitcode disabled. It's not important
        # unless submitting to the App Store and currently bitcode is optional.
        XCODE_ATTRIBUTE_ENABLE_BITCODE NO
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH YES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME ${KTX_APP_ICON_BASENAME}
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone and iPad
        # This is to silence a "not stripping because it is signed" warning
        # from Xcode during copying by EMBED_FRAMEWORKS. It has no effect
        # on the code because (a) all the Vulkan SDK dylibs and frameworks
        # are Release config so have no symbols and (b) we need to keep the
        # symbols in the Debug config of libktx.
        XCODE_ATTRIBUTE_COPY_PHASE_STRIP NO
    )
    unset(product_name)
    unset(bundle_identifier)

    # The generated project code for building an Apple bundle automatically
    # copies the executable and all files with the RESOURCE property to the
    # bundle adjusting for the difference in bundle layout between iOS &
    # macOS.

    if(IOS)
        set_target_properties( vkloadtests PROPERTIES
            XCODE_EMBED_FRAMEWORKS "${Vulkan_MoltenVK_LIBRARY};${Vulkan_LIBRARIES};${Vulkan_Layer_VALIDATION}"
            XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY		"YES"
            XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY	"YES"
            # Set RPATH to find frameworks
            INSTALL_RPATH @executable_path/Frameworks
        )
    else()
        # Why is XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY not set here?
        # Excellent question. The Vulkan, MoltenVk and VkLayer dylibs are
        # all signed by LunarG, the ktx dylib by us so no need. On the other
        # hand the Vulkan and MoltenVK frameworks in the iOS SDK are not
        # signed. hence it is set there.
        set_target_properties( vkloadtests PROPERTIES
            XCODE_EMBED_FRAMEWORKS "${Vulkan_LIBRARY_REAL_PATH_NAME};${Vulkan_MoltenVK_LIBRARY};${Vulkan_Layer_VALIDATION}"
            # Set RPATH to find frameworks and dylibs
            INSTALL_RPATH @executable_path/../Frameworks
        )
        if(BUILD_SHARED_LIBS)
            # XCODE_EMBED_FRAMEWORKS does not appear to support generator
            # expressions hence this instead of a genex in the above.
            set_property( TARGET vkloadtests
                APPEND PROPERTY XCODE_EMBED_FRAMEWORKS
                ktx
            )
            add_custom_command( TARGET vkloadtests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_NAME:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/$<TARGET_SONAME_FILE_NAME:ktx>"
                COMMENT "Create symlink for KTX library (ld name to real name"
            )
        endif()
        add_custom_command( TARGET vkloadtests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${Vulkan_LIBRARY_REAL_FILE_NAME}" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/${Vulkan_LIBRARY_SONAME_FILE_NAME}"
            COMMENT "Create symlink for Vulkan library (ld name to real name)"
        )
        # Re. SDL3 & assimp: no copy required.: vcpkg libs are static or else
        # vcpkg arranges copy. Brew libs cannot be bundled.

        # Specify destination for cmake --install.
        install(TARGETS vkloadtests
            BUNDLE
                DESTINATION ${CMAKE_INSTALL_PREFIX}/Applications
                COMPONENT VkLoadTestApp
        )

        ## Uncomment for Bundle analysis
        # install( CODE "
        #     include(BundleUtilities)
        #     verify_app($<TARGET_BUNDLE_DIR:vkloadtests>)
        #     #fixup_bundle($<TARGET_BUNDLE_DIR:vkloadtests> \"\" \"\")"
        # )
    endif()
else()
    # This is for other platforms.
    # This copies the resources next to the executable for ease
    # of use during debugging and testing.
    add_custom_command( TARGET vkloadtests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
          $<TARGET_FILE_DIR:vkloadtests>/../resources
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${KTX_RESOURCES} ${SHADER_SOURCES}
          $<TARGET_FILE_DIR:vkloadtests>/../resources
    )

    # To keep the resources (test images and models) close to the
    # executable and to be compliant with the Filesystem Hierarchy
    # Standard https://refspecs.linuxfoundation.org/FHS_3.0/fhs/index.html
    # we have chosen to install the apps and data in /opt/<target>.
    # Each target has a `bin` directory with the executable and a
    # `resources` directory with the resources. We install a symbolic
    # link to the executable in ${CMAKE_INSTALL_BINDIR}, usually
    # /usr/local/bin, instead of adding /opt/<target>/bin to $PATH.
    #
    # TODO: Figure out how to handle libktx so installs of tools only,
    # tools + loadtests and loadtests only are supported. Only put
    # library in /usr/local/lib? Duplicate it in /opt/<provider>/lib
    # from where it is shared by gl3loadtests and vkloadtests? Only
    # put it in /opt/<provider>/lib with link from
    # ${CMAKE_INSTALL_LIBDIR}? NOTE: if we put lib in /opt/<provider>
    # then consider putting the executables in /opt/provider/<target>.

    # TODO: Before adding this target to the release packages, ensure
    # this RPATH will work for alternate install root.
    set_target_properties( vkloadtests PROPERTIES
        INSTALL_RPATH "\$ORIGIN;${CMAKE_INSTALL_FULL_LIBDIR}"
    )

    ######### IMPORTANT ######
    # When installing via `cmake --install` ALSO install the
    # library component. There seems no way to make a dependency.
    ##########################

#    set( destroot "${LOAD_TEST_DESTROOT}/$<TARGET_FILE_NAME:vkloadtests>")
#    # NOTE: WHEN RUNNING MANUAL INSTALLS INSTALL library COMPONENT TOO.
#    install(TARGETS vkloadtests
#        RUNTIME
#            DESTINATION ${destroot}/bin
#            COMPONENT VkLoadTestApp
#        RESOURCE
#            DESTINATION ${destroot}/resources
#            COMPONENT VkLoadTestApp
#    )
#    if(LINUX)
#        # Add a link from the regular bin directory to put command
#        # on PATH.
#        install(CODE "
#           EXECUTE_PROCESS(COMMAND ln -s ${destroot}/bin/$<TARGET_FILE_NAME:vkloadtests> ${CMAKE_INSTALL_FULL_BINDIR}
#           )"
#           COMPONENT VkLoadTestApp
#        )
#        install(FILES
#            vkloadtests/resources/linux/vkloadtests.desktop
#            DESTINATION /usr/share/applications
#            COMPONENT VkLoadTestApp
#        )
#    endif()
endif()

add_dependencies(
    vkloadtests
    spirv_shaders
)

