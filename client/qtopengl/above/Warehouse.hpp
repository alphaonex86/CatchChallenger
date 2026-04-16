#ifndef Warehouse_H
#define Warehouse_H

#include <QObject>
#include <QListWidget>
#include "../ScreenInput.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../SpinBox.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class ConnexionManager;

class Warehouse : public ScreenInput
{
    Q_OBJECT
public:
    explicit Warehouse();
    ~Warehouse();

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager, const std::string &npcName, const uint8_t &skinId);
private slots:
    void newLanguage();
    void onDepositMonster();
    void onWithdrawMonster();
    void onValidate();
private:
    void updateContent();

    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    QGraphicsPixmapItem *npcImage;
    QGraphicsTextItem *npcNameText;

    QGraphicsTextItem *playerMonstersTitle;
    QListWidget *playerMonsters;
    QGraphicsProxyWidget *playerMonstersProxy;

    QGraphicsTextItem *warehouseMonstersTitle;
    QListWidget *warehouseMonsters;
    QGraphicsProxyWidget *warehouseMonstersProxy;

    CustomButton *depositBtn;
    CustomButton *withdrawBtn;
    CustomButton *validateBtn;

    ConnexionManager *connexionManager;
signals:
    void setAbove(ScreenInput *widget);
    void showTip(const std::string &tip);
};

#endif // Warehouse_H
