#!/bin/bash

#if [ `id -u` -ne 0 ]; then
#        echo "You need root privileges to run this script"
#        exit 1
#fi

#
# install the niuhttpd System
#

if uname -o | grep -i cygwin;
then
	test -x /usr/bin/niuhttpd && /usr/bin/niuhttpd stop
else
	test -x /etc/init.d/niuhttpd && /etc/init.d/niuhttpd stop
fi
sleep 3
killall niuhttpd >/dev/null 2>&1
sleep 1

path=$(dirname $0)
oldpwd=$(pwd)
cd ${path}
path=$(pwd)

echo "Copy the files to your system."


test -x /tmp/niuhttpd || mkdir /tmp/niuhttpd
test -x /etc/niuhttpd || mkdir /etc/niuhttpd
test -x /var/niuhttpd || mkdir /var/niuhttpd
test -x /var/niuhttpd/html || mkdir /var/niuhttpd/html
test -x /var/niuhttpd/cookie || mkdir /var/niuhttpd/cookie
test -x /var/niuhttpd/variable || mkdir /var/niuhttpd/variable
test -x /tmp/niuhttpd/private || mkdir /tmp/niuhttpd/private
test -x /var/niuhttpd/cert || mkdir /var/niuhttpd/cert
test -x /var/niuhttpd/api || mkdir /var/niuhttpd/api
test -x /var/niuhttpd/cgi-bin || mkdir /var/niuhttpd/cgi-bin
test -x /var/niuhttpd/ws || mkdir /var/niuhttpd/ws

cp -rf ${path}/html/*  /var/niuhttpd/html/ 

cp -f ${path}/niuhttpd.conf /etc/niuhttpd/niuhttpd.conf
chmod 600 /etc/niuhttpd/niuhttpd.conf

cp -f ${path}/niuhttpd-root.crt /var/niuhttpd/cert/niuhttpd-root.crt
chmod 600 /var/niuhttpd//cert/niuhttpd-root.crt

cp -f ${path}/niuhttpd-server.key /var/niuhttpd/cert/niuhttpd-server.key
chmod 600 /var/niuhttpd/cert/niuhttpd-server.key

cp -f ${path}/niuhttpd-server.crt /var/niuhttpd/cert/niuhttpd-server.crt
chmod 600 /var/niuhttpd/cert/niuhttpd-server.crt

cp -f ${path}/niuhttpd-server.key /var/niuhttpd/cert/niuhttpd-client.key
chmod 600 /var/niuhttpd/cert/niuhttpd-client.key
 
cp -f ${path}/niuhttpd-server.crt /var/niuhttpd/cert/niuhttpd-client.crt
chmod 600 /var/niuhttpd/cert/niuhttpd-client.crt

cp -f ${path}/permit.list /etc/niuhttpd/permit.list
chmod a-x /etc/niuhttpd/permit.list
cp -f ${path}/reject.list /etc/niuhttpd/reject.list
chmod a-x /etc/niuhttpd/reject.list
cp -f ${path}/extension.xml /etc/niuhttpd/extension.xml
chmod a-x /etc/niuhttpd/extension.xml


cp -f ${path}/niuhttpd /usr/bin/niuhttpd
cp -f ${path}/niutool /usr/bin/niutool
chmod a+x /usr/bin/niuhttpd
chmod a+x /usr/bin/niutool
if uname -o | grep -i cygwin;
then
	cp -f ${path}/libniuhttp.so /usr/bin/libniuhttp.so
	cp -f ${path}/libniuauth.so /usr/bin/libniuauth.so
	cp -f ${path}/libniuwebsocket.so /usr/bin/libniuwebsocket.so
else
        if [ -x /usr/lib ]; then
        	cp -f ${path}/libniuhttp.so /usr/lib/libniuhttp.so
		cp -f ${path}/libniuauth.so /usr/lib/libniuauth.so
		cp -f ${path}/libniuwebsocket.so /usr/lib/libniuwebsocket.so
        elif [ -x /usr/lib64 ]; then 
		cp -f ${path}/libniuhttp.so /usr/lib64/libniuhttp.so
		cp -f ${path}/libniuauth.so /usr/lib64/libniuauth.so
		cp -f ${path}/libniuwebsocket.so /usr/lib64/libniuwebsocket.so
	else
    		exit -1
  	fi
  	cp -f ${path}/niuhttpd.sh  /etc/init.d/niuhttpd
	chmod a+x /etc/init.d/niuhttpd
fi

cp -f ${path}/uninstall.sh   /var/niuhttpd/uninstall.sh
chmod a-x  /var/niuhttpd/uninstall.sh

if uname -o | grep -i cygwin;
then
	echo "..."
else
	ln -s /etc/init.d/niuhttpd /etc/rc0.d/K60niuhttpd 2> /dev/null
	ln -s /etc/init.d/niuhttpd /etc/rc1.d/S60niuhttpd 2> /dev/null
	ln -s /etc/init.d/niuhttpd /etc/rc2.d/S60niuhttpd 2> /dev/null
	ln -s /etc/init.d/niuhttpd /etc/rc3.d/S60niuhttpd 2> /dev/null
	ln -s /etc/init.d/niuhttpd /etc/rc4.d/S60niuhttpd 2> /dev/null
	ln -s /etc/init.d/niuhttpd /etc/rc5.d/S60niuhttpd 2> /dev/null
	ln -s /etc/init.d/niuhttpd /etc/rc6.d/K60niuhttpd 2> /dev/null
fi
echo "Done."
echo "Please reference the document named INSTALL to go ahead."
cd ${oldpwd}
