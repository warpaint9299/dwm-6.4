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
	killall-
	[[ $? != 0 ]] && picom --experimental-backends --config ~/.config/picom/picom.conf >> /dev/null 2>&1
	sleep 10;
done &
)

fcitx5 &
nm-applet &
blueman-applet &
flameshot &
xfce4-panel --disable-wm-check &
xautolock -time 3 -locker 'i3lock -c 0e0e0e' &
