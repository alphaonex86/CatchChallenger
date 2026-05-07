#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "ChartWidget.hpp"
#include "gaugewidget.h"
#include <iostream>
// Live-stats hookup that lives in general/base/ (plain C++, no Qt).
// catchchallenger-server-gui's CMakeLists defines CATCHCHALLENGER_GUI_STATS
// so these atomic counters / event ring / log capture are real.  Other
// binaries compile the same headers as empty namespaces.
#include "../../../general/base/CCGuiStats.hpp"
#include "../../../general/base/CCGuiLog.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QAbstractEventDispatcher>
#include <cmath>

#if defined(Q_OS_LINUX)
#include <malloc.h>
#elif defined(Q_OS_MAC)
#include <malloc/malloc.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#endif

static QString formatBytes(qint64 bytes) {
    if (bytes < 1024) return QString::number(bytes) + "B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + "KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + "MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + "GB";
}

static qint64 getMemoryUsage() {
#if defined(Q_OS_LINUX)
    struct mallinfo2 mi = mallinfo2();
    return mi.arena;
#elif defined(Q_OS_MAC)
    malloc_statistics_t stats;
    zone_t zone = malloc_default_zone();
    malloc_zone_statistics(zone, &stats);
    return stats.size_in_use;
#elif defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.PrivateUsage;
    }
    return 0;
#else
    return 0;
#endif
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serverRunning(false),
    dbQueryCount(0),
    maxDbQuerySinceStart(20),
    currentPlayers(0),
    maxPlayers(200),
    tickCount(0)
{
    ui->setupUi(this);
    setupNavigation();
    setupCharts();
    setupConnections();

    ui->stackedWidget->setCurrentIndex(0);
    ui->navList->setCurrentRow(0);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);
    updateTimer->start(1000);

    chartTimer = new QTimer(this);
    connect(chartTimer, &QTimer::timeout, this, &MainWindow::onChartTick);
    chartTimer->start(5000);

    accumulatedProcessingTime = 0;
    QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance();
    connect(dispatcher, &QAbstractEventDispatcher::aboutToBlock, this, &MainWindow::onEventAboutToBlock);
    connect(dispatcher, &QAbstractEventDispatcher::awake, this, &MainWindow::onEventAwake);

    addConsoleLine("Server GUI initialized");
    addConsoleLine("Waiting for server start...", false);

    addAnalyticsLine("Server dashboard loaded");
    addAnalyticsLine("No recent player actions yet");

    updateDashboard();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupNavigation()
{
    ui->navList->clear();
    QStringList items;
    items << "Dashboard" << "Gameplay" << "Performance" << "Chat"
          << "DDOS" << "Map" << "Settings" << "Database" << "Events";
    for (const QString &item : items) {
        QListWidgetItem *listItem = new QListWidgetItem(item, ui->navList);
        listItem->setSizeHint(QSize(0, 36));
    }
    ui->navList->setCurrentRow(0);
}

void MainWindow::setupCharts()
{
    QVector<ChartSeries> playerSeries;
    ChartSeries ps;
    ps.name = "Players";
    ps.color = QColor(74, 144, 217);
    ps.fillColor = QColor(74, 144, 217, 60);
    playerSeries << ps;
    ui->chartPlayers->setTitle("Last 24 hours");
    ui->chartPlayers->setSeries(playerSeries);

    QVector<ChartSeries> networkSeries;
    ChartSeries ns1, ns2;
    ns1.name = "RX";
    ns1.color = QColor(80, 200, 120);
    ns1.fillColor = QColor(80, 200, 120, 40);
    ns2.name = "TX";
    ns2.color = QColor(217, 144, 74);
    ns2.fillColor = QColor(217, 144, 74, 40);
    networkSeries << ns1 << ns2;
    ui->chartNetwork->setTitle("Network traffic");
    ui->chartNetwork->setSeries(networkSeries);
    ui->chartNetwork->setUnitSuffix("/s");

    QVector<ChartSeries> pingSeries;
    ChartSeries pMin, pAvg, pMax;
    pMin.name = "Min";
    pMin.color = QColor(80, 200, 120);
    pMin.fillColor = QColor(80, 200, 120, 20);
    pAvg.name = "Avg";
    pAvg.color = QColor(74, 144, 217);
    pAvg.fillColor = QColor(74, 144, 217, 30);
    pMax.name = "Max";
    pMax.color = QColor(217, 74, 74);
    pMax.fillColor = QColor(217, 74, 74, 20);
    pingSeries << pMin << pAvg << pMax;
    ui->chartPing->setTitle("Player ping");
    ui->chartPing->setSeries(pingSeries);
    ui->chartPing->setUnitSuffix("ms");
}

void MainWindow::setupConnections()
{
    connect(ui->navList, &QListWidget::currentRowChanged, this, &MainWindow::onNavClicked);
    connect(ui->startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(ui->compressionLevel, &QSlider::valueChanged, this, &MainWindow::onCompressionLevelChanged);
    connect(ui->dbType, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &MainWindow::onDbTypeChanged);
    connect(ui->eventAddBtn, &QPushButton::clicked, this, &MainWindow::onEventAdd);
    connect(ui->eventEditBtn, &QPushButton::clicked, this, &MainWindow::onEventEdit);
    connect(ui->eventRemoveBtn, &QPushButton::clicked, this, &MainWindow::onEventRemove);
    connect(ui->dbSqliteBrowse, &QPushButton::clicked, this, &MainWindow::onDbSqliteBrowse);
    connect(ui->mapVisibilityAlgorithm, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onMapVisibilityChanged);

    ui->dbSqliteGroup->setVisible(false);
    ui->dbMysqlGroup->setVisible(true);

    onMapVisibilityChanged(0);
}

void MainWindow::onNavClicked(int row)
{
    ui->stackedWidget->setCurrentIndex(row);
}

void MainWindow::autostart()
{
    // Programmatic Start, used by main(--autostart). Idempotent: if the
    // server is already running we leave it alone. We deliberately go
    // through onStartStopClicked() instead of duplicating its body so
    // any future logic added to the manual-click path stays in sync.
    std::cout << "[autostart] clicking Start" << std::endl;
    std::cout.flush();
    if (!serverRunning)
        onStartStopClicked();
}

void MainWindow::onStartStopClicked()
{
    serverRunning = !serverRunning;
    if (serverRunning) {
        serverStartTime = QDateTime::currentDateTime();
        ui->valueMainServer->setText("Running");
        ui->valueMainServer->setStyleSheet("color: #50c878;");
        ui->startStopButton->setText("Stop");
        ui->startStopButton->setObjectName("stopButton");
        ui->startStopButton->setStyleSheet("QPushButton#stopButton { background-color: #d94a4a; }"
                                           "QPushButton#stopButton:hover { background-color: #e95b5b; }");
        addConsoleLine("Server started successfully");
        addAnalyticsLine("Server started", true);
    } else {
        ui->valueMainServer->setText("Not Running");
        ui->valueMainServer->setStyleSheet("color: #d94a4a;");
        ui->startStopButton->setText("Start");
        ui->startStopButton->setStyleSheet("");
        addConsoleLine("Server stopped");
        addAnalyticsLine("Server stopped", true);
    }
    updateDashboard();
}

void MainWindow::onTimerTick()
{
    tickCount++;
    if (serverRunning) {
        // Real DB-query rate: cumulative counter delta since the last
        // tick.  CCGuiStats::db_query_total is bumped from
        // server/qt/db/QtDatabase + (when CATCHCHALLENGER_GUI_STATS is
        // on) the non-Qt DB layer.  No more random.
        const uint64_t dbTotal = CatchChallenger::gui_stats::db_query_total.load();
        dbQueryCount = static_cast<int>(dbTotal - lastTickDbQueries_);
        lastTickDbQueries_ = dbTotal;
        if (dbQueryCount > maxDbQuerySinceStart)
            maxDbQuerySinceStart = dbQueryCount;

        maxPlayers = ui->maxPlayer->value();

        // Real connected-player count (engine bumps it on accept /
        // disconnect).  Falls back to 0 if no engine has registered
        // yet — which means the server hasn't actually accepted
        // anybody, exactly the right answer.
        currentPlayers = qBound(0,
            static_cast<int>(CatchChallenger::gui_stats::players_connected.load()),
            maxPlayers);

        // Drain engine-pushed analytics events.  Each event becomes a
        // [HH:MM:SS] <player> <text> line.  Time mark is generated
        // here, GUI-side, per project policy ("all time mark
        // generated into server/qt/gui/").
        auto evs = CatchChallenger::gui_stats::drain_events();
        for (const auto &e : evs) {
            const QString stamp = QTime::currentTime().toString("[HH:mm:ss]");
            const QString player = QString::fromStdString(e.player);
            const QString text   = QString::fromStdString(e.text);
            QString line;
            if (player.isEmpty())
                line = QString("<span style='color: grey'>%1</span> %2")
                           .arg(stamp).arg(text);
            else
                line = QString("<span style='color: grey'>%1</span> "
                               "<b>%2</b> %3")
                           .arg(stamp).arg(player).arg(text);
            addAnalyticsLine(line, e.bold);
        }

        // Drain captured stdout / stderr.  addConsoleLine() already
        // prepends a `[HH:mm:ss]` of its own, so we hand it the bare
        // captured text — wrapping a stamp here would produce the
        // duplicate `[18:58:59] [18:58:59] …` pattern.  err lines are
        // tinted red so the operator spots crashes / warnings at a
        // glance.
        auto logs = CatchChallenger::gui_log::drain_classified_lines();
        for (const auto &l : logs) {
            const QString text = QString::fromStdString(l.text).toHtmlEscaped();
            if (l.is_err)
                addConsoleLine(QString("<span style='color: #d94a4a'>%1</span>").arg(text));
            else
                addConsoleLine(text);
        }
    }
    updateDashboard();
}

void MainWindow::onChartTick()
{
    if (!serverRunning) return;
    generateChartData();
}

void MainWindow::generateChartData()
{
    // Player chart: live count, no smoothing.
    const int playerCount = static_cast<int>(
        CatchChallenger::gui_stats::players_connected.load());
    ui->chartPlayers->addDataPoint(0, playerCount);

    // Network chart: bytes/sec since the last chart tick.  Counters
    // are cumulative; convert to delta divided by the chart interval.
    const uint64_t rxNow = CatchChallenger::gui_stats::net_bytes_received.load();
    const uint64_t txNow = CatchChallenger::gui_stats::net_bytes_sent.load();
    const double dtSecs = 5.0;  // chartTimer interval — see setupCharts().
    const double rx = (rxNow - lastChartRxBytes_) / dtSecs;
    const double tx = (txNow - lastChartTxBytes_) / dtSecs;
    lastChartRxBytes_ = rxNow;
    lastChartTxBytes_ = txNow;
    ui->chartNetwork->addDataPoint(0, rx);
    ui->chartNetwork->addDataPoint(1, tx);

    // Ping chart: engine-reported min/avg/max.  Stored in ms so no
    // unit conversion needed.  When the engine hasn't set them yet
    // (server idle, no clients) all three read 0 — chart shows a
    // flat baseline, which is the right answer.
    const double minPing = static_cast<double>(
        CatchChallenger::gui_stats::player_latency_ms_min.load());
    const double avgPing = static_cast<double>(
        CatchChallenger::gui_stats::player_latency_ms_avg.load());
    const double maxPing = static_cast<double>(
        CatchChallenger::gui_stats::player_latency_ms_max.load());
    ui->chartPing->addDataPoint(0, minPing);
    ui->chartPing->addDataPoint(1, avgPing);
    ui->chartPing->addDataPoint(2, maxPing);
}

void MainWindow::updateDashboard()
{
    updateUptime();

    ui->valueDbQuery->setText(QString::number(dbQueryCount));
    ui->progressDbQuery->setMaximum(maxDbQuerySinceStart);
    ui->progressDbQuery->setValue(dbQueryCount);
    ui->progressDbQuery->setBarColor(QColor(74, 144, 217));

    qint64 memBytes = getMemoryUsage();
    ui->valueMemory->setText(formatBytes(memBytes > 0 ? memBytes : static_cast<qint64>(1.5 * 1024 * 1024)));

    if (serverRunning) {
        // Average player latency from the engine's RTT tracker.  0ms
        // before any client has been pinged — same as old random
        // baseline when nobody's connected.
        const uint64_t pingMs =
            CatchChallenger::gui_stats::player_latency_ms_avg.load();
        ui->valueLatency->setText(QString("%1ms").arg(pingMs));
        // SQL latency: most recent query duration converted ns→ms.
        const uint64_t sqlNs =
            CatchChallenger::gui_stats::db_query_last_duration_ns.load();
        ui->valueSqlLatency->setText(QString("%1ms").arg(sqlNs / 1000000));
        ui->valueCpuLatency->setText(QString("%1ms").arg(accumulatedProcessingTime));
    } else {
        ui->valueLatency->setText("0ms");
        ui->valueSqlLatency->setText("0ms");
        ui->valueCpuLatency->setText("0ms");
    }

    accumulatedProcessingTime = 0;

    maxPlayers = ui->maxPlayer->value();
    ui->progressPlayers->setMaximum(maxPlayers);
    ui->valuePlayers->setText(QString("%1/%2").arg(currentPlayers).arg(maxPlayers));
    ui->progressPlayers->setValue(currentPlayers);
    ui->progressPlayers->setBarColor(QColor(74, 144, 217));
}

void MainWindow::updateUptime()
{
    if (!serverRunning) {
        ui->valueUptime->setText("00h00");
        return;
    }
    qint64 secs = serverStartTime.secsTo(QDateTime::currentDateTime());
    int hours = secs / 3600;
    int mins = (secs % 3600) / 60;
    ui->valueUptime->setText(QString("%1h%2").arg(hours, 2, 10, QChar('0')).arg(mins, 2, 10, QChar('0')));
}

void MainWindow::addConsoleLine(const QString &text, bool bold)
{
    ui->textEditConsole->document()->setMaximumBlockCount(500);

    QString time = QTime::currentTime().toString("HH:mm:ss");
    QString line = QString("<span style='color: grey'>[%1]</span> %2").arg(time).arg(text);

    QTextCursor cursor(ui->textEditConsole->document());
    cursor.movePosition(QTextCursor::End);

    if (bold) {
        QTextCharFormat fmt;
        fmt.setFontWeight(QFont::Bold);
        cursor.insertHtml(line + "<br>");
    } else {
        cursor.insertHtml(line + "<br>");
    }

    ui->textEditConsole->verticalScrollBar()->setValue(
        ui->textEditConsole->verticalScrollBar()->maximum());
}

void MainWindow::addAnalyticsLine(const QString &text, bool bold)
{
    ui->textEditAnalytics->document()->setMaximumBlockCount(500);

    QTextCursor cursor(ui->textEditAnalytics->document());
    cursor.movePosition(QTextCursor::End);

    if (text.contains("<")) {
        cursor.insertHtml(text + "<br>");
    } else if (bold) {
        QTextCharFormat fmt;
        fmt.setFontWeight(QFont::Bold);
        cursor.insertText(text + "\n", fmt);
    } else {
        cursor.insertText(text + "\n");
    }

    ui->textEditAnalytics->verticalScrollBar()->setValue(
        ui->textEditAnalytics->verticalScrollBar()->maximum());
}

void MainWindow::onCompressionLevelChanged(int value)
{
    ui->compressionLevelValue->setText(QString::number(value));
}

void MainWindow::onDbTypeChanged(const QString &type)
{
    bool isSqlite = (type == "SQLite");
    ui->dbSqliteGroup->setVisible(isSqlite);
    ui->dbMysqlGroup->setVisible(!isSqlite);
}

void MainWindow::onEventAdd()
{
    QString eventType = ui->eventType->currentText();
    if (eventType.isEmpty()) {
        QMessageBox::information(this, "Add Event", "Please select an event type first.");
        return;
    }
    QString freq = ui->captureFrequency->currentText();
    QString day = ui->captureDay->currentText();
    QString time = ui->captureTime->time().toString("HH:mm");
    QString itemText = QString("%1 [%2 %3 at %4]").arg(eventType).arg(freq).arg(day).arg(time);
    ui->programmedEventList->addItem(itemText);
    addConsoleLine(QString("Event added: %1").arg(itemText));
}

void MainWindow::onEventEdit()
{
    QListWidgetItem *item = ui->programmedEventList->currentItem();
    if (!item) {
        QMessageBox::information(this, "Edit Event", "Please select an event to edit.");
        return;
    }
    QString newText = QInputDialog::getText(this, "Edit Event", "Event details:", QLineEdit::Normal, item->text());
    if (!newText.isEmpty()) {
        item->setText(newText);
        addConsoleLine(QString("Event edited: %1").arg(newText));
    }
}

void MainWindow::onEventRemove()
{
    QListWidgetItem *item = ui->programmedEventList->currentItem();
    if (!item) {
        QMessageBox::information(this, "Remove Event", "Please select an event to remove.");
        return;
    }
    delete ui->programmedEventList->takeItem(ui->programmedEventList->row(item));
    addConsoleLine("Event removed");
}

void MainWindow::onDbSqliteBrowse()
{
    QString file = QFileDialog::getOpenFileName(this, "Select SQLite Database", "", "SQLite Files (*.db *.sqlite);;All Files (*)");
    if (!file.isEmpty()) {
        ui->dbSqliteFile->setText(file);
    }
}

void MainWindow::onMapVisibilityChanged(int index)
{
    Q_UNUSED(index);
}

void MainWindow::onEventAwake()
{
    eventProcessingTimer.start();
}

void MainWindow::onEventAboutToBlock()
{
    if (eventProcessingTimer.isValid()) {
        accumulatedProcessingTime += eventProcessingTimer.elapsed();
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
}
