/****************************************************************************
** Meta object code from reading C++ file 'MapController.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/interface/MapController.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapController[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      44,   15,   14,   14, 0x0a,
     154,  142,   14,   14, 0x0a,
     221,  218,   14,   14, 0x0a,
     261,  244,   14,   14, 0x0a,
     346,  321,   14,   14, 0x0a,
     414,   14,   14,   14, 0x0a,
     451,  438,   14,   14, 0x0a,
     532,  527,   14,   14, 0x0a,
     557,   14,   14,   14, 0x08,
     567,   14,   14,   14, 0x08,
     592,  588,  583,   14, 0x08,
     622,   14,   14,   14, 0x08,
     649,   14,   14,   14, 0x08,
     691,  678,  583,   14, 0x08,
     722,   14,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MapController[] = {
    "MapController\0\0player,mapName,x,y,direction\0"
    "insert_player(Pokecraft::Player_public_informations,QString,quint16,qu"
    "int16,Pokecraft::Direction)\0"
    "id,movement\0"
    "move_player(quint16,QList<QPair<quint8,Pokecraft::Direction> >)\0"
    "id\0remove_player(quint16)\0id,x,y,direction\0"
    "reinsert_player(quint16,quint8,quint8,Pokecraft::Direction)\0"
    "id,mapName,x,y,direction\0"
    "reinsert_player(quint16,QString,quint8,quint8,Pokecraft::Direction)\0"
    "dropAllPlayerOnTheMap()\0informations\0"
    "have_current_player_info(Pokecraft::Player_private_and_public_informat"
    "ions)\0"
    "path\0setDatapackPath(QString)\0botMove()\0"
    "botManagement()\0bool\0bot\0"
    "botMoveStepSlot(OtherPlayer*)\0"
    "loadPlayerFromCurrentMap()\0"
    "unloadPlayerFromCurrentMap()\0fileName,x,y\0"
    "loadMap(QString,quint8,quint8)\0"
    "haveTheDatapack()\0"
};

void MapController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapController *_t = static_cast<MapController *>(_o);
        switch (_id) {
        case 0: _t->insert_player((*reinterpret_cast< Pokecraft::Player_public_informations(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< quint16(*)>(_a[3])),(*reinterpret_cast< quint16(*)>(_a[4])),(*reinterpret_cast< Pokecraft::Direction(*)>(_a[5]))); break;
        case 1: _t->move_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< QList<QPair<quint8,Pokecraft::Direction> >(*)>(_a[2]))); break;
        case 2: _t->remove_player((*reinterpret_cast< quint16(*)>(_a[1]))); break;
        case 3: _t->reinsert_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< quint8(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< Pokecraft::Direction(*)>(_a[4]))); break;
        case 4: _t->reinsert_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint8(*)>(_a[4])),(*reinterpret_cast< Pokecraft::Direction(*)>(_a[5]))); break;
        case 5: _t->dropAllPlayerOnTheMap(); break;
        case 6: _t->have_current_player_info((*reinterpret_cast< Pokecraft::Player_private_and_public_informations(*)>(_a[1]))); break;
        case 7: _t->setDatapackPath((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->botMove(); break;
        case 9: _t->botManagement(); break;
        case 10: { bool _r = _t->botMoveStepSlot((*reinterpret_cast< OtherPlayer*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 11: _t->loadPlayerFromCurrentMap(); break;
        case 12: _t->unloadPlayerFromCurrentMap(); break;
        case 13: { bool _r = _t->loadMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 14: _t->haveTheDatapack(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MapController::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MapController::staticMetaObject = {
    { &MapVisualiserPlayer::staticMetaObject, qt_meta_stringdata_MapController,
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
    return MapVisualiserPlayer::qt_metacast(_clname);
}

int MapController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapVisualiserPlayer::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
