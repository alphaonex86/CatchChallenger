#!/bin/bash

export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export CATCHCHALLENGERSOURCESPATH="/root/catchchallenger/sources/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGERSOURCESPATH}/general/base/GeneralVariable.h | grep -F "0." | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${CATCHCHALLENGER_VERSION}

rm -Rf /home/ultracopier-temp/*
rm -Rf ${TEMP_PATH} > /dev/null 2>&1
mkdir -p ${TEMP_PATH}
find ../ -name "Thumbs.db" -exec rm {} \; >> /dev/null 2>&1
find ../ -name ".directory" -exec rm {} \; >> /dev/null 2>&1

echo "Do the test folder..."
source sub-script/test.sh
cd ${BASE_PWD}
echo "Do the test folder... done"

exit

./4-clean-all.sh

rm /home/first-world.info/files-rw/temp/catchchallenger-*
cp /home/ultracopier-temp/*.* /home/first-world.info/files-rw/temp/
cd /home/first-world.info/files-rw/temp/
for f in `ls -1 catchchallenger-*`
do
    echo "http://files.first-world.info/temp/"${f}
done


