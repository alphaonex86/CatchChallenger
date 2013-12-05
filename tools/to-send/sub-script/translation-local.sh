#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

rm -Rf ${TEMP_PATH}

cd ${CATCHCHALLENGER_SOURCE}
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



