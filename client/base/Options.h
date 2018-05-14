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
    uint16_t getFPS() const;
    bool getLimitedFPS() const;
    uint16_t getFinalFPS() const;
    uint8_t getForcedZoom() const;
    uint8_t getAudioVolume() const;
    std::string getLanguage() const;//the main code
    int getIndexDevice() const;
signals:
    void newFPS(const uint16_t &fps);
    void newLimitedFPS(const bool &fpsLimited);
    void newFinalFPS(const uint16_t &fps);
    void newZoomEnabled(const uint8_t &zoom/*0 is no forced*/);
    void newAudioVolume(const uint8_t &audioVolume);
    void newLanguage(const std::string &language);//the main code
    void newAudioDevice(const int &indexDevice);//the main code
public slots:
    void setFPS(const uint16_t &fps);
    void setLimitedFPS(const bool &limitedFPS);
    void setForcedZoom(const uint8_t &zoom/*0 is no forced*/);
    void setAudioVolume(const uint8_t &audioVolume);
    void setLanguage(const std::string &language);//the main code
    void setDeviceIndex(const int &indexDevice);

    void setAudioDeviceList(const QStringList &devices);
private:
    QSettings *settings;
    uint16_t fps;
    bool limitedFPS;
    uint8_t zoom;//0 is no forced
    uint8_t audioVolume;
    std::string language;
    int indexDevice;
    std::vector<std::string> devices;
};

#endif // OPTIONS_H
