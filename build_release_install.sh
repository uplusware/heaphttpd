#!/bin/bash
VERSION=1.3
OS=centos8
NAME=dev
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/
make clean
make ASYNC=1
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh ${VERSION} ${NAME} ${OS}
chmod a+x ${SCRIPT_DIR}/${OS}-heaphttpd-bin-${NAME}-x86_64-linux/install.sh
sudo ${SCRIPT_DIR}/${OS}-heaphttpd-bin-${NAME}-x86_64-linux/install.sh
sudo /etc/init.d/heaphttpd restart
