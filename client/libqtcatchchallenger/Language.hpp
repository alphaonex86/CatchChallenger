#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <QString>
#include <QObject>
#include <QTranslator>

class Language : public QObject
{
    Q_OBJECT
public:
    Language();
    static Language language;
    void setLanguage(const QString &l);
    const QString &getLanguage() const;
signals:
    void newLanguage(const QString &l);
private:
    QString m_lang;
    QList<QTranslator> translators;
};

#endif // LANGUAGE_H
