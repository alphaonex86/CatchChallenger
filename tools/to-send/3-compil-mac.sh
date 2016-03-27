#!/bin/bash

export TEMP_PATH="/mnt/world/CatchChallenger-temp/"
export CATCHCHALLENGER_SOURCE="/home/user/Desktop/CatchChallenger/"
export DATAPACK_SOURCE="/home/user/Desktop/CatchChallenger/git-datapack/datapack/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGER_SOURCE}/general/base/GeneralVariable.h | grep -F "0." | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${CATCHCHALLENGER_VERSION}

echo "Assemble mac version..."
source sub-script/mac.sh
cd ${BASE_PWD}
echo "Assemble mac version... done"



