# Terminology for the ASTC Encoder

Like most software, the `astcenc` code base has a set of naming conventions
for variables which are used to ensure both accuracy and reasonable brevity.

:construction: These conventions are being used for new patches, so new code
will conform to this, but older code is still being cleaned up to follow
these conventions.

## Counts

For counts of things prefer `<x>_count` rather than `<x>s`. For example:

* `plane_count`
* `weight_count`
* `texel_count`

Where possible aim for descriptive loop variables, as these are more literate
than simple `i` or `j` variables. For example:

* `plane_index`
* `weight_index`
* `texel_index`

## Ideal, Unpacked Quantized, vs Packed Quantized

Variables that are quantized, such as endpoint colors and weights, have
multiple states depending on how they are being used.

**Ideal values** represent arbitrary numeric values that can take any value.
These are often used during compression to work out the best value before
any quantization is applied. For example, integer weights in the 0-64 range can
take any of the 65 values available.

**Quant uvalues** represent the unpacked numeric value after any quantization
rounding has been applied. These are often used during compression to work out
the error for the quantized value compared to the ideal value. For example,
`QUANT_3` weights in the 0-64 range can only take one of `[0, 32, 64]`.

**Quant pvalues** represent the packed numeric value in the quantized alphabet.
This is what ends up encoded in the ASTC data, although note that the encoded
ordering is scrambled to simplify hardware. For example, `QUANT_3` weights
originally in the 0-64 range can only take one of `[0, 1, 2]`.

For example:

* `weights_ideal_value`
* `weights_quant_uvalue`
* `weights_quant_pvalue`

## Full vs Decimated interpolation weights

Weight grids have multiple states depending on how they are being used.

**full_weights** represent per texel weight grids, storing one weight per texel.

**decimated_weights** represent reduced weight grids, which can store fewer
weights and which are bilinear interpolated to generate the full weight grid.

Full weights have no variable prefix,but decimated weights are stored with
a `dec_` prefix.

* `dec_weights_ideal_value`
* `dec_weights_quant_uvalue`
* `dec_weights_quant_pvalue`

## Weight vs Significance

The original encoder used "weight" for multiple purposes - texel significance
(weight the error), color channel significance (weight the error), and endpoint
interpolation weights. This gets very confusing in functions using all three!

We are slowly refactoring the code to only use "weight" to mean the endpoint
interpolation weights. The error weighting factors used for other purposes are
being updated to use the using the term "significance".

- - -

_Copyright © 2020-2022, Arm Limited and contributors. All rights reserved._
