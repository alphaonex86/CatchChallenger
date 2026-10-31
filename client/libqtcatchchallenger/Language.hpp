#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <QString>
#include <QObject>
#include <QTranslator>
#include <string>
#include <unordered_map>

#include "../../general/base/lib.h"

class DLL_PUBLIC Language : public QObject
{
    Q_OBJECT
public:
    Language();
    static Language language;
    void setLanguage(const QString &l);
    const QString &getLanguage() const;
    //the saved language if available, else the system one (language name, then locale name), else "en"
    static std::string pickLanguage(const std::string &savedLanguage,const bool savedLanguageAvailable,
                                    const std::unordered_map<std::string,std::string> &languagesByShortName,
                                    const std::string &systemLanguageName,const std::string &systemLocaleName);
signals:
    void newLanguage(const QString &l);
private:
    QString m_lang;
    QList<QTranslator> translators;
};

#endif // LANGUAGE_H
