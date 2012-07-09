#!/usr/bin/env python

import syslog, sys, os

syslog.openlog("python", syslog.LOG_PID, syslog.LOG_DAEMON)
syslog.syslog("Hello world")
syslog.closelog()


