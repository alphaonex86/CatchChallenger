#include <QAudioDeviceInfo>
#include <iostream>

int main(int argc, char *argv[])
{
    const QList<QAudioDeviceInfo> list=QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    if(list.isEmpty())
    {
        std::cerr << "list.isEmpty()" << std::endl;
        return EXIT_FAILURE;
    }
    std::cerr << "list ok" << std::endl;
    return 0;
}
