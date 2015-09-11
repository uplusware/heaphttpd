for NIUPID in `ps -ef | grep niuhttpd | grep -v grep | awk '{print $2;}'`
do
	echo ${NIUPID}
	kill ${NIUPID}
done
make
./niuhttpd start
