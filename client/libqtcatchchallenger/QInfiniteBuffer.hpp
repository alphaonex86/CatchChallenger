#ifndef QINFINITEBUFFER_H
#define QINFINITEBUFFER_H

#include <QBuffer>

#include "../../general/base/lib.h"

class DLL_PUBLIC QInfiniteBuffer : public QBuffer
{
public:
    QInfiniteBuffer(QByteArray *buf, QObject *parent = nullptr);
    QInfiniteBuffer(QObject *parent = nullptr);
    qint64 readData(char *output, qint64 maxlen) override;
private:
    qint64 m_pos;
};

#endif // QINFINITEBUFFER_H
