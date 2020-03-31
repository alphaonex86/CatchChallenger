#include "Language.hpp"
#include <QTranslator>
#include <QCoreApplication>

Language Language::language;

Language::Language() :
    m_lang("en")
{
}

void Language::setLanguage(const QString &l)
{
    if(m_lang==l)
        return;
    m_lang=l;
    QTranslator *translator=new QTranslator();
    translator->load(":/CC/languages/"+m_lang+"/translation.qm");
    QCoreApplication::installTranslator(translator);
    emit newLanguage(m_lang);
}

const QString &Language::getLanguage()
{
    return m_lang;
}
