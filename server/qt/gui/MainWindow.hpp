#ifndef CATCHCHALLENGER_MAINWINDOW_H
#define CATCHCHALLENGER_MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QVector>
#include <QElapsedTimer>

#include "GUIServer.hpp"
#include "../base/TinyXMLSettings.hpp"

namespace Ui {
    class MainWindow;
}

struct ChartSeries;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Programmatic equivalent of clicking the start/stop button — used
    // by main(--autostart) so testingqtserver.py can boot the server
    // headlessly without a UI click. Invokes onStartStopClicked() once.
    void autostart();

protected:
    void changeEvent(QEvent *e) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    // Real TCP listener — started/stopped from the GUI Start/Stop button.
    CatchChallenger::GUIServer server_;
    // server-properties.xml reader. NormalServerGlobal::checkSettingsFile
    // is called once at construction with this object so the file gets
    // seeded with the same defaults the headless server would write.
    TinyXMLSettings *xmlSettings_;
    bool serverRunning;
    QDateTime serverStartTime;
    QTimer *updateTimer;
    QTimer *chartTimer;
    int dbQueryCount;
    int maxDbQuerySinceStart;
    int currentPlayers;
    int maxPlayers;
    int tickCount;

    QElapsedTimer eventProcessingTimer;
    qint64 accumulatedProcessingTime;

    // Last-tick snapshots used to compute deltas from the cumulative
    // probe counters in QtDatabase / QtClient (atomic, ever-increasing).
    uint64_t lastTickDbQueries_ = 0;
    uint64_t lastChartRxBytes_ = 0;
    uint64_t lastChartTxBytes_ = 0;

    void setupNavigation();
    void setupCharts();
    void setupConnections();
    // Bind every settings widget to QSettings (~/.config/CatchChallenger/
    // server-gui.conf) for load-and-persist. Defaults match
    // NormalServerGlobal::checkSettingsFile so a fresh run shows the
    // same numbers the headless server would seed into
    // server-properties.xml.
    void wireSettings();
    void updateDashboard();
    void updateUptime();
    void addConsoleLine(const QString &text, bool bold = false);
    void addAnalyticsLine(const QString &text, bool bold = false);
    void generateChartData();
    // Settings → Datapack code combos. populateDatapackMain() lists every
    // `<datapack>/map/main/<X>/` subdir; populateDatapackSub() lists
    // `<datapack>/map/main/<currentMain>/sub/<X>/`.
    void populateDatapackMain();
    void populateDatapackSub();

private slots:
    void onNavClicked(int row);
    void onStartStopClicked();
    void onTimerTick();
    void onChartTick();
    void onCompressionLevelChanged(int value);
    void onDbTypeChanged(const QString &type);
    void onEventAdd();
    void onEventEdit();
    void onEventRemove();
    void onDbSqliteBrowse();
    void onMapVisibilityChanged(int index);
    void onEventAwake();
    void onEventAboutToBlock();
};

#endif // CATCHCHALLENGER_MAINWINDOW_H
