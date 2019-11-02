#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

ARCHITECTURE="x86"

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
source ${BASE_PWD}/sub-script/assemble.sh

compil "single-player" 1 1 0 0 32 "-mtune=generic -march=i686" 0
rm -Rf ${TEMP_PATH}/catchchallenger-single-player-debug-real-windows-x86
mv ${TEMP_PATH}/catchchallenger-single-player-windows-x86 ${TEMP_PATH}/catchchallenger-single-player-debug-real-windows-x86
assemble "single-player-debug-real" "${ARCHITECTURE}" 1 1
compil "single-server" 1 1 0 0 32 "-mtune=generic -march=i686" 0
rm -Rf ${TEMP_PATH}/catchchallenger-single-server-debug-real-windows-x86
mv ${TEMP_PATH}/catchchallenger-single-server-windows-x86 ${TEMP_PATH}/catchchallenger-single-server-debug-real-windows-x86
assemble "single-server-debug-real" "${ARCHITECTURE}" 1 1
compil "ultimate" 1 1 0 0 32 "-mtune=generic -march=i686" 0
rm -Rf ${TEMP_PATH}/catchchallenger-ultimate-debug-real-windows-x86
mv ${TEMP_PATH}/catchchallenger-ultimate-windows-x86 ${TEMP_PATH}/catchchallenger-ultimate-debug-real-windows-x86
assemble "ultimate-debug-real" "${ARCHITECTURE}" 1 1
compil "server" 1 1 0 0 32 "-mtune=generic -march=i686" 0
rm -Rf ${TEMP_PATH}/catchchallenger-server-debug-real-windows-x86
mv ${TEMP_PATH}/catchchallenger-server-windows-x86 ${TEMP_PATH}/catchchallenger-server-debug-real-windows-x86
assembleserver "server-debug-real" "${ARCHITECTURE}" 1 1

compil "single-player" 0 0 0 0 32 "-mtune=generic -march=i686" 0
assemble "single-player" "${ARCHITECTURE}" 0 0
compil "single-server" 0 0 0 0 32 "-mtune=generic -march=i686" 0
assemble "single-server" "${ARCHITECTURE}" 0 0
rm -Rf /home/ultracopier-temp/catchchallenger-ultimate*
compil "ultimate" 0 0 0 0 32 "-mtune=generic -march=i686" 1
assemble "ultimate" "${ARCHITECTURE}" 0 0
mkdir -p /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/
mv /home/ultracopier-temp/catchchallenger-ultimate-windows-x86-${CATCHCHALLENGER_VERSION}-setup.exe /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/catchchallenger-ultimate-windows-x86-${CATCHCHALLENGER_VERSION}-cracked-setup.exe
mv /home/ultracopier-temp/catchchallenger-ultimate-windows-x86-${CATCHCHALLENGER_VERSION}.zip /home/first-world.info/files-rw/catchchallenger/${CATCHCHALLENGER_VERSION}/catchchallenger-ultimate-windows-x86-${CATCHCHALLENGER_VERSION}-cracked.zip
rm -Rf /home/ultracopier-temp/catchchallenger-ultimate*
compil "ultimate" 0 0 0 0 32 "-mtune=generic -march=i686" 0
assemble "ultimate" "${ARCHITECTURE}" 0 0
compil "server" 0 0 0 0 32 "-mtune=generic -march=i686" 0
assembleserver "server" "${ARCHITECTURE}" 0 0
