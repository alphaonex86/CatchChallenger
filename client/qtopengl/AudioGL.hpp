#ifndef AudioGL_H
#define AudioGL_H

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../libqtcatchchallenger/Audio.hpp"

class AudioGL : public QObject, public Audio
{
    Q_OBJECT
public:
    AudioGL();
    ~AudioGL();
public slots:
    std::string startAmbiance(const std::string &soundPath) override;
    void stopCurrentAmbiance() override;
};
#endif

#endif
