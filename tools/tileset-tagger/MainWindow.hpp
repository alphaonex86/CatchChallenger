#ifndef TILESETTAGGER_MAINWINDOW_HPP
#define TILESETTAGGER_MAINWINDOW_HPP

// The tagger window: a tileset canvas (TilesetView) + a side panel with NO
// free-text input (error-prone).  Everything is a fixed QComboBox or a QCheckBox:
//   - Category: a curated combo (the owner requests new entries, we add them).
//   - Repeat flags: checkboxes (horizontalRepeat, horizontalMiddleRepeat, ...).
//   - Size and the item-group name are AUTO-DERIVED from the selected rectangle.
// The window title + a red label always show the untagged-pixel count (the guard).

#include "MapUsageIndex.hpp"

#include <QMainWindow>
#include <vector>

class TagModel;
class TilesetView;
class MapUsageView;
class QComboBox;
class QCheckBox;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
    bool openTsx(const QString &path);

private slots:
    void onOpen();
    void onTag();
    void onClearTag();
    void onSave();
    void onSelection(int col0,int row0,int col1,int row1,int tileCount);
    void onSelectionFinished(int col0,int row0,int col1,int row1,int tileCount);
    void onMapPicked(int index);
    void onToggleUntagged(bool on);
    void onNextUntagged();
    void onSuggest();   // bootstrap: auto-tag the unambiguous terrain

private:
    TagModel *model_;
    TilesetView *view_;
    MapUsageIndex *usage_;
    MapUsageView *usageView_;
    QComboBox *categoryBox_;
    QComboBox *mapCombo_;
    QCheckBox *animated_;
    QCheckBox *hRepeat_;
    QCheckBox *hMidRepeat_;
    QCheckBox *vRepeat_;
    QCheckBox *vMidRepeat_;
    QLabel *selLabel_;
    QLabel *detectedLabel_;
    QLabel *guardLabel_;
    std::vector<MapUsageIndex::Usage> currentUsages_;
    // logical info auto-DERIVED from the maps (not tagged by hand) — attached at
    // Tag time so the generator has it for free.
    QString derivedLayer_;
    int derivedWalkable_;        // -1 unknown, 0 no, 1 yes
    bool derivedFromMaps_;
    int selC0_;
    int selR0_;
    int selC1_;
    int selR1_;
    void refreshGuard();
    void updateTitle();
    void prefillFromUsage(const std::vector<int> &ids);   // guess tag from the maps
};

#endif // TILESETTAGGER_MAINWINDOW_HPP
