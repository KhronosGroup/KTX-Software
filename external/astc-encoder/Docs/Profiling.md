# Profiling astcenc

This page contains some developer notes on profiling `astcenc` using command
line tools on Linux hosts.

## Building for profiling

It is recommended to profile release builds, but you will need debug symbols.
It is also recommended to disable link-time optimization to get call stacks
that vaguely resemble the source code, although beware because this means that
you are not quite profiling the reality of full release builds.

Both of these can be achieved using the following CMake build type:

```shell
 -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## Running Callgrind tools

We provide a helper script that wraps Callgrind for hotspot profiling, although
beware that it only currently supports profiling LDR input images and the
single compression mode.

This script requires the following tools on your `PATH`:

  * valgrind
  * gprof2dot
  * dot

Run the helper script from the root of the repository using, e.g.:

```shell
python3 ./Test/astc_profile_valgrind.py <image.png> --test-quality fastest
```

The output will be two files:

- perf_&lt;quality&gt;.png: an annotated call graph.
- perf_&lt;quality&gt;.txt: the top N functions table.

### Viewing disassembly

Standard syntax x86-64 disassembly can be generated using:

```shell
objdump -C -M intel --no-show-raw -d -S <binary> > dis.txt
```

- - -

_Copyright © 2020-2022, Arm Limited and contributors. All rights reserved._
