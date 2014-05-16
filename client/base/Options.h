#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>
#include <QSettings>
#include <QStringList>

class Options : public QObject
{
    Q_OBJECT
private:
    explicit Options();
    ~Options();
public:
    static Options options;
    quint16 getFPS() const;
    bool getLimitedFPS() const;
    quint16 getFinalFPS() const;
    bool getForcedZoom() const;
    quint8 getAudioVolume() const;
    QString getLanguage() const;//the main code
    int getIndexDevice() const;
signals:
    void newFPS(const quint16 &fps);
    void newLimitedFPS(const bool &fpsLimited);
    void newFinalFPS(const quint16 &fps);
    void newZoomEnabled(const quint8 &zoom/*0 is no forced*/);
    void newAudioVolume(const quint8 &audioVolume);
    void newLanguage(const QString &language);//the main code
    void newAudioDevice(const int &indexDevice);//the main code
public slots:
    void setFPS(const quint16 &fps);
    void setLimitedFPS(const bool &limitedFPS);
    void setForcedZoom(const quint8 &zoom/*0 is no forced*/);
    void setAudioVolume(const quint8 &audioVolume);
    void setLanguage(const QString &language);//the main code
    void setDeviceIndex(const int &indexDevice);

    void setAudioDeviceList(const QStringList &devices);
private:
    QSettings *settings;
    quint16 fps;
    bool limitedFPS;
    quint8 zoom;//0 is no forced
    quint8 audioVolume;
    QString language;
    int indexDevice;
    QStringList devices;
};

#endif // OPTIONS_H
