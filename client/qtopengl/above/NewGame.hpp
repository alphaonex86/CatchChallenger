#ifndef NewGame_H
#define NewGame_H

#include <QObject>
#include <QComboBox>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../CCGraphicsTextItem.hpp"
#include "../CCSliderH.hpp"
#include "../LineEdit.hpp"
#include "../SpinBox.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class NewGame : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit NewGame();
    ~NewGame();
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void mouseMoveEventXY(const QPointF &p, bool &pressValidated) override;
    void setDatapack(const std::string &skinPath, const std::string &monsterPath, std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup, const std::vector<uint8_t> &forcedSkin);
    void updateSkin();
    void on_cancel_clicked();
    void on_ok_clicked();
    bool haveSkin() const;
    bool isOk() const;
    void on_next_clicked();
    void on_previous_clicked();
    void on_pseudo_returnPressed();
    std::string pseudo();
    uint8_t skinId();
    uint8_t monsterGroupId();
    bool haveTheInformation();
    bool okCanBeEnabled();
    std::string skin();
    void on_pseudo_textChanged(const QString &);
private slots:
    void newLanguage();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;
    CustomButton *validate;

    CustomButton *previous;
    QList<QGraphicsPixmapItem *> centerImages;
    CustomButton *next;
    LineEdit *uipseudo;
    QGraphicsTextItem *warning;

    int x,y;

    std::vector<uint8_t> forcedSkin;
    std::string monsterPath;
    std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup;
    enum Step
    {
        Step1,
        Step2,
        StepOk,
    };
    Step step;
    bool ok;
    bool skinLoaded;
    std::vector<std::string> skinList;
    std::vector<uint8_t> skinListId;
    uint8_t currentSkin;
    uint8_t currentMonsterGroup;
    std::string skinPath;
signals:
    void removeAbove();
};

#endif // OPTIONSDIALOG_H
