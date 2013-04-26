/****************************************************************************
** Meta object code from reading C++ file 'MapControllerMP.h'
**
** Created: Thu Apr 11 17:51:25 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/interface/MapControllerMP.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapControllerMP.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapControllerMP[] = {

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
      44,   17,   16,   16, 0x0a,
     166,  154,   16,   16, 0x0a,
     239,  236,   16,   16, 0x0a,
     279,  262,   16,   16, 0x0a,
     368,  345,   16,   16, 0x0a,
     442,   16,   16,   16, 0x0a,
     486,  466,   16,   16, 0x0a,
     562,  549,   16,   16, 0x0a,
     649,  644,   16,   16, 0x0a,
     674,   16,   16,   16, 0x0a,
     691,   16,   16,   16, 0x0a,
     728,  715,  710,   16, 0x08,
     765,   16,   16,   16, 0x08,
     783,   16,   16,   16, 0x08,
     829,  809,   16,   16, 0x09,
     882,  870,   16,   16, 0x29,
     918,  870,   16,   16, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_MapControllerMP[] = {
    "MapControllerMP\0\0player,mapId,x,y,direction\0"
    "insert_player(CatchChallenger::Player_public_informations,quint32,quin"
    "t16,quint16,CatchChallenger::Direction)\0"
    "id,movement\0"
    "move_player(quint16,QList<QPair<quint8,CatchChallenger::Direction> >)\0"
    "id\0remove_player(quint16)\0id,x,y,direction\0"
    "reinsert_player(quint16,quint8,quint8,CatchChallenger::Direction)\0"
    "id,mapId,x,y,direction\0"
    "reinsert_player(quint16,quint32,quint8,quint8,CatchChallenger::Directi"
    "on)\0"
    "dropAllPlayerOnTheMap()\0mapId,x,y,direction\0"
    "teleportTo(quint32,quint16,quint16,CatchChallenger::Direction)\0"
    "informations\0"
    "have_current_player_info(CatchChallenger::Player_private_and_public_in"
    "formations)\0"
    "path\0setDatapackPath(QString)\0"
    "datapackParsed()\0reinject_signals()\0"
    "bool\0fileName,x,y\0"
    "loadPlayerMap(QString,quint8,quint8)\0"
    "removeUnusedMap()\0moveOtherPlayerStepSlot()\0"
    "otherPlayer,display\0"
    "loadOtherPlayerFromMap(OtherPlayer,bool)\0"
    "otherPlayer\0loadOtherPlayerFromMap(OtherPlayer)\0"
    "unloadOtherPlayerFromMap(OtherPlayer)\0"
};

void MapControllerMP::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapControllerMP *_t = static_cast<MapControllerMP *>(_o);
        switch (_id) {
        case 0: _t->insert_player((*reinterpret_cast< const CatchChallenger::Player_public_informations(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])),(*reinterpret_cast< const quint16(*)>(_a[3])),(*reinterpret_cast< const quint16(*)>(_a[4])),(*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[5]))); break;
        case 1: _t->move_player((*reinterpret_cast< const quint16(*)>(_a[1])),(*reinterpret_cast< const QList<QPair<quint8,CatchChallenger::Direction> >(*)>(_a[2]))); break;
        case 2: _t->remove_player((*reinterpret_cast< const quint16(*)>(_a[1]))); break;
        case 3: _t->reinsert_player((*reinterpret_cast< const quint16(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[4]))); break;
        case 4: _t->reinsert_player((*reinterpret_cast< const quint16(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const quint8(*)>(_a[4])),(*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[5]))); break;
        case 5: _t->dropAllPlayerOnTheMap(); break;
        case 6: _t->teleportTo((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint16(*)>(_a[3])),(*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[4]))); break;
        case 7: _t->have_current_player_info((*reinterpret_cast< const CatchChallenger::Player_private_and_public_informations(*)>(_a[1]))); break;
        case 8: _t->setDatapackPath((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->datapackParsed(); break;
        case 10: _t->reinject_signals(); break;
        case 11: { bool _r = _t->loadPlayerMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 12: _t->removeUnusedMap(); break;
        case 13: _t->moveOtherPlayerStepSlot(); break;
        case 14: _t->loadOtherPlayerFromMap((*reinterpret_cast< OtherPlayer(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 15: _t->loadOtherPlayerFromMap((*reinterpret_cast< OtherPlayer(*)>(_a[1]))); break;
        case 16: _t->unloadOtherPlayerFromMap((*reinterpret_cast< OtherPlayer(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MapControllerMP::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MapControllerMP::staticMetaObject = {
    { &MapVisualiserPlayerWithFight::staticMetaObject, qt_meta_stringdata_MapControllerMP,
      qt_meta_data_MapControllerMP, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MapControllerMP::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MapControllerMP::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MapControllerMP::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MapControllerMP))
        return static_cast<void*>(const_cast< MapControllerMP*>(this));
    return MapVisualiserPlayerWithFight::qt_metacast(_clname);
}

int MapControllerMP::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapVisualiserPlayerWithFight::qt_metacall(_c, _id, _a);
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
