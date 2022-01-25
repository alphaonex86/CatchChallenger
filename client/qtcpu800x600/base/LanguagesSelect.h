#ifndef LANGUAGESSELECT_H
#define LANGUAGESSELECT_H

#include <QDialog>
#include <QHash>
#include <QString>
#include <QList>
#include <QTranslator>
#include "unordered_map"

namespace Ui {
class LanguagesSelect;
}

class LanguagesSelect : public QDialog
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
public slots:
    void show();
    void updateContent();
    int exec();
private slots:
    void on_automatic_clicked();
    std::string getTheRightLanguage() const;
    void setCurrentLanguage(const std::string &newLanguage);
    void on_cancel_clicked();
    void on_ok_clicked();
private:
    Ui::LanguagesSelect *ui;
    std::unordered_map<std::string,Language> languagesByMainCode;
    std::unordered_map<std::string,std::string> languagesByShortName;
    std::vector<QTranslator *> installedTranslator;
    std::string currentLanguage;
};

#endif // LANGUAGESSELECT_H
