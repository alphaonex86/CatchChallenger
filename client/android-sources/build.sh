#!/bin/sh
cd /home/user/src/
git pull
cd /home/user/
mkdir /home/user/build && cd /home/user/build
mkdir /home/user/build/dist/
#qmake -r /home/user/src/client/catchchallenger.pro CONFIG+=debug ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so
qmake -r /home/user/src/client/catchchallenger.pro ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so
make -j5
make install INSTALL_ROOT=/home/user/build/dist/
#make install INSTALL_ROOT=/home/user/dist/
cd /home/user/build/
#androiddeployqt --input android-libcatchchallenger.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --debug
androiddeployqt --input android-libcatchchallenger.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --release
