# Voltmeter
*Voltmeter* is a flexible and lightweight power and performance profiling tool for heterogeneous platforms.
It generates performance counters and power measurements traces to be analyzed post-mortem.

Voltmeter supports tracing for the following platforms:
- NVIDIA Jetson AGX Xavier
  - CPU (Carmel SoC), driver based on ARM PMUv3 registers access
  - GPU (NVIDIA Volta GPU), driver based on NVIDIA CUPTI Event API

## Installation \& usage
The steps for *default* Voltmeter installation and usage are described in the following.

To compile Voltmeter, first make sure you have configured your manifest `Voltmeter.yml`, then execute
```bash
make all
```
You can run Voltmeter with
```bash
make run
```

### Configuration
Voltmeter compilation and execution (Makefile targets `all` and `run`, respectively) depend on a YML manifest. If you change the manifest, Voltmeter will need to be recompiled in most cases. To automatically handle this, you are suggested to run Voltmeter only through the Makefile.

By default, the manifest is `Voltmeter.yml`, in this project's root directory. However, any file can be used by setting the environment variable `VOLTMETER_YML`, e.g.
```bash
VOLTMETER_YML=./utils/ymls/Voltmeter-default.yml make run
```
The directory `utils/ymls/` contains a collection of manifest files for useful, pre-defined Voltmeter configurations.
For a detailed description of the manifest file, its parameters, and Voltmeter configuration, consult:
- the documentation within the default manifest file `./utils/ymls/Voltmeter-default.yml`
- Voltmeter's documentation: `./install/voltmeter --help`
- [Manifest file and profiler configuration](#manifest-file-and-profiler-configuration) paragraph

### Platform-specific steps
Depending on the target platform, it might be necessary to perform further installation steps.
#### NVIDIA Jetson AGX Xavier
NVIDIA Volta GPU's profiler is based on CUPTI Event API part of the CUDA toolkit. If different from the default one, you can point to your CUDA installation in `config/config.mk`.

Additionally, to make the performance counters of the Carmel SoC CPU observable from userspace, a kernel module has to be installed.
```bash
make kernelmod
```

## Manifest file and profiler configuration
Voltmeter comes with many profiling modes and parameters, which you can set up in the manifest YML file.
- Platform parameters:
  - `platform`: Select the target platform for the profiler to be compiled and deployed. The possible choices are:
    - `jetson_agx_xavier` = NVIDIA Jetson AGX Xavier board; its CPU and GPU are supported.
  - `profile_cpu`: Enable the profiling of CPU performance counters. It can be either `True` or `False`.
  - `profile_gpu`: Enable the profiling of GPU performance counters. It can be either `True` or `False`.
  - `frequencies_cpu`: CPU frequencies to run the profiling. It is a list of integer values, e.g., `[2265600]`. Required if `profile_cpu` is `True`.
  - `frequencies_gpu`: GPU frequencies to run the profiling. It is a list of integer values, e.g., `[522750000, 1377000000]`. Required if `profile_gpu` is `True`.

- Profiler parameters:
  - `num_run`: Number of times to repeat each profiled benchmark in a given configuration, useful for averaging purposes. Default is `3`. Data from different runs of the same benchmark in the same configuration is collected in the same trace file.
  - `sample_period_us`: Sample period for performance counter values and power measures (in microseconds). Default is `100000` (i.e., 0.1 s).
  - `debug_gdb`: Compile Voltmeter's binary with debug information for `gdb`. It can be either `True` or `False`.

- Voltmeter arguments:
  - `events`: Decide how to pass the events to profile to Voltmeter. The possible options are:
    - `all_events` = Profile all events exposed by the devices enabled for profiling. The way *all* events are collected is defined within Voltmeter source code and depends on the platform. You can customize it to your needs.
    - `config` = Take events from a JSON configuration file. You can find examples in `utils/jetson_agx_xavier/perf-events/`
    - `cli` = Pass the IDs of the events to profile to Voltmeter through command-line interface.
  - `config_cpu`: Path of the JSON file containing CPU event IDs to profile for each frequency (at least for the frequencies selected in `frequencies_cpu`). Either absolute, or relative to this project's root directory. Required if `events` is `config` and `profile_cpu` is `True`.
  - `config_gpu`: Path of the JSON file containing GPU event IDs to profile for each frequency (at least for the frequencies selected in `frequencies_gpu`). Either absolute, or relative to this project's root directory. Required if `events` is `config` and `profile_gpu` is `True`.
  - `cli_cpu`: A list of IDs for the CPU events to be profiled. Is should contain the event IDs for each core, in the format `[event0_core0, event1_core0, ... eventN_core0, event0_core1, ... eventN_coreM]`. Required if `events` is `cli` and `profile_cpu` is `True`.
  - `cli_gpu`: A list of IDs for the GPU events to be profiled. Required if `events` is `cli` and `profile_gpu` is `True`.
  - `mode`: The execution mode of the profiler. The possible options are:
    - `characterization` = Platform characterization: any set of events can be profiled, independently on the compatibility among them; the required number of serial passes (to profile all incompatible events) is automatically calculated and executed; multiple traces are generated, one for each serial pass.
    - `profile` = Enforce that only events compatible with each other (i.e., that can be profiled all together with only 1 pass) are used for the profiling; this mode is useful to collect a dataset for power model training.
    - `num_passes` = Voltmeter only takes in a set of events and computes how many serial passes would be necessary to track all of them, e.g., whether the events are compatible among each other. No profiling happens in this mode.
  - `trace_dir`: Directory to save the traces; either absolute, or relative to this project's root directory. The traces are binary files and their format depends on the platform and its profiled devices. Details on traces format are documented within Voltmeter source code.
  - `benchmarks`: A sequence of items describing the benchmarks to profile in Voltmeter, with the following parameters:
    - `name`: Name of the benchmark, for labeling purposes.
    - `path`: Path of the benchmark, either absolue, or relative to this project's root directory. The benchmark must be compiled as a shared library, which is then included in Voltmeter's compilation flow through this parameter. You can usually compile your benchmark as a shared library by using `-o *.so -fPIC -shared`, or `-o *.so -shared -Xcompiler -fPIC` for cross-compilers. Running benchmarks as a shared library is required as some performance counters APIs (i.e., CUPTI) can only access the performance counters data triggered by the same process from where they are being collected. A benchmark suite already prepared for usage with Voltmeter is available under `utils/workloads/`. Read `utils/workloads/README.md` for further information.
    - `args`: The argument to be passed to the benchmark, as you would pass them through CLI.

## Publications
```
@inproceedings{mazzola2022data,
  title={A Data-Driven Approach to Lightweight DVFS-Aware Counter-Based Power Modeling for Heterogeneous Platforms},
  author={Mazzola, Sergio and Benz, Thomas and Forsberg, Bj{\"o}rn and Benini, Luca},
  booktitle={Embedded Computer Systems: Architectures, Modeling, and Simulation: 22nd International Conference, SAMOS 2022, Samos, Greece, July 3--7, 2022, Proceedings},
  pages={346--361},
  year={2022},
  organization={Springer}
}
```

## License
Voltmeter is made available under permissive open-source licenses.
All files but the ones contained in `src/deps` are released under Apache License 2.0 (`Apache-2.0`), see `LICENSE`.
The `src/deps` directory contains third-party sources that come with their own licenses. See their respective folders for the licenses used.
