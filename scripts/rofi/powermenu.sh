#!/usr/bin/env bash

## Author : Aditya Shakya (adi1090x)
## Github : @adi1090x
## Modifier: @warpaint9299
## Rofi   : Power Menu

# Options
# shutdown='Shutdown'
# reboot='Reboot'
# lock='Lock'
# suspend='Suspend'
# logout='Logout'
# yes='Yes'
# no='No'

dir="$HOME/.dwm/scripts/rofi"
theme="power_menu-style"

# Options
shutdown=''
reboot=''
lock=''
suspend=''
logout=''
yes=''
no=''

# Rofi CMD
rofi_cmd() {
    rofi -dmenu \
        -p "Power Menu" \
        -theme ${dir}/${theme}.rasi
}

# Confirmation CMD
confirm_cmd() {
    rofi -dmenu \
        -p "Are you sure ?" \
        -theme ${dir}/shared/confirm.rasi
}

# Ask for confirmation
confirm_exit() {
    echo -e "$yes\n$no" | confirm_cmd
}

# Pass variables to rofi dmenu
run_rofi() {
    echo -e "$lock\n$suspend\n$logout\n$reboot\n$shutdown" | rofi_cmd
}

# Execute Command
run_cmd() {
    selected="$(confirm_exit)"
    if [[ "$selected" == "$yes" ]]; then
        if [[ $1 == '--shutdown' ]]; then
            systemctl poweroff
        elif [[ $1 == '--reboot' ]]; then
            systemctl reboot
        elif [[ $1 == '--suspend' ]]; then
            mpc -q pause
            amixer set Master mute
            systemctl suspend
        elif [[ $1 == '--logout' ]]; then
            if [[ "$DESKTOP_SESSION" == 'openbox' ]]; then
                openbox --exit
            elif [[ "$DESKTOP_SESSION" == 'bspwm' ]]; then
                bspc quit
            elif [[ "$DESKTOP_SESSION" == 'i3' ]]; then
                i3-msg exit
            elif [[ "$DESKTOP_SESSION" == 'plasma' ]]; then
                qdbus org.kde.ksmserver /KSMServer logout 0 0 0
            else
                pkill dwm && \
                loginctl terminate-session ${XDG_SESSION_ID-}
            fi
        fi
    else
        exit 0
    fi
}

# Actions
chosen="$(run_rofi)"
case ${chosen} in
    $shutdown)
        run_cmd --shutdown
        ;;
    $reboot)
        run_cmd --reboot
        ;;
    $lock)
        if [[ -x '/usr/bin/betterlockscreen' ]]; then
            betterlockscreen -l
        elif [[ -x '/usr/bin/i3lock' ]]; then
            i3lock -c 0e0e0e -n
        fi
        ;;
    $suspend)
        run_cmd --suspend
        ;;
    $logout)
        run_cmd --logout
        ;;
esac
