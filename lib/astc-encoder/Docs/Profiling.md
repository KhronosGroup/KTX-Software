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

## Struct sizes

Optimizing structures for size and spatial locality is important, so we keep
half an eye on the sizes of the structures to make sure we're not exploding the
sizes a lot!

Useful snippet for dumping the main structure sizes:

```c++
	printf("partition_info: %zu\n", sizeof(partition_info));
	printf("decimation_table: %zu\n", sizeof(decimation_table));
	printf("block_size_descriptor: %zu\n", sizeof(block_size_descriptor));
	printf("imageblock: %zu\n", sizeof(imageblock));
	printf("error_weight_block: %zu\n", sizeof(error_weight_block));
	printf("symbolic_compressed_block: %zu\n", sizeof(symbolic_compressed_block));
	printf("compress_fixed_partition_buffers: %zu\n", sizeof(compress_fixed_partition_buffers));
	printf("compress_symbolic_block_buffers: %zu\n", sizeof(compress_symbolic_block_buffers));
```

Released builds return the following sizes of things (in bytes)

| Structure                        | v2.1    |
| -------------------------------- | ------- |
| partition_info                   |    1120 |
| decimation_table                 |  364896 |
| block_size_descriptor            | 3473152 |
| imageblock                       |    4176 |
| error_weight_block               |   14704 |
| symbolic_compressed_block        |     380 |
| compress_fixed_partition_buffers | 1729280 |
| compress_symbolic_block_buffers  | 1745504 |

A lot things are allocated with worst-case `MAX_TEXELS_PER_BLOCK` counts, as
this avoids indirect loads. Setting this to 36 (i.e. enough for 6x6 blocks)
improves overall performance by ~5%. Dynamic sizing would be interesting to
explore, but we really want to avoid indirect pointer-chasing loads on critical
paths.

| Structure                        | v2.1    |
| -------------------------------- | ------- |
| partition_info                   |     224 |
| decimation_table                 |   60876 |
| block_size_descriptor            |  718704 |
| imageblock                       |     752 |
| error_weight_block               |    2464 |
| symbolic_compressed_block        |     380 |
| compress_fixed_partition_buffers | 1475840 |
| compress_symbolic_block_buffers  | 1479840 |
