#!/usr/bin/env bash

DEVICE1="Wacom Intuos BT M Pen stylus"
DEVICE2="Wacom Intuos BT M Pad pad"

case "$1" in
    xournalpp)
        xsetwacom set "$DEVICE1" MapToOutput HEAD-0
        xsetwacom set "$DEVICE2" Button 1 "key Ctrl z"
        xsetwacom set "$DEVICE2" Button 2 "key Shift Ctrl o"
        xsetwacom set "$DEVICE2" Button 3 "key Ctrl plus"
        xsetwacom set "$DEVICE2" Button 8 "key Ctrl minus"
        ;;
esac
