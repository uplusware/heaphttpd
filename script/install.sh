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
# install the heaphttpd System
#

if uname -o | grep -i cygwin;
then
	test -x /usr/bin/heaphttpd && /usr/bin/heaphttpd stop
else
	test -x /etc/init.d/heaphttpd && /etc/init.d/heaphttpd stop
fi
sleep 3
killall heaphttpd >/dev/null 2>&1
sleep 1

path=$(dirname $0)
oldpwd=$(pwd)
cd ${path}
path=$(pwd)

echo "Copy the files to your system."


test -x /tmp/heaphttpd || mkdir /tmp/heaphttpd
test -x /etc/heaphttpd || mkdir /etc/heaphttpd
test -x /etc/heaphttpd/extension || mkdir /etc/heaphttpd/extension
test -x /var/heaphttpd || mkdir /var/heaphttpd
test -x /var/heaphttpd/html || mkdir /var/heaphttpd/html
test -x /var/heaphttpd/cookie || mkdir /var/heaphttpd/cookie
test -x /var/heaphttpd/variable || mkdir /var/heaphttpd/variable
test -x /tmp/heaphttpd/private || mkdir /tmp/heaphttpd/private
test -x /var/heaphttpd/cert || mkdir /var/heaphttpd/cert
test -x /var/heaphttpd/api || mkdir /var/heaphttpd/api
test -x /var/heaphttpd/cgi-bin || mkdir /var/heaphttpd/cgi-bin
test -x /var/heaphttpd/ws || mkdir /var/heaphttpd/ws
test -x /var/heaphttpd/webpy || mkdir /var/heaphttpd/webpy
test -x /var/heaphttpd/scgi || mkdir /var/heaphttpd/scgi
test -x /var/heaphttpd/fcgi || mkdir /var/heaphttpd/fcgi
test -x /var/heaphttpd/ext || mkdir /var/heaphttpd/ext

cp -rf ${path}/html/*  /var/heaphttpd/html/

test -x /etc/heaphttpd/heaphttpd.conf && mv /etc/heaphttpd/heaphttpd.conf /etc/heaphttpd/heaphttpd.conf.$((`date "+%Y%m%d%H%M%S"`))
cp -f ${path}/heaphttpd.conf /etc/heaphttpd/heaphttpd.conf
chmod 600 /etc/heaphttpd/heaphttpd.conf

test -x /var/heaphttpd/cert/ca.crt || cp -f ${path}/ca.crt /var/heaphttpd/cert/ca.crt
chmod 600 /var/heaphttpd/cert/ca.crt

test -x /var/heaphttpd/cert/server.key || cp -f ${path}/server.key /var/heaphttpd/cert/server.key
chmod 600 /var/heaphttpd/cert/server.key

test -x /var/heaphttpd/cert/server.crt || cp -f ${path}/server.crt /var/heaphttpd/cert/server.crt
chmod 600 /var/heaphttpd/cert/server.crt

test -x /var/heaphttpd/cert/client.key || cp -f ${path}/client.key /var/heaphttpd/cert/client.key
chmod 600 /var/heaphttpd/cert/client.key

test -x /var/heaphttpd/cert/client.crt || cp -f ${path}/client.crt /var/heaphttpd/cert/client.crt
chmod 600 /var/heaphttpd/cert/client.crt

test -x /etc/heaphttpd/permit.list || cp -f ${path}/permit.list /etc/heaphttpd/permit.list
chmod a-x /etc/heaphttpd/permit.list

test -x /etc/heaphttpd/reject.list || cp -f ${path}/reject.list /etc/heaphttpd/reject.list
chmod a-x /etc/heaphttpd/reject.list

test -x /etc/heaphttpd/extension.xml || cp -f ${path}/extension.xml /etc/heaphttpd/extension.xml
chmod a-x /etc/heaphttpd/extension.xml


cp -f ${path}/heaphttpd /usr/bin/heaphttpd
cp -f ${path}/heaptool /usr/bin/heaptool
chmod a+x /usr/bin/heaphttpd
chmod a+x /usr/bin/heaptool
if uname -o | grep -i cygwin;
then
	cp -f ${path}/libheaphttp.so /usr/bin/libheaphttp.so
	cp -f ${path}/libheapauth.so /usr/bin/libheapauth.so
	cp -f ${path}/libheapwebsocket.so /usr/bin/libheapwebsocket.so
else
        if [ -x /usr/lib ]; then
        	cp -f ${path}/libheaphttp.so /usr/lib/libheaphttp.so
		cp -f ${path}/libheapauth.so /usr/lib/libheapauth.so
		cp -f ${path}/libheapwebsocket.so /usr/lib/libheapwebsocket.so
        elif [ -x /usr/lib64 ]; then 
		cp -f ${path}/libheaphttp.so /usr/lib64/libheaphttp.so
		cp -f ${path}/libheapauth.so /usr/lib64/libheapauth.so
		cp -f ${path}/libheapwebsocket.so /usr/lib64/libheapwebsocket.so
	else
    		exit -1
  	fi
  	cp -f ${path}/heaphttpd.sh  /etc/init.d/heaphttpd
	chmod a+x /etc/init.d/heaphttpd
fi

cp -f ${path}/uninstall.sh   /var/heaphttpd/uninstall.sh
chmod a-x  /var/heaphttpd/uninstall.sh

if uname -o | grep -i cygwin;
then
	echo "..."
else
	ln -s /etc/init.d/heaphttpd /etc/rc0.d/K60heaphttpd 2> /dev/null
	ln -s /etc/init.d/heaphttpd /etc/rc1.d/S60heaphttpd 2> /dev/null
	ln -s /etc/init.d/heaphttpd /etc/rc2.d/S60heaphttpd 2> /dev/null
	ln -s /etc/init.d/heaphttpd /etc/rc3.d/S60heaphttpd 2> /dev/null
	ln -s /etc/init.d/heaphttpd /etc/rc4.d/S60heaphttpd 2> /dev/null
	ln -s /etc/init.d/heaphttpd /etc/rc5.d/S60heaphttpd 2> /dev/null
	ln -s /etc/init.d/heaphttpd /etc/rc6.d/K60heaphttpd 2> /dev/null
fi
echo "Done."
echo "Please reference the document named INSTALL to go ahead."
cd ${oldpwd}
