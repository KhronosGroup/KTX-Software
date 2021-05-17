# astcenc 2.1 optimization log

This page is a working document explaining the current understanding of the
performance and quality impact of some of the compression trials, refinement
passes, and the heuristics which drive them.

This information will hopefully allow a "tuned up" future version of the codec.
It includes a mixture of both ongoing research, and a log of optimizations
that have been applied during the current post-2.0 development.

## Configuration sensitivity

The most important thing to know for this analysis is that the impact of each
trial and refinement can vary dramatically depending on the codec
configuration. Principally this means:

* block size,
* quality preset,
* image color format.

For example, removing the initial 1 partition 1 plane fast-early-out
optimization actually improves the performance of 4x4 blocks in `-fast` and
`-medium`. However, it makes 5x5 blocks marginally slower and the slow down
increases as block size increases.


## Optimization one: Only search one component for 2/3 partition dual plane

astcenc 2.0 checks the two color components that are least correlated when
testing for use of second plane of weights.

The first change made during the review is to change this to check only the
least correlated color component when encoding two partitions. This causes
negligible quality impact (< 0.008 dB) when using `-fast` and `-medium`,
and slightly larger with `-thorough`. It buys up to ~10% performance
improvement for `-medium`, so it is a significant optimization.

Performance improvement depends on block size; it helps larger block sizes
more than smaller once.

Kodak: 4x4
  Coding time:    Mean: +1.01x    Std: 0.02x
  Image quality:  Mean: -0.00 dB  Std: 0.00 dB

Kodak: 6x6
  Coding time:    Mean: +1.07x    Std: 0.03x
  Image quality:  Mean: -0.00 dB  Std: 0.00 dB

Kodak: 8x8
  Coding time:    Mean: +1.08x    Std: 0.03x
  Image quality:  Mean: -0.00 dB  Std: 0.00 dB

This optimizations also allows some simplification of the implementation of
`find_best_partitionings()` as we only have to manage and return a single
result.
