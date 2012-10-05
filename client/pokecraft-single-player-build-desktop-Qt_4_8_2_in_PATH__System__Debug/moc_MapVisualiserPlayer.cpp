/****************************************************************************
** Meta object code from reading C++ file 'MapVisualiserPlayer.h'
**
** Created: Fri Oct 5 13:40:08 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/render/MapVisualiserPlayer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapVisualiserPlayer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapVisualiserPlayer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      35,   21,   20,   20, 0x05,

 // slots: signature, parameters, type, tag, flags
      79,   20,   20,   20, 0x08,
      95,   20,   20,   20, 0x08,
     110,   20,   20,   20, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MapVisualiserPlayer[] = {
    "MapVisualiserPlayer\0\0the_direction\0"
    "send_player_direction(Pokecraft::Direction)\0"
    "keyPressParse()\0moveStepSlot()\0"
    "transformLookToMove()\0"
};

void MapVisualiserPlayer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapVisualiserPlayer *_t = static_cast<MapVisualiserPlayer *>(_o);
        switch (_id) {
        case 0: _t->send_player_direction((*reinterpret_cast< const Pokecraft::Direction(*)>(_a[1]))); break;
        case 1: _t->keyPressParse(); break;
        case 2: _t->moveStepSlot(); break;
        case 3: _t->transformLookToMove(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MapVisualiserPlayer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MapVisualiserPlayer::staticMetaObject = {
    { &MapVisualiser::staticMetaObject, qt_meta_stringdata_MapVisualiserPlayer,
      qt_meta_data_MapVisualiserPlayer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MapVisualiserPlayer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MapVisualiserPlayer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MapVisualiserPlayer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MapVisualiserPlayer))
        return static_cast<void*>(const_cast< MapVisualiserPlayer*>(this));
    return MapVisualiser::qt_metacast(_clname);
}

int MapVisualiserPlayer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapVisualiser::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void MapVisualiserPlayer::send_player_direction(const Pokecraft::Direction & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
