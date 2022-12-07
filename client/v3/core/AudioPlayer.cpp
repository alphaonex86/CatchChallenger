// Copyright 2021 CatchChallenger
#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioPlayer.hpp"

#include <QAudioDeviceInfo>
#include <QCoreApplication>
#include <iostream>

#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/base/cpp11addition.hpp"
#include "../../libqtcatchchallenger/PlatformMacro.hpp"
#include "AssetsLoader.hpp"

AudioPlayer *AudioPlayer::instance_ = nullptr;

AudioPlayer::AudioPlayer() {
  // init audio here
  format_.setSampleRate(48000);
  format_.setChannelCount(2);
  format_.setSampleSize(16);
  format_.setCodec("audio/pcm");
  format_.setByteOrder(QAudioFormat::LittleEndian);
  format_.setSampleType(QAudioFormat::SignedInt);

  QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
  if (!info.isFormatSupported(format_)) {
    std::cerr << "raw audio format not supported by backend, cannot play audio."
              << std::endl;
    return;
  }

  volume_ = 100;
  ambiance_player_ = nullptr;
  ambiance_buffer_ = nullptr;
}

AudioPlayer::~AudioPlayer() { StopCurrentAmbiance(); }

AudioPlayer *AudioPlayer::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new AudioPlayer();
  }
  return instance_;
}

QAudioFormat AudioPlayer::Format() const { return format_; }

void AudioPlayer::SetVolume(const int &volume) {
  std::cout << "Audio volume set to: " << volume << std::endl;
  unsigned int index = 0;
  while (index < player_list_.size()) {
    player_list_.at(index)->setVolume((qreal)volume / 100);
    index++;
  }
  volume_ = volume;
}

void AudioPlayer::AddPlayer(QAudioOutput *const player) {
  if (vectorcontainsAtLeastOne(player_list_, player)) return;
  player_list_.push_back(player);
  player->setVolume((qreal)volume_ / 100);
}

void AudioPlayer::RemovePlayer(QAudioOutput *const player) {
  vectorremoveOne(player_list_, player);
}

void AudioPlayer::SetPlayerVolume(QAudioOutput *const player) {
  player->setVolume((qreal)volume_ / 100);
}

QStringList AudioPlayer::OutputList() {
  QStringList outputs;
  return outputs;
}

bool AudioPlayer::DecodeOpus(const std::string &filePath, QByteArray &data) {
  QBuffer buffer(&data);
  buffer.open(QBuffer::ReadWrite);

  int ret;
  OggOpusFile *of = op_open_file(filePath.c_str(), &ret);
  if (of == NULL) {
    fprintf(stderr, "Audio Failed to open file '%s': %i\n", filePath.c_str(),
            ret);
    return false;
  }
  ogg_int64_t pcm_offset;
  ogg_int64_t nsamples;
  nsamples = 0;
  pcm_offset = op_pcm_tell(of);
  if (pcm_offset != 0)
    fprintf(stderr, "Non-zero starting PCM offset: %li\n", (long)pcm_offset);
  for (;;) {
    ogg_int64_t next_pcm_offset;
    opus_int16 pcm[120 * 48 * 2];
    unsigned char out[120 * 48 * 2 * 2];
    int si;
    ret = op_read_stereo(of, pcm, sizeof(pcm) / sizeof(*pcm));
    if (ret == OP_HOLE) {
      fprintf(stderr, "\nHole detected! Corrupt file segment?\n");
      continue;
    } else if (ret < 0) {
      fprintf(stderr, "\nError decoding '%s': %i\n", "file.opus", ret);
      ret = EXIT_FAILURE;
      break;
    }
    next_pcm_offset = op_pcm_tell(of);
    pcm_offset = next_pcm_offset;
    if (ret <= 0) {
      ret = EXIT_SUCCESS;
      break;
    }
    for (si = 0; si < 2 * ret;
         si++) {  /// Ensure the data is little-endian before writing it out.
      out[2 * si + 0] = (unsigned char)(pcm[si] & 0xFF);
      out[2 * si + 1] = (unsigned char)(pcm[si] >> 8 & 0xFF);
    }
    buffer.write(reinterpret_cast<char *>(out), sizeof(*out) * 4 * ret);
    nsamples += ret;
  }
  op_free(of);

  buffer.seek(0);
  return ret == EXIT_SUCCESS;
}

// if already playing ambiance then call stopCurrentAmbiance
std::string AudioPlayer::StartAmbiance(const std::string &soundPath) {
  /*this is blocking and take 5s:
     if(QAudioDeviceInfo::availableDevices(QAudioPlayer::AudioOutput).isEmpty())
          return "No audio device found!";*/

  last_sound_path = soundPath;
  const QString QtsoundPath = QString::fromStdString(soundPath);
  if (!QtsoundPath.startsWith(":/")) {
    StopCurrentAmbiance();

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format_))
      return "raw audio format not supported by backend, cannot play audio.";

    ambiance_player_ = new QAudioOutput(Format());
    // Create a new Media
    if (ambiance_player_ != nullptr) {
      // decode file
      if (DecodeOpus(soundPath, ambiance_data_)) {
        ambiance_buffer_ = new QInfiniteBuffer(&ambiance_data_);
        ambiance_buffer_->open(QBuffer::ReadOnly);
        ambiance_buffer_->seek(0);
        ambiance_player_->start(ambiance_buffer_);

        AddPlayer(ambiance_player_);
        return std::string();
      } else {
        delete ambiance_player_;
        ambiance_player_ = nullptr;
        ambiance_data_.clear();
        return "DecodeOpus failed";
      }
    }
    return "ambiance_player_==nullptr";
  }
  StopCurrentAmbiance();

  QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
  if (!info.isFormatSupported(format_))
    return std::to_string((int)format_.byteOrder()) + "," +
           std::to_string((int)format_.channelCount()) + "," +
           format_.codec().toStdString() + "," +
           std::to_string((int)format_.isValid()) + "," +
           std::to_string((int)format_.sampleRate()) + "," +
           std::to_string((int)format_.sampleType()) + "," + " " +
           QAudioDeviceInfo::defaultOutputDevice().deviceName().toStdString();
  ambiance_player_ = new QAudioOutput(Format());
  // Create a new Media
  if (ambiance_player_ != nullptr &&
      AssetsLoader::GetInstance()->musics.contains(QtsoundPath)) {
    ambiance_data_ = *AssetsLoader::GetInstance()->musics.value(QtsoundPath);
    ambiance_buffer_ = new QInfiniteBuffer(&ambiance_data_);
    ambiance_buffer_->open(QBuffer::ReadOnly);
    ambiance_buffer_->seek(0);
    ambiance_player_->start(ambiance_buffer_);
    AddPlayer(ambiance_player_);
    return std::string();
  }
  if (ambiance_player_ != nullptr)
    return "QAudioOutput nullptr";
  else
    return "!AssetsLoader::GetInstance()->musics.contains(QtsoundPath)";
}

void AudioPlayer::StopCurrentAmbiance() {
  if (ambiance_player_ != nullptr) {
    RemovePlayer(ambiance_player_);
    ambiance_player_->stop();
    delete ambiance_player_;
    ambiance_player_ = nullptr;
    ambiance_data_.clear();
  }
}

void AudioPlayer::StartLastAmbiance() {
  StartAmbiance(last_sound_path);
}
#endif
