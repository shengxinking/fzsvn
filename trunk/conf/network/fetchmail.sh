#!/bin/sh
# Begin $rc_base/init.d/fetchmail


. /etc/sysconfig/rc
. $rc_functions

case "$1" in
	start)
		boot_mesg "Starting fetchmail..."
		loadproc /usr/bin/fetchmail -f /etc/fetchmailrc
		;;

	stop)
		boot_mesg "Stopping fetchmail..."
		killproc /usr/bin/fetchmail
		;;

	restart)
		$0 stop
		sleep 1
		$0 start
		;;

	status)
		statusproc /usr/bin/fetchmail
		;;

	*)
		echo "Usage: $0 {start|stop|restart|status}"
		exit 1
		;;
esac

# End $rc_base/init.d/fetchmail
