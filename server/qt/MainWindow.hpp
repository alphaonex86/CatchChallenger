#ifndef CATCHCHALLENGER_MAINWINDOW_H
#define CATCHCHALLENGER_MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QListWidgetItem>
#include <QTimer>

#include "NormalServer.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/TinyXMLSettings.hpp"
#include "../../general/base/GeneralStructures.hpp"

namespace Ui {
    class MainWindow;
}
namespace CatchChallenger {

// MainWindow — Qt Widgets admin console for catchchallenger-server-gui.
//
// Trimmed down to the controls that still match the current server API
// after the qmake → CMake migration: start / stop / restart, listen
// port + max-players display, online-player list, and a chat panel.
// The full settings tabs from the qmake-era MainWindow are still
// present in the .ui (they're just static widgets), but their
// per-field setters / getters depended on `CommonSettingsServer`,
// `GameServerSettings::compressionLevel`, `FightSync_*`,
// `MapVisibilityAlgorithmSelection_Simple`, and other identifiers that
// were renamed or removed when the settings struct was split. Those
// hooks are dropped here; reviving them is a separate effort once the
// new naming has stabilised.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Programmatic equivalent of clicking "Start server" — invoked by
    // main(--autostart) so testingqtserver.py can boot the server
    // without a clickable UI.
    void autostart();

protected:
    void changeEvent(QEvent *e) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    NormalServer server;
    TinyXMLSettings *settings;
    bool need_be_closed;

    void updateActionButton();

private slots:
    void on_pushButton_server_start_clicked();
    void on_pushButton_server_stop_clicked();
    void on_pushButton_server_restart_clicked();

    void server_is_started(bool is_started);
    void server_need_be_stopped();
    void server_need_be_restarted();
    void new_player_is_connected(Player_private_and_public_informations player);
    void player_is_disconnected(std::string pseudo);
    void new_chat_message(std::string pseudo,Chat_type type,std::string text);
    void server_error(std::string error);
    void haveQuitForCriticalDatabaseQueryFailed();
};

}// namespace CatchChallenger

#endif // CATCHCHALLENGER_MAINWINDOW_H
