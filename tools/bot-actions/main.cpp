#include "MainWindow.h"
#include "DatabaseBot.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("CatchChallenger Bot Actions");
    parser.addHelpOption();

    QCommandLineOption hostOption("host", "Server host address", "address");
    QCommandLineOption portOption("port", "Server port", "port");
    QCommandLineOption botsOption("bots", "Number of bots to connect", "count", "1");
    QCommandLineOption loginOption("login", "Login pattern (use %NUMBER% for bot index)", "login");
    QCommandLineOption passOption("pass", "Password pattern (use %NUMBER% for bot index)", "password");

    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(botsOption);
    parser.addOption(loginOption);
    parser.addOption(passOption);

    parser.process(a);

    DatabaseBot::databaseBot.init();
    a.setOrganizationDomain("CatchChallenger");
    a.setOrganizationName("bot-action");
    MainWindow w;
    w.show();

    if(parser.isSet(hostOption) && parser.isSet(portOption))
    {
        QString host = parser.value(hostOption);
        quint16 port = parser.value(portOption).toUShort();
        int bots = parser.value(botsOption).toInt();
        QString login = parser.isSet(loginOption) ? parser.value(loginOption) : QString();
        QString pass = parser.isSet(passOption) ? parser.value(passOption) : QString();
        w.autoConnect(host, port, bots, login, pass);
    }

    return a.exec();
}
