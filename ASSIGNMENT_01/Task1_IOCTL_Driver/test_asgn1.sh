#!/bin/sh

echo "================================"
echo " ASSIGNMENT 01 - TASK 1 TEST"
echo "================================"

MODULE="/lib/modules/5.15.0/asgn1_driver.ko"

echo "[1] Loading driver..."
if ! insmod "$MODULE"; then
    echo "FAIL: Cannot load driver"
    exit 1
fi

echo "[2] Checking device..."
if [ ! -c /dev/asgn1 ]; then
    echo "Creating /dev/asgn1..."
    mknod /dev/asgn1 c 242 0
fi

echo "[3] Testing read/write..."
echo "Hello Assignment 01" > /dev/asgn1
cat /dev/asgn1

echo "[4] Testing ioctl commands..."
/usr/bin/test_ioctl_arm
TEST_RESULT=$?

echo "[5] Checking procfs..."
cat /proc/asgn1_info

echo "[6] Checking kernel logs..."
dmesg | grep asgn1_driver | tail -n 20

echo "[7] Unloading driver..."
rmmod asgn1_driver

if [ "$TEST_RESULT" -eq 0 ]; then
    echo "TEST PROGRAM COMPLETED"
else
    echo "FAIL: ioctl test program returned an error"
    exit 1
fi

echo "================================"
echo " TASK 1 TEST FINISHED"
echo "================================"
