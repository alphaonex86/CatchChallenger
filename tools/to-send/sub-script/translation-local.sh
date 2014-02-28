#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

rm -Rf ${TEMP_PATH}

cd ${CATCHCHALLENGER_SOURCE}
echo "update the .ts file"

lupdate -no-obsolete client/base/client.pri > /dev/null 2>&1
lrelease -nounfinished -compress -removeidentical client/base/client.pri > /dev/null 2>&1
lupdate -no-obsolete client/base/solo.pri > /dev/null 2>&1
lrelease -nounfinished -compress -removeidentical client/base/solo.pri > /dev/null 2>&1


for project in `find client/ -type f -name specific.pri`
do
        if [ -f ${project} ]
        then
                lupdate -no-obsolete ${project} > /dev/null 2>&1
                lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
        fi
done

for project in `find tools/ -type f -name *.pro`
do
        if [ -f ${project} ]
        then
                lupdate -no-obsolete ${project} > /dev/null 2>&1
                lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
        fi
done



