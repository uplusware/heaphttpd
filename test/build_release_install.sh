#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/
make clean
make
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh 1.1 GA ubuntu
chmod a+x ${SCRIPT_DIR}/ubuntu-heaphttpd-bin-GA-x86_64-linux/install.sh
sudo ${SCRIPT_DIR}/ubuntu-heaphttpd-bin-GA-x86_64-linux/install.sh
sudo /etc/init.d/heaphttpd restart
