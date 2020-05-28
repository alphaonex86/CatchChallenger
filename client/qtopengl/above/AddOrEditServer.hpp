#ifndef AddOrEditServer_H
#define AddOrEditServer_H

#include <QObject>
#include <QComboBox>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ComboBox.hpp"
#include "../CustomText.hpp"
#include "../CCGraphicsTextItem.hpp"
#include "../CCSliderH.hpp"
#include "../LineEdit.hpp"
#include "../SpinBox.hpp"

class AddOrEditServer : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit AddOrEditServer();
    ~AddOrEditServer();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void mouseMoveEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;

    int type() const;
    void on_ok_clicked();
    QString server() const;
    uint16_t port() const;
    QString proxyServer() const;
    uint16_t proxyPort() const;
    QString name() const;
    void setType(const int &type);
    void setEdit(const bool &edit);
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
    bool edit;
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;
    CustomButton *validate;

    ComboBox *m_type;

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
