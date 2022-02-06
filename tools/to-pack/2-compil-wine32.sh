#!/bin/bash

export WINEBASEPATH="/mnt/data/perso/progs/wine/"
export TEMP_PATH="/mnt/data/perso/tmp/catchchallenger-temp/"
export CATCHCHALLENGERSOURCESPATH="/home/user/Desktop/CatchChallenger/git/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGERSOURCESPATH}/general/base/VersionPrivate.hpp | grep -F "0." | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${CATCHCHALLENGER_VERSION}

echo "Compil windows version..."
source sub-script/compil-windows32.sh
cd ${BASE_PWD}
echo "Compil windows version... done"



