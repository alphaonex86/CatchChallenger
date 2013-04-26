/****************************************************************************
** Meta object code from reading C++ file 'MapController.h'
**
** Created: Thu Apr 11 17:51:25 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/interface/MapController.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapController[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      52,   15,   14,   14, 0x09,
     115,  105,   14,   14, 0x09,
     156,  153,   14,   14, 0x09,
     180,  175,   14,   14, 0x09,
     266,  233,  228,   14, 0x09,
     350,   14,   14,   14, 0x0a,
     367,   14,   14,   14, 0x0a,
     412,  386,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MapController[] = {
    "MapController\0\0mapId,x,y,plant_id,seconds_to_mature\0"
    "insert_plant(quint32,quint16,quint16,quint8,quint16)\0"
    "mapId,x,y\0remove_plant(quint32,quint16,quint16)\0"
    "ok\0seed_planted(bool)\0stat\0"
    "plant_collected(CatchChallenger::Plant_collect)\0"
    "bool\0direction,map,x,y,checkCollision\0"
    "canGoTo(CatchChallenger::Direction,CatchChallenger::Map,COORD_TYPE,COO"
    "RD_TYPE,bool)\0"
    "datapackParsed()\0reinject_signals()\0"
    "parsedMap,x,y,lookAt,skin\0"
    "loadBotOnTheMap(Map_full*,quint8,quint8,QString,QString)\0"
};

void MapController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapController *_t = static_cast<MapController *>(_o);
        switch (_id) {
        case 0: _t->insert_plant((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint16(*)>(_a[3])),(*reinterpret_cast< const quint8(*)>(_a[4])),(*reinterpret_cast< const quint16(*)>(_a[5]))); break;
        case 1: _t->remove_plant((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint16(*)>(_a[3]))); break;
        case 2: _t->seed_planted((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 3: _t->plant_collected((*reinterpret_cast< const CatchChallenger::Plant_collect(*)>(_a[1]))); break;
        case 4: { bool _r = _t->canGoTo((*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[1])),(*reinterpret_cast< CatchChallenger::Map(*)>(_a[2])),(*reinterpret_cast< COORD_TYPE(*)>(_a[3])),(*reinterpret_cast< COORD_TYPE(*)>(_a[4])),(*reinterpret_cast< const bool(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 5: _t->datapackParsed(); break;
        case 6: _t->reinject_signals(); break;
        case 7: _t->loadBotOnTheMap((*reinterpret_cast< Map_full*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< const QString(*)>(_a[5]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MapController::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MapController::staticMetaObject = {
    { &MapControllerMP::staticMetaObject, qt_meta_stringdata_MapController,
      qt_meta_data_MapController, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MapController::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MapController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MapController::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MapController))
        return static_cast<void*>(const_cast< MapController*>(this));
    return MapControllerMP::qt_metacast(_clname);
}

int MapController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapControllerMP::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
