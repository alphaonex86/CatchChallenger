#ifndef LANGUAGESSELECT_H
#define LANGUAGESSELECT_H

#ifndef CATCHCHALLENGER_BOT

#include <QHash>
#include <QString>
#include <QList>
#include <QTranslator>
#include "unordered_map"

class LanguagesSelect : public QObject
{
    Q_OBJECT
public:
    static LanguagesSelect *languagesSelect;
    explicit LanguagesSelect();
    ~LanguagesSelect();
    struct Language
    {
        std::string fullName;
        std::string path;
    };
    std::string getCurrentLanguages();
private slots:
    std::string getTheRightLanguage() const;
    void setCurrentLanguage(const std::string &newLanguage);
private:
    std::unordered_map<std::string,Language> languagesByMainCode;
    std::unordered_map<std::string,std::string> languagesByShortName;
    std::vector<QTranslator *> installedTranslator;
    std::string currentLanguage;
};

#endif

#endif // LANGUAGESSELECT_H
