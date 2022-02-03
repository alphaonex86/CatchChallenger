#!/bin/bash

export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export CATCHCHALLENGERSOURCESPATH="/root/catchchallenger/sources/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGERSOURCESPATH}/general/base/GeneralVariable.h | grep -F "0." | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${CATCHCHALLENGER_VERSION}

echo "upload..."
source sub-script/upload.sh
cd ${BASE_PWD}
echo "upload... done"

echo "force-update-datapack-git"
/root/script-custom/force-update-datapack-git.sh
echo "force-update-datapack-git done"


