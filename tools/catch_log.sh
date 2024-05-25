#!/usr/bin/env bash

rm -f crash.log

adb logcat -c

adb logcat > crash.log | tail -f crash.log
