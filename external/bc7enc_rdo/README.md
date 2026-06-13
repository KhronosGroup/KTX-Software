# About

bc7enc_rdo (from: https://github.com/richgel999/bc7enc_rdo.git) contains two
components with some changes:

  - added \[\[maybe_unused\]\] to silence compiler warnings about unused variables
  - removed bc7 encoder/decoder. See [Notes][#Notes] below.
  - removed CLI components.
  - removed utils.h/cpp because it relies on some heavy classes.

Essentially: only what is absolutely necessary and not already provided by Basis
Universal is left.

bc7enc_rdo has two components:

  - rgbcx:  BC1, BC3, BC4, and BC5 encoders/decoders (+ added BC2 decoder).
  - ert:    rate distortion optimization (RDO) (agnostic to encoders/decoders).

However, currently these two components are compiled together under a single
STATIC library `bc7enc_rdo` (if the need arise where only the RDO-part is
needed, this will be separated into multiple components - E.g.,
`bc7enc_rdo::ert` and `bc7enc_rdo::rgbcx`).

Basis Universal already has BC1-BC5 encoders but the ones provided by
`bc7enc_rdo` are more capable. The decoders *should*(?) be the same for both.

## Building

This is intended to be built as part a static library dependency
(see minimal CMakeLists.txt).

Somewhere in your project's CMakeLists:

```bash
# Make sure bc7enc_rdo is built as a STATIC library
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(path-to-bc7enc_rdo)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_RESET})

# Then append it to your target's link dependencies
target_link_libraries(your_target PRIVATE bc7enc_rdo)
```

## Notes

BC2 encoder is currently not implemented because practically it is very rarely
used (if at all) and can almost always be replaced with the more capable BC3
format (which offers same color RGB block encoding as BC1 but with added alpha
support that is usually better than the one implemented by BC2).

BC6HU/BC6HS HDR formats are already originally not supported by this repo.
Basisu provides bc6hu (emphasise on the **u**) encoder and bc6hu/bc6hs decoders.

BC7 encoder/decoder is not included because Basis Universal already provides a
much more capable encoder (bc7f).

Notes about choice with BC1 vs. BC3 vs. BC7 (from Basisu wiki):

> bc7f encodes so rapidly (on average), that apart from lower VRAM consumption
> (4bpp vs. 8bpp) and better GPU texture cache efficiency, there's little need
> to use BC1 now. BC3 still has an advantage vs. BC7, because it very strongly
> separates how RGB is encoded from the alpha channel, in a predictable way.

## TODOs

There are some features that I'd like to add (from most important to least):

1. [] HDR RDO (this is possible but need to be extremely careful when changing
ert.cpp/h code to handle uint16_t instead of chars/uint8_t)
1. [] BC2 encoder (even though it is seldom used)
1. [] BC6HS encoder

## License

MIT License Rich Geldreich.
