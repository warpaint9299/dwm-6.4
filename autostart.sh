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

blueman-applet &
flameshot &
fcitx5 &
greenclip daemon > /dev/null &
xfce4-panel --disable-wm-check &
# xautolock -time 5 -locker 'i3lock -c 0e0e0e' >> /dev/null 2>&1 &
tmux has-session && exec st -e tmux attach || exec st -e tmux &
