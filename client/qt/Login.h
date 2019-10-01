#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>

namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();
    bool isOk() const;
    void setLogin(const QString &text);
    void setPass(const QString &text);
    QString getLogin() const;
    QString getPass() const;
    bool getRememberPassword() const;
private slots:
    void on_pushButtonTryLogin_clicked();
    void on_cancel_clicked();
    void on_lineEditLogin_returnPressed();
    void on_lineEditPass_returnPressed();
private:
    Ui::Login *ui;
    bool ok;
};

#endif // LOGIN_H
