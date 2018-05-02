#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    void on_privatekey_textChanged(const QString &arg1);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
