/****************************************************************************
** Meta object code from reading C++ file 'ClientLocalBroadcast.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientLocalBroadcast.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientLocalBroadcast.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__ClientLocalBroadcast[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      56,   39,   38,   38, 0x05,
      94,   85,   38,   38, 0x05,
     124,  110,   38,   38, 0x05,
     163,  158,   38,   38, 0x25,
     198,  189,   38,   38, 0x05,

 // slots: signature, parameters, type, tag, flags
     220,  215,   38,   38, 0x0a,
     272,  247,   38,   38, 0x0a,
     348,  338,  333,   38, 0x0a,
     390,  370,   38,   38, 0x0a,
     437,  370,   38,   38, 0x0a,
     489,   38,   38,   38, 0x0a,
     523,  505,   38,   38, 0x0a,
     557,  548,   38,   38, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__ClientLocalBroadcast[] = {
    "CatchChallenger::ClientLocalBroadcast\0"
    "\0queryNumber,data\0postReply(quint8,QByteArray)\0"
    "plant_id\0useSeed(quint8)\0item,quantity\0"
    "addObjectAndSend(quint32,quint32)\0"
    "item\0addObjectAndSend(quint32)\0sqlQuery\0"
    "dbQuery(QString)\0text\0sendLocalChatText(QString)\0"
    "text,sender_informations\0"
    "receiveChatText(QString,const Player_internal_informations*)\0"
    "bool\0direction\0singleMove(Direction)\0"
    "map,x,y,orientation\0"
    "put_on_the_map(Map*,quint8,quint8,Orientation)\0"
    "teleportValidatedTo(Map*,quint8,quint8,Orientation)\0"
    "seedValidated()\0query_id,plant_id\0"
    "plantSeed(quint8,quint8)\0query_id\0"
    "collectPlant(quint8)\0"
};

void CatchChallenger::ClientLocalBroadcast::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientLocalBroadcast *_t = static_cast<ClientLocalBroadcast *>(_o);
        switch (_id) {
        case 0: _t->postReply((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 1: _t->useSeed((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 2: _t->addObjectAndSend((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 3: _t->addObjectAndSend((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 4: _t->dbQuery((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->sendLocalChatText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->receiveChatText((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const Player_internal_informations*(*)>(_a[2]))); break;
        case 7: { bool _r = _t->singleMove((*reinterpret_cast< const Direction(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 8: _t->put_on_the_map((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 9: _t->teleportValidatedTo((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 10: _t->seedValidated(); break;
        case 11: _t->plantSeed((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2]))); break;
        case 12: _t->collectPlant((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::ClientLocalBroadcast::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::ClientLocalBroadcast::staticMetaObject = {
    { &MapBasicMove::staticMetaObject, qt_meta_stringdata_CatchChallenger__ClientLocalBroadcast,
      qt_meta_data_CatchChallenger__ClientLocalBroadcast, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::ClientLocalBroadcast::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::ClientLocalBroadcast::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::ClientLocalBroadcast::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__ClientLocalBroadcast))
        return static_cast<void*>(const_cast< ClientLocalBroadcast*>(this));
    return MapBasicMove::qt_metacast(_clname);
}

int CatchChallenger::ClientLocalBroadcast::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapBasicMove::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::ClientLocalBroadcast::postReply(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::ClientLocalBroadcast::useSeed(const quint8 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CatchChallenger::ClientLocalBroadcast::addObjectAndSend(const quint32 & _t1, const quint32 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void CatchChallenger::ClientLocalBroadcast::dbQuery(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_END_MOC_NAMESPACE
