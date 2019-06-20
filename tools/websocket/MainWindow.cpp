#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QtWebSockets/QWebSocket>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnected()
{
    ui->statusBar->showMessage("WebSocket connected");
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived,
            this, &MainWindow::onBinaryMessageReceived);
    //m_webSocket.sendBinaryMessage("Hello, world!");
}

void MainWindow::onClosed()
{
    ui->statusBar->showMessage("WebSocket closed: "+m_webSocket.errorString());
}

void MainWindow::onBinaryMessageReceived(QByteArray message)
{
    ui->statusBar->showMessage("Message received: "+QString(message.toBase64()));
    //m_webSocket.close();
}

void MainWindow::on_tryConnect_clicked()
{
    connect(&m_webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &MainWindow::onClosed);
    QUrl url{QString("ws://localhost:42013/")};
    QNetworkRequest request{url};
    request.setRawHeader("Sec-WebSocket-Protocol", "binary");
    m_webSocket.open(request);
}
