# LAB02 - Driver Structure

## Source files

- driver/lab2_driver.c
- driver/Makefile

## Main concepts

Character devices provide a file-based interface
between user-space applications and Kernel drivers.

Common character-device operations include:

- open()
- read()
- write()
- release()

## Additional interfaces

Procfs can expose Kernel information to user space.

Sysfs can expose device attributes and configuration.

## Related files

- scripts/test_procfs.sh
- driver/README.md
- scripts/README.md
