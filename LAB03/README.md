# LAB 03 - Debugging and Performance Analysis

## Environment
- Ubuntu 26.04.1 LTS (VirtualBox)
- Host Kernel: Linux 7.0.0-31-generic
- Target: ARM Cortex-A9 (QEMU vexpress-a9)
- Target Kernel: Linux 5.15
- Cross Compiler: arm-linux-gnueabihf-gcc

## A. Remote Debugging with GDB

Compiled buggy_app.c for ARM and debugged remotely
using gdb-multiarch and gdbserver.

Issues investigated:
- Array out-of-bounds access
- Use-after-free
- Integer overflow

Debugging operations:
- Set breakpoints
- Step through code
- Inspect variables
- Examine backtrace and ARM registers

## B. Memory Leak Analysis

Used Valgrind to detect memory leaks.

Before fixing:
- Definitely lost: 16,900 bytes in 250 blocks
- Error summary: 3 errors

After fixing:
- Definitely lost: 0 bytes
- Error summary: 0 errors

Also monitored application memory on ARM using:
- /proc/PID/status
- /proc/PID/maps

## C. System Call Tracing and Profiling

Tools:
- strace
- perf

Host profiling:
- Executed perf_target on Ubuntu
- Collected system call statistics
- Collected CPU profiling samples using perf

ARM profiling:
- Cross-compiled perf_target.c
- Executed perf_target_arm in QEMU
- Measured execution time: approximately 0.80 seconds
- Executed strace -c on ARM

ARM strace results:
- Total system calls: 129
- write calls: 75
- mmap2 calls: 10
- openat calls: 8

Note:
Some perf operations encountered limitations or crashes
in the VirtualBox environment.

## D. Real-Time Latency Analysis

Tools:
- cyclictest V 2.80
- gnuplot
- stress-ng

Configuration:
- Threads: 2
- Priority: 80
- Interval: 1000 us
- Loops: 10000

### Standard kernel results

| Metric | Thread 0 | Thread 1 |
|---|---:|---:|
| Min (us) | 3 | 2 |
| Avg (us) | 487 | 486 |
| Max (us) | 5556 | 6517 |

### Second run using CPU stress workflow

| Metric | Thread 0 | Thread 1 |
|---|---:|---:|
| Min (us) | 3 | 4 |
| Avg (us) | 248 | 243 |
| Max (us) | 5062 | 5114 |

Observations:
- Scheduling latency varied between measurements.
- VirtualBox can significantly affect timing results.
- PREEMPT_RT performance was not measured.
- Exact overlap between stress-ng and cyclictest
  was not independently verified.

## Directory Structure

- src/: C source code
- logs/: Trace logs, latency data and histogram
- screenshots/: Screenshots of practical experiments

## Conclusion

LAB 03 demonstrates debugging, memory leak detection,
system call tracing, CPU profiling and real-time
latency analysis on Ubuntu and ARM QEMU.

