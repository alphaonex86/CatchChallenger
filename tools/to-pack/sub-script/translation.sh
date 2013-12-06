#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

TARGET="catchchallenger-translation-${CATCHCHALLENGER_VERSION}"
rm -Rf ${TEMP_PATH}/${TARGET}/
mkdir -p ${TEMP_PATH}/${TARGET}/

cd ${CATCHCHALLENGERSOURCESPATH}
echo "update the .ts file"
for project in `find client/ -type f -name *.pri`
do
        if [ -f ${project} ]
        then
                lupdate ${project} > /dev/null 2>&1
                lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
        fi
done
for project in `find general/ -type f -name *.pri`
do
        if [ -f ${project} ]
        then
                lupdate ${project} > /dev/null 2>&1
                lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
        fi
done
for project in `find server/ -type f -name *.pri`
do
        if [ -f ${project} ]
        then
                lupdate ${project} > /dev/null 2>&1
                lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
        fi
done
for project in `find tools/ -type f -name *.pri`
do
        if [ -f ${project} ]
        then
                lupdate ${project} > /dev/null 2>&1
                lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
        fi
done

PWD_BASE2=`pwd`

rsync -art ${CATCHCHALLENGERSOURCESPATH}/ ${TEMP_PATH}/${TARGET}/
cd ${TEMP_PATH}/${TARGET}/
for project in `find ${TEMP_PATH}/${TARGET}/client/ultimate/languages/ -maxdepth 1 -mindepth 1 -type d`
do
        if [ -f ${project}/specific.ts ]
        then
                mv ${project}/specific.ts ${project}/ultimate-specific.ts
        fi
done
for project in `find ${TEMP_PATH}/${TARGET}/client/single-player/languages/ -maxdepth 1 -mindepth 1 -type d`
do
        if [ -f ${project}/specific.ts ]
        then
                mv ${project}/specific.ts ${project}/single-player-specific.ts
        fi
done
for project in `find ${TEMP_PATH}/${TARGET}/client/single-server/languages/ -maxdepth 1 -mindepth 1 -type d`
do
        if [ -f ${project}/specific.ts ]
        then
                mv ${project}/specific.ts ${project}/single-server-specific.ts
        fi
done
rsync -art ${TEMP_PATH}/${TARGET}/client/*/languages/ ${TEMP_PATH}/${TARGET}/languages/
find ${TEMP_PATH}/${TARGET}/ ! -name "*.ts" -exec rm {} \; > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -maxdepth 1 -mindepth 1 -type d ! -name "languages" -exec rm -Rf {} \; > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1

cd ${TEMP_PATH}/
tar cjpf ${TARGET}.tar.bz2 ${TARGET}/
if [ ! -e ${TARGET}.tar.bz2 ]; then
	echo "${TARGET}.tar.bz2 not exists!";
	exit;
fi
rm -Rf ${TARGET}/
