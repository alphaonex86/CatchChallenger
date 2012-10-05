/****************************************************************************
** Meta object code from reading C++ file 'BaseWindow.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/interface/BaseWindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BaseWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__BaseWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      22,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   22,   22,   22, 0x05,

 // slots: signature, parameters, type, tag, flags
      46,   34,   22,   22, 0x0a,
      97,   89,   22,   22, 0x08,
     121,  114,   22,   22, 0x08,
     143,  114,   22,   22, 0x08,
     162,   22,   22,   22, 0x08,
     171,   22,   22,   22, 0x08,
     198,   22,   22,   22, 0x08,
     216,   22,   22,   22, 0x08,
     254,   22,   22,   22, 0x08,
     293,  273,   22,   22, 0x08,
     325,  319,   22,   22, 0x08,
     367,  340,   22,   22, 0x08,
     457,  442,   22,   22, 0x08,
     514,  503,   22,   22, 0x08,
     554,  548,   22,   22, 0x08,
     601,   22,   22,   22, 0x08,
     615,   22,   22,   22, 0x08,
     638,   22,   22,   22, 0x08,
     677,   22,   22,   22, 0x08,
     716,   22,   22,   22, 0x08,
     758,   22,   22,   22, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__BaseWindow[] = {
    "Pokecraft::BaseWindow\0\0needQuit()\0"
    "socketState\0stateChanged(QAbstractSocket::SocketState)\0"
    "message\0message(QString)\0reason\0"
    "disconnected(QString)\0notLogged(QString)\0"
    "logged()\0have_current_player_info()\0"
    "haveTheDatapack()\0"
    "on_lineEdit_chat_text_returnPressed()\0"
    "protocol_is_good()\0error,detailedError\0"
    "newError(QString,QString)\0error\0"
    "error(QString)\0chat_type,text,pseudo,type\0"
    "new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_t"
    "ype)\0"
    "chat_type,text\0"
    "new_system_text(Pokecraft::Chat_type,QString)\0"
    "number,max\0number_of_player(quint16,quint16)\0"
    "index\0on_comboBox_chat_type_currentIndexChanged(int)\0"
    "update_chat()\0removeNumberForFlood()\0"
    "on_toolButton_interface_quit_clicked()\0"
    "on_toolButton_quit_interface_clicked()\0"
    "on_pushButton_interface_trainer_clicked()\0"
    "on_lineEdit_chat_text_lostFocus()\0"
};

void Pokecraft::BaseWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        BaseWindow *_t = static_cast<BaseWindow *>(_o);
        switch (_id) {
        case 0: _t->needQuit(); break;
        case 1: _t->stateChanged((*reinterpret_cast< QAbstractSocket::SocketState(*)>(_a[1]))); break;
        case 2: _t->message((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->disconnected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->notLogged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->logged(); break;
        case 6: _t->have_current_player_info(); break;
        case 7: _t->haveTheDatapack(); break;
        case 8: _t->on_lineEdit_chat_text_returnPressed(); break;
        case 9: _t->protocol_is_good(); break;
        case 10: _t->newError((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 11: _t->error((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 12: _t->new_chat_text((*reinterpret_cast< Pokecraft::Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< Pokecraft::Player_type(*)>(_a[4]))); break;
        case 13: _t->new_system_text((*reinterpret_cast< Pokecraft::Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 14: _t->number_of_player((*reinterpret_cast< quint16(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2]))); break;
        case 15: _t->on_comboBox_chat_type_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 16: _t->update_chat(); break;
        case 17: _t->removeNumberForFlood(); break;
        case 18: _t->on_toolButton_interface_quit_clicked(); break;
        case 19: _t->on_toolButton_quit_interface_clicked(); break;
        case 20: _t->on_pushButton_interface_trainer_clicked(); break;
        case 21: _t->on_lineEdit_chat_text_lostFocus(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::BaseWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::BaseWindow::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Pokecraft__BaseWindow,
      qt_meta_data_Pokecraft__BaseWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::BaseWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::BaseWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::BaseWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__BaseWindow))
        return static_cast<void*>(const_cast< BaseWindow*>(this));
    return QWidget::qt_metacast(_clname);
}

int Pokecraft::BaseWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 22)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    }
    return _id;
}

// SIGNAL 0
void Pokecraft::BaseWindow::needQuit()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
