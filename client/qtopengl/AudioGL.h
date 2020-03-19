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

    std::string startAmbiance(const std::string &soundPath);
};
#endif

#endif
