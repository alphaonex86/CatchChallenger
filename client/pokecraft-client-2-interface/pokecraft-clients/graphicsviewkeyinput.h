#ifndef GRAPHICSVIEWKEYINPUT_H
#define GRAPHICSVIEWKEYINPUT_H

#include <QGraphicsView>
#include <QKeyEvent>
#include <QObject>
#include <QDebug>

class graphicsviewkeyinput : public QGraphicsView
{
    Q_OBJECT
public:
	explicit graphicsviewkeyinput(QWidget * parent = 0);
    ~graphicsviewkeyinput();
signals:
    void keyPressed( QKeyEvent * event );
    void keyRelease( QKeyEvent * event );
protected:
    void keyPressEvent ( QKeyEvent * event ){ if(event->isAutoRepeat()) return;emit keyPressed(event);}
    void keyReleaseEvent ( QKeyEvent * event ){ if(event->isAutoRepeat()) return;emit keyRelease(event);}
    void wheelEvent( QWheelEvent * ){}
};

#endif // GRAPHICSVIEWKEYINPUT_H
