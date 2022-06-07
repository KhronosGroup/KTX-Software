# 4.x series change log

This page summarizes the major functional and performance changes in each
release of the 4.x series.

All performance data on this page is measured on an Intel Core i5-9600K
clocked at 4.2 GHz, running `astcenc` using AVX2 and 6 threads.

<!-- ---------------------------------------------------------------------- -->
## 4.0

**Status:** In development

The 4.0 release introduces some major performance enhancement, and a number
of larger changes to the heuristics used in the codec to find a more effective
cost:quality trade off.

* **General:**
  * **Feature:** The `-a` alpha weighting option has been re-enabled in the
    backend, and now again applies alpha scaling to the RGB error metrics when
    encoding. This is based on the maximum alpha in each block, not the
    individual texel alpha values used in the earlier implementation.
  * **Feature:** The command line tool now has `-repeats <count>` for testing,
    which will iterate around compression and decompression `count` times.
    Reported performance metrics also now separate compression and
    decompression scores.
  * **Optimization:** Angular endpoint min/max weight selection is restricted
    to weight `QUANT_11` or lower. Higher quantization levels assume default
    0-1 range, which is less accurate but must faster.
  * **Optimization:** Maximum weight quantization for later trials is selected
    based on the weight quantization of the best encoding from the 1 plane 1
    partition trial. This significantly reduces the search space for the later
    trials with more planes or partitions.
  * **Optimization:** Weight scrambling has been moved into the physical layer;
    the rest of the codec now uses linear order weights.
  * **Optimization:** Weight packing has been moved into the physical layer;
    the rest of the codec now uses unpacked weights in the 0-64 range.
  * **Optimization:** Consistently vectorize the creation of unquantized weight
    grids when they are needed.
  * **Optimization:** Remove redundant per-decimation mode copies of endpoint
    and weight structures, which were really read-only.
  * **Optimization:** Early-out the same endpoint mode color calculation if it
    cannot be applied.
- - -

_Copyright © 2022, Arm Limited and contributors. All rights reserved._
