#ifndef LANGUAGESSELECT_H
#define LANGUAGESSELECT_H

#include <QDialog>
#include <QHash>
#include <QString>
#include <QList>
#include <QTranslator>

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
        QString fullName;
        QString path;
    };
    QString getCurrentLanguages();
public slots:
    void show();
    void updateContent();
    int exec();
private slots:
    void on_automatic_clicked();
    QString getTheRightLanguage() const;
    void setCurrentLanguage(const QString &newLanguage);
    void on_cancel_clicked();
    void on_ok_clicked();
private:
    Ui::LanguagesSelect *ui;
    QHash<QString,Language> languagesByMainCode;
    QHash<QString,QString> languagesByShortName;
    QList<QTranslator *> installedTranslator;
    QString currentLanguage;
};

#endif // LANGUAGESSELECT_H
