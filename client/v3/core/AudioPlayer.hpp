// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_AUDIOPLAYER_HPP_
#define CLIENT_QTOPENGL_CORE_AUDIOPLAYER_HPP_

#ifndef CATCHCHALLENGER_NOAUDIO
  #include <QAudioOutput>
  #include <QList>
  #include <QString>
  #include <string>
  #include <vector>

  #include "../../opusfile/opusfile.h"
  #include "../../libqtcatchchallenger/QInfiniteBuffer.hpp"

class AudioPlayer {
 public:
  static AudioPlayer *GetInstance();

  ~AudioPlayer();
  void SetVolume(const int &volume);
  QStringList OutputList();
  void SetPlayerVolume(QAudioOutput *const player);
  QAudioFormat Format() const;
  static bool DecodeOpus(const std::string &file_path, QByteArray &data);

  // if already playing ambiance then call stopCurrentAmbiance
  std::string StartAmbiance(const std::string &sound_path);
  void StopCurrentAmbiance();

 private:
  static AudioPlayer *instance_;
  AudioPlayer();
  std::vector<QAudioOutput *> player_list_;

 protected:
  QAudioFormat format_;
  int volume_;
  QAudioOutput *ambiance_player_;
  QInfiniteBuffer *ambiance_buffer_;
  QByteArray ambiance_data_;

  void AddPlayer(QAudioOutput *const player);
  void RemovePlayer(QAudioOutput *const player);
};
#endif

#endif  // CLIENT_QTOPENGL_CORE_AUDIOPLAYER_HPP_
