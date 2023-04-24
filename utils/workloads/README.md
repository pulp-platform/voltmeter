# Workloads

This directory contains several benchmarks targeting heterogeneous platforms. The provided benchmarks are meant to be compiled as shared libraries, to be specifically used in Voltmeter profiling flow.

The following benchmarks suites are employed:
- `rodinia`: Rodinia 3.1 (patched), containing both CPU (OpenMP, OpenCL) and GPU (CUDA) workloads

Specific platform support is currently provided for:
- NVIDIA Jetson AGX Xavier

## Rodinia

Rodinia is a benchmark suite designed for heterogeneous and parallel systems. It supports OpenMP, OpenCL, and CUDA (see [Rodinia's homepage](http://www.cs.virginia.edu/rodinia/doku.php)).
In this project we patch CUDA with some modifications (`patch/rodinia_3_1_jetson_sharedlibs.patch`) to adapt its compilation flow to usage with the Voltmeter profiling tool.

OpenCL benchmarks are so far not employed.

## Installation

Make sure that:
- CUDA binaries (usually `/usr/local/cuda/bin`) are in your `PATH`
- CUDA libraries (usually `/usr/local/cuda/lib64`) are in your `LD_LIBRARY_PATH`

Download Rodinia 3.1 and apply the patches:
```bash
make rodinia_download
make rodinia_patch
```

Compile Rodinia's OpenMP and CUDA benchmarks:
```bash
make rodinia_omp
make rodinia_cuda
```

Finally, use the scripts provided by Rodinia to generate additional input data for the benchmarks (note that you can modify this basing on the benchmarks and data you employ in your profiler):
```bash
make rodinia_data_gen
```
You can also `make rodinia` to perform all the previous steps.

Note that `make clean` will delete the downloaded `rodinia` directory; to only clean its compiled files, launch `make -C rodinia clean`
