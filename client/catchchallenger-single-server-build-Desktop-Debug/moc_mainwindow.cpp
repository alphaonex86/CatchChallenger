/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created: Thu Apr 11 17:51:26 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../single-server/mainwindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MainWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x08,
      45,   11,   11,   11, 0x08,
      77,   11,   11,   11, 0x08,
     121,  109,   11,   11, 0x08,
     176,  164,   11,   11, 0x08,
     212,   11,   11,   11, 0x08,
     235,  227,   11,   11, 0x08,
     259,  252,   11,   11, 0x08,
     281,   11,   11,   11, 0x08,
     300,   11,   11,   11, 0x08,
     324,  311,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0on_lineEditLogin_returnPressed()\0"
    "on_lineEditPass_returnPressed()\0"
    "on_pushButtonTryLogin_clicked()\0"
    "socketState\0stateChanged(QAbstractSocket::SocketState)\0"
    "socketError\0error(QAbstractSocket::SocketError)\0"
    "haveNewError()\0message\0message(QString)\0"
    "reason\0disconnected(QString)\0"
    "protocol_is_good()\0needQuit()\0"
    "informations\0"
    "have_current_player_info(CatchChallenger::Player_private_and_public_in"
    "formations)\0"
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MainWindow *_t = static_cast<MainWindow *>(_o);
        switch (_id) {
        case 0: _t->on_lineEditLogin_returnPressed(); break;
        case 1: _t->on_lineEditPass_returnPressed(); break;
        case 2: _t->on_pushButtonTryLogin_clicked(); break;
        case 3: _t->stateChanged((*reinterpret_cast< QAbstractSocket::SocketState(*)>(_a[1]))); break;
        case 4: _t->error((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 5: _t->haveNewError(); break;
        case 6: _t->message((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->disconnected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->protocol_is_good(); break;
        case 9: _t->needQuit(); break;
        case 10: _t->have_current_player_info((*reinterpret_cast< const CatchChallenger::Player_private_and_public_informations(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MainWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow,
      qt_meta_data_MainWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
