#ifndef Audio_H
#define Audio_H

#ifndef CATCHCHALLENGER_NOAUDIO
#include <vlc/vlc.h>

#include <QString>
#include <QList>

class Audio
{
public:
    Audio();
    ~Audio();
    static Audio audio;
    libvlc_instance_t *vlcInstance;
    void setVolume(const int &volume);
    QStringList output_list();
    void addPlayer(libvlc_media_player_t * const player);
    void removePlayer(libvlc_media_player_t * const player);
    void setPlayerVolume(libvlc_media_player_t * const player);
private:
    QList<libvlc_media_player_t *> playerList;
    int volume;
};
#endif

#endif // RSSNEWS_H
