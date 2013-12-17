#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

rm -Rf ${CATCHCHALLENGERSOURCESPATH}/../datapack/
cd ${CATCHCHALLENGERSOURCESPATH}/../
rm -Rf datapackorg
/usr/bin/git clone --depth=1 https://github.com/alphaonex86/datapack.git datapackorg
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete ${CATCHCHALLENGERSOURCESPATH}/../datapackorg/datapack/ ${CATCHCHALLENGERSOURCESPATH}/../datapack/ --exclude=*.xcf --exclude='* *' --exclude=*.ogg --exclude=*/map.png > /dev/null 2>&1
cd ${CATCHCHALLENGERSOURCESPATH}/../datapack/
/root/script/datapack-compressor/map-cleaner.sh > /dev/null 2>&1
/root/script/datapack-compressor/png-compress.sh > /dev/null 2>&1
/root/script/datapack-compressor/xml-compress.sh > /dev/null 2>&1
rm -Rf ${CATCHCHALLENGERSOURCESPATH}/../datapackorg/