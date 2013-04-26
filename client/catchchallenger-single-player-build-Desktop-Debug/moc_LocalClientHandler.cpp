/****************************************************************************
** Meta object code from reading C++ file 'LocalClientHandler.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/LocalClientHandler.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LocalClientHandler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__LocalClientHandler[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      48,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: signature, parameters, type, tag, flags
      46,   37,   36,   36, 0x05,
      63,   36,   36,   36, 0x05,
      96,   81,   36,   36, 0x05,
     133,  128,   36,   36, 0x25,
     177,  160,   36,   36, 0x05,
     211,  206,   36,   36, 0x05,
     240,   36,   36,   36, 0x05,
     276,  256,   36,   36, 0x05,

 // slots: signature, parameters, type, tag, flags
     319,  256,   36,   36, 0x0a,
     407,  379,  374,   36, 0x0a,
     459,  449,  439,   36, 0x0a,
     492,  481,   36,   36, 0x0a,
     529,  520,   36,   36, 0x0a,
     564,  545,   36,   36, 0x0a,
     604,  590,   36,   36, 0x0a,
     643,  638,   36,   36, 0x2a,
     669,  590,   36,   36, 0x0a,
     696,  638,   36,   36, 0x2a,
     715,  638,   36,   36, 0x0a,
     752,  590,  744,   36, 0x0a,
     782,  638,  744,   36, 0x2a,
     804,  590,   36,   36, 0x0a,
     838,  638,   36,   36, 0x2a,
     864,  638,  744,   36, 0x0a,
     903,  888,   36,   36, 0x0a,
     930,  925,   36,   36, 0x2a,
     947,  925,   36,   36, 0x0a,
     985,  967,   36,   36, 0x0a,
    1037, 1021,   36,   36, 0x0a,
    1084, 1068,   36,   36, 0x0a,
    1110,  256,   36,   36, 0x0a,
    1160,  256,   36,   36, 0x0a,
    1228, 1212,   36,   36, 0x0a,
    1297, 1257,   36,   36, 0x0a,
    1348, 1257,   36,   36, 0x0a,
    1400,   36,   36,   36, 0x0a,
    1420, 1412,  374,   36, 0x0a,
    1474, 1468,   36,   36, 0x0a,
    1508, 1492,  374,   36, 0x0a,
    1536, 1492,  374,   36, 0x0a,
    1572,   36,   36,   36, 0x0a,
    1588,   36,   36,   36, 0x0a,
    1604,   36,   36,   36, 0x0a,
    1620,  925,   36,   36, 0x0a,
    1647,  590,   36,   36, 0x0a,
    1694, 1684,   36,   36, 0x0a,
    1724,   36,   36,   36, 0x08,
    1736,   36,   36,   36, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__LocalClientHandler[] = {
    "CatchChallenger::LocalClientHandler\0"
    "\0sqlQuery\0dbQuery(QString)\0askRandomNumber()\0"
    "text,important\0receiveSystemText(QString,bool)\0"
    "text\0receiveSystemText(QString)\0"
    "queryNumber,data\0postReply(quint8,QByteArray)\0"
    "data\0sendTradeRequest(QByteArray)\0"
    "seedValidated()\0map,x,y,orientation\0"
    "teleportTo(Map*,quint8,quint8,Orientation)\0"
    "put_on_the_map(Map*,COORD_TYPE,COORD_TYPE,Orientation)\0"
    "bool\0previousMovedUnit,direction\0"
    "moveThePlayer(quint8,Direction)\0"
    "Direction\0direction\0lookToMove(Direction)\0"
    "randomData\0newRandomNumber(QByteArray)\0"
    "plant_id\0useSeed(quint8)\0query_id,recipe_id\0"
    "useRecipe(quint8,quint32)\0item,quantity\0"
    "addObjectAndSend(quint32,quint32)\0"
    "item\0addObjectAndSend(quint32)\0"
    "addObject(quint32,quint32)\0"
    "addObject(quint32)\0saveObjectRetention(quint32)\0"
    "quint32\0removeObject(quint32,quint32)\0"
    "removeObject(quint32)\0"
    "sendRemoveObject(quint32,quint32)\0"
    "sendRemoveObject(quint32)\0"
    "objectQuantity(quint32)\0cash,forceSave\0"
    "addCash(quint64,bool)\0cash\0addCash(quint64)\0"
    "removeCash(quint64)\0command,extraText\0"
    "sendHandlerCommand(QString,QString)\0"
    "itemId,quantity\0destroyObject(quint32,quint32)\0"
    "query_id,itemId\0useObject(quint8,quint32)\0"
    "receiveTeleportTo(Map*,quint8,quint8,Orientation)\0"
    "teleportValidatedTo(Map*,quint8,quint8,Orientation)\0"
    "query_id,shopId\0getShopList(quint32,quint32)\0"
    "query_id,shopId,objectId,quantity,price\0"
    "buyObject(quint32,quint32,quint32,quint32,quint32)\0"
    "sellObject(quint32,quint32,quint32,quint32,quint32)\0"
    "tryEscape()\0map,x,y\0"
    "checkFightCollision(Map*,COORD_TYPE,COORD_TYPE)\0"
    "skill\0useSkill(quint32)\0monsterId,skill\0"
    "learnSkill(quint32,quint32)\0"
    "learnSkillInternal(quint32,quint32)\0"
    "tradeCanceled()\0tradeAccepted()\0"
    "tradeFinished()\0tradeAddTradeCash(quint64)\0"
    "tradeAddTradeObject(quint32,quint32)\0"
    "monsterId\0tradeAddTradeMonster(quint32)\0"
    "extraStop()\0savePosition()\0"
};

void CatchChallenger::LocalClientHandler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        LocalClientHandler *_t = static_cast<LocalClientHandler *>(_o);
        switch (_id) {
        case 0: _t->dbQuery((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->askRandomNumber(); break;
        case 2: _t->receiveSystemText((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 3: _t->receiveSystemText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->postReply((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 5: _t->sendTradeRequest((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 6: _t->seedValidated(); break;
        case 7: _t->teleportTo((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 8: _t->put_on_the_map((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const COORD_TYPE(*)>(_a[2])),(*reinterpret_cast< const COORD_TYPE(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 9: { bool _r = _t->moveThePlayer((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const Direction(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 10: { Direction _r = _t->lookToMove((*reinterpret_cast< const Direction(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< Direction*>(_a[0]) = _r; }  break;
        case 11: _t->newRandomNumber((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 12: _t->useSeed((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 13: _t->useRecipe((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 14: _t->addObjectAndSend((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 15: _t->addObjectAndSend((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 16: _t->addObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 17: _t->addObject((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 18: _t->saveObjectRetention((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 19: { quint32 _r = _t->removeObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< quint32*>(_a[0]) = _r; }  break;
        case 20: { quint32 _r = _t->removeObject((*reinterpret_cast< const quint32(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< quint32*>(_a[0]) = _r; }  break;
        case 21: _t->sendRemoveObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 22: _t->sendRemoveObject((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 23: { quint32 _r = _t->objectQuantity((*reinterpret_cast< const quint32(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< quint32*>(_a[0]) = _r; }  break;
        case 24: _t->addCash((*reinterpret_cast< const quint64(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 25: _t->addCash((*reinterpret_cast< const quint64(*)>(_a[1]))); break;
        case 26: _t->removeCash((*reinterpret_cast< const quint64(*)>(_a[1]))); break;
        case 27: _t->sendHandlerCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 28: _t->destroyObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 29: _t->useObject((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 30: _t->receiveTeleportTo((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 31: _t->teleportValidatedTo((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 32: _t->getShopList((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 33: _t->buyObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])),(*reinterpret_cast< const quint32(*)>(_a[3])),(*reinterpret_cast< const quint32(*)>(_a[4])),(*reinterpret_cast< const quint32(*)>(_a[5]))); break;
        case 34: _t->sellObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])),(*reinterpret_cast< const quint32(*)>(_a[3])),(*reinterpret_cast< const quint32(*)>(_a[4])),(*reinterpret_cast< const quint32(*)>(_a[5]))); break;
        case 35: _t->tryEscape(); break;
        case 36: { bool _r = _t->checkFightCollision((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const COORD_TYPE(*)>(_a[2])),(*reinterpret_cast< const COORD_TYPE(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 37: _t->useSkill((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 38: { bool _r = _t->learnSkill((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 39: { bool _r = _t->learnSkillInternal((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 40: _t->tradeCanceled(); break;
        case 41: _t->tradeAccepted(); break;
        case 42: _t->tradeFinished(); break;
        case 43: _t->tradeAddTradeCash((*reinterpret_cast< const quint64(*)>(_a[1]))); break;
        case 44: _t->tradeAddTradeObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 45: _t->tradeAddTradeMonster((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 46: _t->extraStop(); break;
        case 47: _t->savePosition(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::LocalClientHandler::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::LocalClientHandler::staticMetaObject = {
    { &MapBasicMove::staticMetaObject, qt_meta_stringdata_CatchChallenger__LocalClientHandler,
      qt_meta_data_CatchChallenger__LocalClientHandler, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::LocalClientHandler::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::LocalClientHandler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::LocalClientHandler::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__LocalClientHandler))
        return static_cast<void*>(const_cast< LocalClientHandler*>(this));
    return MapBasicMove::qt_metacast(_clname);
}

int CatchChallenger::LocalClientHandler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapBasicMove::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 48)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 48;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::LocalClientHandler::dbQuery(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::LocalClientHandler::askRandomNumber()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void CatchChallenger::LocalClientHandler::receiveSystemText(const QString & _t1, const bool & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void CatchChallenger::LocalClientHandler::postReply(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CatchChallenger::LocalClientHandler::sendTradeRequest(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CatchChallenger::LocalClientHandler::seedValidated()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void CatchChallenger::LocalClientHandler::teleportTo(Map * _t1, const quint8 & _t2, const quint8 & _t3, const Orientation & _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_END_MOC_NAMESPACE
