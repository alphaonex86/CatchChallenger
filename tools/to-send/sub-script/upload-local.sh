#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/CatchChallenger-temp/*.dmg root@ssh.first-world.info:/home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/ --timeout=120
RETURNA=$?
while [ ${RETURNA} -ne 0 ] && [ ${RETURNA} -ne 20 ] && [ ${RETURNA} -ne 255 ]
do
	rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/CatchChallenger-temp/*.dmg root@ssh.first-world.info:/home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/ --timeout=120
	RETURNA=$?
	echo ${RETURNA}
done

