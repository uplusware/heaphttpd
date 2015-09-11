#!/bin/bash

if [ $# = 3 ]
then
	echo $1
else
	echo "release 0.1 rel-alias entos"
	exit 1	
fi

path=$(dirname $0)
oldpwd=$(pwd)
cd ${path}
path=$(pwd)

cd ${path}

#############################################################################
# Platform
m=`uname -m`
if uname -o | grep -i linux;
then
	o=linux
	cd src/
#	make clean
	make
	cd ..
elif uname -o | grep -i solaris;
then
	o=solaris-`isainfo -b`bit
	cd src/
	gmake clean
	gmake SOLARIS=1
	cd ..
elif uname -o | grep -i freebsd;
then
	o=freebsd
	cd src/
	make clean
	make FREEBSD=1
	cd ..
elif uname -o | grep -i cygwin;
then
	o=cygwin
	cd src/
	make clean
	make CYGWIN=1
	cd ..
fi


rm -rf $3-niuhttpd-bin-$2-${m}-${o}
mkdir $3-niuhttpd-bin-$2-${m}-${o}
mkdir $3-niuhttpd-bin-$2-${m}-${o}/html

cp html/* $3-niuhttpd-bin-$2-${m}-${o}/html/

cp src/niuhttpd $3-niuhttpd-bin-$2-${m}-${o}/niuhttpd
cp src/niutool $3-niuhttpd-bin-$2-${m}-${o}/niutool
cp src/libniuhttp.so $3-niuhttpd-bin-$2-${m}-${o}/libniuhttp.so
cp src/libniuauth.so $3-niuhttpd-bin-$2-${m}-${o}/libniuauth.so
cp src/libniuwebsocket.so $3-niuhttpd-bin-$2-${m}-${o}/libniuwebsocket.so

cp script/install.sh $3-niuhttpd-bin-$2-${m}-${o}/
cp script/uninstall.sh $3-niuhttpd-bin-$2-${m}-${o}/

cp script/niuhttpd.conf $3-niuhttpd-bin-$2-${m}-${o}/niuhttpd.conf
cp script/permit.list $3-niuhttpd-bin-$2-${m}-${o}/
cp script/reject.list $3-niuhttpd-bin-$2-${m}-${o}/
cp script/extension.xml $3-niuhttpd-bin-$2-${m}-${o}/

cp script/niuhttpd.sh $3-niuhttpd-bin-$2-${m}-${o}/

cp ca/niuhttpd-root.crt $3-niuhttpd-bin-$2-${m}-${o}/niuhttpd-root.crt

cp ca/niuhttpd-server.crt $3-niuhttpd-bin-$2-${m}-${o}/niuhttpd-server.crt
cp ca/niuhttpd-server.key $3-niuhttpd-bin-$2-${m}-${o}/niuhttpd-server.key

cp ca/niuhttpd-client.crt $3-niuhttpd-bin-$2-${m}-${o}/niuhttpd-client.crt
cp ca/niuhttpd-client.key $3-niuhttpd-bin-$2-${m}-${o}/niuhttpd-client.key

chmod a+x $3-niuhttpd-bin-$2-${m}-${o}/*
#ls -al $3-niuhttpd-bin-$2-${m}-${o}
tar zcf $3-niuhttpd-bin-$2-${m}-${o}-$1.tar.gz $3-niuhttpd-bin-$2-${m}-${o}
cd ${oldpwd}
