#!/bin/bash

# Force OpenOffice.org to use GTK theme
export OOO_FORCE_DESKTOP=gnome

# Set background
feh --bg-scale /home/work/media/wallpaper/meihua.png

# read Xresources
xrdb -merge ~/.Xdefaults

##
#killall xcompmgr >/dev/null
#xcompmgr -Ss -n -Cc -fF -I-10 -O-10 -D1 -t-3 -l-4 -r4 &
#xcompmgr -c &

# run xscreensaver
killall xscreensaver > /dev/null 2>&1
xscreensaver -nosplash &

# Run fbpanel
killall fbpanel > /dev/null 2>&1
fbpanel &

# Run xpad
killall xpad > /dev/null 2>&1
xpad &



