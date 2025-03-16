#!/usr/bin/env bash

systemctl --user import-environment

(
while true; do
	feh --recursive --randomize --bg-fill ~/.wallpaper/*.png >> /dev/null 2>&1
	sleep 120;
done &
)

(
while true; do
	[[ $(pgrep -x 'picom') ]] || picom --config ~/.config/picom/picom.conf >> /dev/null 2>&1
	sleep 3;
done &
)

# When any browser is focused on YouTube and in fullscreen mode, kill xautolock
(
while true; do
	for browser in "firefox" "brave" "chrome"; do
		xdotool search --class "${browser}" | while read wm_id; do
			if [[ 0 -ne $(xprop -id ${wm_id} | grep '_NET_WM_STATE_FULLSCREEN' | wc -l) ]]; then
				if [[ 0 -ne $(xprop -id ${wm_id} | grep -E 'YouTube|小宝影院' | wc -l) ]]; then
					[[ $(pgrep -x 'xautolock') ]] && killall xautolock
				fi
			fi
		done
	done
	sleep 5;
done &
)
(

while true; do
	[[ $(pgrep -x 'xautolock') ]] || xautolock -detectsleep -time 5 -locker 'i3lock -c 0e0e0e -n' >> /dev/null 2>&1 &
	sleep 300;
done &
)

xfce4-panel --disable-wm-check &
blueman-applet &
flameshot &
fcitx5 &
tmux has-session && exec kitty -e tmux attach || exec kitty -e tmux &
exec xset -b & # disable console bell volume
