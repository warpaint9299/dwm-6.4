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
	for browser in "firefox" "brave" "chrome" "chromium"; do
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

# wacom settings
(
DEVICE1="Wacom Intuos BT M Pen stylus"
DEVICE2="Wacom Intuos BT M Pad pad"
while true; do
    if [[ $(xsetwacom list | wc -c ) -ne 0 ]]; then
        APP=$(xdotool getwindowfocus getwindowclassname)
        case "$APP" in
            okular)
                xsetwacom set "$DEVICE2" Button 1 "key Ctrl 1"
                xsetwacom set "$DEVICE2" Button 2 "key Ctrl 4"
                xsetwacom set "$DEVICE2" Button 3 "key F9"
                xsetwacom set "$DEVICE2" Button 8 "key Shift F9"
                ;;
            Com.github.xournalpp.xournalpp)
                # xsetwacom set "$DEVICE1" MapToOutput HEAD-1
                xsetwacom set "$DEVICE2" Button 1 "key Ctrl z"
                xsetwacom set "$DEVICE2" Button 2 "key Shift Ctrl o"
                xsetwacom set "$DEVICE2" Button 3 "key F9"
                xsetwacom set "$DEVICE2" Button 8 "key Shift F9"
                ;;
            *)
                # xsetwacom set "$DEVICE2" Button 3 "key F12"
                # xsetwacom set "$DEVICE2" Button 8 "key Shift F12"
                xsetwacom set "$DEVICE2" Button 3 "key F9"
                xsetwacom set "$DEVICE2" Button 8 "key Shift F9"
                ;;
        esac
    fi
    sleep 1;
done &
)

blueman-applet &
fcitx5 &
flameshot &
# xfce4-panel --disable-wm-check &
sleep 0.3
gromit-mpx &
opensnitch-ui &
tmux has-session && exec kitty tmux attach || exec kitty tmux &
# exec setxkbmap us dvorak & 
exec xset -b & # disable console bell volume
