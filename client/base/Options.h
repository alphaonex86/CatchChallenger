#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>
#include <QSettings>

class Options : public QObject
{
    Q_OBJECT
private:
    explicit Options();
    ~Options();
public:
    static Options options;
    quint16 getFPS();
    bool getLimitedFPS();
    quint16 getFinalFPS();
    bool getZoomEnabled();
    quint8 getAudioVolume();
signals:
    void newFPS(const quint16 &fps);
    void newLimitedFPS(const bool &fpsLimited);
    void newFinalFPS(const quint16 &fps);
    void newZoomEnabled(const bool &zoom);
    void newAudioVolume(const quint8 &audioVolume);
public slots:
    void setFPS(const quint16 &fps);
    void setLimitedFPS(const bool &limitedFPS);
    void setZoomEnabled(const bool &zoom);
    void setAudioVolume(const quint8 &audioVolume);
private:
    QSettings *settings;
    quint16 fps;
    bool limitedFPS;
    bool zoom;
    quint8 audioVolume;
};

#endif // OPTIONS_H
