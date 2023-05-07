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
    void setVisible(bool visible);
    void setFixedWidth(int w);
    int currentIndex() const;
    QVariant itemData(int index, int role = Qt::UserRole) const;
    void setItemData(int index, const QVariant &value, int role = Qt::UserRole);
    void setItemText(int index, const QString &text);
    void clear();
    int	count() const;
private:
    QComboBox *m_ComboBox;
signals:
    void currentIndexChanged(int index);
};

#endif // ComboBox_H
