while true;do
	xsetroot -name " $(date '+%A %B %d,%Y %H:%M') "
	sleep 1;
done &

exec dwm > /dev/null 2>&1
xrandr --auto --output eDP-1 --mode 1920x1080 --rate 60
#xrandr --auto --output eDP-1 --mode 1920x1080 --rate 60 --left-of HDMI-A-0 
#xrandr --auto --output eDP-1 --mode 1920x1080 --rate 60 --asme-as HDMI-A-0 
export GTK_IM_MODULE=fcitx
export QT_IM_MODULE=fcitx
export XMODIFIERS=@im=fcitx
export SDL_IM_MODULE=fcitx
export GLFW_IM_MODULE=ibus
export EDITOR=nvim
export _JAVA_AWT_WM_NONREPARENTING=1
