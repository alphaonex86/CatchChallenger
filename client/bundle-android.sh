#rm -r build-android
mkdir -p build-android
cd build-android
echo "Configuring for Native..."
qmake -platform android-clang -r ../catchchallenger-v3.pro CONFIG+=debug CONFIG-=release
echo "Building for Native..."
make -j16

echo "Preparing datapack..."
#zip -r datapack.zip datapack

# Bug from QT5.15 builder
rm -r dist
mkdir -p dist/libs/
cd dist/libs/
mkdir armeabi-v7a
mkdir x86
mkdir arm64-v8a
mkdir x86_64
cd ../../
cp libcatchchallenger_arm64-v8a.so dist/libs/arm64-v8a
cp libcatchchallenger_armeabi-v7a.so dist/libs/armeabi-v7a
cp libcatchchallenger_x86.so dist/libs/x86
cp libcatchchallenger_x86_64.so dist/libs/x86_64

echo "Building for Android..."
androiddeployqt --aab --input android-catchchallenger-deployment-settings.json --output dist/ --deployment bundled --gradle
