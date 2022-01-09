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
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh ${VERSION} ${NAME} ${OS}
