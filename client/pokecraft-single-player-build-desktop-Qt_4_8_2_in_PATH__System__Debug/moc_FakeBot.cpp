/****************************************************************************
** Meta object code from reading C++ file 'FakeBot.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/Bot/FakeBot.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FakeBot.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__FakeBot[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x0a,
      32,   19,   19,   19, 0x0a,
      39,   19,   19,   19, 0x0a,
      48,   19,   19,   19, 0x08,
      95,   66,   19,   19, 0x08,
     183,  171,   19,   19, 0x08,
     276,  256,   19,   19, 0x08,
     308,  302,   19,   19, 0x08,
     353,   19,   19,   19, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__FakeBot[] = {
    "Pokecraft::FakeBot\0\0stop_step()\0stop()\0"
    "doStep()\0random_new_step()\0"
    "player,mapName,x,y,direction\0"
    "insert_player(Player_public_informations,QString,quint16,quint16,Direc"
    "tion)\0"
    "info,pseudo\0"
    "have_current_player_info(Player_private_and_public_informations,QStrin"
    "g)\0"
    "error,detailedError\0newError(QString,QString)\0"
    "error\0newSocketError(QAbstractSocket::SocketError)\0"
    "disconnected()\0"
};

void Pokecraft::FakeBot::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        FakeBot *_t = static_cast<FakeBot *>(_o);
        switch (_id) {
        case 0: _t->stop_step(); break;
        case 1: _t->stop(); break;
        case 2: _t->doStep(); break;
        case 3: _t->random_new_step(); break;
        case 4: _t->insert_player((*reinterpret_cast< Player_public_informations(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< quint16(*)>(_a[3])),(*reinterpret_cast< quint16(*)>(_a[4])),(*reinterpret_cast< Direction(*)>(_a[5]))); break;
        case 5: _t->have_current_player_info((*reinterpret_cast< Player_private_and_public_informations(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 6: _t->newError((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 7: _t->newSocketError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 8: _t->disconnected(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::FakeBot::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::FakeBot::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Pokecraft__FakeBot,
      qt_meta_data_Pokecraft__FakeBot, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::FakeBot::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::FakeBot::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::FakeBot::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__FakeBot))
        return static_cast<void*>(const_cast< FakeBot*>(this));
    if (!strcmp(_clname, "MoveOnTheMap"))
        return static_cast< MoveOnTheMap*>(const_cast< FakeBot*>(this));
    return QObject::qt_metacast(_clname);
}

int Pokecraft::FakeBot::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
