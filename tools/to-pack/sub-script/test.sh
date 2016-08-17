#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

ARCHITECTURE="x86"

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
source ${BASE_PWD}/sub-script/assemble.sh

compil "ultimate" 0 0 0 0 32 "-mtune=generic -march=i686" 0
exit

#TARGET=$1
#DEBUG=$2
#DEBUG_REAL=$3
#PORTABLE=$4
#PORTABLEAPPS=$5
#BITS=$6
#CFLAGSCUSTOM="$7"
#CRACKED=$8
compil "ultimate" 1 1 0 0 32 "-mtune=generic -march=i686" 0
#TARGET=$1
#ARCHITECTURE=$2
#DEBUG=$3
#DEBUG_REAL=$4
assemble "ultimate" "${ARCHITECTURE}" 1 1

#for cracked version
#mv /home/ultracopier-temp/catchchallenger-ultimate-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}-setup.exe /home/ultracopier-temp/catchchallenger-ultimate-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}-cracked-setup.exe
#mv /home/ultracopier-temp/catchchallenger-ultimate-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}.zip /home/ultracopier-temp/catchchallenger-ultimate-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}-cracked.zip

#mv /home/ultracopier-temp/catchchallenger-ultimate-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}.7z /home/ultracopier-temp/catchchallenger-ultimate-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}-cracked.7z
#compil "ultimate" 0 1 0 0 32 "-mtune=generic -march=i686" 0
#assemble "ultimate" "${ARCHITECTURE}" 1 1
