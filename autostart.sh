#!/usr/bin/env bash

systemctl --user import-environment

(
while true; do
	feh --randomize --bg-fill ~/.wallpaper/*.png >> /dev/null 2>&1
	sleep 300;
done &
)

(
while true; do
	[[ $(pgrep -x 'picom') ]] || picom --experimental-backends --config ~/.config/picom/picom.conf >> /dev/null 2>&1
	sleep 3;
done &
)
fcitx5 &
flameshot &
nm-applet &
blueman-applet &
xfce4-panel --disable-wm-check &
xautolock -time 3 -locker 'i3lock -c 0e0e0e' &
