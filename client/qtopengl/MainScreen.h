#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include "CustomButton.h"
#include "CCWidget.h"
#include "CCTitle.h"
#include "ScreenInput.h"
#include "../qt/FeedNews.h"

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../qt/QInfiniteBuffer.h"
#include <QAudioOutput>
#endif

namespace Ui {
class MainScreen;
}

class MainScreen : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit MainScreen();
    ~MainScreen();
    void setError(const std::string &error);
    QRectF boundingRect() const override;
private:
    QGraphicsTextItem *update;
    QGraphicsTextItem *updateStar;
    QGraphicsTextItem *updateText;
    CCTitle *title;
    CustomButton *solo;
    CustomButton *multi;
    CustomButton *options;
    CustomButton *facebook;
    CustomButton *website;
    CCWidget *news;
    QGraphicsTextItem *newsText;
    QGraphicsPixmapItem *newsWait;
    CustomButton *newsUpdate;
    QGraphicsTextItem *warning;
    QString warningString;
    std::vector<FeedNews::FeedEntry> entryList;
    uint8_t currentNewsType;
    bool haveFreshFeed;

    bool haveUpdate;
protected:
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget) override;
    void mousePressEventXY(const QPointF &p) override;
    void mouseReleaseEventXY(const QPointF &p) override;
    void updateNews();

    #ifndef __EMSCRIPTEN__
    void newUpdate(const std::string &version);
    #endif
    void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error=std::string());
    void openWebsite();
    void openFacebook();
    void openUpdate();
signals:
    void goToOptions();
    void goToSolo();
    void goToMulti();
};

#endif // MAINSCREEN_H
