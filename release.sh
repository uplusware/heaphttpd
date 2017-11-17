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
fi


rm -rf $3-heaphttpd-bin-$2-${m}-${o}
mkdir $3-heaphttpd-bin-$2-${m}-${o}
mkdir $3-heaphttpd-bin-$2-${m}-${o}/html

cp html/* $3-heaphttpd-bin-$2-${m}-${o}/html/

cp src/heaphttpd $3-heaphttpd-bin-$2-${m}-${o}/heaphttpd
cp src/heaptool $3-heaphttpd-bin-$2-${m}-${o}/heaptool
cp src/libheaphttp.so $3-heaphttpd-bin-$2-${m}-${o}/libheaphttp.so
cp src/libheapauth.so $3-heaphttpd-bin-$2-${m}-${o}/libheapauth.so
cp src/libheapwebsocket.so $3-heaphttpd-bin-$2-${m}-${o}/libheapwebsocket.so

cp script/install.sh $3-heaphttpd-bin-$2-${m}-${o}/
cp script/uninstall.sh $3-heaphttpd-bin-$2-${m}-${o}/

cp script/heaphttpd.conf $3-heaphttpd-bin-$2-${m}-${o}/heaphttpd.conf
cp script/permit.list $3-heaphttpd-bin-$2-${m}-${o}/
cp script/reject.list $3-heaphttpd-bin-$2-${m}-${o}/
cp script/extension.xml $3-heaphttpd-bin-$2-${m}-${o}/
cp script/httpreverse.xml $3-heaphttpd-bin-$2-${m}-${o}/
cp script/users.xml $3-heaphttpd-bin-$2-${m}-${o}/

cp script/heaphttpd.sh $3-heaphttpd-bin-$2-${m}-${o}/

cp ca/ca.crt $3-heaphttpd-bin-$2-${m}-${o}/ca.crt

cp ca/server.p12 $3-heaphttpd-bin-$2-${m}-${o}/server.p12
cp ca/server.crt $3-heaphttpd-bin-$2-${m}-${o}/server.crt
cp ca/server.key $3-heaphttpd-bin-$2-${m}-${o}/server.key

cp ca/client.p12 $3-heaphttpd-bin-$2-${m}-${o}/client.p12
cp ca/client.crt $3-heaphttpd-bin-$2-${m}-${o}/client.crt
cp ca/client.key $3-heaphttpd-bin-$2-${m}-${o}/client.key

chmod a+x $3-heaphttpd-bin-$2-${m}-${o}/*
tar zcf $3-heaphttpd-bin-$2-${m}-${o}-$1.tar.gz $3-heaphttpd-bin-$2-${m}-${o}
cd ${oldpwd}
