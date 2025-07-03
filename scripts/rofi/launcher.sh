#!/usr/bin/env bash

## Author : Aditya Shakya (adi1090x)
## Github : @adi1090x
## Modifier: @warpaint9299
#
## Rofi   : Launcher (Modi Drun, Run, File Browser, Window)
#
## Available Styles
#
## launcher-style

dir="$HOME/.dwm/scripts/rofi"
theme="launcher-style"

rofi -show drun  \
     -modes drun,run \
     -theme ${dir}/${theme}.rasi
