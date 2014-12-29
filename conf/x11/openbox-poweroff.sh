#!/usr/bin/env bash
ans=$(zenity --list --title "Make your choice" --text "What should I do?" --radiolist --column "Choice" --column "Action" logout logout reboot reboot poweroff poweroff);
echo $ans
case $ans in
	'logout')
	echo "logout"
	killall openbox
	;;
	'reboot')
	echo "reboot"
	sudo reboot
	;;
	'poweroff')
	echo "poweroff"
	sudo poweroff
	;;
esac

