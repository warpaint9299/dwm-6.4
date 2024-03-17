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

(
while true; do
	wm_id_chrome = $(xdotool search --onlyvisible --class 'google-chrome');
	wm_id_firefox-esr = $(xdotool search --onlyvisible --class 'firefox-esr');

	if [[ -n ${wm_id_chrome} ]]; then
		if [[ '_NET_WM_STATE_FULLSCREEN' -eq $(xprop -id ${wm_id_chrome} _NET_WM_STATE | cut -d\= -f2 | tr -d ' ') ]]; then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	fi

	if [[ -n ${wm_id_firefox-esr} ]]; then
		if [[ '_NET_WM_STATE_FULLSCREEN' -eq $(xprop -id ${wm_id_firefox-esr} _NET_WM_STATE | cut -d\= -f2 | tr -d ' ') ]];then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	fi
	sleep 150;
done &
)

(
while true; do
	[[ $(pgrep -x 'xautolock') ]] || xautolock -time 5 -locker 'i3lock -c 0e0e0e' >> /dev/null 2>&1 &
	sleep 300;
done &
)

blueman-applet &
flameshot &
fcitx5 &
greenclip daemon > /dev/null &
xfce4-panel --disable-wm-check &
tmux has-session && exec st -e tmux attach || exec st -e tmux &

