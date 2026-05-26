# About

bc7enc (from: https://github.com/richgel999/bc7enc_rdo.git) contains two
components with minimal changes (e.g., to silence compiler warnings about unused
variables):

  - rgbcx:  BC1, BC3, BC4, and BC5 encoders/decoders.
  - rdo:    rate distortion optimization (RDO) (agnostic to encoders/decoders).

# Notes

BC2 encoders are not implemented because practically it is very rarely used (if
at all) and can almost always be replaced with the more capable BC3 format
(which offers same color RGB block encoding as BC1 but with added alpha support
that is usually better than the one implemented by BC2).

BC6HU/BC6HS HDR formats are already originally not supported by this repo.
Basisu provides bc6hu (enfasize on the **u**) encoder and bc6hu/bc6hs decoders.

BC7 encoder/decoder is not included because Basis Universal already provides a
much more capable encoder (bc7f).

Notes about choice with BC1 vs. BC3 vs. BC7 (from Basisu wiki):

> bc7f encodes so rapidly (on average), that apart from lower VRAM consumption
> (4bpp vs. 8bpp) and better GPU texture cache efficiency, there's little need
> to use BC1 now. BC3 still has an advantage vs. BC7, because it very strongly
> separates how RGB is encoded from the alpha channel, in a predictable way.

## License

MIT License Rich Geldreich.
