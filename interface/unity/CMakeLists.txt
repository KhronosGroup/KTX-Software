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

set(KTX_UNITY_LIB_SRC
    unity_wrapper.cpp
    )
set(UNITY_TYPE MODULE)
if(IOS)
    set(UNITY_TYPE STATIC)
endif()

add_library(ktx_unity ${UNITY_TYPE} ${KTX_UNITY_LIB_SRC} $<TARGET_OBJECTS:objKtx> $<TARGET_OBJECTS:basisu>)

get_property(objKtx_include_dirs TARGET objKtx PROPERTY INCLUDE_DIRECTORIES)
get_property(basisu_include_dirs TARGET basisu PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(ktx_unity PUBLIC ${objKtx_include_dirs} ${basisu_include_dirs})

if(APPLE)
    set_target_properties(ktx_unity PROPERTIES BUNDLE true)
endif()

if(UNITY_PLUGIN_DIR)
    install(TARGETS ktx_unity DESTINATION ${UNITY_PLUGIN_DIR})
endif()
