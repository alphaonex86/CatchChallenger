#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

docker start android_arm64_v8a_cc
docker exec -t android_arm64_v8a_cc rm -Rf /home/user/src /home/user/build
#docker exec -it android_arm64_v8a_cc git clone --recursive --depth=1 https://github.com/alphaonex86/Ultracopier.git /home/user/src
rsync -art /home/user/Desktop/catchchallenger/git/ /var/lib/docker/overlay/2790627fb4af1335486addb4c3c290362bf8332ec5185c69fb944c5ba53343d4/merged/home/user/src/
docker exec -t android_arm64_v8a_cc /bin/bash /home/user/src/android-sources/build.sh

cp /var/lib/docker/overlay/2790627fb4af1335486addb4c3c290362bf8332ec5185c69fb944c5ba53343d4/merged/home/user/build/dist/build/outputs/apk/release/dist-release-unsigned.apk /tmp/catchchallenger-org.apk
jarsigner -verbose -keystore /home/user/.android/alpha_one_x86.keystore -sigalg SHA1withRSA -digestalg SHA1 /tmp/catchchallenger-org.apk alpha_one_x86 -storepass krfkrf
#/home/user/Android/Sdk/build-tools/29.0.2/zipalign -f -v 4 /tmp/catchchallenger-org.apk /tmp/catchchallenger.apk
#mv /tmp/catchchallenger-org.apk ${TEMP_PATH}/catchchallenger-android-${ULTRACOPIER_VERSION}.apk
rsync -avrt /tmp/catchchallenger-org.apk root@ssh.first-world.info:/home/first-world.info/files-rw/catchchallenger/2.2.4.X/catchchallenger-android-2.2.4.X.apk -e 'ssh -p 54973'
