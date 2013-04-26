/****************************************************************************
** Meta object code from reading C++ file 'MapVisualiserPlayerWithFight.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../fight/interface/MapVisualiserPlayerWithFight.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapVisualiserPlayerWithFight.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapVisualiserPlayerWithFight[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      30,   29,   29,   29, 0x09,
      51,   29,   46,   29, 0x09,
     105,   72,   46,   29, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_MapVisualiserPlayerWithFight[] = {
    "MapVisualiserPlayerWithFight\0\0"
    "keyPressParse()\0bool\0haveStopTileAction()\0"
    "direction,map,x,y,checkCollision\0"
    "canGoTo(CatchChallenger::Direction,CatchChallenger::Map,COORD_TYPE,COO"
    "RD_TYPE,bool)\0"
};

void MapVisualiserPlayerWithFight::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapVisualiserPlayerWithFight *_t = static_cast<MapVisualiserPlayerWithFight *>(_o);
        switch (_id) {
        case 0: _t->keyPressParse(); break;
        case 1: { bool _r = _t->haveStopTileAction();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 2: { bool _r = _t->canGoTo((*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[1])),(*reinterpret_cast< CatchChallenger::Map(*)>(_a[2])),(*reinterpret_cast< COORD_TYPE(*)>(_a[3])),(*reinterpret_cast< COORD_TYPE(*)>(_a[4])),(*reinterpret_cast< const bool(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MapVisualiserPlayerWithFight::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MapVisualiserPlayerWithFight::staticMetaObject = {
    { &MapVisualiserPlayer::staticMetaObject, qt_meta_stringdata_MapVisualiserPlayerWithFight,
      qt_meta_data_MapVisualiserPlayerWithFight, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MapVisualiserPlayerWithFight::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MapVisualiserPlayerWithFight::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MapVisualiserPlayerWithFight::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MapVisualiserPlayerWithFight))
        return static_cast<void*>(const_cast< MapVisualiserPlayerWithFight*>(this));
    return MapVisualiserPlayer::qt_metacast(_clname);
}

int MapVisualiserPlayerWithFight::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapVisualiserPlayer::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
