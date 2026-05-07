#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "ChartWidget.hpp"
#include "gaugewidget.h"
#include <iostream>

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
        dbQueryCount = QRandomGenerator::global()->bounded(0, 21);
        if (dbQueryCount > maxDbQuerySinceStart)
            maxDbQuerySinceStart = dbQueryCount;

        maxPlayers = ui->maxPlayer->value();

        double basePlayers = 15 + sin(tickCount * 0.05) * 10;
        currentPlayers = qBound(0, static_cast<int>(basePlayers + QRandomGenerator::global()->bounded(-3, 4)), maxPlayers);

        if (tickCount % 30 == 0) {
            QStringList names = {"Ash", "Misty", "Brock", "Pikachu", "Charizard", "Greninja", "Lucario", "Eevee"};
            QStringList actions = {"joined the server", "left the game", "caught a wild Pokemon", "started a battle",
                                   "completed a quest", "traded a monster"};
            QString name = names[QRandomGenerator::global()->bounded(names.size())];
            QString action = actions[QRandomGenerator::global()->bounded(actions.size())];
            int mins = QRandomGenerator::global()->bounded(1, 60);
            QString time = QTime::currentTime().toString("HH:mm:ss");
            addAnalyticsLine(QString("<b>%1</b> %2 <span style='color: grey'>(%3 min ago)</span> [%4]")
                                .arg(name).arg(action).arg(mins).arg(time));
        }

        if (tickCount % 15 == 0) {
            QString logMsg;
            int msgType = QRandomGenerator::global()->bounded(4);
            if (msgType == 0) logMsg = QString("[Player] %1 connected").arg(QRandomGenerator::global()->bounded(1, 50));
            else if (msgType == 1) logMsg = "[DB] Query executed in " + QString::number(QRandomGenerator::global()->bounded(1, 15)) + "ms";
            else if (msgType == 2) logMsg = "[Network] Packet processed: " + formatBytes(QRandomGenerator::global()->bounded(100, 5000));
            else logMsg = "[System] Heartbeat OK";
            addConsoleLine(logMsg);
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
    int playerCount = QRandomGenerator::global()->bounded(5, 50);
    ui->chartPlayers->addDataPoint(0, playerCount);

    double rx = 50.0 + QRandomGenerator::global()->generateDouble() * 450.0;
    double tx = 30.0 + QRandomGenerator::global()->generateDouble() * 270.0;
    ui->chartNetwork->addDataPoint(0, rx);
    ui->chartNetwork->addDataPoint(1, tx);

    double minPing = 5.0 + QRandomGenerator::global()->generateDouble() * 20.0;
    double avgPing = minPing + 10.0 + QRandomGenerator::global()->generateDouble() * 30.0;
    double maxPing = avgPing + 20.0 + QRandomGenerator::global()->generateDouble() * 60.0;
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
        int latency = 5 + QRandomGenerator::global()->bounded(15);
        ui->valueLatency->setText(QString("%1ms").arg(latency));
        ui->valueSqlLatency->setText(QString("%1ms").arg(2 + QRandomGenerator::global()->bounded(8)));
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
