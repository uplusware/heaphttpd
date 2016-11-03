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
# uninstall the niuhttpd web server
#

echo "Sure to remove niuhttpd from you computer? [yes/no]"
read uack
if [ ${uack} = "yes" ]
then
	echo "Remove the niuhttpd...."
else
	exit 1
fi

/etc/init.d/niuhttpd stop

rm -rf /usr/bin/niuhttpd
rm -rf /etc/init.d/niuhttpd
rm -rf /etc/niuhttpd

rm -f /etc/rc0.d/K60niuhttpd
rm -f /etc/rc1.d/S60niuhttpd
rm -f /etc/rc2.d/S60niuhttpd
rm -f /etc/rc3.d/S60niuhttpd
rm -f /etc/rc4.d/S60niuhttpd
rm -f /etc/rc5.d/S60niuhttpd
rm -f /etc/rc6.d/K60niuhttpd

echo "Done"
