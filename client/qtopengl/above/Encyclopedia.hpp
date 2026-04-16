#ifndef Encyclopedia_H
#define Encyclopedia_H

#include <QObject>
#include <QListWidget>
#include "../ScreenInput.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../CheckBox.hpp"

class ConnexionManager;

class Encyclopedia : public ScreenInput
{
    Q_OBJECT
public:
    explicit Encyclopedia();
    ~Encyclopedia();

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager);
private slots:
    void newLanguage();
    void on_monsterList_itemSelectionChanged();
    void on_itemList_itemSelectionChanged();
    void onMonsterTab();
    void onItemTab();
    void onKnownFilterChanged();
private:
    void populateMonsterList();
    void populateItemList();

    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    CustomButton *monsterTab;
    CustomButton *itemTab;

    QListWidget *monsterList;
    QGraphicsProxyWidget *monsterListProxy;
    QListWidget *itemList;
    QGraphicsProxyWidget *itemListProxy;

    ImagesStrechMiddle *descriptionBack;
    QGraphicsTextItem *description;

    CheckBox *knownOnlyFilter;

    ConnexionManager *connexionManager;
    bool showingMonsters;// true=monsters tab, false=items tab
signals:
    void setAbove(ScreenInput *widget);
};

#endif // Encyclopedia_H
