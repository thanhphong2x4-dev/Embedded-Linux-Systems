# Embedded Linux Systems

A collection of three laboratory exercises and two assignments covering embedded Linux system construction, kernel drivers, debugging, performance analysis, and a multithreaded sensor-monitoring application.

## Contents

| Module | Topic | Key deliverables |
| --- | --- | --- |
| [LAB 01](#lab-01--embedded-linux-system-development) | Embedded Linux system development | Linux 5.15 `zImage`, U-Boot, device tree, BusyBox initramfs, QEMU boot |
| [LAB 02](#lab-02--linux-device-driver-and-flash-storage) | Linux drivers and flash storage | Character device, procfs/sysfs, NAND simulation, JFFS2, BusyBox initialization |
| [LAB 03](#lab-03--debugging-and-performance-analysis) | Debugging and performance analysis | GDB, Valgrind, strace, perf, cyclictest and latency logs |
| [ASSIGNMENT 01](#assignment-01--ioctl-and-atomic-counter-drivers) | Kernel-driver programming | IOCTL character driver and atomic counter driver |
| [ASSIGNMENT 02](#assignment-02--sensor-monitoring-system) | Multithreaded sensor monitoring | Sensor driver, four-thread application, memory/debugging and jitter experiments |

> **Scope of evidence:** This README documents repository contents and the results recorded in the available lab notes. Where a test outcome or numerical measurement has not been independently checked against its underlying log, no additional result is asserted.

---

## LAB 01 – Embedded Linux System Development

### Environment

- Target platform: ARM Cortex-A9, emulated with QEMU `vexpress-a9`.
- Target Linux kernel: 5.15.
- Bootloader: U-Boot 2022.04.
- Minimal user space: BusyBox and an initramfs root filesystem.

### A. Linux kernel configuration and build

Configured and built the Linux kernel for the ARM target. The compressed kernel image is provided as [`LAB01/output/zImage`](LAB01/output/zImage). The recorded kernel configuration is available at [`LAB01/configs/kernel.config`](LAB01/configs/kernel.config).

### B. U-Boot bootloader

Built U-Boot for the emulated target. U-Boot initializes the boot environment and transfers control to the Linux kernel. The build artifact is [`LAB01/output/u-boot`](LAB01/output/u-boot).

### C. Device Tree, BusyBox and initramfs

- The device tree blob [`vexpress-v2p-ca9.dtb`](LAB01/output/vexpress-v2p-ca9.dtb) describes target hardware to the kernel.
- BusyBox supplies a small set of Unix command-line utilities suitable for embedded systems.
- The initial root filesystem is packaged as [`initramfs.cpio.gz`](LAB01/output/initramfs.cpio.gz).
- The BusyBox build configuration is preserved in [`LAB01/configs/busybox.config`](LAB01/configs/busybox.config).

### D. QEMU boot and verification

The lab integrates the bootloader, kernel, device tree and initial root filesystem to boot an ARM Linux environment on QEMU. The exercise includes boot-log inspection and verification of the BusyBox user-space shell.

### Build artifacts and documentation

| File | Purpose |
| --- | --- |
| [`output/zImage`](LAB01/output/zImage) | Compressed Linux kernel image |
| [`output/u-boot`](LAB01/output/u-boot) | U-Boot executable |
| [`output/vexpress-v2p-ca9.dtb`](LAB01/output/vexpress-v2p-ca9.dtb) | Device tree blob |
| [`output/initramfs.cpio.gz`](LAB01/output/initramfs.cpio.gz) | Initial root filesystem archive |
| [`configs/kernel.config`](LAB01/configs/kernel.config) | Kernel configuration |
| [`configs/busybox.config`](LAB01/configs/busybox.config) | BusyBox configuration |

Additional documentation: [`COMPONENTS.md`](LAB01/docs/COMPONENTS.md), [`BUILD_ARTIFACTS.md`](LAB01/docs/BUILD_ARTIFACTS.md), and the [LAB 01 report](LAB01/bao_cao/SE182514_NguyenThanhPhong_Lab01_BaoCao.pdf).

### Conclusion

LAB 01 demonstrates the relationship between U-Boot, the Linux kernel, the device tree, and a BusyBox-based root filesystem in a reproducible ARM/QEMU learning environment.

---

## LAB 02 – Linux Device Driver and Flash Storage

### Environment

- Embedded Linux target: ARM/QEMU environment developed in LAB 01.
- Kernel module source and Makefile: [`LAB02/driver/`](LAB02/driver/).
- Test scripts, notes and experiment screenshots are maintained in the LAB 02 directory.

### A. Character device driver

Implemented a Linux character device driver with the principal file operations `open()`, `read()`, `write()` and `release()`. Created a Makefile for compiling the kernel module and tested reading and writing data through the device interface.

### B. Procfs, sysfs and extended driver operations

- Added a procfs information interface, including `/proc/lab2_info`.
- Added sysfs attributes for exposing device state and settings.
- The driver implementation also includes seeking and reading behavior addressed during the lab extension, including `llseek`, `reverse_read`, and an overwrite counter.
- Checked the interfaces in the ARM/QEMU setup.

### C. NAND flash emulation and JFFS2

- Used `nandsim` to emulate NAND flash and inspected the associated MTD information.
- Performed flash erase operations and prepared a JFFS2 image.
- Mounted JFFS2 and exercised file writes and persistence checks.
- Captured NAND dump evidence.

Supporting screenshots include [`03_lab2_nandsim_mtd.png`](LAB02/screenshots/03_lab2_nandsim_mtd.png), [`09_lab2_jffs2_mount.png`](LAB02/screenshots/09_lab2_jffs2_mount.png), [`11_lab2_jffs2_persistence.png`](LAB02/screenshots/11_lab2_jffs2_persistence.png), and [`12_lab2_nandump.png`](LAB02/screenshots/12_lab2_nandump.png).

### D. BusyBox init and module auto-loading

Worked with BusyBox initialization to load kernel modules automatically during system startup.

### Source files and evidence

- Driver: [`lab2_driver.c`](LAB02/driver/lab2_driver.c) and [`Makefile`](LAB02/driver/Makefile).
- Test script: [`test_procfs.sh`](LAB02/scripts/test_procfs.sh).
- Design notes: [`DRIVER_STRUCTURE.md`](LAB02/docs/DRIVER_STRUCTURE.md).
- Test guide: [`TESTING_GUIDE.md`](LAB02/docs/TESTING_GUIDE.md).
- Screenshots: [`LAB02/screenshots/`](LAB02/screenshots/).

### Conclusion

LAB 02 extends the ARM Linux environment with kernel device-driver operations, user-visible procfs/sysfs interfaces, NAND flash simulation and the JFFS2 filesystem.

---

## LAB 03 – Debugging and Performance Analysis

### Environment

- Ubuntu 26.04.1 LTS in VirtualBox.
- Host kernel: Linux 7.0.0-31-generic.
- Target: ARM Cortex-A9 (QEMU `vexpress-a9`).
- Target kernel: Linux 5.15.
- Cross compiler: `arm-linux-gnueabihf-gcc`.

### A. Remote debugging with GDB

Compiled `buggy_app.c` for ARM and debugged it remotely using `gdb-multiarch` and `gdbserver`. Investigated array out-of-bounds access, use-after-free, and integer overflow. Debugging work included breakpoints, stepping, variable inspection, backtraces and ARM register inspection.

### B. Memory leak analysis

Used Valgrind to compare memory behavior before and after correcting an application.

| Valgrind metric | Before fixing | After fixing |
| --- | ---: | ---: |
| Definitely lost | 16,900 bytes in 250 blocks | 0 bytes |
| Error summary | 3 errors | 0 errors |

Also inspected ARM process memory through `/proc/PID/status` and `/proc/PID/maps`.

### C. System-call tracing and profiling

- Used `strace` for system-call tracing and `perf` for host-side CPU profiling.
- Cross-compiled `perf_target.c` and executed `perf_target_arm` under QEMU.
- Recorded ARM execution time of approximately **0.80 seconds**.

Recorded ARM `strace -c` summary:

| System call metric | Count |
| --- | ---: |
| Total calls | 129 |
| `write` | 75 |
| `mmap2` | 10 |
| `openat` | 8 |

Some `perf` operations encountered limitations or crashes within the VirtualBox environment.

### D. Real-time latency analysis

Tools: `cyclictest` v2.80, `gnuplot`, and `stress-ng`.

Configuration: 2 threads; priority 80; interval 1,000 µs; 10,000 loops.

**Standard kernel run**

| Metric | Thread 0 | Thread 1 |
| --- | ---: | ---: |
| Minimum (µs) | 3 | 2 |
| Average (µs) | 487 | 486 |
| Maximum (µs) | 5,556 | 6,517 |

**Second run using the CPU-stress workflow**

| Metric | Thread 0 | Thread 1 |
| --- | ---: | ---: |
| Minimum (µs) | 3 | 4 |
| Average (µs) | 248 | 243 |
| Maximum (µs) | 5,062 | 5,114 |

Latency varied between measurements. VirtualBox virtualization may substantially influence the results. **PREEMPT_RT was not measured**, and exact overlap between `stress-ng` and `cyclictest` was not independently verified. The runs should therefore not be interpreted as proof that stressing the CPU improved latency.

### Source files, logs and evidence

- C source code: [`LAB03/src/`](LAB03/src/).
- System-call logs and latency measurements: [`LAB03/logs/`](LAB03/logs/).
- Histogram image: [`latency_histogram.png`](LAB03/logs/latency_histogram.png).
- Debugging notes: [`DEBUGGING_GUIDE.md`](LAB03/docs/DEBUGGING_GUIDE.md).
- Analysis notes: [`PERFORMANCE_ANALYSIS.md`](LAB03/docs/PERFORMANCE_ANALYSIS.md).
- Experiment screenshots: [`LAB03/screenshots/`](LAB03/screenshots/).

### Conclusion

LAB 03 explores remote application debugging, memory leak detection, syscall tracing, CPU profiling and scheduling-latency measurement on Ubuntu and ARM QEMU, including the limitations of running measurements inside a virtual machine.

---

## ASSIGNMENT 01 – IOCTL and Atomic Counter Drivers

### Overview

ASSIGNMENT 01 contains two Linux kernel-driver tasks: an extended character device with an IOCTL interface, and a separate atomic counter driver.

### Task 1. Character device driver with IOCTL

- Implemented kernel file operations including `open()`, `read()`, `write()` and `release()`.
- Added an `unlocked_ioctl` handler to handle device-specific commands.
- Used a mutex to protect shared driver state.
- Exposed information via procfs and read-only sysfs attributes such as `buffer_len`, `open_count` and `last_data`.
- Included test program and shell-script support.

**Files:** [`asgn1_driver.c`](ASSIGNMENT_01/Task1_IOCTL_Driver/asgn1_driver.c), [`asgn1_ioctl.h`](ASSIGNMENT_01/Task1_IOCTL_Driver/asgn1_ioctl.h), [`test_ioctl.c`](ASSIGNMENT_01/Task1_IOCTL_Driver/test_ioctl.c), [`test_asgn1.sh`](ASSIGNMENT_01/Task1_IOCTL_Driver/test_asgn1.sh), and [`Makefile`](ASSIGNMENT_01/Task1_IOCTL_Driver/Makefile).

**Evidence:** [`Task 1 screenshots`](ASSIGNMENT_01/Task1_IOCTL_Driver/screenshots/).

### Task 2. Atomic counter driver

- Implemented a counter using `atomic_t` to support atomic updates.
- The read path obtains the current value and increments the counter.
- The write path sets the counter to a non-negative integer.
- Exposed counter statistics via `/proc/counter_info`.
- Registered the counter as a miscellaneous device and included module load/unload logic.
- Supplied a shell test script for counter operations and invalid-input checks.

**Files:** [`counter_driver.c`](ASSIGNMENT_01/Task2_Counter_Driver/counter_driver.c), [`test_counter.sh`](ASSIGNMENT_01/Task2_Counter_Driver/test_counter.sh), and [`Makefile`](ASSIGNMENT_01/Task2_Counter_Driver/Makefile).

**Evidence:** [`Task 2 screenshots`](ASSIGNMENT_01/Task2_Counter_Driver/screenshots/).

### Report and conclusion

The [ASSIGNMENT 01 report](ASSIGNMENT_01/Assignment_01_SE182514_Nguyen_Thanh_Phong.pdf) accompanies the two driver implementations. The assignment demonstrates extending the Linux character-device interface with IOCTL, protecting shared state, and building an atomic counter accessible from user space.

---

## ASSIGNMENT 02 – Sensor Monitoring System

### Overview

Developed a sensor-monitoring application with a Linux sensor kernel module and a multithreaded user-space program. The repository contains both real-device and mock sensor paths, ARM/host application builds, experiment logs, and a written report.

### Task 1. Sensor driver and multithreaded application

- Implemented a kernel module in [`sms_sensor_driver.c`](ASSIGNMENT_02/driver/sms_sensor_driver.c), with mutex protection and an IOCTL handler.
- Used the `SMS_SET_INTERVAL` IOCTL command to configure a sampling interval.
- Organized the user application around **sensor**, **processor**, **alert**, and **logger** threads created with `pthread_create()` and joined with `pthread_join()`.
- Implemented a circular buffer for sharing sensor data, using a mutex and condition variable for synchronization.
- Included mock and real sensor-thread implementations, together with tests for interval configuration and timed execution.

**Key code:** [`app/`](ASSIGNMENT_02/app/), [`driver/`](ASSIGNMENT_02/driver/), and [`test_sensor_ioctl.c`](ASSIGNMENT_02/driver/test_sensor_ioctl.c).

### Task 2. Memory monitoring and leak analysis

- Captured application memory samples and process memory maps.
- Recorded a Valgrind run and saved memory-analysis screenshots.
- Preserved logs for examining allocation behavior and heap/anonymous mappings.

**Logs:** [`logs/task2/`](ASSIGNMENT_02/logs/task2/).

### Task 3. Debugging, tracing and profiling

- Captured GDB thread inspection and backtrace evidence.
- Recorded system-call traces using `strace`.
- Saved `perf` statistics and profiling reports.

**Logs:** [`logs/task3/`](ASSIGNMENT_02/logs/task3/).

### Task 4. Scheduling jitter experiments

- Included a timing test application, [`jitter_test.c`](ASSIGNMENT_02/app/jitter_test.c).
- Recorded separate jitter outputs for **SCHED_OTHER** and **SCHED_FIFO** scheduling workflows.
- Saved comparison screenshots and latency logs. The repository file listing confirms the evidence files, but numerical comparisons should be read from the underlying logs rather than assumed here.

**Logs:** [`logs/task4/`](ASSIGNMENT_02/logs/task4/).

### Directory structure and evidence

| Path | Contents |
| --- | --- |
| [`app/`](ASSIGNMENT_02/app/) | Sensor-monitoring app, worker threads, circular buffer and jitter test |
| [`driver/`](ASSIGNMENT_02/driver/) | Sensor kernel driver, header, Makefile and IOCTL test |
| [`logs/task2/`](ASSIGNMENT_02/logs/task2/) | Memory usage, maps and Valgrind logs |
| [`logs/task3/`](ASSIGNMENT_02/logs/task3/) | GDB, strace and perf logs |
| [`logs/task4/`](ASSIGNMENT_02/logs/task4/) | Jitter experiment outputs |
| [`screenshots/`](ASSIGNMENT_02/screenshots/) | Screenshots organized by task |
| [`bao_cao/`](ASSIGNMENT_02/bao_cao/) | Written assignment report |

See the [ASSIGNMENT 02 report](ASSIGNMENT_02/bao_cao/Assignment02_Sensor_Monitoring_Report.pdf) and the [assignment's existing README](ASSIGNMENT_02/README.md) for further context.

### Conclusion

ASSIGNMENT 02 integrates kernel-to-user communication, synchronized multithreaded processing, application memory inspection, debugging/profiling tools, and scheduling-jitter experiments into one sensor-monitoring project.

---

## Repository structure

```text
Embedded-Linux-Systems/
├── LAB01/             # Kernel, bootloader, device tree and initramfs
├── LAB02/             # Character driver, storage experiments and evidence
├── LAB03/             # Debugging programs, logs and latency analysis
├── ASSIGNMENT_01/     # IOCTL driver and atomic counter driver
├── ASSIGNMENT_02/     # Sensor-monitoring application and experiments
├── docs/              # Additional repository documentation
└── README.md          # Consolidated overview of all five exercises
```

## Notes

- The lab environments and results above are based on the available descriptions and command outputs, not a new execution of the experiments.
- Measurements obtained in a virtual machine may vary across runs and host conditions.
- Subdirectory documentation and original source files are retained; this root README is the single entry point for reviewing all five exercises.
