#ifndef Audio_H
#define Audio_H

#include <vlc/vlc.h>

class Audio
{
public:
    Audio();
    ~Audio();
    static Audio audio;
    libvlc_instance_t *vlcInstance;
};

#endif // RSSNEWS_H
