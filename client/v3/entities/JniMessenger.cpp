// Copyright 2021 CatchChallenger
#include "JniMessenger.hpp"

#include <QAndroidJniObject>
#include <iostream>

QString JniMessenger::DecompressDatapack() {
    QAndroidJniObject response = QAndroidJniObject::callStaticObjectMethod<jstring>(
        "org/catchchallenger/client/JniMessenger", "decompressDatapack");
    return response.toString();
}
