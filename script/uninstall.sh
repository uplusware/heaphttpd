#!/bin/bash
#
#	Copyright (c) openheap, uplusware
#	uplusware@gmail.com
#
if [ `id -u` -ne 0 ]; then
        echo "You need root privileges to run this script"
        exit 1
fi

#
# uninstall the heaphttpd web server
#

echo "Sure to remove heaphttpd from you computer? [yes/no]"
read uack
if [ ${uack} = "yes" ]
then
	echo "Remove the heaphttpd...."
else
	exit 1
fi

/etc/init.d/heaphttpd stop

rm -rf /usr/bin/heaphttpd
rm -rf /etc/init.d/heaphttpd
rm -rf /etc/heaphttpd

rm -f /etc/rc0.d/K60heaphttpd
rm -f /etc/rc1.d/S60heaphttpd
rm -f /etc/rc2.d/S60heaphttpd
rm -f /etc/rc3.d/S60heaphttpd
rm -f /etc/rc4.d/S60heaphttpd
rm -f /etc/rc5.d/S60heaphttpd
rm -f /etc/rc6.d/K60heaphttpd

echo "Done"
