#include "Language.hpp"
#include <QTranslator>
#include <QCoreApplication>
#include <iostream>

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
    if(!translator->load(":/CC/languages/"+m_lang+"/translation.qm"))
        std::cerr << "unable to load :/CC/languages/" << m_lang.toStdString() << "/translation.qm" << std::endl;
    QCoreApplication::installTranslator(translator);
    emit newLanguage(m_lang);
}

const QString &Language::getLanguage() const
{
    return m_lang;
}
