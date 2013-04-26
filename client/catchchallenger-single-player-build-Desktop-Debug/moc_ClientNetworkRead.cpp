/****************************************************************************
** Meta object code from reading C++ file 'ClientNetworkRead.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientNetworkRead.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientNetworkRead.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__ClientNetworkRead[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      38,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      33,       // signalCount

 // signals: signature, parameters, type, tag, flags
      66,   36,   35,   35, 0x05,
     129,  104,   35,   35, 0x25,
     174,  156,   35,   35, 0x05,
     217,  204,   35,   35, 0x25,
     272,  236,   35,   35, 0x05,
     333,  316,   35,   35, 0x05,
     362,   35,   35,   35, 0x05,
     398,  378,   35,   35, 0x05,
     460,  434,   35,   35, 0x05,
     536,  508,   35,   35, 0x05,
     588,  568,   35,   35, 0x05,
     640,   35,   35,   35, 0x05,
     656,   35,   35,   35, 0x05,
     672,   35,   35,   35, 0x05,
     693,  688,   35,   35, 0x05,
     734,  720,   35,   35, 0x05,
     781,  771,   35,   35, 0x05,
     823,  811,   35,   35, 0x05,
     861,  847,   35,   35, 0x05,
     898,  893,   35,   35, 0x05,
     943,  925,   35,   35, 0x05,
     981,  925,   35,   35, 0x05,
    1035, 1017,   35,   35, 0x05,
    1069, 1060,   35,   35, 0x05,
    1109, 1090,   35,   35, 0x05,
    1151, 1135,   35,   35, 0x05,
    1198, 1182,   35,   35, 0x05,
    1240, 1224,   35,   35, 0x05,
    1309, 1269,   35,   35, 0x05,
    1360, 1269,   35,   35, 0x05,
    1412,   35,   35,   35, 0x05,
    1430, 1424,   35,   35, 0x05,
    1464, 1448,   35,   35, 0x05,

 // slots: signature, parameters, type, tag, flags
    1497, 1492,   35,   35, 0x0a,
    1527,   35,   35,   35, 0x0a,
    1545,   35,   35,   35, 0x0a,
    1566,  568,   35,   35, 0x0a,
    1609, 1492,   35,   35, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__ClientNetworkRead[] = {
    "CatchChallenger::ClientNetworkRead\0\0"
    "mainCodeType,subCodeType,data\0"
    "sendPacket(quint8,quint16,QByteArray)\0"
    "mainCodeType,subCodeType\0"
    "sendPacket(quint8,quint16)\0mainCodeType,data\0"
    "sendPacket(quint8,QByteArray)\0"
    "mainCodeType\0sendPacket(quint8)\0"
    "mainIdent,subIdent,queryNumber,data\0"
    "sendQuery(quint8,quint16,quint8,QByteArray)\0"
    "queryNumber,data\0postReply(quint8,QByteArray)\0"
    "isReadyToStop()\0query_id,login,hash\0"
    "askLogin(quint8,QString,QByteArray)\0"
    "query_id,files,timestamps\0"
    "datapackList(quint8,QStringList,QList<quint64>)\0"
    "previousMovedUnit,direction\0"
    "moveThePlayer(quint8,Direction)\0"
    "map,x,y,orientation\0"
    "teleportValidatedTo(Map*,quint8,quint8,Orientation)\0"
    "tradeCanceled()\0tradeAccepted()\0"
    "tradeFinished()\0cash\0tradeAddTradeCash(quint64)\0"
    "item,quantity\0tradeAddTradeObject(quint32,quint32)\0"
    "monsterId\0tradeAddTradeMonster(quint32)\0"
    "text,pseudo\0sendPM(QString,QString)\0"
    "chatType,text\0sendChatText(Chat_type,QString)\0"
    "text\0sendLocalChatText(QString)\0"
    "command,extraText\0"
    "sendBroadCastCommand(QString,QString)\0"
    "sendHandlerCommand(QString,QString)\0"
    "query_id,plant_id\0plantSeed(quint8,quint8)\0"
    "query_id\0collectPlant(quint8)\0"
    "query_id,recipe_id\0useRecipe(quint8,quint32)\0"
    "itemId,quantity\0destroyObject(quint32,quint32)\0"
    "query_id,itemId\0useObject(quint8,quint32)\0"
    "query_id,shopId\0getShopList(quint32,quint32)\0"
    "query_id,shopId,objectId,quantity,price\0"
    "buyObject(quint32,quint32,quint32,quint32,quint32)\0"
    "sellObject(quint32,quint32,quint32,quint32,quint32)\0"
    "tryEscape()\0skill\0useSkill(quint32)\0"
    "monsterId,skill\0learnSkill(quint32,quint32)\0"
    "data\0fake_receive_data(QByteArray)\0"
    "purgeReadBuffer()\0askIfIsReadyToStop()\0"
    "teleportTo(Map*,quint8,quint8,Orientation)\0"
    "sendTradeRequest(QByteArray)\0"
};

void CatchChallenger::ClientNetworkRead::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientNetworkRead *_t = static_cast<ClientNetworkRead *>(_o);
        switch (_id) {
        case 0: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 1: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2]))); break;
        case 2: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 3: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 4: _t->sendQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 5: _t->postReply((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 6: _t->isReadyToStop(); break;
        case 7: _t->askLogin((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 8: _t->datapackList((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QList<quint64>(*)>(_a[3]))); break;
        case 9: _t->moveThePlayer((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const Direction(*)>(_a[2]))); break;
        case 10: _t->teleportValidatedTo((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 11: _t->tradeCanceled(); break;
        case 12: _t->tradeAccepted(); break;
        case 13: _t->tradeFinished(); break;
        case 14: _t->tradeAddTradeCash((*reinterpret_cast< const quint64(*)>(_a[1]))); break;
        case 15: _t->tradeAddTradeObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 16: _t->tradeAddTradeMonster((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 17: _t->sendPM((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 18: _t->sendChatText((*reinterpret_cast< const Chat_type(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 19: _t->sendLocalChatText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->sendBroadCastCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 21: _t->sendHandlerCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 22: _t->plantSeed((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2]))); break;
        case 23: _t->collectPlant((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 24: _t->useRecipe((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 25: _t->destroyObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 26: _t->useObject((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 27: _t->getShopList((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 28: _t->buyObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])),(*reinterpret_cast< const quint32(*)>(_a[3])),(*reinterpret_cast< const quint32(*)>(_a[4])),(*reinterpret_cast< const quint32(*)>(_a[5]))); break;
        case 29: _t->sellObject((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])),(*reinterpret_cast< const quint32(*)>(_a[3])),(*reinterpret_cast< const quint32(*)>(_a[4])),(*reinterpret_cast< const quint32(*)>(_a[5]))); break;
        case 30: _t->tryEscape(); break;
        case 31: _t->useSkill((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 32: _t->learnSkill((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2]))); break;
        case 33: _t->fake_receive_data((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 34: _t->purgeReadBuffer(); break;
        case 35: _t->askIfIsReadyToStop(); break;
        case 36: _t->teleportTo((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 37: _t->sendTradeRequest((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::ClientNetworkRead::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::ClientNetworkRead::staticMetaObject = {
    { &ProtocolParsingInput::staticMetaObject, qt_meta_stringdata_CatchChallenger__ClientNetworkRead,
      qt_meta_data_CatchChallenger__ClientNetworkRead, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::ClientNetworkRead::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::ClientNetworkRead::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::ClientNetworkRead::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__ClientNetworkRead))
        return static_cast<void*>(const_cast< ClientNetworkRead*>(this));
    return ProtocolParsingInput::qt_metacast(_clname);
}

int CatchChallenger::ClientNetworkRead::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProtocolParsingInput::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 38)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 38;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::ClientNetworkRead::sendPacket(const quint8 & _t1, const quint16 & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void CatchChallenger::ClientNetworkRead::sendPacket(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void CatchChallenger::ClientNetworkRead::sendQuery(const quint8 & _t1, const quint16 & _t2, const quint8 & _t3, const QByteArray & _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CatchChallenger::ClientNetworkRead::postReply(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CatchChallenger::ClientNetworkRead::isReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void CatchChallenger::ClientNetworkRead::askLogin(const quint8 & _t1, const QString & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void CatchChallenger::ClientNetworkRead::datapackList(const quint8 & _t1, const QStringList & _t2, const QList<quint64> & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void CatchChallenger::ClientNetworkRead::moveThePlayer(const quint8 & _t1, const Direction & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void CatchChallenger::ClientNetworkRead::teleportValidatedTo(Map * _t1, const quint8 & _t2, const quint8 & _t3, const Orientation & _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void CatchChallenger::ClientNetworkRead::tradeCanceled()
{
    QMetaObject::activate(this, &staticMetaObject, 11, 0);
}

// SIGNAL 12
void CatchChallenger::ClientNetworkRead::tradeAccepted()
{
    QMetaObject::activate(this, &staticMetaObject, 12, 0);
}

// SIGNAL 13
void CatchChallenger::ClientNetworkRead::tradeFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 13, 0);
}

// SIGNAL 14
void CatchChallenger::ClientNetworkRead::tradeAddTradeCash(const quint64 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void CatchChallenger::ClientNetworkRead::tradeAddTradeObject(const quint32 & _t1, const quint32 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void CatchChallenger::ClientNetworkRead::tradeAddTradeMonster(const quint32 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void CatchChallenger::ClientNetworkRead::sendPM(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void CatchChallenger::ClientNetworkRead::sendChatText(const Chat_type & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void CatchChallenger::ClientNetworkRead::sendLocalChatText(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void CatchChallenger::ClientNetworkRead::sendBroadCastCommand(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}

// SIGNAL 21
void CatchChallenger::ClientNetworkRead::sendHandlerCommand(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 21, _a);
}

// SIGNAL 22
void CatchChallenger::ClientNetworkRead::plantSeed(const quint8 & _t1, const quint8 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 22, _a);
}

// SIGNAL 23
void CatchChallenger::ClientNetworkRead::collectPlant(const quint8 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 23, _a);
}

// SIGNAL 24
void CatchChallenger::ClientNetworkRead::useRecipe(const quint8 & _t1, const quint32 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 24, _a);
}

// SIGNAL 25
void CatchChallenger::ClientNetworkRead::destroyObject(const quint32 & _t1, const quint32 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 25, _a);
}

// SIGNAL 26
void CatchChallenger::ClientNetworkRead::useObject(const quint8 & _t1, const quint32 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 26, _a);
}

// SIGNAL 27
void CatchChallenger::ClientNetworkRead::getShopList(const quint32 & _t1, const quint32 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 27, _a);
}

// SIGNAL 28
void CatchChallenger::ClientNetworkRead::buyObject(const quint32 & _t1, const quint32 & _t2, const quint32 & _t3, const quint32 & _t4, const quint32 & _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 28, _a);
}

// SIGNAL 29
void CatchChallenger::ClientNetworkRead::sellObject(const quint32 & _t1, const quint32 & _t2, const quint32 & _t3, const quint32 & _t4, const quint32 & _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 29, _a);
}

// SIGNAL 30
void CatchChallenger::ClientNetworkRead::tryEscape()
{
    QMetaObject::activate(this, &staticMetaObject, 30, 0);
}

// SIGNAL 31
void CatchChallenger::ClientNetworkRead::useSkill(const quint32 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 31, _a);
}

// SIGNAL 32
void CatchChallenger::ClientNetworkRead::learnSkill(const quint32 & _t1, const quint32 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 32, _a);
}
QT_END_MOC_NAMESPACE
