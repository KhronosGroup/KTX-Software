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

## License

MIT License Rich Geldreich.
