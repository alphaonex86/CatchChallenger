#!/bin/bash

export WINEBASEPATH="/mnt/data/perso/progs/wine/"
export TEMP_PATH="/mnt/data/perso/tmp/catchchallenger-temp/"
export CATCHCHALLENGERSOURCESPATH="/home/user/Desktop/CatchChallenger/git/"
#export CATCHCHALLENGERSOURCESPATH="/home/user/Desktop/CatchChallenger/lanstat/CatchChallenger-responsive-opengl/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export CATCHCHALLENGER_VERSION=`grep -F "CATCHCHALLENGER_VERSION" ${CATCHCHALLENGERSOURCESPATH}/general/base/VersionPrivate.hpp | grep -F "0." | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
function valid_ip()
{
    local  ip=$1
    local  stat=1
    if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
        OIFS=$IFS
        IFS='.'
        ip=($ip)
        IFS=$OIFS
        [[ ${ip[0]} -le 255 && ${ip[1]} -le 255 \
            && ${ip[2]} -le 255 && ${ip[3]} -le 255 ]]
        stat=$?
    fi
    return $stat
}
if ! valid_ip ${CATCHCHALLENGER_VERSION}; then
        echo Wrong version: ${CATCHCHALLENGER_VERSION}
        exit
fi
echo Version: ${CATCHCHALLENGER_VERSION}

echo "Assemble mac version..."
source sub-script/mac.sh
cd ${BASE_PWD}
echo "Assemble mac version... done"



