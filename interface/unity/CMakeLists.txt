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
