#!/bin/bash

function compil {
	TARGET="catchchallenger-$1-windows-x86"
	DEBUG=$2
	DEBUG_REAL=$3
	PORTABLE=$4
	PORTABLEAPPS=$5
	BITS=$6
	CFLAGSCUSTOM="$7"
	CRACKED=$8
	cd ${BASE_PWD}
	echo "${TARGET} rsync..."
	rsync -aqrt ${CATCHCHALLENGERSOURCESPATH} ${TEMP_PATH}/${TARGET}/
	for project in `find ${TEMP_PATH}/${TARGET}/ -type f -name *.pro`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done
	for project in `find ${TEMP_PATH}/${TARGET}/ -type f -name *.pro`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done
	for project in `find ${TEMP_PATH}/${TARGET}/ -type f -name *.pro`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done
	for project in `find ${TEMP_PATH}/${TARGET}/ -type f -name *.pro`
	do
		if [ -f ${project} ]
		then
			lupdate ${project} > /dev/null 2>&1
			lrelease -nounfinished -compress -removeidentical ${project} > /dev/null 2>&1
		fi
	done

	find ${TEMP_PATH}/${TARGET}/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -name "GeneralVariable.h" -exec sed -i "s/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/#define CATCHCHALLENGER_EXTRA_CHECK/g" {} \; > /dev/null 2>&1
    if [ ${BITS} -eq 32 ]
    then
        MXEPATH='/mnt/data/perso/progs/mxe/i686/'
        MXEPATHQMAKE='/mnt/data/perso/progs/mxe/i686/usr/bin/i686-w64-mingw32.shared-qmake-qt5'
        export PATH=/mnt/data/perso/progs/mxe/i686/usr/bin:$PATH
    fi
    if [ ${BITS} -eq 64 ]
    then
        MXEPATH='/mnt/data/perso/progs/mxe/x86_64/'
        MXEPATHQMAKE='/mnt/data/perso/progs/mxe/x86_64/usr/bin/x86_64-w64-mingw32.shared-qmake-qt5'
        export PATH=/mnt/data/perso/progs/mxe/x86_64/usr/bin:$PATH
    fi
    REAL_WINEPREFIX="${MXEPATH}"
    mkdir -p ${REAL_WINEPREFIX}/drive_c/temp/
    COMPIL_SUFFIX="release"
    COMPIL_FOLDER="release"
    rsync -art --delete ${TEMP_PATH}/${TARGET}/ ${REAL_WINEPREFIX}/drive_c/temp/
    if [ $? -ne 0 ]
    then
        echo line: $LINENO
        echo rsync -art --delete ${TEMP_PATH}/${TARGET}/ ${REAL_WINEPREFIX}/drive_c/temp/
        exit 1
    fi
    cd ${REAL_WINEPREFIX}/drive_c/temp/
    cd client/qtcpu800x600/
    echo "${TARGET} application..."
    ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" qtcpu800x600.pro
    if [ $? -ne 0 ]
    then
        echo ${MXEPATHQMAKE} fail into `pwd` $LINENO
        exit 1
    fi
    make -j16 ${COMPIL_SUFFIX} > /dev/null
    if [ ! -f ${COMPIL_FOLDER}/catchchallenger.exe ]
    then
        make -j16 ${COMPIL_SUFFIX} > /tmp/bug.log 2>&1
        if [ ! -f ${COMPIL_FOLDER}/catchchallenger.exe ]
        then
            cat /tmp/bug.log
            echo "application not created"
            exit
        fi
    fi
    cd ${REAL_WINEPREFIX}/drive_c/temp/
    rsync -art --delete ${REAL_WINEPREFIX}/drive_c/temp/ ${TEMP_PATH}/${TARGET}/
    if [ $? -ne 0 ]
    then
        echo line: $LINENO
        echo rsync -art --delete ${REAL_WINEPREFIX}/drive_c/temp/ ${TEMP_PATH}/${TARGET}/
        exit 1
    fi
    rm -Rf ${REAL_WINEPREFIX}/drive_c/temp/
    cd ${TEMP_PATH}/${TARGET}/
    if [ "${COMPIL_FOLDER}" != "./" ]
    then
        if [ ! -e client/qtcpu800x600/${COMPIL_FOLDER}/catchchallenger.exe ]
        then
            echo ${COMPIL_FOLDER}/catchchallenger.exe not found into `pwd`
            exit
        fi
        mv client/qtcpu800x600/${COMPIL_FOLDER}/catchchallenger.exe ./
    fi

	lupdate -no-obsolete ultracopier.pro > /dev/null 2>&1
	lrelease -nounfinished -compress -removeidentical ultracopier.pro > /dev/null 2>&1
	PWD_BASE2=`pwd`
	echo "update the .ts file"
	cd ${TEMP_PATH}/${TARGET}/

#    cp ${TEMP_PATH}/${TARGET}/resources/finish.opus ${TEMP_PATH}/${TARGET}/
    /usr/bin/find ${TEMP_PATH}/${TARGET}/ -type f -not \( -name "*.xml" -or -name "*.dll" -or -name "*.a" -or -name "*.exe" -or -name "*.txt" -or -name "*.qm" -or -name "*.opus" \) -exec rm -f {} \;
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    echo "${TARGET} compilation... done"
} 
