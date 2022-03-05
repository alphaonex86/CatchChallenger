#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

rsync -avrt --partial --progress ${TEMP_PATH}/catchchallenger-*${CATCHCHALLENGER_VERSION}.dmg root@763.vps.confiared.com:/home/first-world.info/catchchallenger/files/${CATCHCHALLENGER_VERSION}/ --timeout=120
rsync -avrt --partial --progress ${TEMP_PATH}/catchchallenger-*${CATCHCHALLENGER_VERSION}*.exe root@763.vps.confiared.com:/home/first-world.info/catchchallenger/files/${CATCHCHALLENGER_VERSION}/ --timeout=120
rsync -avrt --partial --progress ${TEMP_PATH}/catchchallenger-*${CATCHCHALLENGER_VERSION}*.zip root@763.vps.confiared.com:/home/first-world.info/catchchallenger/files/${CATCHCHALLENGER_VERSION}/ --timeout=120
