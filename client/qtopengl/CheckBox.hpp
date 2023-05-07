#ifndef CheckBox_H
#define CheckBox_H

#include <QGraphicsProxyWidget>
#include <QCheckBox>

class CheckBox : public QGraphicsProxyWidget
{
    Q_OBJECT
public:
    CheckBox(QGraphicsItem * parent = 0);
    ~CheckBox();
    bool isChecked() const;
    void setChecked(bool);
private:
    QCheckBox *m_checkBox;
};

#endif // CheckBox_H
