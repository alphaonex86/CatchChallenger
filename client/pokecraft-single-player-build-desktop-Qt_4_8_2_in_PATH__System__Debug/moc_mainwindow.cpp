/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created: Fri Oct 5 13:40:08 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../single-player/mainwindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MainWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      24,   12,   11,   11, 0x08,
      79,   67,   11,   11, 0x08,
     115,   11,   11,   11, 0x08,
     138,  130,   11,   11, 0x08,
     155,   11,   11,   11, 0x08,
     177,   11,   11,   11, 0x08,
     196,   11,   11,   11, 0x08,
     214,   11,   11,   11, 0x08,
     240,   11,   11,   11, 0x08,
     263,   11,   11,   11, 0x08,
     285,   11,   11,   11, 0x08,
     314,   11,   11,   11, 0x08,
     343,   11,   11,   11, 0x08,
     370,   11,   11,   11, 0x08,
     397,   11,   11,   11, 0x08,
     408,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0socketState\0"
    "stateChanged(QAbstractSocket::SocketState)\0"
    "socketError\0error(QAbstractSocket::SocketError)\0"
    "haveNewError()\0message\0message(QString)\0"
    "disconnected(QString)\0protocol_is_good()\0"
    "try_stop_server()\0on_SaveGame_New_clicked()\0"
    "savegameLabelClicked()\0savegameLabelUpdate()\0"
    "on_SaveGame_Delete_clicked()\0"
    "on_SaveGame_Rename_clicked()\0"
    "on_SaveGame_Copy_clicked()\0"
    "on_SaveGame_Play_clicked()\0needQuit()\0"
    "serverIsReady()\0"
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MainWindow *_t = static_cast<MainWindow *>(_o);
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast< QAbstractSocket::SocketState(*)>(_a[1]))); break;
        case 1: _t->error((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 2: _t->haveNewError(); break;
        case 3: _t->message((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->disconnected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->protocol_is_good(); break;
        case 6: _t->try_stop_server(); break;
        case 7: _t->on_SaveGame_New_clicked(); break;
        case 8: _t->savegameLabelClicked(); break;
        case 9: _t->savegameLabelUpdate(); break;
        case 10: _t->on_SaveGame_Delete_clicked(); break;
        case 11: _t->on_SaveGame_Rename_clicked(); break;
        case 12: _t->on_SaveGame_Copy_clicked(); break;
        case 13: _t->on_SaveGame_Play_clicked(); break;
        case 14: _t->needQuit(); break;
        case 15: _t->serverIsReady(); break;
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
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
