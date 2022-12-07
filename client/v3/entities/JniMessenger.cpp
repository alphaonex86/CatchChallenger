// Copyright 2021 CatchChallenger
#include "JniMessenger.hpp"

#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QStandardPaths>
#include <QtAndroid>
#include <iostream>

#include "../core/AudioPlayer.hpp"

static void onEventApplication(JNIEnv *env, jobject /*thiz*/, jstring value) {
    auto buffer = env->GetStringUTFChars(value, nullptr);
    std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
              << "asdasd " << buffer << std::endl;
    QString event(buffer);

    if (event == "resume") {
        AudioPlayer::GetInstance()->StartLastAmbiance();
    } else if (event == "pause") {
        AudioPlayer::GetInstance()->StopCurrentAmbiance();
    }
}

JniMessenger::JniMessenger(QObject *parent) : QObject(parent) {
    JNINativeMethod methods[]{{"onEventApplication", "(Ljava/lang/String;)V",
                               reinterpret_cast<void *>(onEventApplication)}};
    QAndroidJniObject javaClass("org/catchchallenger/client/JniMessenger");

    QAndroidJniEnvironment env;
    jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
    env->RegisterNatives(objectClass, methods,
                         sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(objectClass);
}

QString JniMessenger::DecompressDatapack() {
    QString path =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QAndroidJniObject javaPath = QAndroidJniObject::fromString(path);
    QAndroidJniObject::callStaticMethod<void>(
        "org/catchchallenger/client/JniMessenger", "decompressDatapack",
        "(Ljava/lang/String;)V", javaPath.object<jstring>());
    return path + "/datapack/";
}
