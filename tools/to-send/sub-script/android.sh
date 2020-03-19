#!/bin/bash

cd /home/user/Desktop/CatchChallenger;./sync.sh
docker start android_armv7_cc
docker exec -t android_armv7_cc rm -Rf /home/user/src /home/user/build
rsync -art --delete /home/user/Desktop/CatchChallenger/git/ /var/lib/docker/overlay/913835a6a4eaf720ff2570ce01ff59db402b886dc3df77f6852f7778294252e3/merged/home/user/src/
docker exec -t android_armv7_cc /bin/bash /home/user/src/client/android-sources/build.sh

cp /var/lib/docker/overlay/913835a6a4eaf720ff2570ce01ff59db402b886dc3df77f6852f7778294252e3/merged/home/user/build/dist/build/outputs/apk/release/dist-release-unsigned.apk /tmp/catchchallenger-org.apk
jarsigner -verbose -keystore /home/user/.android/alpha_one_x86.keystore -sigalg SHA1withRSA -digestalg SHA1 /tmp/catchchallenger-org.apk alpha_one_x86 -storepass krfkrf
#/home/user/Android/Sdk/build-tools/29.0.2/zipalign -f -v 4 /tmp/catchchallenger-org.apk /tmp/catchchallenger.apk
#mv /tmp/catchchallenger-org.apk ${TEMP_PATH}/catchchallenger-android-${ULTRACOPIER_VERSION}.apk
#rsync -avrt /tmp/catchchallenger-org.apk root@ssh.first-world.info:/home/first-world.info/files-rw/catchchallenger/2.2.3.X/catchchallenger-android-2.2.3.X.apk -e 'ssh -p 54973'


#64Bits
#docker start android_arm64_v8a_cc
#docker exec -it android_arm64_v8a_cc rm -Rf /home/user/src /home/user/build
#rsync -avrt --delete /home/user/Desktop/CatchChallenger/git/ /var/lib/docker/overlay/913835a6a4eaf720ff2570ce01ff59db402b886dc3df77f6852f7778294252e3/merged/home/user/src/
#docker exec -it android_arm64_v8a_cc /bin/bash /home/user/src/client/android-sources/build.sh
#cp /var/lib/docker/overlay/913835a6a4eaf720ff2570ce01ff59db402b886dc3df77f6852f7778294252e3/merged/home/user/build/dist/build/outputs/apk/release/dist-release-unsigned.apk /tmp/catchchallenger-org.apk
#jarsigner -verbose -keystore /home/user/.android/alpha_one_x86.keystore -sigalg SHA1withRSA -digestalg SHA1 /tmp/catchchallenger-org.apk alpha_one_x86 -storepass krfkrf
