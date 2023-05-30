#ifndef Crafting_H
#define Crafting_H

#include <QObject>
#include <QListWidget>
#include "../../ImagesStrechMiddle.hpp"
#include "../../ScreenInput.hpp"
#include "../../CustomButton.hpp"
#include "../../CustomText.hpp"
#include "../../CCGraphicsTextItem.hpp"
#include "../../CCSliderH.hpp"
#include "../../LineEdit.hpp"
#include "../../SpinBox.hpp"
#include "../../../../general/base/GeneralStructures.hpp"

class ConnexionManager;

class Crafting : public ScreenInput
{
    Q_OBJECT
public:
    explicit Crafting();
    ~Crafting();

    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager, int lastItemSelected=-1);

    void on_listCraftingList_itemSelectionChanged();
    void inventoryUse_slot();
    void recipeUsed(const CatchChallenger::RecipeUsage &recipeUsage);
private slots:
    void newLanguage();
    void updateCrafting();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    CustomButton *next;
    CustomButton *back;

    QGraphicsTextItem *products_title;
    QGraphicsTextItem *materials_title;

    QListWidget *listCraftingList;
    QGraphicsProxyWidget *productsListProxy;
    QListWidget *listCraftingMaterials;
    QGraphicsProxyWidget *materialsListProxy;

    ImagesStrechMiddle *descriptionBack;
    QGraphicsTextItem *inventory_description;
    CustomButton *craftingUse;

    ConnexionManager *connexionManager;
    uint8_t lastItemSize;
    int lastItemSelected;
    //crafting
    std::vector<std::vector<std::pair<uint16_t,uint32_t> > > materialOfRecipeInUsing;
    std::vector<std::pair<uint16_t,uint32_t> > productOfRecipeInUsing;
signals:
    void setAbove(ScreenInput *widget);//first plan popup
    void sendNext();
    void sendBack();
    void useRecipe(uint16_t recipe);
    void add_to_inventory(const uint16_t &item, const uint32_t &quantity=1, const bool &showGain=true);
    void remove_to_inventory(const uint16_t &itemId, const uint32_t &quantity=1);
    void appendReputationRewards(const std::vector<CatchChallenger::ReputationRewards> &reputationList);
};

#endif // OPTIONSDIALOG_H
