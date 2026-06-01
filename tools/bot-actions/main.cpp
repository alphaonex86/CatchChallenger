#include "MainWindow.h"
#include "DatabaseBot.h"
#include <QApplication>
#include <QCommandLineParser>
#ifdef CATCHCHALLENGER_BENCHMARK
#include "LatencyRecorder.h"
#include <csignal>
static void latency_sigint_handler(int)
{
    //async-signal-safe: only set a flag. The dump runs later from the event
    //loop (a QTimer in MainWindow polls g_latencySigint).
    g_latencySigint=1;
}
#endif

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
    QCommandLineOption latencyOption("latency", "Client-side latency benchmark mode (dump BENCH lines on SIGINT)");
    QCommandLineOption latencySecondsOption("latency-seconds", "Fixed measurement window in seconds (excludes startup); dump+exit after it", "seconds", "15");

    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(botsOption);
    parser.addOption(loginOption);
    parser.addOption(passOption);
    parser.addOption(latencyOption);
    parser.addOption(latencySecondsOption);

    parser.process(a);

    if(parser.isSet(latencyOption))
    {
#ifdef CATCHCHALLENGER_BENCHMARK
        g_latencyMode=true;
        if(parser.isSet(latencySecondsOption))
        {
            bool ok=false;
            const int s=parser.value(latencySecondsOption).toInt(&ok);
            if(ok && s>0)
                g_latencySeconds=s;
        }
        signal(SIGINT,latency_sigint_handler);
#else
        qCritical("--latency requires a CATCHCHALLENGER_BENCHMARK build");
        return 2;
#endif
    }

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
