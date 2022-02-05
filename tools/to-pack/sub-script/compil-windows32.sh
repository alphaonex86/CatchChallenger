#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

ARCHITECTURE="x86"

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
source ${BASE_PWD}/sub-script/assemble.sh

compil "qtcpu800x600" 0 0 0 0 32 "-mtune=generic -march=i686" 0
assemble "qtcpu800x600" "${ARCHITECTURE}" 0 0
