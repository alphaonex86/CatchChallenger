#!/bin/bash

export TEMP_PATH="/mnt/data/perso/tmp/catchchallenger-temp/"
export CATCHCHALLENGERSOURCESPATH="/home/user/Desktop/CatchChallenger/git/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGERSOURCESPATH}/general/base/VersionPrivate.hpp | grep -F "0." | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${CATCHCHALLENGER_VERSION}

echo "Clean the temp folder..."
source sub-script/clean-all.sh
cd ${BASE_PWD}
echo "Clean the temp folder... done"


