// Copyright 2022 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_JNIMESSENGER_HPP_
#define CLIENT_V3_ENTITIES_JNIMESSENGER_HPP_

#include <QObject>

class JniMessenger : public QObject {
  Q_OBJECT
 public:
  explicit JniMessenger(QObject *parent = nullptr);

  Q_INVOKABLE static QString DecompressDatapack();
};
#endif  // CLIENT_V3_ENTITIES_JNIMESSENGER_HPP_
