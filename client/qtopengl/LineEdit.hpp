#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QGraphicsProxyWidget>
#include <QLineEdit>

class LineEdit : public QGraphicsProxyWidget
{
    Q_OBJECT
public:
    LineEdit(QGraphicsItem * parent = 0);
    ~LineEdit();
    void setText(const QString &text);
    void setStyleSheet(const QString& styleSheet);
    void setMinimumSize(int minw, int minh);
    void setMaximumSize(int maxw, int maxh);
    void setFixedWidth(int w);
    void setFixedHeight(int h);
    void setFixedSize(const QSize &s);
    void setFixedSize(int w, int h);
    void setPlaceholderText(const QString &);
    void setEchoMode(QLineEdit::EchoMode e);
    int width() const;
    int height() const;
    QString text() const;
private:
    QLineEdit *m_lineEdit;
signals:
    void textChanged(const QString &);
};

#endif // LINEEDIT_H
