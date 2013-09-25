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
    QString getLanguage();//the main code
signals:
    void newFPS(const quint16 &fps);
    void newLimitedFPS(const bool &fpsLimited);
    void newFinalFPS(const quint16 &fps);
    void newZoomEnabled(const bool &zoom);
    void newAudioVolume(const quint8 &audioVolume);
    void newLanguage(const QString &language);//the main code
public slots:
    void setFPS(const quint16 &fps);
    void setLimitedFPS(const bool &limitedFPS);
    void setZoomEnabled(const bool &zoom);
    void setAudioVolume(const quint8 &audioVolume);
    void setLanguage(const QString &language);//the main code
private:
    QSettings *settings;
    quint16 fps;
    bool limitedFPS;
    bool zoom;
    quint8 audioVolume;
    QString language;
};

#endif // OPTIONS_H
