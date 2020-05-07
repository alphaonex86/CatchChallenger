#ifndef CharacterList_H
#define CharacterList_H

#include <QString>
#include <QHash>
#include <QSet>
#include <vector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QListWidget>
#include <QGraphicsProxyWidget>
#include "../CCWidget.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ConnexionInfo.hpp"
#include "../CCScrollZone.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class AddCharacter;
class NewGame;
class ConnexionManager;

class CharacterList : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit CharacterList();
    ~CharacterList();
    void displayServerList();
    void add_clicked();
    void add_finished();
    void newGame_finished();
    void select_clicked();
    void remove_clicked();
    void newLanguage();
    void updateCharacterList();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void connectToSubServer(const int indexSubServer, ConnexionManager *connexionManager);
    void newCharacterId(const uint8_t &returnCode, const uint32_t &characterId);
private:
    AddCharacter *addCharacter;
    NewGame *newGame;

    CustomButton *add;
    CustomButton *remove;
    CustomButton *select;
    CustomButton *back;
    QGraphicsTextItem *warning;

    QListWidget *characterEntryList;
    QGraphicsProxyWidget *characterEntryListProxy;

    CCWidget *wdialog;
    unsigned int serverSelected;
    ConnexionManager *connexionManager;
    std::unordered_map<uint8_t/*character group index*/,std::pair<uint8_t/*server count*/,uint8_t/*temp Index to display*/> > serverByCharacterGroup;
    std::vector<std::vector<CatchChallenger::CharacterEntry> > characterListForSelection;
    std::vector<CatchChallenger::CharacterEntry> characterEntryListInWaiting;
    unsigned int profileIndex;
signals:
    void backSubServer();
    void setAbove(ScreenInput *widget);//first plan popup
    void selectCharacter(const int indexSubServer,const int indexCharacter);
};

#endif // MULTI_H
