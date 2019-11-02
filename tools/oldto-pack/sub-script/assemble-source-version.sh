#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

cd ${TEMP_PATH}/

FINAL_ARCHIVE="catchchallenger-src-${CATCHCHALLENGER_VERSION}.tar.xz"
if [ ! -e ${FINAL_ARCHIVE} ]; then
	rm -Rf ${TEMP_PATH}/catchchallenger-src/
	cp -aRf ${CATCHCHALLENGERSOURCESPATH}/ ${TEMP_PATH}/catchchallenger-src/
	find ${TEMP_PATH}/catchchallenger-src/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-src/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-src/ -name "*-build-desktop*" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-src/ -name "*Qt_in_*" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-src/ -name "GeneralVariable.h" -exec sed -i "s/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/#define CATCHCHALLENGER_EXTRA_CHECK/g" {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-src/ -iname "*.qm" -exec rm {} \; > /dev/null 2>&1

	tar cJf ${FINAL_ARCHIVE} catchchallenger-src/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar
	if [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "${FINAL_ARCHIVE} not exists!";
		exit;
	fi
fi
