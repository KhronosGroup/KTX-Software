# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

add_test( NAME toktx-test.help
    COMMAND toktx --help
)

add_test( NAME toktx-test.version
    COMMAND toktx --version
)
set_tests_properties(
    toktx-test.version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^toktx v[0-9\\.]+"
)

add_test( NAME toktx-test.foobar
    COMMAND toktx --foobar
)

add_test( NAME toktx-test.automipmap-mipmaps
    COMMAND toktx --automipmap --mipmaps a b
)
add_test( NAME toktx-test.alpha
    COMMAND toktx --alpha a b
)
add_test( NAME toktx-test.luminance
    COMMAND toktx --luminance a b
)
add_test( NAME toktx-test.zcmp-bcmp
    COMMAND toktx --zcmp --bcmp a b
)
add_test( NAME toktx-test.bcmp-uastc
    COMMAND toktx --bcmp --uastc a b
)
add_test( NAME toktx-test.scale-resize
    COMMAND toktx --scale 0.5 --resize 10x40 a b
)
add_test( NAME toktx-test.mipmap-resize
    COMMAND toktx --mipmap --resize 10x40 a b c
)
add_test( NAME toktx-test.only-max-endpoints
    COMMAND toktx --max_endpoints 5000 a b
)
add_test( NAME toktx-test.only-max-selectors
    COMMAND toktx --max_selectors 6000 a b
)

add_test( NAME toktx-test.swizzle-gt-4
    COMMAND toktx --swizzle rgbargba a b
)

add_test( NAME toktx-test.invalid-swizzle-char
    COMMAND toktx --swizzle rrrh a b
)

add_test( NAME toktx-test.invalid-target-type
    COMMAND toktx --target_type RGBH a b
)

add_test( NAME toktx-test.set-oetf-second-file-no-error
    COMMAND toktx --lower_left_maps_to_s0t0 --mipmap --nometadata --assign_oetf linear -- - ../srcimages/level0.ppm ../srcimages/level1.ppm ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME toktx-test.convert-oetf-second-file-no-error
    COMMAND toktx --lower_left_maps_to_s0t0 --mipmap --nometadata --convert_oetf linear -- - ../srcimages/level0.ppm ../srcimages/level1.ppm ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME toktx-test.change-target-type-second-file-no-error
    COMMAND toktx --lower_left_maps_to_s0t0 --mipmap --nometadata --target_type RGBA -- - ../srcimages/level0.ppm ../srcimages/level1.ppm ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME toktx-test.different-colortype-second-file-error
    COMMAND toktx --lower_left_maps_to_s0t0 --mipmap --nometadata -- - ../srcimages/level0.ppm ../srcimages/level1-alpha.pam ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME toktx-test.different-colortype-second-file-warning
    COMMAND toktx --lower_left_maps_to_s0t0 --mipmap --nometadata --target_type RGBA -- - ../srcimages/level0.ppm ../srcimages/level1-alpha.pam ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME toktx-test.depth-layers
    COMMAND toktx --depth 4 --layers 4 a b c d e
)

add_test( NAME toktx-test.depth-genmipmap
    COMMAND toktx --test --depth 7 --genmipmap --t2 3dtex_7_mipmap_reference_u.ktx2 ../srcimages/red16.png ../srcimages/orange16.png ../srcimages/yellow16.png ../srcimages/green16.png ../srcimages/blue16.png ../srcimages/indigo16.png ../srcimages/violet16.png
)

add_test( NAME toktx-test.layers-lt-one
    COMMAND toktx --layers 0 a b
)

set_tests_properties(
    toktx-test.foobar
    toktx-test.automipmap-mipmaps
    toktx-test.alpha
    toktx-test.luminance
    toktx-test.zcmp-bcmp
    toktx-test.bcmp-uastc
    toktx-test.scale-resize
    toktx-test.mipmap-resize
    toktx-test.only-max-endpoints
    toktx-test.only-max-selectors
    toktx-test.swizzle-gt-4
    toktx-test.invalid-swizzle-char
    toktx-test.invalid-target-type
    toktx-test.different-colortype-second-file-error
    toktx-test.depth-layers
    toktx-test.depth-genmipmap
    toktx-test.layers-lt-one
PROPERTIES
    WILL_FAIL TRUE
)

set_tests_properties(
    toktx-test.different-colortype-second-file-warning
PROPERTIES
    PASS_REGULAR_EXPRESSION "^toktx warning! Image in ../srcimages/level1-alpha.pam\\(0,0\\) has a different component count"
)

function( gencmpktx test_name reference source args env files )
    if(files)
        add_test( NAME toktx-test.cmp-${test_name}
            COMMAND ${BASH_EXECUTABLE} -c "printf \"${files}\" > ${source} && $<TARGET_FILE:toktx> --test ${args} toktx.${reference} @${source} && diff ${reference} toktx.${reference} && rm toktx.${reference}; rm ${source}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
        )
    else()
        add_test( NAME toktx-test.cmp-${test_name}
            COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:toktx> --test ${args} toktx.${reference} ${source} && diff ${reference} toktx.${reference} && rm toktx.${reference}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
        )
    endif()

    set_tests_properties(
        toktx-test.cmp-${test_name}
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
gencmpktx( rgb-mipmap-reference-u rgb-mipmap-reference-u.ktx2 "../srcimages/level0.ppm ../srcimages/level1.ppm ../srcimages/level2.ppm ../srcimages/level3.ppm ../srcimages/level4.ppm ../srcimages/level5.ppm ../srcimages/level6.ppm" "--t2 --mipmap" "" "" )

if(APPLE)
  # Run only on macOS until we figure out the BasisLZ/ETC1S compressor non-determinancy.
  gencmpktx( alpha_simple_basis alpha_simple_basis.ktx2 ../srcimages/alpha_simple.png "--bcmp" "" "" )
  gencmpktx( color_grid_basis color_grid_basis.ktx2 ../srcimages/color_grid.png "--bcmp" "" "" )
  if (NOT ${CPU_ARCHITECTURE} STREQUAL "arm64" )
    gencmpktx( kodim17_basis kodim17_basis.ktx2 ../srcimages/kodim17.png "--bcmp" "" "" )
    gencmpktx( 16bit_png_basis camera_camera_BaseColor_basis.ktx2 ../srcimages/camera_camera_BaseColor_16bit.png "--bcmp --nowarn" "" "" )
    gencmpktx( paletted_png CesiumLogoFlat.ktx2 ../srcimages/CesiumLogoFlat_palette.png "--bcmp --nowarn" "" "" )
  endif()
endif()
if (NOT ${CPU_ARCHITECTURE} STREQUAL "arm64" )
  gencmpktx( cimg5293_uastc cimg5293_uastc.ktx2 ../srcimages/CIMG5293.jpg "--uastc --genmipmap --test" "" "" )
  gencmpktx( cimg5293_uastc_zstd cimg5293_uastc_zstd.ktx2 ../srcimages/CIMG5293.jpg "--zcmp --uastc --genmipmap --test" "" "" )
  gencmpktx( 16bit_png_uastc camera_camera_BaseColor_uastc.ktx2 ../srcimages/camera_camera_BaseColor_16bit.png "--uastc 1 --nowarn" "" "" )
endif()

gencmpktx( luminance_reference_u luminance_reference_u.ktx2 ../srcimages/luminance.pgm "--t2 --convert_oetf linear" "" "" )
gencmpktx( luminance_reference_uastc luminance_reference_uastc.ktx2 ../srcimages/luminance.pgm "--t2 --uastc --" "" "" )
if(APPLE)
# Before VS2022 version 17.5 Windows got the same result as Mac.
gencmpktx( luminance_reference_basis luminance_reference_basis.ktx2 ../srcimages/luminance.pgm "--t2 --bcmp" "" "" )
endif()
gencmpktx( luminance_alpha_reference_u luminance_alpha_reference_u.ktx2 ../srcimages/basn4a08.png "--t2" "" "" )
gencmpktx( luminance_alpha_reference_uastc luminance_alpha_reference_uastc.ktx2 ../srcimages/basn4a08.png "--t2 --uastc --" "" "" )
if(APPLE)
gencmpktx( luminance_alpha_reference_basis luminance_alpha_reference_basis.ktx2 ../srcimages/basn4a08.png "--t2 --bcmp" "" "" )
endif()
gencmpktx( r_reference_u r_reference_u.ktx2 ../srcimages/luminance.pgm "--t2 --convert_oetf linear --target_type R" "" "" )
gencmpktx( r_reference_uastc r_reference_uastc.ktx2 ../srcimages/luminance.pgm "--t2 --uastc --target_type R --swizzle r001 --" "" "" )
if(APPLE)
# Before VS2022 version 17.5 Windows got the same result as Mac.
gencmpktx( r_reference_basis r_reference_basis.ktx2 ../srcimages/luminance.pgm "--t2 --bcmp --target_type R --swizzle r001" "" "" )
endif()
gencmpktx( rg_reference_u rg_reference_u.ktx2 ../srcimages/basn4a08.png "--t2 --convert_oetf linear --target_type RG" "" "" )
gencmpktx( rg_reference_uastc rg_reference_uastc.ktx2 ../srcimages/basn4a08.png "--t2 --uastc --target_type RG --swizzle rg01 --" "" "" )
if(APPLE)
gencmpktx( rg_reference_basis rg_reference_basis.ktx2 ../srcimages/basn4a08.png "--t2 --bcmp --target_type RG --swizzle ra01" "" "" )
endif()

# Input to the following tests is a red RGB texture.
gencmpktx( swizzle_r_to_g_u green_rgb_reference_u.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --input_swizzle 0r01" "" "" )
gencmpktx( swizzle_r_to_gb_convert_to_rgba_u cyan_rgba_reference_u.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --input_swizzle 0rr1 --target_type RGBA" "" "" )
# BasisU encoders notice that alpha is all 1 and removes it, hence RGB output for these.
gencmpktx( swizzle_r_to_gb_convert_to_rgba_basis cyan_rgb_reference_basis.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --input_swizzle 0rr1 --target_type RGBA --bcmp" "" "" )
gencmpktx( swizzle_r_to_gb_convert_to_rgba_uastc cyan_rgb_reference_uastc.ktx2 ../srcimages/level0.ppm "--nowarn --t2 --input_swizzle 0rr1 --target_type RGBA --uastc --" "" "" )

if (NOT ${CPU_ARCHITECTURE} STREQUAL "arm64" )
  gencmpktx( uastc_Iron_Bars_001_normal      uastc_Iron_Bars_001_normal.ktx2     ../srcimages/Iron_Bars/Iron_Bars_001_normal_unnormalized.png "--assign_oetf linear --genmipmap --normalize --normal_mode --encode uastc --zcmp 5" "" "")
  if(APPLE)
    gencmpktx( etc1s_Iron_Bars_001_normal      etc1s_Iron_Bars_001_normal.ktx2     ../srcimages/Iron_Bars/Iron_Bars_001_normal_unnormalized.png "--assign_oetf linear --genmipmap --normalize --normal_mode --encode etc1s" "" "")
  endif()
endif()

gencmpktx( gAMA_chunk_png g03n2c08.ktx2 ../srcimages/g03n2c08.png "--t2 --convert_oetf srgb" "" "" )
gencmpktx( cHRM_chunk_png ccwn2c08.ktx2 ../srcimages/ccwn2c08.png "--t2" "" "" )
gencmpktx( tRNS_chunk_rgb_png tbrn2c08.ktx2 ../srcimages/tbrn2c08.png "--t2" "" "" )
gencmpktx( tRNS_chunk_palette_8-bit_png tbyn3p08.ktx2 ../srcimages/tbyn3p08.png "--t2" "" "" )
gencmpktx( tRNS_chunk_palette_2-bit_png tm3n3p02.ktx2 ../srcimages/tm3n3p02.png "--t2" "" "" )
gencmpktx(
    rgb-mipmap-reference-list
    rgb-mipmap-reference.ktx
    "toktx.filelist.txt"
    "--lower_left_maps_to_s0t0 --mipmap --nometadata"
    ""
    "../srcimages/level0.ppm\\n../srcimages/level1.ppm\\n../srcimages/level2.ppm\\n../srcimages/level3.ppm\\n../srcimages/level4.ppm\\n../srcimages/level5.ppm\\n../srcimages/level6.ppm"
)

add_test( NAME toktx-test.cmp-rgb-reference-2
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:toktx> --nometadata - ../srcimages/rgb.ppm > toktx-test.cmp-rgb-reference-2.ktx && diff rgb-reference.ktx toktx-test.cmp-rgb-reference-2.ktx; rm toktx-test.cmp-rgb-reference-2.ktx"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)
set_tests_properties(
    toktx-test.cmp-rgb-reference-2
PROPERTIES
    ENVIRONMENT TOKTX_OPTIONS=--lower_left_maps_to_s0t0
)

if (NOT ${CPU_ARCHITECTURE} STREQUAL "arm64" )
  gencmpktx( astc_mipmap_ldr_cubemap_6x6 astc_mipmap_ldr_cubemap_6x6.ktx2 "../srcimages/Yokohama3/posx.jpg ../srcimages/Yokohama3/negx.jpg ../srcimages/Yokohama3/posy.jpg ../srcimages/Yokohama3/negy.jpg ../srcimages/Yokohama3/posz.jpg ../srcimages/Yokohama3/negz.jpg" "--test --encode astc --astc_blk_d 6x6 --genmipmap --cubemap" "" "" )
endif()
gencmpktx( astc_ldr_cubemap_6x6 astc_ldr_cubemap_6x6.ktx2 "../srcimages/Yokohama3/posx.jpg ../srcimages/Yokohama3/negx.jpg ../srcimages/Yokohama3/posy.jpg ../srcimages/Yokohama3/negy.jpg ../srcimages/Yokohama3/posz.jpg ../srcimages/Yokohama3/negz.jpg" "--test --encode astc --astc_blk_d 6x6 --cubemap" "" "" )

gencmpktx( astc_ldr_6x6_posx               astc_ldr_6x6_posx.ktx2 ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 6x6" "" "" )
if (NOT ${CPU_ARCHITECTURE} STREQUAL "arm64" )
  gencmpktx( astc_mipmap_ldr_6x6_posx astc_mipmap_ldr_6x6_posx.ktx2 ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 6x6 --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_6x6_posz astc_mipmap_ldr_6x6_posz.ktx2 ../srcimages/Yokohama3/posz.jpg "--test --encode astc --astc_blk_d 6x6 --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_6x6_posy astc_mipmap_ldr_6x6_posy.ktx2 ../srcimages/Yokohama3/posy.jpg "--test --encode astc --astc_blk_d 6x6 --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_6x6_kodim17_fastest    astc_mipmap_ldr_6x6_kodim17_fastest.ktx2    ../srcimages/kodim17.png "--test --encode astc --astc_blk_d 6x6 --genmipmap --astc_quality fastest   " "" "" )
  gencmpktx( astc_mipmap_ldr_6x6_kodim17_fast       astc_mipmap_ldr_6x6_kodim17_fast.ktx2       ../srcimages/kodim17.png "--test --encode astc --astc_blk_d 6x6 --genmipmap --astc_quality fast      " "" "" )
endif()
gencmpktx( astc_mipmap_ldr_6x6_kodim17_medium     astc_mipmap_ldr_6x6_kodim17_medium.ktx2     ../srcimages/kodim17.png "--test --encode astc --astc_blk_d 6x6 --genmipmap --astc_quality medium    " "" "" )

if (NOT ${CPU_ARCHITECTURE} STREQUAL "arm64" )
  gencmpktx( astc_mipmap_ldr_4x4_posx     astc_mipmap_ldr_4x4_posx.ktx2   ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 4x4   --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_6x5_posx     astc_mipmap_ldr_6x5_posx.ktx2   ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 6x5   --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_8x6_posx     astc_mipmap_ldr_8x6_posx.ktx2   ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 8x6   --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_10x5_posx    astc_mipmap_ldr_10x5_posx.ktx2  ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 10x5  --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_8x8_posx     astc_mipmap_ldr_8x8_posx.ktx2   ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 8x8   --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_12x10_posx   astc_mipmap_ldr_12x10_posx.ktx2 ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 12x10 --genmipmap" "" "" )
  gencmpktx( astc_mipmap_ldr_12x12_posx   astc_mipmap_ldr_12x12_posx.ktx2 ../srcimages/Yokohama3/posx.jpg "--test --encode astc --astc_blk_d 12x12 --genmipmap" "" "" )
endif()

gencmpktx( astc_ldr_4x4_FlightHelmet_baseColor    astc_ldr_4x4_FlightHelmet_baseColor.ktx2   ../srcimages/FlightHelmet_baseColor.png "--test --encode astc --astc_blk_d 4x4" "" "")
gencmpktx( astc_ldr_6x5_FlightHelmet_baseColor    astc_ldr_6x5_FlightHelmet_baseColor.ktx2   ../srcimages/FlightHelmet_baseColor.png "--test --encode astc --astc_blk_d 6x5" "" "")
gencmpktx( astc_ldr_8x6_FlightHelmet_baseColor    astc_ldr_8x6_FlightHelmet_baseColor.ktx2   ../srcimages/FlightHelmet_baseColor.png "--test --encode astc --astc_blk_d 8x6" "" "")
gencmpktx( astc_ldr_10x5_FlightHelmet_baseColor   astc_ldr_10x5_FlightHelmet_baseColor.ktx2  ../srcimages/FlightHelmet_baseColor.png "--test --encode astc --astc_blk_d 10x5" "" "")
gencmpktx( astc_ldr_8x8_FlightHelmet_baseColor    astc_ldr_8x8_FlightHelmet_baseColor.ktx2   ../srcimages/FlightHelmet_baseColor.png "--test --encode astc --astc_blk_d 8x8" "" "")
gencmpktx( astc_ldr_12x10_FlightHelmet_baseColor  astc_ldr_12x10_FlightHelmet_baseColor.ktx2 ../srcimages/FlightHelmet_baseColor.png "--test --encode astc --astc_blk_d 12x10" "" "")
gencmpktx( astc_ldr_12x12_FlightHelmet_baseColor  astc_ldr_12x12_FlightHelmet_baseColor.ktx2 ../srcimages/FlightHelmet_baseColor.png "--test --encode astc --astc_blk_d 12x12" "" "")
gencmpktx( astc_ldr_6x6_Iron_Bars_001_normal      astc_ldr_6x6_Iron_Bars_001_normal.ktx2     ../srcimages/Iron_Bars/Iron_Bars_001_normal_unnormalized.png "--test --assign_oetf linear --normalize --normal_mode --encode astc --astc_blk_d 6x6" "" "")
gencmpktx( astc_ldr_5x4_Iron_Bars_001_normal      astc_ldr_5x4_Iron_Bars_001_normal.ktx2     ../srcimages/Iron_Bars/Iron_Bars_001_normal_unnormalized.png "--test --assign_oetf linear --normalize --normal_mode --encode astc --astc_blk_d 5x4" "" "")

gencmpktx( astc_ldr_6x6_arraytex_7 astc_ldr_6x6_arraytex_7.ktx2 "../srcimages/red16.png ../srcimages/orange16.png ../srcimages/yellow16.png ../srcimages/green16.png ../srcimages/blue16.png ../srcimages/indigo16.png ../srcimages/violet16.png" "--test --layers 7 --encode astc --astc_blk_d 6x6" "" "")
gencmpktx( astc_ldr_6x6_arraytex_7_mipmap astc_ldr_6x6_arraytex_7_mipmap.ktx2 "../srcimages/red16.png ../srcimages/orange16.png ../srcimages/yellow16.png ../srcimages/green16.png ../srcimages/blue16.png ../srcimages/indigo16.png ../srcimages/violet16.png" "--test --layers 7 --encode astc --astc_blk_d 6x6 --genmipmap" "" "")
gencmpktx( astc_ldr_6x6_3dtex_7 astc_ldr_6x6_3dtex_7.ktx2 "../srcimages/red16.png ../srcimages/orange16.png ../srcimages/yellow16.png ../srcimages/green16.png ../srcimages/blue16.png ../srcimages/indigo16.png ../srcimages/violet16.png" "--test --depth 7 --encode astc --astc_blk_d 6x6" "" "")

gencmpktx( 3dtex_1_reference_u 3dtex_1_reference_u.ktx2 "../srcimages/red16.png" "--test --t2 --depth 1" "" "")
gencmpktx( 3dtex_7_reference_u 3dtex_7_reference_u.ktx2 "../srcimages/red16.png ../srcimages/orange16.png ../srcimages/yellow16.png ../srcimages/green16.png ../srcimages/blue16.png ../srcimages/indigo16.png ../srcimages/violet16.png" "--test --t2 --depth 7" "" "")
gencmpktx( arraytex_1_reference_u arraytex_1_reference_u.ktx2 "../srcimages/red16.png" "--test --t2 --layers 1" "" "")
gencmpktx( arraytex_7_reference_u arraytex_7_reference_u.ktx2 "../srcimages/red16.png ../srcimages/orange16.png ../srcimages/yellow16.png ../srcimages/green16.png ../srcimages/blue16.png ../srcimages/indigo16.png ../srcimages/violet16.png" "--test --t2 --layers 7" "" "")
gencmpktx( arraytex_7_mipmap_reference_u arraytex_7_mipmap_reference_u.ktx2 "../srcimages/red16.png ../srcimages/orange16.png ../srcimages/yellow16.png ../srcimages/green16.png ../srcimages/blue16.png ../srcimages/indigo16.png ../srcimages/violet16.png" "--test --t2 --layers 7 --genmipmap" "" "")
