#!/bin/sh
# kFreeBSD do not accept scripts as interpreters, using #!/bin/sh and sourcing.
### BEGIN INIT INFO
# Provides:          uplusware
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: niuhttpd control script
# Description:       This file is copied from /etc/init.d/skeleton
### END INIT INFO
# Author: uplusware.org <uplusware@gmail.com>
#
DESC="niuhttpd control script"
DAEMON=/usr/bin/niuhttpd

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
	sleep 5
}

niuhttpd_status()
{
	/usr/bin/niuhttpd status
}

niuhttpd_reload()
{
	/usr/bin/niuhttpd reload
}

niuhttpd_access()
{
	/usr/bin/niuhttpd access
}

niuhttpd_reject()
{
	/usr/bin/niuhttpd reject $1
}

niuhttpd_extension()
{
	/usr/bin/niuhttpd extension
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
	
	'access')
	niuhttpd_access
	;;

	'reject')
	niuhttpd_reject $2
	;;

    'extension')
	niuhttpd_extension
	;;

	'status')
	niuhttpd_status
	;;
	
	'version')
	niuhttpd_version
	;;
	
	*)
	echo "Usage: $SELF Usage:niuhttpd start | stop | status | reload | access | reject [ip] | extension | version"
	exit 1
	;;
esac

