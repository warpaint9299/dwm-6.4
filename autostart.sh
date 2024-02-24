#!/usr/bin/env bash
feh --randomize --bg-fill ~/.wallpaper/*.png &
picom --experimental-backends --config ~/.config/picom/picom.conf >> /dev/null 2>&1 & 
nm-applet &
fcitx5 &
flameshot &
xfce4-panel --disable-wm-check &
