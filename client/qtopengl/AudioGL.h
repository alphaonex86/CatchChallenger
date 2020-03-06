#ifndef AudioGL_H
#define AudioGL_H

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../qt/Audio.h"

class AudioGL : public Audio
{
public:
    AudioGL();
    ~AudioGL();
    static AudioGL audioGL;

    bool startAmbiance(const std::string &soundPath);
};
#endif

#endif
