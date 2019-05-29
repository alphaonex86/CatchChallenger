#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

IPMAC="192.168.158.34"
SSHUSER="user"
QTVERSION="5.9.4"
QTVERSIONMAJ="5.9"

echo try first connect:
ssh user@${IPMAC} exit
if [ `ssh user@${IPMAC} 'echo lVt75gJ4sJXjq2gWxzXd8pV8' | grep lVt75gJ4sJXjq2gWxzXd8pV8 | wc -l` -ne 1 ]
then
    echo no connect...
    exit
fi

ssh user@${IPMAC} "cd /usr/bin/;ln -s /Applications/Xcode.app/Contents/Developer//Toolchains/XcodeDefault.xctoolchain/usr/bin/otool > /dev/null 2>&1"
ssh user@${IPMAC} "mkdir /opt/ > /dev/null 2>&1;mkdir /opt/local/ > /dev/null 2>&1;mkdir /opt/local/lib/ > /dev/null 2>&1;cd /opt/local/lib/;ln -s /usr/lib/libz.1.dylib > /dev/null 2>&1"

function compil {
    cd ${TEMP_PATH}/
    TARGET=$1
    FINAL_ARCHIVE="catchchallenger-mac-os-x-${CATCHCHALLENGER_VERSION}.zip"
    if [ ! -e ${FINAL_ARCHIVE} ]
    then
        echo "Making Mac zip: ${FINAL_ARCHIVE} ..."

        rm -Rf ${TEMP_PATH}/${TARGET}-mac-os-x/
        cp -aRf ${CATCHCHALLENGER_SOURCE}/ ${TEMP_PATH}/${TARGET}-mac-os-x/
        find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "GeneralVariable.h" -exec sed -i "s/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/#define CATCHCHALLENGER_EXTRA_CHECK/g" {} \; > /dev/null 2>&1
        echo "try connect"
        ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/CatchChallenger/"
        echo "try rsync 1"
        ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/"
        echo "try rsync 2"
        ssh user@${IPMAC} "chown -f -R ${SSHUSER} /Users/${SSHUSER}/Desktop/CatchChallenger/"
        echo "try rsync 3"
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/server/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        echo "try rsync 4"
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/general/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/general/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/client/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*

        echo "try qmake"
        BASEAPPNAME="catchchallenger-${TARGET}.app"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/qmake *.pro -spec macx-clang -config release"
        echo "try make"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null 2>&1"
        RETURN_CODE=$?
        if [ $? -ne 0 ]
        then
            echo "make failed on the mac: ${RETURN_CODE}"
            exit
        fi
        rm -Rf /tmp/macbuild/
        rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/*.app/Contents/MacOS/catchchallenger-* /tmp/macbuild/
        if [ ! -f /tmp/macbuild/catchchallenger-* ]
        then
            ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3"
            echo "make failed on the mac: ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/*.app/Contents/MacOS/catchchallenger-*"
            echo rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/*.app/Contents/MacOS/catchchallenger-* /tmp/macbuild/
            exit
        fi
        minimumsize=1000000
        actualsize=$(wc -c </tmp/macbuild/catchchallenger-*)
        if [ $actualsize -ge $minimumsize ]
        then
            echo /tmp/macbuild/catchchallenger-* size is over $minimumsize bytes, all is good
        else
            ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3"
            echo /tmp/macbuild/catchchallenger-* $actualsize size is under $minimumsize bytes: abort
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
            rsync -art ${DATAPACK_SOURCE} ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/ --exclude=.git --exclude=*.git*
        fi
        if [ ${TARGET} == "ultimate" ]
        then
            ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/"
            ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/internal/"
            rsync -art ${DATAPACK_SOURCE} ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/internal/ --exclude=.git --exclude=*.git*
        fi
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;mv catchchallenger-${TARGET}.app/ catchchallenger.app/ > /dev/null 2>&1"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/macdeployqt catchchallenger.app/;zip -r9 catchchallenger.zip catchchallenger.app/ > /dev/null 2>&1"
        rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/catchchallenger.zip ${TEMP_PATH}/${FINAL_ARCHIVE}
        if [ ! -e ${FINAL_ARCHIVE} ]; then
            echo ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/client/${TARGET}/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/macdeployqt catchchallenger.app/;zip -r9 catchchallenger.zip catchchallenger.app/"
            echo "${FINAL_ARCHIVE} not exists!";
            exit;
        fi
        minimumsize=10000000
        actualsize=$(wc -c <"${FINAL_ARCHIVE}")
        if [ $actualsize -ge $minimumsize ]; then
            echo ${FINAL_ARCHIVE} size is over $minimumsize bytes, all is good
        else
            echo $actualsize size is under $minimumsize bytes: abort
            exit
        fi
        ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/CatchChallenger/"
        echo "Making binary debug Mac zip... done"
    else
        echo "Archive already exists: ${FINAL_ARCHIVE}"
    fi
}

function compilserver {
    cd ${TEMP_PATH}/
    TARGET=$1
    FINAL_ARCHIVE="catchchallenger-${TARGET}-mac-os-x-${CATCHCHALLENGER_VERSION}.zip"
    if [ ! -e ${FINAL_ARCHIVE} ]
    then
        echo "Making Mac zip: ${FINAL_ARCHIVE} ..."

        rm -Rf ${TEMP_PATH}/${TARGET}-mac-os-x/
        cp -aRf ${CATCHCHALLENGER_SOURCE}/ ${TEMP_PATH}/${TARGET}-mac-os-x/
        find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "GeneralVariable.h" -exec sed -i "s/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/#define CATCHCHALLENGER_EXTRA_CHECK/g" {} \; > /dev/null 2>&1
        echo "try connect"
        ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/CatchChallenger/"
        echo "try rsync"
        ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/"
        ssh user@${IPMAC} "chown -f -R ${SSHUSER} /Users/${SSHUSER}/Desktop/CatchChallenger/"
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/server/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/general/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/general/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/client/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/client/ --exclude=*.pro.user* --exclude=*Qt5-Debug --exclude=*build-* --exclude=*build-Desktop* --exclude=*Desktop-Debug*

        echo "try qmake"
        BASEAPPNAME="catchchallenger-${TARGET}.app"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/qmake catchchallenger-${TARGET}.pro -spec macx-clang -config release"
        echo "try make"
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null 2>&1"
        RETURN_CODE=$?
        if [ $? -ne 0 ]
        then
            echo "make failed on the mac server: ${RETURN_CODE}"
            exit
        fi
        rm -Rf /tmp/macbuild/
        rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.app/Contents/MacOS/catchchallenger-* /tmp/macbuild/
        if [ ! -f /tmp/macbuild/catchchallenger-* ]
        then
            ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3"
            echo "make server failed on the mac"
            exit
        fi
        minimumsize=1000000
        actualsize=$(wc -c </tmp/macbuild/catchchallenger-*)
        if [ $actualsize -ge $minimumsize ]
        then
            echo /tmp/macbuild/catchchallenger-* size is over $minimumsize bytes, all is good
        else
            ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3"
            echo /tmp/macbuild/catchchallenger-* size $actualsize is under $minimumsize bytes: abort
            exit
        fi
        
        ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/"
        
        rsync -art ${DATAPACK_SOURCE} ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.app/Contents/MacOS/datapack/ --exclude=.git --exclude=*.git*
        rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/server/databases/catchchallenger.db.sqlite ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger-${TARGET}.app/Contents/MacOS/
        ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/CatchChallenger/server/;/Users/user/Qt${QTVERSION}/${QTVERSION}/clang_64/bin/macdeployqt ${BASEAPPNAME}/;zip -r9 catchchallenger.zip catchchallenger-${TARGET}.app/ > /dev/null 2>&1"
        rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/CatchChallenger/server/catchchallenger.zip ${TEMP_PATH}/${FINAL_ARCHIVE}
        if [ ! -e ${FINAL_ARCHIVE} ]; then
            echo "${FINAL_ARCHIVE} not exists!";
            exit;
        fi
        minimumsize=10000000
        actualsize=$(wc -c <"${FINAL_ARCHIVE}")
        if [ $actualsize -ge $minimumsize ]; then
            echo ${FINAL_ARCHIVE} size is over $minimumsize bytes, all is good
        else
            echo $actualsize size is under $minimumsize bytes: abort
            exit
        fi
        ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/CatchChallenger/"
        echo "Making binary debug Mac zip... done"
    else
        echo "Archive already exists: ${FINAL_ARCHIVE}"
    fi
}

compil "ultimate"
compilserver "server-cli"
compilserver "server-gui"
