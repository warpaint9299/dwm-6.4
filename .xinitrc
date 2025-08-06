export XDG_SESSION_TYPE=x11
xrandr --output eDP-1 --primary --mode 2560x1600 --scale 0.7x0.7 --pos 0x0 --rotate normal --output DP-1-0 --off --output DP-1-1 --off --output DP-1-2 --mode 1440x900 --pos 1792x0 --rotate normal --output DP-1-3 --off --output HDMI-1-0 --off --output DP-1-4 --off
xrdb -merge ~/.Xresources
exec dwm >> /tmp/dwm.log 2>&1

