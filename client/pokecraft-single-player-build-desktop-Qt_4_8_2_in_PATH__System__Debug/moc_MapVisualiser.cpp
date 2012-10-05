/****************************************************************************
** Meta object code from reading C++ file 'MapVisualiser.h'
**
** Created: Fri Oct 5 13:40:08 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/render/MapVisualiser.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapVisualiser.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapVisualiser[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      32,   23,   15,   14, 0x0a,
      54,   14,   14,   14, 0x0a,
     100,   71,   14,   14, 0x08,
     170,  149,   14,   14, 0x28,
     225,  212,   14,   14, 0x28,
     271,  260,   14,   14, 0x28,
     299,   23,   14,   14, 0x28,
     326,  320,   14,   14, 0x08,
     351,   14,   14,   14, 0x08,
     363,   14,   14,   14, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_MapVisualiser[] = {
    "MapVisualiser\0\0QString\0fileName\0"
    "loadOtherMap(QString)\0loadCurrentMap()\0"
    "fileName,x,y,x_pixel,y_pixel\0"
    "loadNearMap(QString,qint32,qint32,qint32,qint32)\0"
    "fileName,x,y,x_pixel\0"
    "loadNearMap(QString,qint32,qint32,qint32)\0"
    "fileName,x,y\0loadNearMap(QString,qint32,qint32)\0"
    "fileName,x\0loadNearMap(QString,qint32)\0"
    "loadNearMap(QString)\0event\0"
    "paintEvent(QPaintEvent*)\0updateFPS()\0"
    "render()\0"
};

void MapVisualiser::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapVisualiser *_t = static_cast<MapVisualiser *>(_o);
        switch (_id) {
        case 0: { QString _r = _t->loadOtherMap((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 1: _t->loadCurrentMap(); break;
        case 2: _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const qint32(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3])),(*reinterpret_cast< const qint32(*)>(_a[4])),(*reinterpret_cast< const qint32(*)>(_a[5]))); break;
        case 3: _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const qint32(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3])),(*reinterpret_cast< const qint32(*)>(_a[4]))); break;
        case 4: _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const qint32(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3]))); break;
        case 5: _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const qint32(*)>(_a[2]))); break;
        case 6: _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->paintEvent((*reinterpret_cast< QPaintEvent*(*)>(_a[1]))); break;
        case 8: _t->updateFPS(); break;
        case 9: _t->render(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MapVisualiser::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MapVisualiser::staticMetaObject = {
    { &QGraphicsView::staticMetaObject, qt_meta_stringdata_MapVisualiser,
      qt_meta_data_MapVisualiser, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MapVisualiser::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MapVisualiser::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MapVisualiser::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MapVisualiser))
        return static_cast<void*>(const_cast< MapVisualiser*>(this));
    return QGraphicsView::qt_metacast(_clname);
}

int MapVisualiser::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
