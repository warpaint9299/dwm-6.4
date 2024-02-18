#!/usr/bin/env bash

fcitx5 &
feh --randomize --bg-fill ~/.wallpaper/*.png &
picom --config ~/.config/picom/picom.conf >> /dev/null 2>&1 & 
