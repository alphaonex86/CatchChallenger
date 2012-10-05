/****************************************************************************
** Meta object code from reading C++ file 'Api_protocol.h'
**
** Created: Fri Oct 5 13:40:08 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/Api_protocol.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Api_protocol.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__Api_protocol[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      19,       // signalCount

 // signals: signature, parameters, type, tag, flags
      45,   25,   24,   24, 0x05,
      78,   71,   24,   24, 0x05,
     100,   71,   24,   24, 0x05,
     119,   24,   24,   24, 0x05,
     128,   24,   24,   24, 0x05,
     165,  147,   24,   24, 0x05,
     204,  199,   24,   24, 0x05,
     258,  229,   24,   24, 0x05,
     368,  356,   24,   24, 0x05,
     424,  421,   24,   24, 0x05,
     464,  447,   24,   24, 0x05,
     549,  524,   24,   24, 0x05,
     617,   24,   24,   24, 0x05,
     668,  641,   24,   24, 0x05,
     758,  743,   24,   24, 0x05,
     817,  804,   24,   24, 0x05,
     893,   24,   24,   24, 0x05,
     931,  911,   24,   24, 0x05,
     976,  967,   24,   24, 0x05,

 // slots: signature, parameters, type, tag, flags
    1010,  996,   24,   24, 0x0a,
    1075, 1054,   24,   24, 0x0a,
    1135, 1121,   24,   24, 0x0a,
    1190, 1178,   24,   24, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__Api_protocol[] = {
    "Pokecraft::Api_protocol\0\0error,detailedError\0"
    "newError(QString,QString)\0reason\0"
    "disconnected(QString)\0notLogged(QString)\0"
    "logged()\0protocol_is_good()\0"
    "number,max_player\0number_of_player(quint16,quint16)\0"
    "data\0random_seeds(QByteArray)\0"
    "player,mapName,x,y,direction\0"
    "insert_player(Pokecraft::Player_public_informations,QString,quint16,qu"
    "int16,Pokecraft::Direction)\0"
    "id,movement\0"
    "move_player(quint16,QList<QPair<quint8,Direction> >)\0"
    "id\0remove_player(quint16)\0id,x,y,direction\0"
    "reinsert_player(quint16,quint8,quint8,Pokecraft::Direction)\0"
    "id,mapName,x,y,direction\0"
    "reinsert_player(quint16,QString,quint8,quint8,Pokecraft::Direction)\0"
    "dropAllPlayerOnTheMap()\0"
    "chat_type,text,pseudo,type\0"
    "new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_t"
    "ype)\0"
    "chat_type,text\0"
    "new_system_text(Pokecraft::Chat_type,QString)\0"
    "informations\0"
    "have_current_player_info(Pokecraft::Player_private_and_public_informat"
    "ions)\0"
    "haveTheDatapack()\0fileName,data,mtime\0"
    "newFile(QString,QByteArray,quint32)\0"
    "fileName\0removeFile(QString)\0the_direction\0"
    "send_player_direction(Pokecraft::Direction)\0"
    "moved_unit,direction\0"
    "send_player_move(quint8,Pokecraft::Direction)\0"
    "chatType,text\0sendChatText(Pokecraft::Chat_type,QString)\0"
    "text,pseudo\0sendPM(QString,QString)\0"
};

void Pokecraft::Api_protocol::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Api_protocol *_t = static_cast<Api_protocol *>(_o);
        switch (_id) {
        case 0: _t->newError((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 1: _t->disconnected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->notLogged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->logged(); break;
        case 4: _t->protocol_is_good(); break;
        case 5: _t->number_of_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2]))); break;
        case 6: _t->random_seeds((*reinterpret_cast< QByteArray(*)>(_a[1]))); break;
        case 7: _t->insert_player((*reinterpret_cast< Pokecraft::Player_public_informations(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< quint16(*)>(_a[3])),(*reinterpret_cast< quint16(*)>(_a[4])),(*reinterpret_cast< Pokecraft::Direction(*)>(_a[5]))); break;
        case 8: _t->move_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< QList<QPair<quint8,Direction> >(*)>(_a[2]))); break;
        case 9: _t->remove_player((*reinterpret_cast< quint16(*)>(_a[1]))); break;
        case 10: _t->reinsert_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< quint8(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< Pokecraft::Direction(*)>(_a[4]))); break;
        case 11: _t->reinsert_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint8(*)>(_a[4])),(*reinterpret_cast< Pokecraft::Direction(*)>(_a[5]))); break;
        case 12: _t->dropAllPlayerOnTheMap(); break;
        case 13: _t->new_chat_text((*reinterpret_cast< Pokecraft::Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< Pokecraft::Player_type(*)>(_a[4]))); break;
        case 14: _t->new_system_text((*reinterpret_cast< Pokecraft::Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 15: _t->have_current_player_info((*reinterpret_cast< Pokecraft::Player_private_and_public_informations(*)>(_a[1]))); break;
        case 16: _t->haveTheDatapack(); break;
        case 17: _t->newFile((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2])),(*reinterpret_cast< const quint32(*)>(_a[3]))); break;
        case 18: _t->removeFile((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 19: _t->send_player_direction((*reinterpret_cast< const Pokecraft::Direction(*)>(_a[1]))); break;
        case 20: _t->send_player_move((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const Pokecraft::Direction(*)>(_a[2]))); break;
        case 21: _t->sendChatText((*reinterpret_cast< Pokecraft::Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 22: _t->sendPM((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::Api_protocol::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::Api_protocol::staticMetaObject = {
    { &ProtocolParsingInput::staticMetaObject, qt_meta_stringdata_Pokecraft__Api_protocol,
      qt_meta_data_Pokecraft__Api_protocol, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::Api_protocol::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::Api_protocol::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::Api_protocol::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__Api_protocol))
        return static_cast<void*>(const_cast< Api_protocol*>(this));
    if (!strcmp(_clname, "MoveOnTheMap"))
        return static_cast< MoveOnTheMap*>(const_cast< Api_protocol*>(this));
    return ProtocolParsingInput::qt_metacast(_clname);
}

int Pokecraft::Api_protocol::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProtocolParsingInput::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    return _id;
}

// SIGNAL 0
void Pokecraft::Api_protocol::newError(QString _t1, QString _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Pokecraft::Api_protocol::disconnected(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Pokecraft::Api_protocol::notLogged(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Pokecraft::Api_protocol::logged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void Pokecraft::Api_protocol::protocol_is_good()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void Pokecraft::Api_protocol::number_of_player(quint16 _t1, quint16 _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void Pokecraft::Api_protocol::random_seeds(QByteArray _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Pokecraft::Api_protocol::insert_player(Pokecraft::Player_public_informations _t1, QString _t2, quint16 _t3, quint16 _t4, Pokecraft::Direction _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Pokecraft::Api_protocol::move_player(quint16 _t1, QList<QPair<quint8,Direction> > _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void Pokecraft::Api_protocol::remove_player(quint16 _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void Pokecraft::Api_protocol::reinsert_player(quint16 _t1, quint8 _t2, quint8 _t3, Pokecraft::Direction _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void Pokecraft::Api_protocol::reinsert_player(quint16 _t1, QString _t2, quint8 _t3, quint8 _t4, Pokecraft::Direction _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void Pokecraft::Api_protocol::dropAllPlayerOnTheMap()
{
    QMetaObject::activate(this, &staticMetaObject, 12, 0);
}

// SIGNAL 13
void Pokecraft::Api_protocol::new_chat_text(Pokecraft::Chat_type _t1, QString _t2, QString _t3, Pokecraft::Player_type _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void Pokecraft::Api_protocol::new_system_text(Pokecraft::Chat_type _t1, QString _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void Pokecraft::Api_protocol::have_current_player_info(Pokecraft::Player_private_and_public_informations _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void Pokecraft::Api_protocol::haveTheDatapack()
{
    QMetaObject::activate(this, &staticMetaObject, 16, 0);
}

// SIGNAL 17
void Pokecraft::Api_protocol::newFile(const QString & _t1, const QByteArray & _t2, const quint32 & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void Pokecraft::Api_protocol::removeFile(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}
QT_END_MOC_NAMESPACE
