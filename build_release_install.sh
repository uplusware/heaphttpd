#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
VERSION=1.3
OS=centos8

#async arch
NAME=async
cd ${SCRIPT_DIR}/src/
make clean
make ASYNC=1
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh ${VERSION} ${NAME} ${OS}

#thread arch
NAME=thread
cd ${SCRIPT_DIR}/src/
make clean
make
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh ${VERSION} ${NAME} ${OS}

#pick one and install
chmod a+x ${SCRIPT_DIR}/${OS}-heaphttpd-bin-${NAME}-x86_64-linux/install.sh
sudo ${SCRIPT_DIR}/${OS}-heaphttpd-bin-${NAME}-x86_64-linux/install.sh
sudo /etc/init.d/heaphttpd restart
