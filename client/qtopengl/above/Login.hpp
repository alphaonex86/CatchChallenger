#ifndef Login_H
#define Login_H

#include <QString>
#include <QHash>
#include <QSet>
#include <vector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ConnexionInfo.hpp"
#include "../CCScrollZone.hpp"
#include "../LineEdit.hpp"
#include "../CheckBox.hpp"
#include "../CustomText.hpp"

class Login : public ScreenInput
{
    Q_OBJECT
public:
    explicit Login();
    ~Login();
    QRectF boundingRect() const;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void setAuth(const QStringList &list);
    void setLinks(const QString &site_page,const QString &register_page);
    QStringList getAuth();
    QString getLogin();
    QString getPass();
    bool isOk();

    void newLanguage();
    void validate();
private:
    ImagesStrechMiddle *wdialog;
    QGraphicsPixmapItem label;
    CustomText *title;

    QGraphicsTextItem *warning;
    QGraphicsTextItem *loginText;
    LineEdit *login;
    QGraphicsTextItem *passwordText;
    LineEdit *password;
    CheckBox *remember;
    QGraphicsTextItem *rememberText;
    QGraphicsTextItem *htmlText;

    CustomButton *back;
    CustomButton *server_select;

    int x,y;
    QStringList m_auth;
    bool validated;
signals:
    void removeAbove();
};

#endif // Login_H
