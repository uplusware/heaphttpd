#!/bin/sh
# kFreeBSD do not accept scripts as interpreters, using #!/bin/sh and sourcing.
### BEGIN INIT INFO
# Provides:          uplusware
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: heaphttpd control script
# Description:       This file is copied from /etc/init.d/skeleton
### END INIT INFO
# Author: uplusware.org <uplusware@gmail.com>
#
DESC="heaphttpd control script"
DAEMON=/usr/bin/heaphttpd

test -x /usr/bin/heaphttpd || exit 0

SELF=$(cd $(dirname $0); pwd -P)/$(basename $0)

cd /
umask 0

heaphttpd_start()
{
	/usr/bin/heaphttpd start
}

heaphttpd_stop()
{
	/usr/bin/heaphttpd stop
	sleep 5
}

heaphttpd_status()
{
	/usr/bin/heaphttpd status
}

heaphttpd_reload()
{
	/usr/bin/heaphttpd reload
}

heaphttpd_access()
{
	/usr/bin/heaphttpd access
}

heaphttpd_reject()
{
	/usr/bin/heaphttpd reject $1
}

heaphttpd_extension()
{
	/usr/bin/heaphttpd extension
}

heaphttpd_version()
{
	/usr/bin/heaphttpd version
}

heaphttpd_restart()
{
	heaphttpd_stop
	heaphttpd_start
}

case "${1:-''}" in
	'start')
	heaphttpd_start
	;;
	
	'stop')
	heaphttpd_stop
	;;
	
	'restart')
	heaphttpd_restart
	;;
	
	'reload')
	heaphttpd_reload
	;;
	
	'access')
	heaphttpd_access
	;;

	'reject')
	heaphttpd_reject $2
	;;

    'extension')
	heaphttpd_extension
	;;

	'status')
	heaphttpd_status
	;;
	
	'version')
	heaphttpd_version
	;;
	
	*)
	echo "Usage: $SELF Usage:heaphttpd start | stop | status | reload | access | reject [ip] | extension | version"
	exit 1
	;;
esac

