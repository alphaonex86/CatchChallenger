#!/bin/bash

export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export CATCHCHALLENGERSOURCESPATH="/root/catchchallenger/sources/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGERSOURCESPATH}/general/base/GeneralVariable.h | grep -F "0." | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${CATCHCHALLENGER_VERSION}
cd ../
find ./ -name "Thumbs.db" -exec rm {} \; >> /dev/null 2>&1
find ./ -name ".directory" -exec rm {} \; >> /dev/null 2>&1

cd ${BASE_PWD}

echo "Update the translation..."
source sub-script/translation.sh
cd ${BASE_PWD}
echo "Update the translation... done"

echo "Assemble source version..."
source sub-script/assemble-source-version.sh
cd ${BASE_PWD}
echo "Assemble source version... done"




