1. move xdm-horse into /etc/X11/xdm.

2. change /etc/inittab file, add:

x:5:respawn:/usr/bin/xdm -nodaemon -config /etc/X11/xdm/xdm-horse/xdm-config
id:5:initdefault:

3. start xdm using following command for testing.

xdm -nodaemon -config /etc/X11/xdm/xdm-horse/xdm-config

