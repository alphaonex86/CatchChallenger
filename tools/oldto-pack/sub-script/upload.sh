#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi
if [ "${CATCHCHALLENGER_VERSION}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

echo "Move some elements..."

mkdir -p /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/
mv ${TEMP_PATH}/catchchallenger-*.tar.xz /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/
mv ${TEMP_PATH}/catchchallenger-*.7z /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/
mv ${TEMP_PATH}/catchchallenger-*.zip /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/
mv ${TEMP_PATH}/catchchallenger-*-setup.exe /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/
mv ${TEMP_PATH}/catchchallenger-*.tar.bz2 /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/
echo "Move some elements... done"

#echo "Upload to the shop..."
#cp /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/catchchallenger-ultimate-windows-x86-${CATCHCHALLENGER_VERSION}-setup.exe /home/first-world.info/catchchallenger/shop/download/ad3829d5cd2c2a002f354e56731734ff7c6e5e61
#cp /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/catchchallenger-ultimate-mac-os-x-${CATCHCHALLENGER_VERSION}.dmg /home/first-world.info/catchchallenger/shop/download/a31decd017178110c69cc00ddd34afd7e9b9b6e4
#/usr/bin/php /home/first-world.info/catchchallenger/shop/update_catchchallenger_version.php ${CATCHCHALLENGER_VERSION}
#echo "Upload to the shop... done"

rm ${TEMP_PATH}/*

echo "Change the static files..."
echo -ne ${CATCHCHALLENGER_VERSION} > /home/first-world.info/catchchallenger/updater.txt
echo -ne ${CATCHCHALLENGER_VERSION} > /home/first-world.info/catchchallenger-update/updater.txt
/etc/init.d/lighttpd restart
echo "Change the static files... done"
