#!/bin/bash

function assemble {
	TARGET=$1
	ARCHITECTURE=$2
	cd ${TEMP_PATH}/
	FINAL_ARCHIVE="catchchallenger-${TARGET}-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}"
	if [ ! -d ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ ]
	then
		echo "no previous compilation folder found into ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/..."
		exit
	fi
        if [ ! -e ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/*.exe ]
        then
                echo "no application found into ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/..."
                exit
        fi
	if [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "creating the archive ${TARGET}..."
		find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -iname "*.a" -exec rm {} \; > /dev/null 2>&1
		rsync -art ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/client/${TARGET}/languages/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/languages/
		rsync -art ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/client/base/languages/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/languages/
		rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/languages/en/
#		cp -Rf ${CATCHCHALLENGERSOURCESPATH}/README ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/README.txt
#		cp -Rf ${CATCHCHALLENGERSOURCESPATH}/COPYING ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/COPYING.txt
		upx --lzma -9 ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/catchchallenger*.exe > /dev/null 2>&1
		cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt/lib* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
		cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt/* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
# 		cp -f ${BASE_PWD}/data/qm-translation/fr.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/fr/qt.qm
# 		cp -f ${BASE_PWD}/data/qm-translation/ar.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ar/qt.qm
# 		cp -f ${BASE_PWD}/data/qm-translation/es.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/es/qt.qm
# 		cp -f ${BASE_PWD}/data/qm-translation/ja.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ja/qt.qm
# 		cp -f ${BASE_PWD}/data/qm-translation/ko.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ko/qt.qm
# 		cp -f ${BASE_PWD}/data/qm-translation/pl.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/pl/qt.qm
# 		cp -f ${BASE_PWD}/data/qm-translation/pt.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/pt/qt.qm
# 		cp -f ${BASE_PWD}/data/qm-translation/ru.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ru/qt.qm
		rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/client/
		rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/tools/
		if [ "${TARGET}" == "single-player" ]
		then
			rsync -aqrt ${CATCHCHALLENGERSOURCESPATH}/../datapack/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/datapack/
		fi
		if [ "${TARGET}" == "ultimate" ]
		then
			mkdir -p ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/datapack/
			rsync -aqrt ${CATCHCHALLENGERSOURCESPATH}/../datapack/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/datapack/internal/
		fi
		find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -iname "*.ts" -exec rm {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
		find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
		find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
		
		#zip -r -q -9 ${FINAL_ARCHIVE}.zip catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ > /dev/null 2>&1
		7za a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ${FINAL_ARCHIVE}.7z catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ > /dev/null 2>&1
		#nice -n 15 ionice -c 3 tar cpf - catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ | nice -n 15 ionice -c 3 xz -z -9 -e > ${FINAL_ARCHIVE}.tar.xz
# 		if [ ! -e ${FINAL_ARCHIVE} ]; then
# 			echo "${FINAL_ARCHIVE} not exists!";
# 			exit;
# 		fi
		echo "creating the archive ${TARGET}... done"
	fi
	FINAL_ARCHIVE="catchchallenger-${TARGET}-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}-setup.exe"
	if [ ${DEBUG} -eq 0 ] && [ ${PORTABLE} -eq 0 ] && [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "creating the installer ${TARGET}..."
		cd ${TEMP_PATH}/
		rm -Rf ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
		mkdir -p ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
		cd ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
		cp -aRf ${BASE_PWD}/data/windows/install.nsi ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
		#cp -aRf ${BASE_PWD}/data/windows/catchchallenger.ico ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
		rsync -art ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
		cd ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
		sed -i -r "s/X.X.X.X/${CATCHCHALLENGER_VERSION}/g" *.nsi > /dev/null 2>&1
		sed -i -r "s/CATCHCHALLENGER_SUBVERSION/${TARGET}/g" *.nsi > /dev/null 2>&1
		DISPLAY="na" WINEPREFIX="${WINEBASEPATH}/ultracopier-general/" /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine "${WINEBASEPATH}/ultracopier-general/drive_c/Program Files (x86)/NSIS/makensis.exe" *.nsi > /dev/null 2>&1
		if [ ! -e *setup.exe ]; then
			echo "${TEMP_PATH}/${FINAL_ARCHIVE} not exists!";
			pwd
			exit;
		fi
		mv *setup.exe ${TEMP_PATH}/${FINAL_ARCHIVE}
                cd ${TEMP_PATH}/
		echo "creating the installer ${TARGET}... done"
	fi
	rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
} 
 
