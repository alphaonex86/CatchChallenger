#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

rm -Rf ${CATCHCHALLENGERSOURCESPATH}/../datapack/
cd ${CATCHCHALLENGERSOURCESPATH}/../
rm -Rf datapackorg
/usr/bin/git clone --depth=1 -b version-2 https://github.com/alphaonex86/CatchChallenger-datapack datapackorg
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete ${CATCHCHALLENGERSOURCESPATH}/../datapackorg/datapack/ ${CATCHCHALLENGERSOURCESPATH}/../datapack/ --exclude=*.xcf --exclude='* *' --exclude=*/map.png > /dev/null 2>&1
cd ${CATCHCHALLENGERSOURCESPATH}/../datapack/
/root/script-custom/datapack-compressor/map-cleaner.sh > /dev/null 2>&1
/root/script-custom/datapack-compressor/png-compress.sh > /dev/null 2>&1
/root/script-custom/datapack-compressor/xml-compress.sh > /dev/null 2>&1
echo clean into: ${CATCHCHALLENGERSOURCESPATH}/../datapack/
rm -Rf ${CATCHCHALLENGERSOURCESPATH}/../datapackorg/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete ${CATCHCHALLENGERSOURCESPATH}/../datapack/ /home/first-world.info/catchchallenger-datapack/
chown lighttpd.lighttpd -Rf /home/first-world.info/catchchallenger-datapack/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete ${CATCHCHALLENGERSOURCESPATH}/../datapack/ /home/catchchallenger/datapack/
chown catchchallenger.catchchallenger -Rf /home/catchchallenger/datapack/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete ${CATCHCHALLENGERSOURCESPATH}/../datapack/ /home/first-world.info/catchchallenger/datapack/
chown apache.apache -Rf /home/first-world.info/catchchallenger/datapack/
