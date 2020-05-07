#ifndef AddCharacter_H
#define AddCharacter_H

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

class AddCharacter : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit AddCharacter();
    ~AddCharacter();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void mouseMoveEventXY(const QPointF &p, bool &pressValidated) override;

    unsigned int getProfileIndex();
    bool isOk() const;
    void setDatapack(std::string path);
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
