#ifndef NewGame_H
#define NewGame_H

#include <QObject>
#include <QComboBox>
#include "../CCWidget.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CCDialogTitle.hpp"
#include "../CCGraphicsTextItem.hpp"
#include "../CCSliderH.hpp"
#include "../LineEdit.hpp"
#include "../SpinBox.hpp"

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
    bool haveSkin() const;
    bool isOk() const;
    std::string pseudo();
    uint8_t skinId();
    uint8_t monsterGroupId();
private slots:
    void newLanguage();
private:
    bool edit;
    CCWidget *wdialog;
    CustomButton *quit;
    CCDialogTitle *title;
    QGraphicsPixmapItem label;
    CustomButton *validate;

    QComboBox *m_type;
    QGraphicsProxyWidget *typeListProxy;

    QGraphicsTextItem *serverText;
    LineEdit *serverInput;
    SpinBox *portInput;

    QGraphicsTextItem *nameText;
    LineEdit *nameInput;

    QGraphicsTextItem *proxyText;
    LineEdit *proxyInput;
    SpinBox *proxyPortInput;

    QGraphicsTextItem *warning;

    int x,y;
    bool ok;

    QString serverPrevious;
    QString portPrevious;
    QString namePrevious;
    QString proxyPrevious;
    QString proxyPortPrevious;
signals:
    void removeAbove();
};

#endif // OPTIONSDIALOG_H
