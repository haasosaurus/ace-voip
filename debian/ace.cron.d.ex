#
# Regular cron jobs for the ace package
#
0 4	* * *	root	[ -x /usr/bin/ace_maintenance ] && /usr/bin/ace_maintenance
