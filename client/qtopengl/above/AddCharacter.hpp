#ifndef AddCharacter_H
#define AddCharacter_H

#include <QObject>
#include <QComboBox>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../CCGraphicsTextItem.hpp"
#include "../CCSliderH.hpp"
#include "../LineEdit.hpp"
#include "../SpinBox.hpp"
#include "../ComboBox.hpp"

class AddCharacter : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit AddCharacter();
    ~AddCharacter();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void mouseMoveEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;

    bool isOk() const;
    void setDatapack(std::string path);
    void on_cancel_clicked();
    void on_ok_clicked();

    void loadProfileText();
    int getProfileIndex();
    int getProfileCount();
    void on_comboBox_currentIndexChanged(int index);
private slots:
    void newLanguage();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;
    CustomButton *validate;

    ComboBox *comboBox;

    QGraphicsTextItem *description;

    int x,y;
    bool ok;

    std::string datapackPath;
    struct ProfileText
    {
        std::string name;
        std::string description;
    };
    std::vector<ProfileText> profileTextList;
signals:
    void removeAbove();
};

#endif // OPTIONSDIALOG_H
