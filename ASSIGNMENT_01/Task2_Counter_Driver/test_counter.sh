#!/bin/sh

MODULE="/lib/modules/5.15.0/counter_driver.ko"

echo "================================="
echo " ASSIGNMENT 01 - TASK 2 TEST"
echo "================================="

echo "[1] Loading Counter Driver..."
if ! insmod "$MODULE"; then
    echo "FAIL: Cannot load driver"
    exit 1
fi

echo "[2] Checking device..."
ls -l /dev/counter

echo "[3] Reading counter three times..."
cat /dev/counter
cat /dev/counter
cat /dev/counter

echo "[4] Setting counter to 100..."
echo 100 > /dev/counter
cat /dev/counter

echo "[5] Testing invalid input..."
if echo -5 > /dev/counter; then
    echo "FAIL: Negative number accepted"
else
    echo "PASS: Negative number rejected"
fi

if echo abc > /dev/counter; then
    echo "FAIL: Text accepted"
else
    echo "PASS: Text rejected"
fi

echo "[6] Checking procfs statistics..."
cat /proc/counter_info

echo "[7] Unloading driver..."
rmmod counter_driver

echo "================================="
echo " TASK 2 TEST FINISHED"
echo "================================="
