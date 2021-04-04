# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

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
add_test( NAME toktx-only-max-endpoints
    COMMAND toktx --max_endpoints 5000 a b
)
add_test( NAME toktx-only-max-selectors
    COMMAND toktx --max_selectors 6000 a b
)

add_test( NAME toktx-swizzle-gt-4
    COMMAND toktx --swizzle rgbargba a b
)

add_test( NAME toktx-invalid-swizzle-char
    COMMAND toktx --swizzle rrrh a b
)

add_test( NAME toktx-invalid-target-type
    COMMAND toktx --target_type RGBH a b
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
    toktx-only-max-endpoints
    toktx-only-max-selectors
    toktx-swizzle-gt-4
    toktx-invalid-swizzle-char
    toktx-invalid-target-type
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
            COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:toktx> ${args} toktx.${reference} ${source} && diff ${reference} toktx.${reference} && rm toktx.${reference}"
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
  # Run only on macOS until we figure out the BasisLZ/ETC1S compressor non-determinancy.
  gencmpktx( alpha_simple_basis alpha_simple_basis.ktx2 ../srcimages/alpha_simple.png "--test --bcmp" "" "" )
  gencmpktx( kodim17_basis kodim17_basis.ktx2 ../srcimages/kodim17.png "--test --bcmp" "" "" )
  gencmpktx( color_grid_basis color_grid_basis.ktx2 ../srcimages/color_grid.png "--test --bcmp" "" "" )
  gencmpktx( 16bit_png_basis camera_camera_BaseColor_basis.ktx2 ../srcimages/camera_camera_BaseColor_16bit.png "--bcmp --test --nowarn" "" "" )
  gencmpktx( paletted_png CesiumLogoFlat.ktx2 ../srcimages/CesiumLogoFlat_palette.png "--bcmp --test --nowarn" "" "" )
endif()
  gencmpktx( cimg5293_uastc cimg5293_uastc.ktx2 ../srcimages/CIMG5293.jpg "--uastc --genmipmap --test" "" "" )
  gencmpktx( cimg5293_uastc_zstd cimg5293_uastc_zstd.ktx2 ../srcimages/CIMG5293.jpg "--zcmp --uastc --genmipmap --test" "" "" )
  gencmpktx( 16bit_png_uastc camera_camera_BaseColor_uastc.ktx2 ../srcimages/camera_camera_BaseColor_16bit.png "--uastc 1 --test --nowarn" "" "" )

gencmpktx( luminance_reference_u luminance_reference_u.ktx2 ../srcimages/luminance.pgm "--test --t2 --convert_oetf linear" "" "" )
gencmpktx( luminance_reference_uastc luminance_reference_uastc.ktx2 ../srcimages/luminance.pgm "--test --t2 --uastc --" "" "" )
gencmpktx( luminance_reference_basis luminance_reference_basis.ktx2 ../srcimages/luminance.pgm "--test --t2 --bcmp" "" "" )
gencmpktx( luminance_alpha_reference_u luminance_alpha_reference_u.ktx2 ../srcimages/basn4a08.png "--test --t2" "" "" )
gencmpktx( luminance_alpha_reference_uastc luminance_alpha_reference_uastc.ktx2 ../srcimages/basn4a08.png "--test --t2 --uastc --" "" "" )
if(APPLE)
gencmpktx( luminance_alpha_reference_basis luminance_alpha_reference_basis.ktx2 ../srcimages/basn4a08.png "--test --t2 --bcmp" "" "" )
endif()
gencmpktx( r_reference_u r_reference_u.ktx2 ../srcimages/luminance.pgm "--test --t2 --convert_oetf linear --target_type R" "" "" )
gencmpktx( r_reference_uastc r_reference_uastc.ktx2 ../srcimages/luminance.pgm "--test --t2 --uastc --target_type R --swizzle r001 --" "" "" )
# This one seems to be okay.
gencmpktx( r_reference_basis r_reference_basis.ktx2 ../srcimages/luminance.pgm "--test --t2 --bcmp --target_type R --swizzle r001" "" "" )
gencmpktx( rg_reference_u rg_reference_u.ktx2 ../srcimages/basn4a08.png "--test --t2 --convert_oetf linear --target_type RG" "" "" )
gencmpktx( rg_reference_uastc rg_reference_uastc.ktx2 ../srcimages/basn4a08.png "--test --t2 --uastc --target_type RG --swizzle rg01 --" "" "" )
if(APPLE)
gencmpktx( rg_reference_basis rg_reference_basis.ktx2 ../srcimages/basn4a08.png "--test --t2 --bcmp --target_type RG --swizzle ra01" "" "" )
endif()

# Input to the following tests is a red RGB texture.
gencmpktx( swizzle_r_to_g_u green_rgb_reference_u.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --test --input_swizzle 0r01" "" "" )
gencmpktx( swizzle_r_to_gb_convert_to_rgba_u cyan_rgba_reference_u.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --test --input_swizzle 0rr1 --target_type RGBA" "" "" )
# BasisU encoders notice that alpha is all 1 and removes it, hence RGB output for these.
gencmpktx( swizzle_r_to_gb_convert_to_rgba_basis cyan_rgb_reference_basis.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --test --input_swizzle 0rr1 --target_type RGBA --bcmp" "" "" )
gencmpktx( swizzle_r_to_gb_convert_to_rgba_uastc cyan_rgb_reference_uastc.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --test --input_swizzle 0rr1 --target_type RGBA --uastc --" "" "" )


gencmpktx( gAMA_chunk_png g03n2c08.ktx2 ../srcimages/g03n2c08.png "--test --t2" "" "" )
gencmpktx( cHRM_chunk_png ccwn2c08.ktx2 ../srcimages/ccwn2c08.png "--test --t2" "" "" )
gencmpktx( tRNS_chunk_rgb_png tbrn2c08.ktx2 ../srcimages/tbrn2c08.png "--test --t2" "" "" )
gencmpktx( tRNS_chunk_palette_8-bit_png tbyn3p08.ktx2 ../srcimages/tbyn3p08.png "--test --t2" "" "" )
gencmpktx( tRNS_chunk_palette_2-bit_png tm3n3p02.ktx2 ../srcimages/tm3n3p02.png "--test --t2" "" "" )
gencmpktx(
    rgb-mipmap-reference-list
    rgb-mipmap-reference.ktx
    "toktx.filelist.txt"
    "--lower_left_maps_to_s0t0 --mipmap --nometadata"
    ""
    "../srcimages/level0.ppm\\n../srcimages/level1.ppm\\n../srcimages/level2.ppm\\n../srcimages/level3.ppm\\n../srcimages/level4.ppm\\n../srcimages/level5.ppm\\n../srcimages/level6.ppm"
)

add_test( NAME toktx-cmp-rgb-reference-2
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:toktx> --nometadata - ../srcimages/rgb.ppm > toktx-cmp-rgb-reference-2.ktx && diff rgb-reference.ktx toktx-cmp-rgb-reference-2.ktx; rm toktx-cmp-rgb-reference-2.ktx"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)
set_tests_properties(
    toktx-cmp-rgb-reference-2
PROPERTIES
    ENVIRONMENT TOKTX_OPTIONS=--lower_left_maps_to_s0t0
)
