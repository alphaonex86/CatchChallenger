#include "GlobalControler.h"
#include <QCoreApplication>
#include <iostream>

/* return code:
 * 23: server or charactersGroupIndex not found
 * 24: charactersGroupIndex not found
 * 25: character not found
 * 26: receive an error message
 * 27: Your password need to be at minimum of 6 characters
 * 28: Your login need to be at minimum of 3 characters
 * 29: Query too slow to responds
 * 30: Timeout
 */

#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
extern "C" {
const char* __asan_default_options() { return "alloc_dealloc_mismatch=0:detect_container_overflow=0:detect_leaks=0"; }
}  // extern "C"
#  endif
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    std::string config=QCoreApplication::applicationDirPath().toStdString();
    if(!QCoreApplication::applicationDirPath().endsWith("/"))
        config+="/";
    config+="bottest.xml";
    if(argc==2)
    {
        QString v(argv[1]);
        v=v.replace("\\\\","\\");
        v=v.replace("//","/");
        config=v.toStdString();
    }
    std::cerr << "\e[32m\e[1m" << config << "\e[39m\e[0m" << std::endl;
    a.setOrganizationDomain("CatchChallenger");
    a.setOrganizationName("bot-test-connect-to-gameserver");
    {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(proxy);
    }
    GlobalControler w(config);
    return a.exec();
}
