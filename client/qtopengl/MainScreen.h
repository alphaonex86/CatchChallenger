#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include "CustomButton.h"
#include "CCWidget.h"
#include "CCTitle.h"
#include "../qt/FeedNews.h"

namespace Ui {
class MainScreen;
}

class MainScreen : public QWidget
{
    Q_OBJECT
public:
    explicit MainScreen(QWidget *parent = nullptr);
    ~MainScreen();
    void setError(const std::string &error);
private:
    Ui::MainScreen *ui;

    QLabel *update;
    QLabel *updateStar;
    QLabel *updateText;
    CCTitle *title;
    CustomButton *updateButton;
    CustomButton *solo;
    CustomButton *multi;
    CustomButton *options;
    CustomButton *facebook;
    CustomButton *website;
    CCWidget *news;
    QLabel *newsText;
    QLabel *newsWait;
    CustomButton *newsUpdate;
    QHBoxLayout *verticalLayoutNews;

    bool haveUpdate;
protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

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
