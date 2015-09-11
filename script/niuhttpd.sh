#!/bin/bash

#
# niuhttpd control script
#

test -x /usr/bin/niuhttpd || exit 0

SELF=$(cd $(dirname $0); pwd -P)/$(basename $0)

cd /
umask 0

niuhttpd_start()
{
	/usr/bin/niuhttpd start
}

niuhttpd_stop()
{
	/usr/bin/niuhttpd stop
	sleep 3
}

niuhttpd_status()
{
	/usr/bin/niuhttpd status
}

niuhttpd_reload()
{
	/usr/bin/niuhttpd reload
}

niuhttpd_version()
{
	/usr/bin/niuhttpd version
}

niuhttpd_restart()
{
	niuhttpd_stop
	niuhttpd_start
}

case "${1:-''}" in
	'start')
	niuhttpd_start
	;;
	
	'stop')
	niuhttpd_stop
	;;
	
	'restart')
	niuhttpd_restart
	;;
	
	'reload')
	niuhttpd_reload
	;;
	
	'status')
	niuhttpd_status
	;;
	
	'version')
	niuhttpd_version
	;;
	
	*)
	echo "Usage: $SELF Usage:niuhttpd start | stop | status | reload | version [CONFIG FILE]"
	exit 1
	;;
esac

