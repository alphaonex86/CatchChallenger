#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <nettle/eddsa.h>
#include <nettle/curve25519.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void on_makekeys_clicked();
    void on_sign_clicked();
    void on_verify_clicked();
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
