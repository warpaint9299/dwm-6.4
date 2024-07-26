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
	[[ $(pgrep -x 'picom') ]] || picom --config ~/.config/picom/picom.conf >> /dev/null 2>&1
	sleep 3;
done &
)

(
dir_cache_chrome="${HOME}/.cache/google-chrome";
dir_cache_firefox="${HOME}/.cache/mozilla";
dir_cache_brave="${HOME}/.cache/BraveSoftware";
while true; do
	wm_id_chrome=$(xdotool search --onlyvisible --class 'google-chrome');
	wm_id_firefox=$(xdotool search --onlyvisible --class 'firefox-esr');
	wm_id_brave=$(xdotool search --onlyvisible --class 'brave-browser');

	if [[ -n ${wm_id_chrome} ]]; then
		if [[ '_NET_WM_STATE_FULLSCREEN' -eq $(xprop -id ${wm_id_chrome} _NET_WM_STATE | cut -d\= -f2 | tr -d ' ') ]]; then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	else
		[[ -d "${dir_cache_chrome}" ]] && rm -rf ${dir_cache_chrome}
	fi

	if [[ -n ${wm_id_firefox} ]]; then
		if [[ 0 -ne $( ps -aux | grep firefox-esr | grep -v grep | wc -l ) ]];then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	else
		[[ -d "${dir_cache_firefox}" ]] && rm -rf ${dir_cache_firefox}
	fi

	if [[ -n ${wm_id_brave} ]]; then
		if [[ 0 -ne $( ps -aux | grep brave | grep -v grep | wc -l ) ]];then
			[[ $(pgrep -x 'xautolock') ]] && killall xautolock
		fi
	else
		[[ -d "${dir_cache_brave}" ]] && rm -rf ${dir_cache_brave}
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
tmux has-session && exec st -e tmux attach || exec st -e tmux new -s Workspace01 &
xfce4-panel --disable-wm-check &
exec xset -b & # disable console bell volume
