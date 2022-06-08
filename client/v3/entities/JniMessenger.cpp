// Copyright 2021 CatchChallenger
#include "JniMessenger.hpp"

#include <QAndroidJniObject>
#include <QStandardPaths>
#include <iostream>

QString JniMessenger::DecompressDatapack() {
    QString path =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QAndroidJniObject javaPath = QAndroidJniObject::fromString(path);
    QAndroidJniObject::callStaticMethod<void>(
            "org/catchchallenger/client/JniMessenger", "decompressDatapack",
            "(Ljava/lang/String;)V", javaPath.object<jstring>());
    return path + "/datapack/";
}
