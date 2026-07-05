#ifndef TextInput_H
#define TextInput_H

#include <QObject>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../LineEdit.hpp"

//A small content-sized text-entry popup in the qtopengl idiom (replaces the old
//QInputDialog::getText). Used for name entry (clan name, ...). Emits
//accepted(text) on OK/Return with a non-empty value, canceled() otherwise.
class TextInput : public ScreenInput
{
    Q_OBJECT
public:
    explicit TextInput();
    ~TextInput();

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    //titleText labels the dialog; maxLength bounds the input (0 = no bound)
    void setVar(const QString &titleText,const QString &placeholder,const int maxLength=0);
private slots:
    void onCancel();
    void onValidate();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;
    LineEdit *input;
    CustomButton *okButton;
signals:
    void setAbove(ScreenInput *widget);//first plan popup
    void accepted(const QString &text);
    void canceled();
};

#endif // TextInput_H
