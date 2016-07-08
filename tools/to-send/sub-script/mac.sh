#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

IPMAC="192.168.158.34"
SSHUSER="user"
QTVERSION="5.4.1"
QTVERSIONMAJ="5.4"

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
        ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/"
        ssh root@${IPMAC} "chown -f -R ${SSHUSER} /Users/${SSHUSER}/Desktop/CatchChallenger/"
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/server/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/general/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/general/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/client/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*

        echo "try qmake"
        BASEAPPNAME="catchchallenger-${TARGET}.app"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake *.pro -spec macx-clang -config release"
        echo "try make"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3"
        RETURN_CODE=$?
        if [ $? -ne 0 ]
        then
            echo "make failed on the mac: ${RETURN_CODE}"
            exit
        fi
        
        #vlc
        ssh ${SSHUSER}@${IPMAC} "rsync -art /Users/${SSHUSER}/Desktop/VLC.app/Contents/MacOS/share/ /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/share/"
        ssh ${SSHUSER}@${IPMAC} "rsync -art /Users/${SSHUSER}/Desktop/VLC.app/Contents/MacOS/lib/ /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/lib/"
        ssh ${SSHUSER}@${IPMAC} "rsync -art /Users/${SSHUSER}/Desktop/VLC.app/Contents/MacOS/plugins/ /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/plugins/"
        
        #music
        ssh ${SSHUSER}@${IPMAC} "rsync -art /Users/${SSHUSER}/Desktop/CatchChallenger/client/base/resources/music/ /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/music/"
        
        ssh ${SSHUSER}@${IPMAC} "rsync -art /Users/${SSHUSER}/Desktop/CatchChallenger/client/base/languages/ /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/languages/ --exclude=*.ts"
        ssh ${SSHUSER}@${IPMAC} "rsync -art /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/languages/ /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/languages/ --exclude=*.ts"
        ssh ${SSHUSER}@${IPMAC} "rm -Rf /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/languages/en/"
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

function compilserver {
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
        ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/"
        ssh root@${IPMAC} "chown -f -R ${SSHUSER} /Users/${SSHUSER}/Desktop/CatchChallenger/"
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/server/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/general/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/general/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/client/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*

        echo "try qmake"
        BASEAPPNAME="catchchallenger-${TARGET}.app"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake catchchallenger-${TARGET}.pro -spec macx-clang -config release"
        echo "try make"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3"
        RETURN_CODE=$?
        if [ $? -ne 0 ]
        then
            echo "make failed on the mac: ${RETURN_CODE}"
            exit
        fi
        
        ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/"
        
        rsync -art ${DATAPACK_SOURCE} ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/server/databases/catchchallenger.db.sqlite ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.app/Contents/MacOS/
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/macdeployqt ${BASEAPPNAME}/ -dmg"
        rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.dmg ${TEMP_PATH}/${FINAL_ARCHIVE}
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

compil "single-player"
compil "ultimate"
compil "single-server"
compilserver "server-cli"
compilserver "server-gui"
