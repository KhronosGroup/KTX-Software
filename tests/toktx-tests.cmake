add_test( NAME toktx-test-help
    COMMAND toktx --help
)

add_test( NAME toktx-test-version
    COMMAND toktx --version
)
set_tests_properties(
    toktx-test-version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^toktx v[0-9\\.]+"
)

add_test( NAME toktx-test-foobar
    COMMAND toktx --foobar
)

add_test( NAME toktx-automipmap-mipmaps
    COMMAND toktx --automipmap --mipmaps a b
)
add_test( NAME toktx-alpha
    COMMAND toktx --alpha a b
)
add_test( NAME toktx-luminance
    COMMAND toktx --luminance a b
)
add_test( NAME toktx-zcmp-bcmp
    COMMAND toktx --zcmp --bcmp a b
)
add_test( NAME toktx-bcmp-uastc
    COMMAND toktx --bcmp --uastc a b
)
add_test( NAME toktx-scale-resize
    COMMAND toktx --scale 0.5 --resize 10x40 a b
)
add_test( NAME toktx-mipmap-resize
    COMMAND toktx --mipmap --resize 10x40 a b c
)

set_tests_properties(
    toktx-test-foobar
    toktx-automipmap-mipmaps
    toktx-alpha
    toktx-luminance
    toktx-zcmp-bcmp
    toktx-bcmp-uastc
    toktx-scale-resize
    toktx-mipmap-resize
PROPERTIES
    WILL_FAIL TRUE
)

function( gencmpktx test_name reference source args env files )
    if(files)
        add_test( NAME toktx-cmp-${test_name}
            COMMAND ${BASH_EXECUTABLE} -c "printf \"${files}\" > ${source} && $<TARGET_FILE:toktx> ${args} toktx.${reference} @${source} && diff ${reference} toktx.${reference} && rm toktx.${reference}; rm ${source}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
        )
    else()
        add_test( NAME toktx-cmp-${test_name}
            COMMAND ${BASH_EXECUTABLE} -c "($<TARGET_FILE:toktx> ${args} toktx.${reference} ${source} && diff ${reference} toktx.${reference} && rm toktx.${reference}) || <TARGET_FILE:ktxinfo ${reference} toktx.${reference}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
        )
    endif()

    set_tests_properties(
        toktx-cmp-${test_name}
    PROPERTIES
        ENVIRONMENT TOKTX_OPTIONS=${env}
    )

endfunction()

gencmpktx( rgb-reference rgb-reference.ktx "../srcimages/rgb.ppm" "--lower_left_maps_to_s0t0 --nometadata" "" "" )
gencmpktx( rgb-amg-reference rgb-amg-reference.ktx "../srcimages/rgb.ppm" "--automipmap --lower_left_maps_to_s0t0 --linear --nometadata" "" "" )
gencmpktx( orient-up orient-up.ktx "../srcimages/up.ppm" "" "--lower_left_maps_to_s0t0 --nometadata" "" )
gencmpktx( orient-up-metadata orient-up-metadata.ktx "../srcimages/up.ppm" "--lower_left_maps_to_s0t0" "" "" )

gencmpktx( orient-down-metadata orient-down-metadata.ktx ../srcimages/up.ppm "" "" "" )
gencmpktx( rgba-reference rgba-reference.ktx ../srcimages/rgba.pam "--lower_left_maps_to_s0t0 --nometadata" "" "" )
gencmpktx( rgb-mipmap-reference rgb-mipmap-reference.ktx "../srcimages/level0.ppm ../srcimages/level1.ppm ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm" "--lower_left_maps_to_s0t0 --mipmap --nometadata" "" "" )
gencmpktx( rgb-mipmap-reference-u rgb-mipmap-reference-u.ktx2 "../srcimages/level0.ppm ../srcimages/level1.ppm ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm" "--test --t2 --mipmap" "" "" )

if(APPLE)
  # Run only on macOS until we figure out the Basis compressor non-determinancy.
  gencmpktx( alpha_simple_basis alpha_simple_basis.ktx2 ../srcimages/alpha_simple.png "--test --bcmp" "" "" )
  gencmpktx( kodim17_basis kodim17_basis.ktx2 ../srcimages/kodim17.png "--test --bcmp" "" "" )
  gencmpktx( color_grid_basis color_grid_basis.ktx2 ../srcimages/color_grid.png "--test --bcmp" "" "" )
  gencmpktx( cimg5293_uastc cimg5293_uastc.ktx2 ../srcimages/CIMG5293.jpg "--uastc --genmipmap --test" "" "" )
  gencmpktx( cimg5293_uastc_zstd cimg5293_uastc_zstd.ktx2 ../srcimages/CIMG5293.jpg "--zcmp --uastc --genmipmap --test" "" "" )
endif()

gencmpktx(
    rgb-mipmap-reference-list
    rgb-mipmap-reference.ktx
    "toktx.filelist.txt"
    "--lower_left_maps_to_s0t0 --mipmap --nometadata"
    ""
    "../srcimages/level0.ppm\\n../srcimages/level1.ppm\\n../srcimages/level2.ppm\\n../srcimages/level3.ppm\\n../srcimages/level4.ppm\\n../srcimages/level5.ppm\\n../srcimages/level6.ppm"
)
gencmpktx( 16bit_png camera_camera_BaseColor.ktx2 ../srcimages/camera_camera_BaseColor_16bit.png "--bcmp --test --nowarn" "" "" )
gencmpktx( paletted_png CesiumLogoFlat.ktx2 ../srcimages/CesiumLogoFlat_palette.png "--bcmp --test --nowarn" "" "" )
gencmpktx( gAMA_chunk_png g03n2c08.ktx2 ../srcimages/g03n2c08.png "--test --t2" "" "" )
gencmpktx( cHRM_chunk_png ccwn2c08.ktx2 ../srcimages/ccwn2c08.png "--test --t2" "" "" )

add_test( NAME toktx-cmp-rgb-reference-2
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:toktx> --nometadata - ../srcimages/rgb.ppm > toktx-cmp-rgb-reference-2.ktx && diff rgb-reference.ktx toktx-cmp-rgb-reference-2.ktx; rm toktx-cmp-rgb-reference-2.ktx"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)
set_tests_properties(
    toktx-cmp-rgb-reference-2
PROPERTIES
    ENVIRONMENT TOKTX_OPTIONS=--lower_left_maps_to_s0t0
)
