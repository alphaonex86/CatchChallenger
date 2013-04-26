/****************************************************************************
** Meta object code from reading C++ file 'FakeBot.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/Bot/FakeBot.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FakeBot.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__FakeBot[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      26,   25,   25,   25, 0x05,

 // slots: signature, parameters, type, tag, flags
      43,   25,   25,   25, 0x0a,
      55,   25,   25,   25, 0x0a,
      62,   25,   25,   25, 0x0a,
      71,   25,   25,   25, 0x08,
     116,   89,   25,   25, 0x08,
     239,  226,   25,   25, 0x08,
     341,  321,   25,   25, 0x08,
     373,  367,   25,   25, 0x08,
     418,   25,   25,   25, 0x08,
     433,   25,   25,   25, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__FakeBot[] = {
    "CatchChallenger::FakeBot\0\0isDisconnected()\0"
    "stop_step()\0stop()\0doStep()\0"
    "random_new_step()\0player,mapId,x,y,direction\0"
    "insert_player(CatchChallenger::Player_public_informations,quint32,quin"
    "t16,quint16,CatchChallenger::Direction)\0"
    "informations\0"
    "have_current_player_info(CatchChallenger::Player_private_and_public_in"
    "formations)\0"
    "error,detailedError\0newError(QString,QString)\0"
    "error\0newSocketError(QAbstractSocket::SocketError)\0"
    "disconnected()\0tryLink()\0"
};

void CatchChallenger::FakeBot::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        FakeBot *_t = static_cast<FakeBot *>(_o);
        switch (_id) {
        case 0: _t->isDisconnected(); break;
        case 1: _t->stop_step(); break;
        case 2: _t->stop(); break;
        case 3: _t->doStep(); break;
        case 4: _t->random_new_step(); break;
        case 5: _t->insert_player((*reinterpret_cast< const CatchChallenger::Player_public_informations(*)>(_a[1])),(*reinterpret_cast< const quint32(*)>(_a[2])),(*reinterpret_cast< const quint16(*)>(_a[3])),(*reinterpret_cast< const quint16(*)>(_a[4])),(*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[5]))); break;
        case 6: _t->have_current_player_info((*reinterpret_cast< const CatchChallenger::Player_private_and_public_informations(*)>(_a[1]))); break;
        case 7: _t->newError((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 8: _t->newSocketError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 9: _t->disconnected(); break;
        case 10: _t->tryLink(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::FakeBot::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::FakeBot::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_CatchChallenger__FakeBot,
      qt_meta_data_CatchChallenger__FakeBot, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::FakeBot::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::FakeBot::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::FakeBot::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__FakeBot))
        return static_cast<void*>(const_cast< FakeBot*>(this));
    if (!strcmp(_clname, "MoveOnTheMap"))
        return static_cast< MoveOnTheMap*>(const_cast< FakeBot*>(this));
    return QObject::qt_metacast(_clname);
}

int CatchChallenger::FakeBot::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::FakeBot::isDisconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
