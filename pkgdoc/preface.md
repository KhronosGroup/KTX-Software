### New Features

* `toktx` now supports 16-bit per component images as input for
Basis Universal encoding. Previously they could previously only be
used to create 16-bit format textures. It also supports using
paletted images as input. These will be expanded to RGB8 or RGBA8
depending on presence of alpha.

* The WASM modules for the libktx and msc_basis_transcoder JS
bindings now include the BC7 and ETC_RG11 transcoders.

### Notable Changes

* `CompressBasisEx` in `libktx` now requires explicit setting of
the `compressionLevel` in its `params` argument. To get the same
behavior as before callers should set this field to
`KTX_DEFAULT_ETC1S_COMPRESSION_LEVEL`.

### Known Issues

* Users making Basisu encoded or block compressed textures for WebGL
must be aware of WebGL restrictions with regard to texture size and
may need to resize images appropriately using the --resize feature of `toktx`.
In general the dimensions of block compressed textures must be a
multiple of the block size and, if `WEBGL_compressed_texture_s3tc` on WebGL 1.0 is expected to be
one of the targets, then the dimensions must be a power of 2.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are
non-deterministic across platforms. Results are valid but level
sizes and data will differ slightly.
See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in
the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading
is disabled. As with the preceeding issue results are valid but
level sizes ``will differ slightly.
See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151)
in the basis_universal repository.

