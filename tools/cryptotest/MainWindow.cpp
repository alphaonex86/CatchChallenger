#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <iostream>
#include <stdio.h>
#include <nettle/eddsa.h>

/*Constant: ED25519_KEY_SIZE
The size of a private or public Ed25519 key, 32 octets.

Constant: ED25519_SIGNATURE_SIZE
The size of an Ed25519 signature, 64 octets.

Function: void ed25519_sha512_public_key (uint8_t *pub, const uint8_t *priv)
Computes the public key corresponding to the given private key. Both input and output are of size ED25519_KEY_SIZE.

Function: void ed25519_sha512_sign (const uint8_t *pub, const uint8_t *priv, size_t length, const uint8_t *msg, uint8_t *signature)
Signs a message using the provided key pair.

Function: int ed25519_sha512_verify (const uint8_t *pub, size_t length, const uint8_t *msg, const uint8_t *signature)
Verifies a message using the provided public key. Returns 1 if the signature is valid, otherwise 0.*/

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    on_makekeys_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_makekeys_clicked()
{
    uint8_t privatekey[ED25519_KEY_SIZE];

    FILE *ptr = fopen("/dev/random","rb");  // r for read, b for binary
    if(ptr == NULL)
        abort();
    const int readSize=fread(privatekey,1,sizeof(privatekey),ptr);
    if(readSize != sizeof(privatekey))
        abort();
    fclose(ptr);

    QString privatekeyhex(QByteArray((char *)privatekey,sizeof(privatekey)).toHex());
    ui->privatekey->setText(privatekeyhex);
    on_privatekey_textChanged(privatekeyhex);
}

void MainWindow::on_privatekey_textChanged(const QString &arg1)
{
    const QByteArray privatekey=QByteArray::fromHex(arg1.toUtf8());
    if(privatekey.size()!=ED25519_KEY_SIZE)
        return;
    uint8_t publickey[ED25519_KEY_SIZE];
    ed25519_sha512_public_key(publickey,reinterpret_cast<const uint8_t *>(privatekey.constData()));
    ui->publickey->setText(QString(QByteArray((char *)publickey,sizeof(publickey)).toHex()));
}

void MainWindow::on_sign_clicked()
{
    QByteArray data=QByteArray::fromHex(ui->sign_privatekey->text().toUtf8());
    if(data.size()!=ED25519_KEY_SIZE)
    {
        QMessageBox::critical(this,"Key size","The input key size is wrong o not into Hex");
        return;
    }

    const uint8_t *privatekey=reinterpret_cast<const uint8_t *>(data.constData());
    uint8_t publickey[ED25519_KEY_SIZE];
    ed25519_sha512_public_key(publickey,privatekey);
    uint8_t signature[ED25519_SIGNATURE_SIZE];

    const QByteArray msg=QByteArray::fromHex(ui->sign_input->toPlainText().toUtf8());

    ed25519_sha512_sign(publickey,privatekey,msg.size(),reinterpret_cast<const uint8_t *>(msg.constData()),signature);

    ui->sign_output->setText(QString(QByteArray(reinterpret_cast<char *>(signature),sizeof(signature)).toHex()));
}

void MainWindow::on_verify_clicked()
{
    const QByteArray msg=QByteArray::fromHex(ui->verify_input->toPlainText().toUtf8());

    QByteArray data=QByteArray::fromHex(ui->verify_publickey->text().toUtf8());
    if(data.size()!=ED25519_KEY_SIZE)
    {
        QMessageBox::critical(this,"Key size","The input key size is wrong o not into Hex");
        return;
    }
    const uint8_t *publickey=reinterpret_cast<const uint8_t *>(data.constData());

    QByteArray datasignature=QByteArray::fromHex(ui->verify_firm->text().toUtf8());
    if(datasignature.size()!=ED25519_SIGNATURE_SIZE)
    {
        QMessageBox::critical(this,"Key size","The input key size is wrong o not into Hex");
        return;
    }
    const uint8_t *signature=reinterpret_cast<const uint8_t *>(datasignature.constData());

    const int rc = ed25519_sha512_verify (publickey,msg.size(),reinterpret_cast<const uint8_t *>(msg.constData()),signature);

    if(rc == 1) {
        QMessageBox::information(this,"Verification","Verified signature");
    } else {
        QMessageBox::critical(this,"Verification","Failed to verify signature, return code "+QString::number(rc));
    }
}
