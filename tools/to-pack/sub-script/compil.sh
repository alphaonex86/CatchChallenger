#!/bin/bash

function compil {
	TARGET=$1
	DEBUG=$2
	DEBUG_REAL=$3
	PORTABLE=$4
	PORTABLEAPPS=$5
	BITS=$6
	CFLAGSCUSTOM="$7"
	cd ${BASE_PWD}
	echo "catchchallenger-${TARGET}-windows-x86 rsync..."
	rsync -aqrt ${CATCHCHALLENGERSOURCESPATH} ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/
	for project in `find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type f -name *.pri`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done
	for project in `find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type f -name *.pri`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done
	for project in `find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type f -name *.pri`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done
	for project in `find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type f -name *.pri`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done

	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -name "GeneralVariable.h" -exec sed -i "s/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/#define CATCHCHALLENGER_EXTRA_CHECK/g" {} \; > /dev/null 2>&1
	if [ ${BITS} -eq 32 ]
	then
		REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-32Bits-for-catchchallenger/"
	fi
	COMPIL_SUFFIX="release"
	COMPIL_FOLDER="release"
    if [ "${TARGET}" == "server" ]
    then
        cd ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/server/
        echo "catchchallenger-${TARGET}-windows-x86 server cli..."
        DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" catchchallenger-server-cli.pro
        DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine mingw32-make -j4 ${COMPIL_SUFFIX} > /dev/null 2>&1
        if [ ! -f ${COMPIL_FOLDER}/*.exe ]
        then
            DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine mingw32-make -j4 ${COMPIL_SUFFIX} > /tmp/bug.log 2>&1
            if [ ! -f ${COMPIL_FOLDER}/*.exe ]
            then
                cat /tmp/bug.log
                echo "application not created"
                exit
            fi
        fi
        if [ ${BITS} -eq 32 ]
        then
            upx --lzma -9 ${COMPIL_FOLDER}/*.exe > /dev/null 2>&1
        fi
        mv ${COMPIL_FOLDER}/*.exe ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/
        echo "catchchallenger-${TARGET}-windows-x86 server gui..."
        DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" catchchallenger-server-gui.pro
        DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine mingw32-make -j4 ${COMPIL_SUFFIX} > /dev/null 2>&1
        if [ ! -f ${COMPIL_FOLDER}/*.exe ]
        then
            DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine mingw32-make -j4 ${COMPIL_SUFFIX} > /tmp/bug.log 2>&1
            if [ ! -f ${COMPIL_FOLDER}/*.exe ]
            then
                cat /tmp/bug.log
                echo "application not created"
                exit
            fi
        fi
        if [ ${BITS} -eq 32 ]
        then
            upx --lzma -9 ${COMPIL_FOLDER}/*.exe > /dev/null 2>&1
        fi
        mv ${COMPIL_FOLDER}/*.exe ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/
        /usr/bin/find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type f -not \( -name "*.xml" -or -name "*.dll" -or -name "*.a" -or -name "*.exe" -or -name "*.txt" -or -name "*.qm" -or -name "*.png" \) -exec rm -f {} \;
    else
        cp -Rf ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/client/base/resources/music/ ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/music/
        cd ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/client/${TARGET}/
        echo "catchchallenger-${TARGET}-windows-x86 application..."
        DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" *.pro
        DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine mingw32-make -j4 ${COMPIL_SUFFIX} > /dev/null 2>&1
        if [ ! -f ${COMPIL_FOLDER}/catchchallenger*.exe ]
        then
            DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine mingw32-make -j4 ${COMPIL_SUFFIX} > /tmp/bug.log 2>&1
            if [ ! -f ${COMPIL_FOLDER}/catchchallenger*.exe ]
            then
                cat /tmp/bug.log
                echo "application not created"
                exit
            fi
        fi
        if [ ${BITS} -eq 32 ]
        then
            upx --lzma -9 ${COMPIL_FOLDER}/catchchallenger*.exe > /dev/null 2>&1
        fi
        mv ${COMPIL_FOLDER}/catchchallenger*.exe ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/
        /usr/bin/find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type f -not \( -name "*.xml" -or -name "*.dll" -or -name "*.a" -or -name "*.exe" -or -name "*.txt" -or -name "*.qm" -or -name "*.png" -or -name "*.ogg" \) -exec rm -f {} \;
    fi
	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/catchchallenger-${TARGET}-windows-x86/ -type d -empty -delete > /dev/null 2>&1
	echo "catchchallenger-${TARGET}-windows-x86 compilation... done"
} 
