xrdb -merge ~/.Xresources
exec dwm >> /tmp/dwm.log 2>&1
xrandr --auto --output eDP-1 --mode 2560x1440
