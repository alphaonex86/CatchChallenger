#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

ARCHITECTURE="x86"

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
source ${BASE_PWD}/sub-script/assemble.sh

compil "single-player" 0 0 0 0 32 "-mtune=generic -march=i686"
assemble "single-player" "${ARCHITECTURE}"
compil "single-server" 0 0 0 0 32 "-mtune=generic -march=i686"
assemble "single-server" "${ARCHITECTURE}"
compil "ultimate" 0 0 0 0 32 "-mtune=generic -march=i686"
assemble "ultimate" "${ARCHITECTURE}"
