#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

IPMAC="192.168.158.34"
SSHUSER="user"
QTVERSION="5.1.1"

function compil {
	cd ${TEMP_PATH}/
	TARGET=$1
	FINAL_ARCHIVE="catchchallenger-${TARGET}-mac-os-x-${CATCHCHALLENGER_VERSION}.dmg"
	if [ ! -e ${FINAL_ARCHIVE} ]
	then
		echo "Making Mac dmg: ${FINAL_ARCHIVE} ..."

		rm -Rf ${TEMP_PATH}/${TARGET}-mac-os-x/
		cp -aRf ${CATCHCHALLENGER_SOURCE}/ ${TEMP_PATH}/${TARGET}-mac-os-x/
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "GeneralVariable.h" -exec sed -i "s/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/#define CATCHCHALLENGER_EXTRA_CHECK/g" {} \; > /dev/null 2>&1
		echo "try connect"
		ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/CatchChallenger/"
		echo "try rsync"
		rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/

		echo "try qmake"
		BASEAPPNAME="catchchallenger-${TARGET}.app"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/qmake *.pro -spec macx-g++ -config release"
		echo "try make"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/usr/bin/make -j 3 2> /dev/null > /dev/null"
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
		if [ ${TARGET} == "single-player" ]
		then
			ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/"
			rsync -art ${DATAPACK_SOURCE} ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/
		fi
		if [ ${TARGET} == "ultimate" ]
		then
			ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/"
			ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/internal/"
			rsync -art ${DATAPACK_SOURCE} ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/internal/
		fi
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/macdeployqt ${BASEAPPNAME}/ -dmg"
		rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.dmg ${TEMP_PATH}/${FINAL_ARCHIVE}
		if [ ! -e ${FINAL_ARCHIVE} ]; then
			echo "${FINAL_ARCHIVE} not exists!";
			exit;
		fi
		ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/CatchChallenger/"
		echo "Making binary debug Mac dmg... done"
	else
		echo "Archive already exists: ${FINAL_ARCHIVE}"
	fi
}

compil "ultimate"
compil "single-player"
compil "single-server"
