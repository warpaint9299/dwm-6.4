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

(
while true; do
	wm_id_chrome=$(xdotool search --onlyvisible --class 'google-chrome');
	wm_id_firefox=$(xdotool search --onlyvisible --class 'firefox');
	wm_id_brave=$(xdotool search --onlyvisible --class 'brave-browser');

	if [[ -n ${wm_id_chrome} ]]; then
		if [[ '_NET_WM_STATE_FULLSCREEN' -eq $(xprop -id ${wm_id_chrome} _NET_WM_STATE | cut -d\= -f2 | tr -d ' ') ]]; then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	fi

	if [[ -n ${wm_id_firefox} ]]; then
		if [[ 0 -ne $( ps -aux | grep firefox | grep -v grep | wc -l ) ]];then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	fi

	if [[ -n ${wm_id_brave} ]]; then
		if [[ 0 -ne $( ps -aux | grep brave | grep -v grep | wc -l ) ]];then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	fi

	sleep 5;
done &
)
(

while true; do
	[[ $(pgrep -x 'xautolock') ]] || xautolock -detectsleep -time 5 -locker 'i3lock -c 0e0e0e -n' >> /dev/null 2>&1 &
	sleep 300;
done &
)

blueman-applet &
flameshot &
fcitx5 &
tmux has-session && exec kitty -e tmux attach || exec kitty -e tmux &
xfce4-panel --disable-wm-check &
exec xset -b & # disable console bell volume
