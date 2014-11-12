#ifndef Audio_H
#define Audio_H

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
    void setVolume(int volume);
    QStringList output_list();
    void addPlayer(libvlc_media_player_t *player);
    void removePlayer(libvlc_media_player_t *player);
private:
    QList<libvlc_media_player_t *> playerList;
    int volume;
};

#endif // RSSNEWS_H
