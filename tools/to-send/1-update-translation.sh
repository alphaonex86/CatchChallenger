#!/bin/bash

export TEMP_PATH="/mnt/data/world/CatchChallenger-temp/"
export CATCHCHALLENGER_SOURCE="/home/user/Desktop/CatchChallenger/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGER_SOURCE}/general/base/Version.h | grep -F "define CATCHCHALLENGER_VERSION" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${CATCHCHALLENGER_VERSION}
if [ "${CATCHCHALLENGER_VERSION}" == "" ]
then
    echo no version found
    exit
fi

echo "Update translation..."
source sub-script/translation-local.sh
cd ${BASE_PWD}
echo "Update translation... done"



