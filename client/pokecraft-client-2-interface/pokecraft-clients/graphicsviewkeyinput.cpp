#include "graphicsviewkeyinput.h"

#ifndef QT_NO_OPENGL
#define USE_OPENGL
#endif

#ifndef USE_OPENGL
#include <QGLWidget>
#endif

graphicsviewkeyinput::graphicsviewkeyinput(QWidget * parent)
	: QGraphicsView(parent)
{
        setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
        setInteractive(false);
#ifndef USE_OPENGL
        if (QGLFormat::hasOpenGL()) {
            if (!qobject_cast<QGLWidget*>(viewport())) {
                QGLFormat format = QGLFormat::defaultFormat();
                format.setDepth(false); // No need for a depth buffer
                format.setSampleBuffers(true); // Enable anti-aliasing
                setViewport(new QGLWidget(format));
            }
        }
        QWidget *v = viewport();
        v->setAttribute(Qt::WA_StaticContents);
        v->setMouseTracking(true);
#endif
}

graphicsviewkeyinput::~graphicsviewkeyinput()
{
	qWarning() << "Graphics view deleted";
}
