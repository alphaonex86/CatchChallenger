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
    //Hard onboarding window: how long to wait for ALL bots to reach the map
    //before giving up. Default 60s suits fast servers; a slow constrained
    //target (e.g. the ESP32 all-in-one, single-threaded over WiFi) onboards N
    //clients much slower, so the harness raises this for that node.
    QCommandLineOption mapTimeoutOption("map-timeout-seconds", "Seconds to wait for all bots to reach the map before aborting (default 60)", "seconds", "60");

    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(botsOption);
    parser.addOption(loginOption);
    parser.addOption(passOption);
    parser.addOption(latencyOption);
    parser.addOption(latencySecondsOption);
    parser.addOption(mapTimeoutOption);

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
        int mapTimeoutSeconds = 60;
        if(parser.isSet(mapTimeoutOption))
        {
            bool ok=false;
            const int s=parser.value(mapTimeoutOption).toInt(&ok);
            if(ok && s>0)
                mapTimeoutSeconds=s;
        }
        w.autoConnect(host, port, bots, login, pass, mapTimeoutSeconds);
    }

    return a.exec();
}
