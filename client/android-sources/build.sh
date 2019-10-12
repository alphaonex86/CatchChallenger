#!/bin/sh
#git clone --recursive https://github.com/alphaonex86/Ultracopier.git ~/src
mkdir ~/build && cd ~/build
qmake -r ~/src/client/catchchallenger.pro ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so
make -j5
make install INSTALL_ROOT=/home/user/build/dist/
androiddeployqt --input android-libcatchchallenger.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --release
