#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/
make clean
make ASYNC=1
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh 1.1 demo ubuntu18lts
chmod a+x ${SCRIPT_DIR}/ubuntu18lts-heaphttpd-bin-demo-x86_64-linux/install.sh
sudo ${SCRIPT_DIR}/ubuntu18lts-heaphttpd-bin-demo-x86_64-linux/install.sh
sudo /etc/init.d/heaphttpd restart
