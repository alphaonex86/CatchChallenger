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
#include <QStringList>
#include <vector>

class TagModel;
class TilesetView;
class MapUsageView;
class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QScrollArea;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
    bool openTsx(const QString &path);
    void openPath(const QString &path);   // a .tsx OR a directory of tilesets
    void hideOpenButton();                // when the folder came from the CLI

protected:
    void resizeEvent(QResizeEvent *event) override;
    // refit when the SCROLL VIEWPORT actually resizes (maximise lays the central
    // area out AFTER MainWindow::resizeEvent, so reading the viewport there is stale)
    bool eventFilter(QObject *watched,QEvent *event) override;

private slots:
    void onOpen();
    void onTag();
    void onClearTag();
    void onSave();
    void onSelection(int tileCount);
    void onSelectionFinished(int tileCount);
    void onCategoryChanged(const QString &category);   // show the category's description
    void onToggleGroups(bool on);    // review mode: animated group outlines
    void onConfirmTileset();         // confirm every tag + save + go to next tileset
    void onMapPicked(int index);
    void onToggleUntagged(bool on);
    void onNextUntagged();        // jump to the next tile needing attention (red or yellow)
    void onVerify();              // apply the current settings as verified + advance
    void onSuggest();   // bootstrap: auto-tag the unambiguous terrain
    void onZoomIn();
    void onZoomOut();

private:
    TagModel *model_;
    QScrollArea *scroll_;       // central viewport host (watched for resizes)
    TilesetView *view_;
    MapUsageIndex *usage_;
    MapUsageView *usageView_;
    QPushButton *openBtn_;
    QComboBox *categoryBox_;
    QLabel *categoryDescLabel_;     // human description of the selected category
    QWidget *terrainGroup_;         // inner/outer texture pickers (only for "terrain")
    QComboBox *innerBox_;           // terrain INNER (fill) texture
    QComboBox *outerBox_;           // terrain OUTER (border) texture
    QComboBox *mapCombo_;
    QCheckBox *animated_;
    QCheckBox *randomized_;         // variant group: maps store the 1st tile, engine randomizes
    QCheckBox *composed_;           // unique tiles forming ONE fixed figure (keep arrangement)
    QCheckBox *hRepeat_;
    QCheckBox *hMidRepeat_;
    QCheckBox *vRepeat_;
    QCheckBox *vMidRepeat_;
    QCheckBox *reviewCheck_;        // toggle the animated group-review overlay
    QLabel *selLabel_;
    QLabel *detectedLabel_;
    QLabel *guardLabel_;
    bool wasComplete_;              // one-shot: fire the "all tagged" review banner once
    std::vector<MapUsageIndex::Usage> currentUsages_;
    int currentGroupNum_;          // group number of the current selection (-1 = none)
    // logical info auto-DERIVED from the maps (not tagged by hand) — attached at
    // Tag time so the generator has it for free.
    QString derivedLayer_;
    int derivedWalkable_;        // -1 unknown, 0 no, 1 yes
    bool derivedFromMaps_;
    int selC0_;
    int selR0_;
    int selC1_;
    int selR1_;
    QStringList tsxQueue_;        // when a directory is opened: all .tsx to tag
    int tsxIndex_;                // current position in tsxQueue_
    void refreshGuard();
    void updateTitle();
    void rememberTexture(const QString &name);   // add a terrain texture to both combos (for linking)
    void prefillFromUsage(const std::vector<int> &ids);   // guess tag from the maps
    int applySelection();         // apply current settings to the selection, verified
    void openNextIncomplete();    // open the next not-fully-verified tileset in the queue
    void fitViewToWindow();       // scale the tileset to the viewport width
};

#endif // TILESETTAGGER_MAINWINDOW_HPP
