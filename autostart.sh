#!/usr/bin/env bash

systemctl --user import-environment
feh --randomize --bg-fill ~/.wallpaper/*.png &
picom --experimental-backends --config ~/.config/picom/picom.conf >> /dev/null 2>&1 & 
fcitx5 &
nm-applet &
blueman-applet &
flameshot &
xfce4-panel --disable-wm-check &
