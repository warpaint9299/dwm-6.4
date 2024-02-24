#while true;do
#	xsetroot -name " Date: $(date '+%A %B %d,%Y %H:%M') | $(sar -n DEV 1 1 | grep -E 'Average' | grep 'wlan0' | awk '{print "wlan0: " $6 " KB/s, " $7 " KB/s"}') | Brightness: $(light)% | Volume: $(amixer sget Master | awk -F"[][]" '/Left:/ { print $2 " " "["$4"]" }') | Battery:$(acpi | awk -F, '{ print $2 }') "
#	xsetroot -name " Date: $(date '+%A %B %d,%Y %H:%M') | Brightness: $(light)% | Volume: $(amixer sget Master | awk -F"[][]" '/Left:/ { print $2 " " "["$4"]" }') | Battery:$(acpi | awk -F, '{ print $2 }') "
#	xsetroot -name " Date: $(date '+%A %B %d,%Y %H:%M') | Battery:$(acpi | awk -F, '{ print $2 }') "
#	xsetroot -name " Date: $(date '+%A %B %d,%Y %H:%M') "
#	sleep 1;
#done &
exec dwm > /dev/null 2>&1
#xrandr --auto --output eDP-1 --mode 1920x1080 --rate 60
