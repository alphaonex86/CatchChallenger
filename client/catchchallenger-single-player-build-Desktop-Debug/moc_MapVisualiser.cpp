/****************************************************************************
** Meta object code from reading C++ file 'MapVisualiser.h'
**
** Created: Fri Apr 26 14:18:58 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/render/MapVisualiser.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapVisualiser.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapVisualiser[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x09,
      43,   34,   26,   14, 0x0a,
      75,   65,   14,   14, 0x0a,
     135,  109,   14,   14, 0x0a,
     192,   34,   14,   14, 0x0a,
     239,  227,  213,   14, 0x0a,
     263,   14,   14,   14, 0x0a,
     285,  281,  213,   14, 0x08,
     370,  311,  213,   14, 0x08,
     475,  438,  213,   14, 0x28,
     558,  529,  213,   14, 0x28,
     626,  605,  213,   14, 0x28,
     685,  666,  213,   14, 0x28,
     735,  718,  213,   14, 0x28,
     767,  761,   14,   14, 0x08,
     792,   14,   14,   14, 0x08,
     804,   14,   14,   14, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_MapVisualiser[] = {
    "MapVisualiser\0\0resetAll()\0QString\0"
    "fileName\0loadOtherMap(QString)\0parsedMap\0"
    "loadOtherMapClientPart(Map_full*)\0"
    "parsedMap,x,y,lookAt,skin\0"
    "loadBotOnTheMap(Map_full*,quint8,quint8,QString,QString)\0"
    "loadBotFile(QString)\0QSet<QString>\0"
    "map,display\0loadMap(Map_full*,bool)\0"
    "removeUnusedMap()\0map\0loadTeleporter(Map_full*)\0"
    "fileName,display,x,y,x_pixel,y_pixel,previousLoadedNearMap\0"
    "loadNearMap(QString,bool,qint32,qint32,qint32,qint32,QSet<QString>)\0"
    "fileName,display,x,y,x_pixel,y_pixel\0"
    "loadNearMap(QString,bool,qint32,qint32,qint32,qint32)\0"
    "fileName,display,x,y,x_pixel\0"
    "loadNearMap(QString,bool,qint32,qint32,qint32)\0"
    "fileName,display,x,y\0"
    "loadNearMap(QString,bool,qint32,qint32)\0"
    "fileName,display,x\0loadNearMap(QString,bool,qint32)\0"
    "fileName,display\0loadNearMap(QString,bool)\0"
    "event\0paintEvent(QPaintEvent*)\0"
    "updateFPS()\0render()\0"
};

void MapVisualiser::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapVisualiser *_t = static_cast<MapVisualiser *>(_o);
        switch (_id) {
        case 0: _t->resetAll(); break;
        case 1: { QString _r = _t->loadOtherMap((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 2: _t->loadOtherMapClientPart((*reinterpret_cast< Map_full*(*)>(_a[1]))); break;
        case 3: _t->loadBotOnTheMap((*reinterpret_cast< Map_full*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< const QString(*)>(_a[5]))); break;
        case 4: _t->loadBotFile((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: { QSet<QString> _r = _t->loadMap((*reinterpret_cast< Map_full*(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 6: _t->removeUnusedMap(); break;
        case 7: { QSet<QString> _r = _t->loadTeleporter((*reinterpret_cast< Map_full*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 8: { QSet<QString> _r = _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3])),(*reinterpret_cast< const qint32(*)>(_a[4])),(*reinterpret_cast< const qint32(*)>(_a[5])),(*reinterpret_cast< const qint32(*)>(_a[6])),(*reinterpret_cast< const QSet<QString>(*)>(_a[7])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 9: { QSet<QString> _r = _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3])),(*reinterpret_cast< const qint32(*)>(_a[4])),(*reinterpret_cast< const qint32(*)>(_a[5])),(*reinterpret_cast< const qint32(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 10: { QSet<QString> _r = _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3])),(*reinterpret_cast< const qint32(*)>(_a[4])),(*reinterpret_cast< const qint32(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 11: { QSet<QString> _r = _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3])),(*reinterpret_cast< const qint32(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 12: { QSet<QString> _r = _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2])),(*reinterpret_cast< const qint32(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 13: { QSet<QString> _r = _t->loadNearMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QSet<QString>*>(_a[0]) = _r; }  break;
        case 14: _t->paintEvent((*reinterpret_cast< QPaintEvent*(*)>(_a[1]))); break;
        case 15: _t->updateFPS(); break;
        case 16: _t->render(); break;
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
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
