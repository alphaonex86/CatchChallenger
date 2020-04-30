#ifndef ComboBox_H
#define ComboBox_H

#include <QGraphicsProxyWidget>
#include <QComboBox>

class ComboBox : public QGraphicsProxyWidget
{
    Q_OBJECT
public:
    ComboBox(QGraphicsItem * parent = 0);
    ~ComboBox();
    void addItem(const QString &text, const QVariant &userData = QVariant());
    void setCurrentIndex(int index);
    int currentIndex() const;
private:
    QComboBox *m_ComboBox;
signals:
    void currentIndexChanged(int index);
};

#endif // ComboBox_H
