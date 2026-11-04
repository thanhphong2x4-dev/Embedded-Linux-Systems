# LAB02 - Testing Guide

## Character device testing

Verify that the device can be opened and closed.

Test writing and reading data.

Check how the driver handles invalid operations.

## Procfs and Sysfs

Inspect available procfs entries and sysfs attributes.

Check whether reported values match driver behavior.

## Flash storage

Review the supplied NAND and JFFS2 screenshots.

Relevant evidence includes:

- NAND simulation and MTD information
- Flash erase operations
- JFFS2 mounting and writing
- Persistence checks
- NAND dump results

## Evidence

Screenshots are stored in LAB02/screenshots/.

These instructions describe checks to perform.
They do not represent newly executed tests.
