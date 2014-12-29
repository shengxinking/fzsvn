#!/bin/sh
# Begin $network-devices/services/dhclient

# Based upon lfs-bootscripts-1.12 $network_devices/if{down,up}
# Rewritten by Nathan Coulson <nathan@linuxfromscratch.org>
# Adapted for dhclient by DJ Lucas <dj@lucasit.com>

#$LastChangedBy: dnicholson $
#$Date: 2006-10-01 13:04:35 -0500 (Sun, 01 Oct 2006) $

. /etc/sysconfig/rc
. $rc_functions
. $IFCONFIG

getipstats()
{
	# Print the last 16 lines of dhclient.leases
	sed -e :a -e '$q;N;17,$D;ba' /var/state/dhcp//dhclient.leases
}

setwifipara()
{
	# Set mode
	if [ -n "$MODE" ]; then
		/usr/sbin/iwconfig $1 mode $MODE
	fi

	# Set Essid
	if [ -n "$ESSID" ]; then
		/usr/sbin/iwconfig $1 essid $ESSID
	else
		bootmesg "        No ESSID for wifi"
		exit 1
	fi

	# Set Channel
	if [ -n "$CHANNEL" ]; then
		/usr/sbin/iwconfig $1 channel $CHANNEL
	fi
	
	# Set WEP key
	if [ -n "$ENC" ]; then
		/usr/sbin/iwconfig $1 enc $ENC
	fi

	
}

case "$2" in
	up)
		boot_mesg "Starting wifi-dhclient on the $1 interface..."
		
		setwifipara $1

		/sbin/dhclient $1 $DHCP_START
		# Save the return value
		RET="$?"
		# Print the assigned settings if requested
		if [ "$RET" = "0" -a "$PRINTIP" = "yes" ]; then
			# Get info from dhclient.leases file
			IPADDR=`getipstats | grep "fixed-address" | \
				sed 's/ fixed-address //' | \
				sed 's/\;//'`
			NETMASK=`getipstats | grep "subnet-mask" | \
				sed 's/ option subnet-mask //' | \
				sed 's/\;//'`
			GATEWAY=`getipstats | grep "routers" | \
				sed 's/ option routers //' | \
				sed 's/\;//'`
			DNS=`getipstats | grep "domain-name-servers" | \
				sed 's/ option domain-name-servers //' | \
				sed 's/\;//' | sed 's/,/ and /'`

			if [ "$PRINTALL" = "yes" ]; then
				$(exit "$RET")
				evaluate_retval
				boot_mesg "           DHCP Assigned Settings for $1:"
				boot_mesg_flush
				boot_mesg "           IP Address:      $IPADDR"
				boot_mesg_flush
				boot_mesg "           Subnet Mask:     $NETMASK"
				boot_mesg_flush
				boot_mesg "           Default Gateway: $GATEWAY"
				boot_mesg_flush
				boot_mesg "           DNS Server:      $DNS"
				boot_mesg_flush
			else
				boot_mesg " IP Addresss:""$IPADDR"
				$(exit "$RET")
				evaluate_retval
			fi
		else
			$(exit "$RET")
			evaluate_retval
		fi
	;;

	down)
		boot_mesg "Stoping wifi-dhclient on the $1 interface..."
		if [ "$DHCP_STOP" = "" ]
		then
			# This breaks multiple interfaces please provide
			# the correct stop arguments.
			killproc dhclient
		else
			/sbin/dhclient $1 $DHCP_STOP
			evaluate_retval
		fi
	;;

	*)
		echo "Usage: $0 [interface] {up|down}"
		exit 1
	;;
esac

# End $network_devices/services/dhclient
