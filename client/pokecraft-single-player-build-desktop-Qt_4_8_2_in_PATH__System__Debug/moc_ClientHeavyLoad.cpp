/****************************************************************************
** Meta object code from reading C++ file 'ClientHeavyLoad.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientHeavyLoad.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientHeavyLoad.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__ClientHeavyLoad[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: signature, parameters, type, tag, flags
      34,   28,   27,   27, 0x05,
      57,   49,   27,   27, 0x05,
      74,   27,   27,   27, 0x05,
     114,   90,   27,   27, 0x05,
     171,  152,   27,   27, 0x25,
     213,  198,   27,   27, 0x05,
     253,  243,   27,   27, 0x25,
     289,  272,   27,   27, 0x05,
     318,   27,   27,   27, 0x05,
     345,   27,   27,   27, 0x05,
     376,  356,   27,   27, 0x05,
     434,  423,   27,   27, 0x05,

 // slots: signature, parameters, type, tag, flags
     482,  462,   27,   27, 0x0a,
     544,  518,   27,   27, 0x0a,
     602,  592,   27,   27, 0x0a,
     619,   27,   27,   27, 0x0a,
     639,   27,   27,   27, 0x0a,
     660,   27,   27,   27, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__ClientHeavyLoad[] = {
    "Pokecraft::ClientHeavyLoad\0\0error\0"
    "error(QString)\0message\0message(QString)\0"
    "isReadyToStop()\0mainIdent,subIdent,data\0"
    "sendPacket(quint8,quint16,QByteArray)\0"
    "mainIdent,subIdent\0sendPacket(quint8,quint16)\0"
    "mainIdent,data\0sendPacket(quint8,QByteArray)\0"
    "mainIdent\0sendPacket(quint8)\0"
    "queryNumber,data\0postReply(quint8,QByteArray)\0"
    "send_player_informations()\0isLogged()\0"
    "map,x,y,orientation\0"
    "put_on_the_map(Map*,quint8,quint8,Orientation)\0"
    "randomData\0newRandomNumber(QByteArray)\0"
    "query_id,login,hash\0"
    "askLogin(quint8,QString,QByteArray)\0"
    "query_id,files,timestamps\0"
    "datapackList(quint8,QStringList,QList<quint32>)\0"
    "queryText\0dbQuery(QString)\0"
    "askedRandomNumber()\0askIfIsReadyToStop()\0"
    "stop()\0"
};

void Pokecraft::ClientHeavyLoad::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientHeavyLoad *_t = static_cast<ClientHeavyLoad *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->isReadyToStop(); break;
        case 3: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 4: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2]))); break;
        case 5: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 6: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 7: _t->postReply((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 8: _t->send_player_informations(); break;
        case 9: _t->isLogged(); break;
        case 10: _t->put_on_the_map((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 11: _t->newRandomNumber((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 12: _t->askLogin((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 13: _t->datapackList((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QList<quint32>(*)>(_a[3]))); break;
        case 14: _t->dbQuery((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 15: _t->askedRandomNumber(); break;
        case 16: _t->askIfIsReadyToStop(); break;
        case 17: _t->stop(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::ClientHeavyLoad::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::ClientHeavyLoad::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Pokecraft__ClientHeavyLoad,
      qt_meta_data_Pokecraft__ClientHeavyLoad, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::ClientHeavyLoad::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::ClientHeavyLoad::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::ClientHeavyLoad::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__ClientHeavyLoad))
        return static_cast<void*>(const_cast< ClientHeavyLoad*>(this));
    return QObject::qt_metacast(_clname);
}

int Pokecraft::ClientHeavyLoad::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void Pokecraft::ClientHeavyLoad::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Pokecraft::ClientHeavyLoad::message(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Pokecraft::ClientHeavyLoad::isReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void Pokecraft::ClientHeavyLoad::sendPacket(const quint8 & _t1, const quint16 & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 5
void Pokecraft::ClientHeavyLoad::sendPacket(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 7
void Pokecraft::ClientHeavyLoad::postReply(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Pokecraft::ClientHeavyLoad::send_player_informations()
{
    QMetaObject::activate(this, &staticMetaObject, 8, 0);
}

// SIGNAL 9
void Pokecraft::ClientHeavyLoad::isLogged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, 0);
}

// SIGNAL 10
void Pokecraft::ClientHeavyLoad::put_on_the_map(Map * _t1, const quint8 & _t2, const quint8 & _t3, const Orientation & _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void Pokecraft::ClientHeavyLoad::newRandomNumber(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_END_MOC_NAMESPACE
