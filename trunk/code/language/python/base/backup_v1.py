#!/usr/bin/python

# filename: backup_v1.py

import os
import time

source = '/home/work/ftp'
target_dir = '/home/forrest/'

target = target_dir + "ftp" + time.strftime("%Y%m%d%H%M%S") + '.tar.gz'

tar_command = "tar zcf %s %s" %(target, source)

if os.system(tar_command) == 0:
	print 'successful backup to', target
else:
	print 'backup FAILED'

