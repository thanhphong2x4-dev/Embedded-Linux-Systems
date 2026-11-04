# LAB01 - Build Artifacts

## Kernel image

File: output/zImage

Compressed Linux Kernel image for the target platform.

## Device Tree

File: output/vexpress-v2p-ca9.dtb

Describes hardware information required by the Kernel.

## Bootloader

File: output/u-boot

U-Boot executable used in the boot process.

## Root filesystem

File: output/initramfs.cpio.gz

Compressed initial root filesystem archive.

## Configuration

- configs/kernel.config
- configs/busybox.config

These files store Kernel and BusyBox configuration settings.
