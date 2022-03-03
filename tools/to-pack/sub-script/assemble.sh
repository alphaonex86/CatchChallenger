#!/bin/bash

function assemble {
    TARGET=$1
    ARCHITECTURE=$2
    DEBUG=$3
    DEBUG_REAL=$4
    cd ${TEMP_PATH}/
    if [ ${DEBUG_REAL} -eq 1 ]
    then
        FINAL_ARCHIVE="catchchallenger-${TARGET}-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}.7z"
    else
        FINAL_ARCHIVE="catchchallenger-${TARGET}-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}.zip"
    fi
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
    rm -f ${FINAL_ARCHIVE}
    if [ ! -e ${FINAL_ARCHIVE} ]; then
        echo "creating the archive ${TARGET}..."
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -iname "*.a" -exec rm {} \; > /dev/null 2>&1
#         rsync -art ${CATCHCHALLENGERSOURCESPATH}/client/${TARGET}/languages/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/languages/
#         rsync -art ${CATCHCHALLENGERSOURCESPATH}/client/base/resources/languages/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/languages/
#         rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/languages/en/
        rsync -art ${CATCHCHALLENGERSOURCESPATH}/client/resources/music/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/music/
#       cp -Rf ${CATCHCHALLENGERSOURCESPATH}/README ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/README.txt
#       cp -Rf ${CATCHCHALLENGERSOURCESPATH}/COPYING ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/COPYING.txt
#        upx --lzma -9 ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/catchchallenger*.exe > /dev/null 2>&1
        if [ ${DEBUG_REAL} -eq 1 ]
        then
            cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt-debug/lib* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
            cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt-debug/* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
        else
            cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt/lib* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
            cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt/* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
        fi
#       cp -f ${BASE_PWD}/data/qm-translation/fr.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/fr/qt.qm
#       cp -f ${BASE_PWD}/data/qm-translation/ar.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ar/qt.qm
#       cp -f ${BASE_PWD}/data/qm-translation/es.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/es/qt.qm
#       cp -f ${BASE_PWD}/data/qm-translation/ja.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ja/qt.qm
#       cp -f ${BASE_PWD}/data/qm-translation/ko.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ko/qt.qm
#       cp -f ${BASE_PWD}/data/qm-translation/pl.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/pl/qt.qm
#       cp -f ${BASE_PWD}/data/qm-translation/pt.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/pt/qt.qm
#       cp -f ${BASE_PWD}/data/qm-translation/ru.qm ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/Languages/ru/qt.qm
        rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/client/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/general/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/server/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/tools/
        mkdir -p ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/datapack/
        rsync -aqrt ${CATCHCHALLENGERSOURCESPATH}/../CatchChallenger-datapack/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/datapack/internal/ --exclude=.git
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -iname "*.ts" -exec rm {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
        
        if [ ${DEBUG_REAL} -eq 1 ]
        then
            mkdir -p ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/catchchallenger/
            mv ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/catchchallenger/
            mv ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/catchchallenger/catchchallenger-debug.bat ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
            cp -Rf /home/wine/qt-5.0-32Bits-for-catchchallenger/drive_c/mingw32/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/mingw32/
        fi

        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -exec chmod 700 "{}" \;
        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type f -exec chmod 600 "{}" \;
        chown -Rf root.root catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type f -exec touch -t 201601020000.00 "{}" \;
        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -exec touch -t 201601020000.00 "{}" \;
        if [ ${DEBUG_REAL} -eq 1 ]
        then
            7za a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ${FINAL_ARCHIVE} catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ > /dev/null
        else
            zip -r -q -9 ${FINAL_ARCHIVE} catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ > /dev/null 2>&1
        fi
        echo "creating the archive ${TARGET}... done"
    fi
    FINAL_ARCHIVE="catchchallenger-${TARGET}-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}-setup.exe"
    rm -f ${FINAL_ARCHIVE}
    if [ ${DEBUG} -eq 0 ] && [ ! -e ${FINAL_ARCHIVE} ]; then
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
        #DISPLAY="na" WINEPREFIX="${WINEBASEPATH}/ultracopier-general/" /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine "${WINEBASEPATH}/ultracopier-general/drive_c/Program Files (x86)/NSIS/makensis.exe" *.nsi > /tmp/nsi.log 2>&1
        /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine "/home/user/.wine/drive_c/Program Files/NSIS/makensis.exe" *.nsi > /dev/null 2>&1
        if [ ! -e *setup.exe ]; then
            echo "${TEMP_PATH}/${FINAL_ARCHIVE} not exists!";
            pwd
            echo /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine "/home/user/.wine/drive_c/Program Files/NSIS/makensis.exe" "*.nsi"
            exit;
        fi
        minimumsize=13000000
        actualsize=$(wc -c <*setup.exe)
        if [ $actualsize -le $minimumsize ]
        then
            cat /tmp/nsi.log
            echo ${TEMP_PATH}/CatchChallenger-installer-windows-${ARCHITECTURE}/
            echo *setup.exe size is under $minimumsize bytes
            exit
        fi
        mv *setup.exe ${TEMP_PATH}/${FINAL_ARCHIVE}
        cd ${TEMP_PATH}/
        echo "creating the installer ${TARGET}... done"
    fi
    rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
} 

function assembleserver {
    TARGET=$1
    ARCHITECTURE=$2
    cd ${TEMP_PATH}/
    FINAL_ARCHIVE="catchchallenger-${TARGET}-windows-${ARCHITECTURE}-${CATCHCHALLENGER_VERSION}"
    if [ ! -d "${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/" ]
    then
        echo "no previous compilation folder found into ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/..."
        exit
    fi
        if [ `find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type f -name '*.exe' | wc -l` -eq 0 ]
        then
                echo "no application found into ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/..."
                exit
        fi
    if [ ! -e ${FINAL_ARCHIVE} ]; then
        echo "creating the archive ${TARGET}..."
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -iname "*.a" -exec rm {} \; > /dev/null 2>&1
#       cp -Rf ${CATCHCHALLENGERSOURCESPATH}/README ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/README.txt
#       cp -Rf ${CATCHCHALLENGERSOURCESPATH}/COPYING ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/COPYING.txt
#        upx --lzma -9 ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/catchchallenger*.exe > /dev/null 2>&1
        cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/server/lib* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
        cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/server/* ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
        rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/client/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/general/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/server/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/tools/
        rsync -aqrt ${CATCHCHALLENGERSOURCESPATH}/../datapack/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/datapack/
        rsync -aqrt ${CATCHCHALLENGERSOURCESPATH}/server/databases/catchchallenger.db.sqlite ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -iname "*.ts" -exec rm {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
        find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
        
        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -exec chmod 700 "{}" \;
        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type f -exec chmod 600 "{}" \;
        chown -Rf root.root catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type f -exec touch -t 201601020000.00 "{}" \;
        find catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ -type d -exec touch -t 201601020000.00 "{}" \;
        #zip -r -q -9 ${FINAL_ARCHIVE}.zip catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ > /dev/null 2>&1
        7za a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ${FINAL_ARCHIVE}.7z catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ > /dev/null 2>&1
        #nice -n 15 ionice -c 3 tar cf - catchchallenger-${TARGET}-windows-${ARCHITECTURE}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | nice -n 15 ionice -c 3 xz -z -9 -e > ${FINAL_ARCHIVE}.tar.xz
#       if [ ! -e ${FINAL_ARCHIVE} ]; then
#           echo "${FINAL_ARCHIVE} not exists!";
#           exit;
#       fi
        echo "creating the archive ${TARGET}... done"
    fi
    rm -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-${ARCHITECTURE}/
} 


