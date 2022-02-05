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

if [ 2 -eq 3 ]
then
find ${TEMP_PATH}/${TARGET}/ ! -name "*.ts" -exec rm {} \; > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -maxdepth 1 -mindepth 1 -type d ! -name "languages" -exec rm -Rf {} \; > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1

cd ${TEMP_PATH}/
tar cjf ${TARGET}.tar.bz2 ${TARGET}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar
echo toto ${TEMP_PATH}/
exit
if [ ! -e ${TARGET}.tar.bz2 ]; then
	echo "${TARGET}.tar.bz2 not exists!";
	exit;
fi
rm -Rf ${TARGET}/
fi
