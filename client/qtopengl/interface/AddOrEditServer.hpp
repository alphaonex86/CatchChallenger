#ifndef AddOrEditServer_H
#define AddOrEditServer_H

#include <QObject>
#include <QComboBox>
#include "../CCWidget.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CCDialogTitle.hpp"
#include "../CCGraphicsTextItem.hpp"
#include "../CCSliderH.hpp"

class AddOrEditServer : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit AddOrEditServer();
    ~AddOrEditServer();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void mouseMoveEventXY(const QPointF &p, bool &pressValidated) override;

    int type() const;
    void setType(const int &type);
    void setEdit(const bool &edit);
    void on_ok_clicked();
    QString server() const;
    uint16_t port() const;
    QString proxyServer() const;
    uint16_t proxyPort() const;
    QString name() const;
    void setServer(const QString &server);
    void setPort(const uint16_t &port);
    void setName(const QString &name);
    void setProxyServer(const QString &proxyServer);
    void setProxyPort(const uint16_t &proxyPort);
    bool isOk() const;
    void on_type_currentIndexChanged(int index);
    void on_cancel_clicked();
private slots:
    void newLanguage();
private:
    CCWidget *wdialog;
    CustomButton *quit;
    CCDialogTitle *title;
    QGraphicsPixmapItem label;
    CustomButton *validate;

    QComboBox *m_type;
    QGraphicsProxyWidget *typeListProxy;
    QGraphicsTextItem *serverText;
    QGraphicsPixmapItem *serverBackground;
    CCGraphicsTextItem *serverInput;
    QGraphicsPixmapItem *portBackground;
    CCGraphicsTextItem *portInput;

    QGraphicsTextItem *nameText;
    QGraphicsPixmapItem *nameBackground;
    CCGraphicsTextItem *nameInput;

    QGraphicsTextItem *proxyText;
    QGraphicsPixmapItem *proxyBackground;
    CCGraphicsTextItem *proxyInput;
    QGraphicsPixmapItem *proxyPortBackground;
    CCGraphicsTextItem *proxyPortInput;

    QGraphicsTextItem *warning;

    int x,y;
    bool ok;

    QString serverPrevious;
    QString portPrevious;
    QString namePrevious;
    QString proxyPrevious;
    QString proxyPortPrevious;
private slots:
    void serverChange();
    void portChange();
    void nameChange();
    void proxyChange();
    void proxyPortChange();
signals:
    void quitOption();
};

#endif // OPTIONSDIALOG_H
