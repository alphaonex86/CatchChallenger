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
rsync -art /home/user/Desktop/CatchChallenger/git/ /var/lib/docker/overlay/2790627fb4af1335486addb4c3c290362bf8332ec5185c69fb944c5ba53343d4/merged/home/user/src/
docker exec -t android_arm64_v8a_cc /bin/bash /home/user/src/client/android-sources/build.sh

cp /var/lib/docker/overlay/2790627fb4af1335486addb4c3c290362bf8332ec5185c69fb944c5ba53343d4/merged/home/user/build/dist/build/outputs/apk/release/dist-release-unsigned.apk /home/user/catchchallenger-org.apk
# Keystore signing password is kept OUT of the repo (a user-owned file, mode 600).
# Override the location with CC_ANDROID_STOREPASS_FILE. -storepass:file also keeps
# the password off the process command line (it would show in `ps` otherwise).
STOREPASS_FILE="${CC_ANDROID_STOREPASS_FILE:-/home/user/.config/CatchChallenger/android-keystore.storepass}"
if [ ! -s "$STOREPASS_FILE" ]
then
	echo "ERROR: keystore password file missing/empty: $STOREPASS_FILE" >&2
	echo "  create it out of the repo:  printf '%s' '<keystore-password>' > \"$STOREPASS_FILE\" && chmod 600 \"$STOREPASS_FILE\"" >&2
	exit 1
fi
jarsigner -verbose -keystore /home/user/.android/alpha_one_x86.keystore -sigalg SHA1withRSA -digestalg SHA1 /home/user/catchchallenger-org.apk alpha_one_x86 -storepass:file "$STOREPASS_FILE"
#/home/user/Android/Sdk/build-tools/29.0.2/zipalign -f -v 4 /home/user/catchchallenger-org.apk /home/user/catchchallenger.apk
#mv /home/user/catchchallenger-org.apk ${TEMP_PATH}/catchchallenger-android-${ULTRACOPIER_VERSION}.apk
rsync -avrt /home/user/catchchallenger-org.apk root@ssh.first-world.info:/home/first-world.info/files-rw/catchchallenger/2.2.4.X/catchchallenger-android-2.2.4.X.apk -e 'ssh -p 54973'
