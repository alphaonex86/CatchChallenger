#!/bin/sh
DEBUG=0
if [ $DEBUG -eq 0 ]
then
    mkdir /home/user/build
    cd /home/user/build
    mkdir /home/user/build/dist/
    #qmake -r /home/user/src/client/catchchallenger.pro CONFIG+=debug ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so QMAKE_CFLAGS+=-funwind-tables QMAKE_CXXFLAGS+=-funwind-tables
    qmake -r /home/user/src/client/catchchallenger.pro ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so
    make -j5
    make install INSTALL_ROOT=/home/user/build/dist/
    #cp -r /home/user/src/client/android-sources/* /home/user/build/dist/
    #make install INSTALL_ROOT=/home/user/dist/
    cd /home/user/build/
    #androiddeployqt --input android-libcatchchallenger.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --debug
    androiddeployqt --input android-libcatchchallenger.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --release
else
    mkdir /home/user/build
    cd /home/user/build
    mkdir /home/user/build/dist/
    qmake -r /home/user/src/client/catchchallenger.pro CONFIG+=debug ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so QMAKE_CFLAGS+=-funwind-tables QMAKE_CXXFLAGS+=-funwind-tables
    make -j5
    make install INSTALL_ROOT=/home/user/build/dist/
    cd /home/user/build/
    androiddeployqt --input android-libcatchchallenger.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --debug
fi
