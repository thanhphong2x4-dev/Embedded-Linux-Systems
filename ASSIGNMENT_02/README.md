# Assignment 02 – Embedded Linux Sensor Monitoring System

## Overview

This project implements a sensor monitoring system using
a Linux kernel module and a multithreaded user-space application.

Target platform: ARM Cortex-A9 (QEMU vexpress-a9)
Target kernel: Linux 5.15
Host OS: Ubuntu 26.04.1 LTS

## System Architecture

The system consists of:

- Kernel sensor driver: /dev/sms_sensor
- Sensor Thread: Acquires sensor measurements
- Circular Buffer: Transfers samples between threads
- Processor Thread: Calculates rolling averages and detects alerts
- Alert Thread: Displays abnormal sensor conditions
- Logger Thread: Saves measurements and rotates log files
- POSIX Message Queues: Support inter-thread communication

## Build

### ARM Application

Compile using arm-linux-gnueabihf-gcc with pthread support.

### Host Mock Application

Compile using GCC with the SMS_HOST_MOCK definition.

### Kernel Module

Build the driver using the Makefile in the driver directory
against the configured Linux 5.15 ARM kernel source.

## Running on QEMU ARM

After booting QEMU, mount POSIX message queues,
load the sensor kernel module, and start the application.

    mkdir -p /dev/mqueue
    mount -t mqueue none /dev/mqueue
    insmod /lib/modules/5.15.0/sms_sensor_driver.ko
    /usr/bin/sms_app_arm -i 500 -t 40 -h 80 -d 60

## Testing

- Task 1: Kernel driver and multithreaded application
- Task 2: Valgrind and memory analysis
- Task 3: GDB, strace and perf profiling
- Task 4: Real-time scheduling and jitter measurement

## Project Structure

- app/: User-space application source and binaries
- driver/: Kernel driver source and module
- logs/: Experimental results
- screenshots/: Testing evidence
- bao_cao/: Technical report in Word and PDF

## Limitations

Jitter measurements are affected by QEMU running inside VirtualBox.
The measured latency does not represent real hardware timing guarantees.

The video demonstration will be prepared separately.
