#include "Login.hpp"
#include "ui_Login.h"
#include "BlacklistPassword.hpp"

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login),
    ok(false)
{
    ui->setupUi(this);
    ui->warning->setVisible(false);
}

Login::~Login()
{
    delete ui;
}

bool Login::isOk() const
{
    return ok;
}

void Login::on_cancel_clicked()
{
    ok=false;
    reject();
}
void Login::setLogin(const QString &text)
{
    ui->lineEditLogin->setText(text);
    ui->checkBoxRememberPassword->setChecked(false);
}

void Login::setPass(const QString &text)
{
    ui->lineEditPass->setText(text);
    ui->checkBoxRememberPassword->setChecked(true);
}

QString Login::getLogin() const
{
    return ui->lineEditLogin->text();
}

QString Login::getPass() const
{
    return ui->lineEditPass->text();
}

bool Login::getRememberPassword() const
{
    return ui->checkBoxRememberPassword->isChecked();
}

void Login::on_lineEditLogin_returnPressed()
{
    if(ui->lineEditPass->text().isEmpty())
        ui->lineEditPass->setFocus();
    else
        on_pushButtonTryLogin_clicked();
}

void Login::on_lineEditPass_returnPressed()
{
    on_pushButtonTryLogin_clicked();
}

void Login::on_pushButtonTryLogin_clicked()
{
    /*if(selectedServer==NULL)
    {
        qDebug() << "selectedServer==NULL, fix it!";
        abort();
    }
    if(!serverConnexion.contains(selectedServer))
    {
        qDebug() << "!serverConnexion.contains(selectedServer)";
        abort();
    }*/
    if(!ui->pushButtonTryLogin->isEnabled())
        return;
    if(ui->lineEditPass->text().size()<6)
    {
        ui->warning->setText(tr("Your password need to be at minimum of 6 characters"));
        ui->warning->setVisible(true);
        return;
    }
    {
        std::string pass=ui->lineEditPass->text().toStdString();
        std::transform(pass.begin(), pass.end(), pass.begin(), ::tolower);
        unsigned int index=0;
        while(index<BlacklistPassword::list.size())
        {
            if(BlacklistPassword::list.at(index)==pass)
            {
                ui->warning->setText(tr("Your password is into the most common password in the world, too easy to crack dude! Change it!"));
                ui->warning->setVisible(true);
                return;
            }
            index++;
        }
    }
    if(ui->lineEditLogin->text().size()<3)
    {
        ui->warning->setText(tr("Your login need to be at minimum of 3 characters"));
        ui->warning->setVisible(true);
        return;
    }
    if(ui->lineEditPass->text()==ui->lineEditLogin->text())
    {
        ui->warning->setText(tr("Your login can't be same as your login"));
        ui->warning->setVisible(true);
        return;
    }

    ok=true;
    accept();
}

void Login::on_toolButton_triggered(QAction *)
{
    if(ui->lineEditPass->echoMode()==QLineEdit::EchoMode::Password)
        ui->lineEditPass->setEchoMode(QLineEdit::EchoMode::Normal);
    else
        ui->lineEditPass->setEchoMode(QLineEdit::EchoMode::Password);
}

void Login::on_toolButton_toggled(bool)
{
    if(ui->lineEditPass->echoMode()==QLineEdit::EchoMode::Password)
        ui->lineEditPass->setEchoMode(QLineEdit::EchoMode::Normal);
    else
        ui->lineEditPass->setEchoMode(QLineEdit::EchoMode::Password);
}
