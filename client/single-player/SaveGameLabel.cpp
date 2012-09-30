#include "SaveGameLabel.h"
#include <QDebug>

SaveGameLabel::SaveGameLabel()
{
}

void SaveGameLabel::mousePressEvent(QMouseEvent *)
{
    emit clicked();
}

void SaveGameLabel::mouseReleaseEvent(QMouseEvent *)
{
}
