# LAB03 - Debugging Guide

## Source files

- src/buggy_app.c
- src/leaky_app_buggy.c
- src/leaky_app.c
- src/leaky_app_monitor.c
- src/perf_target.c

## Debugging workflow

1. Compile the application with debugging symbols.
2. Run the application and reproduce the issue.
3. Use GDB to inspect execution.
4. Set breakpoints when needed.
5. Inspect variables and the call stack.
6. Identify the cause before modifying source code.
7. Rebuild and repeat the test after fixing an issue.

## Memory analysis

Check allocation and release paths.

Monitor memory behavior during execution.

Compare observations before and after a fix.

## Evidence

Debugging screenshots are stored in LAB03/screenshots/.
