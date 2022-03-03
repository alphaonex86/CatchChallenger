#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

IPMAC="192.168.158.34"
SSHUSER="user"
QTVERSION="5.15.2"
QTVERSIONMAJ="5.15"

echo try first connect:
ssh user@${IPMAC} exit
if [ `ssh user@${IPMAC} 'echo lVt75gJ4sJXjq2gWxzXd8pV8' | grep lVt75gJ4sJXjq2gWxzXd8pV8 | wc -l` -ne 1 ]
then
    echo no connect...
    exit
fi

#ssh user@${IPMAC} "cd /usr/bin/;ln -s /Applications/Xcode.app/Contents/Developer//Toolchains/XcodeDefault.xctoolchain/usr/bin/otool > /dev/null 2>&1"
#ssh user@${IPMAC} "mkdir /opt/ > /dev/null 2>&1;mkdir /opt/local/ > /dev/null 2>&1;mkdir /opt/local/lib/ > /dev/null 2>&1;cd /opt/local/lib/;ln -s /usr/lib/libz.1.dylib > /dev/null 2>&1"

#compil "catchchallenger" 1 0
function compil {
	DEBUG=$2
	ULTIMATE=$3
	cd ${TEMP_PATH}/
	TARGET=$1
	STATIC=$4
	SUPERCOPIER=${5}
	DEBUGREAL=${6}
	if [ $DEBUGREAL -eq 1 ]
	then
		LIBEXT="_debug.dylib"
		CONFIGARG="debug"
	else
		LIBEXT=".dylib"
		CONFIGARG="release"
	fi
	FINAL_ARCHIVE="${TARGET}-mac-os-x-${CATCHCHALLENGER_VERSION}.dmg"
	if [ ! -e ${FINAL_ARCHIVE} ]
	then
		echo "Making Mac dmg: ${FINAL_ARCHIVE} ..."

		rm -Rf ${TEMP_PATH}/${TARGET}-mac-os-x/
		cp -aRf ${CATCHCHALLENGERSOURCESPATH}/ ${TEMP_PATH}/${TARGET}-mac-os-x/
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>mac-os-x<\/architecture>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${CATCHCHALLENGER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "informations.xml" -exec sed -i -r "s/<pubDate>.*<\pubDate>/<pubDate>`date +%s`<\pubDate>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define CATCHCHALLENGER_VERSION_PORTABLE/\/\/#define CATCHCHALLENGER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define CATCHCHALLENGER_VERSION_PORTABLEAPPS/\/\/#define CATCHCHALLENGER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
		if [ ${DEBUG} -eq 1 ]
		then
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
		else
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
		fi
		if [ $STATIC -eq 1 ]
		then
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
		else

			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
		fi
		if [ $ULTIMATE -eq 1 ]
		then
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define CATCHCHALLENGER_VERSION_ULTIMATE/#define CATCHCHALLENGER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
		else
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define CATCHCHALLENGER_VERSION_ULTIMATE/\/\/#define CATCHCHALLENGER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
		fi

		echo "try connect"
		ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/catchchallenger/"
		echo "try rsync"
		rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/catchchallenger/

		echo "try qmake"
    	BASEAPPNAME="catchchallenger.app"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/catchchallenger/client/;/Users/user/Qt/${QTVERSION}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/catchchallenger/client/catchchallenger.pro -spec macx-clang -config ${CONFIGARG} DEFINES+=AUDIO"
		echo "try make"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/catchchallenger/client/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j24 > /dev/null > /dev/null"
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
		echo "try import resources"
		ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/datapack/"
        rsync -art /home/user/Desktop/CatchChallenger/CatchChallenger-datapack/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/datapack/internal/
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
        rsync -art /home/user/Desktop/CatchChallenger/client/resources/music/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/music/
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
		
        ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/catchchallenger"
        ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtWidgets.framework/Versions/5/Widgets @executable_path/../Frameworks/Widgets.framework/Versions/5/Widgets /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/catchchallenger"
        ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtNetwork.framework/Versions/5/Network @executable_path/../Frameworks/Network.framework/Versions/5/Network /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/catchchallenger"
        ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtXml.framework/Versions/5/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/5/QtXml /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/catchchallenger"
        ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/MacOS/catchchallenger"
        
        ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/Frameworks/ > /dev/null 2>&1"
        ssh ${SSHUSER}@${IPMAC} "rsync -art /Users/user/Qt/${QTVERSION}/clang_64/lib/QtXml.framework/ /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/Frameworks/QtXml.framework/"
        ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/Frameworks/QtXml.framework/Versions/5/QtXml"
        #ssh ${SSHUSER}@${IPMAC} "cp /usr/lib/libz.1.dylib /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/Contents/Frameworks/"
        
        #ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/catchchallenger/resources/finish.opus /Users/${SSHUSER}/Desktop/catchchallenger/client/${BASEAPPNAME}/"

		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/catchchallenger/client/;/Users/user/Qt/${QTVERSION}/clang_64/bin/macdeployqt ${BASEAPPNAME}/ -dmg"
		if [ $? -ne 0 ]; then
			echo "${FINAL_ARCHIVE} bug abort";
			exit;
		fi
		rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/catchchallenger/client/catchchallenger.dmg ${TEMP_PATH}/${FINAL_ARCHIVE}
		if [ ! -e ${FINAL_ARCHIVE} ]; then
			echo "${FINAL_ARCHIVE} not exists!";
			exit;
		fi
		ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/catchchallenger/"
		echo "Making binary debug Mac dmg... done"
	else
		echo "Archive already exists: ${FINAL_ARCHIVE}"
	fi
}

function compil_plugin {
	DEBUG=$2
	cd ${TEMP_PATH}/
	TARGET=$1
	SUBFOLDER=$3

	rsync -art --delete ${CATCHCHALLENGERSOURCESPATH}/ ${TEMP_PATH}/${TARGET}-mac-os-x/

	cd ${TEMP_PATH}/${TARGET}-mac-os-x/${SUBFOLDER}/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/${TARGET}-mac-os-x/${SUBFOLDER}/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/1\.0\.0\.0/${CATCHCHALLENGER_VERSION}/g" {} \; > /dev/null 2>&1
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					# && [ ${plugins_name} != "Windows" ] -> for the plugins interface
					if [ -d ${plugins_name} ] && [ ! -f "${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-mac-os-x.urc" ] && [ ${plugins_name} != "dbus" ] && [ ${plugins_cat} != "SessionLoader" ]
					then
						echo "pack the mac for the alternative plugin: ${plugins_cat}/${plugins_name}"
						mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/

						find ${plugins_name}/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>mac-os-x<\/architecture>/g" {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define CATCHCHALLENGER_VERSION_PORTABLE/\/\/#define CATCHCHALLENGER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define CATCHCHALLENGER_VERSION_PORTABLEAPPS/\/\/#define CATCHCHALLENGER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
						if [ ${DEBUG} -eq 1 ]
						then
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
						else
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
						fi

						ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/catchchallenger/"
						rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/catchchallenger/
						ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/catchchallenger/${SUBFOLDER}/${plugins_cat}/${plugins_name}/;/Users/user/Qt/${QTVERSION}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/catchchallenger/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*.pro -spec macx-clang -config ${CONFIGARG}"
						ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/catchchallenger/${SUBFOLDER}/${plugins_cat}/${plugins_name}/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j24 > /dev/null"
						RETURN_CODE=$?
						if [ $? -ne 0 ]
						then
							echo "make failed on the mac: ${RETURN_CODE}"
							exit
						fi
						ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/catchchallenger/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}"
						ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/catchchallenger/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}"
						ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change /Users/user/Qt/${QTVERSION}/clang_64/lib/QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork /Users/${SSHUSER}/Desktop/catchchallenger/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}"
						rsync -art "${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/catchchallenger/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}" ${plugins_name}/
						if [ ! -e ${plugins_name}/*.dylib ]; then
							echo "no .dylib file!";
						else
							find ${plugins_name}/ -iname "*.ts" -exec rm {} \;
							find ${plugins_name}/ -maxdepth 1 -mindepth 1 -type f ! -iname "*.dylib" ! -iname "informations.xml" -exec rm {} \;
							find ${plugins_name}/ -maxdepth 1 -mindepth 1 -type d ! -iname "Languages" -exec rm -Rf {} \;
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1

							tar --posix -c -f - ${plugins_name}/ | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-mac-os-x.urc
						fi
					fi
				fi
			done
		fi
		cd ${TEMP_PATH}/${TARGET}-mac-os-x/${SUBFOLDER}/
	done
}

compil "catchchallenger" 0 0 0 0 0
#compil "catchchallenger-ultimate" 0 1 0 0 0
compil "catchchallenger-debug" 1 0 0 0 0

#compil_plugin "catchchallenger" 0 "plugins-alternative"
#compil_plugin "catchchallenger" 0 "plugins"

