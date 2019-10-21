if(KTX_FEATURE_KTX1)
    message(WARNING "KTX_UNITY_LIB does not support KTX_FEATURE_KTX1")
    return()
endif()

if(KTX_FEATURE_UPLOAD)
    message(WARNING "KTX_UNITY_LIB does not support KTX_FEATURE_UPLOAD")
    return()
endif()

if(KTX_FEATURE_WRITE)
    message(WARNING "KTX_UNITY_LIB does not support KTX_FEATURE_WRITE")
    return()
endif()

if(UNITY_PACKAGE_PATH)
    set( DEST_PLUGIN_PATH "${UNITY_PACKAGE_PATH}/Runtime/Plugins")

    if(ANDROID)
    set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/Android/libs/${ANDROID_ABI})
    elseif(IOS)
    set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/iOS)
    elseif(EMSCRIPTEN)
    set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/WebGL)
    else()
    if(bitness EQUAL 32)
    set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/x86)
    else()
    set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/x86_64)
    endif()
    endif()
    
    message(STATUS "Will install Unity native lib to ${UNITY_PLUGIN_DIR}")
    
    if( NOT EXISTS ${UNITY_PLUGIN_DIR})
        message(SEND_ERROR "Invalid Unity package path!")
    endif()
endif()

set(UNITY_TYPE MODULE)
if(IOS)
    set(UNITY_TYPE STATIC)
endif()

add_library(ktx_unity ${UNITY_TYPE}
    dummy.c
    $<TARGET_OBJECTS:obj_ktx_cbind>
    $<TARGET_OBJECTS:objKtxRead>
    $<TARGET_OBJECTS:objBasisu>
)

target_include_directories(ktx_unity PUBLIC ${objKtxRead_include_dirs} ${basisu_include_dirs})

target_link_libraries( ktx_unity ${KTX_LIB_READ_LIBS} )

if(APPLE)
    set_target_properties(ktx_unity PROPERTIES BUNDLE true)
endif()

if(UNITY_PLUGIN_DIR)
    install(TARGETS ktx_unity DESTINATION ${UNITY_PLUGIN_DIR})
endif()
