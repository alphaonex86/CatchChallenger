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

	tar cJpf ${FINAL_ARCHIVE} catchchallenger-src/
	if [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "${FINAL_ARCHIVE} not exists!";
		exit;
	fi
fi

rm -Rf ${CATCHCHALLENGERSOURCESPATH}/../datapack/
cd ${CATCHCHALLENGERSOURCESPATH}/../
rm -Rf datapackorg
/usr/bin/git clone --depth=1 https://github.com/alphaonex86/datapack.git datapackorg
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete ${CATCHCHALLENGERSOURCESPATH}/../datapackorg/ ${CATCHCHALLENGERSOURCESPATH}/../datapack/ --exclude=*.xcf --exclude='* *' --exclude=*.ogg --exclude=*/map.png > /dev/null 2>&1
cd ${CATCHCHALLENGERSOURCESPATH}/../datapack/
/root/script/datapack-compressor/map-cleaner.sh > /dev/null 2>&1
/root/script/datapack-compressor/png-compress.sh > /dev/null 2>&1
/root/script/datapack-compressor/xml-compress.sh > /dev/null 2>&1
rm -Rf ${CATCHCHALLENGERSOURCESPATH}/../datapackorg/