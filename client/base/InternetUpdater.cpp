#include "InternetUpdater.h"
#include "PlatformMacro.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralVariable.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QRegularExpression>

#ifdef Q_OS_UNIX
    #include <unistd.h>
    #include <sys/types.h>
#endif
#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <tchar.h>
    #include <stdio.h>
    #include <strsafe.h>
    typedef void (WINAPI *PGNSI) (LPSYSTEM_INFO);
    typedef BOOL (WINAPI *PGPI) (DWORD, DWORD, DWORD, DWORD, PDWORD);
#endif
#ifdef Q_OS_MAC
#include <QStringList>
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#endif

InternetUpdater *InternetUpdater::internetUpdater=NULL;

InternetUpdater::InternetUpdater()
{
    connect(&newUpdateTimer,&QTimer::timeout,this,&InternetUpdater::downloadFile);
    connect(&firstUpdateTimer,&QTimer::timeout,this,&InternetUpdater::downloadFile);
    newUpdateTimer.start(60*60*1000);
    firstUpdateTimer.setSingleShot(true);
    firstUpdateTimer.start(5);
}

void InternetUpdater::downloadFile()
{
    QString name="CatchChallenger";
    QString catchChallengerVersion;
    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    catchChallengerVersion=QStringLiteral("%1 Ultimate/%2").arg(name).arg(CATCHCHALLENGER_VERSION);
    #else
    catchChallengerVersion=QStringLiteral("%1/%2").arg(name).arg(CATCHCHALLENGER_VERSION);
    #endif
    #ifdef CATCHCHALLENGER_VERSION_PORTABLE
        #ifdef CATCHCHALLENGER_PLUGIN_ALL_IN_ONE
             catchChallengerVersion+=QStringLiteral(" portable/all-in-one");
        #else
             catchChallengerVersion+=QStringLiteral(" portable");
        #endif
    #else
        #ifdef CATCHCHALLENGER_PLUGIN_ALL_IN_ONE
            catchChallengerVersion+=QStringLiteral(" all-in-one");
        #endif
    #endif
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    catchChallengerVersion+=QStringLiteral(" (OS: %1)").arg(GetOSDisplayString());
    #endif
    QNetworkRequest networkRequest(QStringLiteral("%1?platform=%2").arg(CATCHCHALLENGER_UPDATER_URL).arg(CATCHCHALLENGER_PLATFORM_CODE));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,catchChallengerVersion);
    reply = qnam.get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &InternetUpdater::httpFinished);
}

void InternetUpdater::httpFinished()
{
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(reply->error())
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    } else if (!redirectionTarget.isNull()) {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        reply->deleteLater();
        return;
    }
    const QString &newVersion=QString::fromUtf8(reply->readAll());
    if(newVersion.isEmpty())
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("version string is empty"));
        reply->deleteLater();
        return;
    }
    if(!newVersion.contains(QRegularExpression(QLatin1Literal("^[0-9]+(\\.[0-9]+)+$"))))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("version string don't match: %1").arg(newVersion));
        reply->deleteLater();
        return;
    }
    if(newVersion==CATCHCHALLENGER_VERSION)
    {
        reply->deleteLater();
        return;
    }
    if(!versionIsNewer(newVersion))
    {
        reply->deleteLater();
        return;
    }
    qDebug() << "newVersion:" << newVersion;
    emit newUpdate(newVersion);
    reply->deleteLater();
}

bool InternetUpdater::versionIsNewer(const QString &version)
{
    QStringList versionANumber=version.split(QStringLiteral("."));
    QStringList versionBNumber=QStringLiteral(CATCHCHALLENGER_VERSION).split(QStringLiteral("."));
    int index=0;
    int defaultReturnValue=true;
    while(index<versionANumber.size() && index<versionBNumber.size())
    {
        unsigned int reaNumberA=versionANumber.at(index).toUInt();
        unsigned int reaNumberB=versionBNumber.at(index).toUInt();
        if(reaNumberA>reaNumberB)
            return true;
        if(reaNumberA<reaNumberB)
            return false;
        index++;
    }
    return defaultReturnValue;
}

QString InternetUpdater::getText(const QString &version)
{
    QString url;
    #if !defined(CATCHCHALLENGER_VERSION_ULTIMATE)
        url="http://catchchallenger.first-world.info/download.html";
    #else
        url="http://catchchallenger.first-world.info/shop/en/order-history";
    #endif
        return QStringLiteral("<a href=\"%1\" style=\"text-decoration:none;color:#100;\">%2</a>").arg(url).arg(tr("New version: %1").arg(QStringLiteral("<b>%1</b>").arg(version))+"<br />"+
                           #if !defined(CATCHCHALLENGER_VERSION_ULTIMATE)
                               tr("Click here to go on download page")
                           #else
                               tr("Click here to <b>go to the shop</b> and login. Download the new version <b>into the order details</b>.<br />The new version have been sended <b>by email too</b>, look into your spams if needed.")
                           #endif
                                                         );
}

#ifdef Q_OS_WIN32
QString InternetUpdater::GetOSDisplayString()
{
   QString Os;
   OSVERSIONINFOEX osvi;
   SYSTEM_INFO si;
   PGNSI pGNSI;
   PGPI pGPI;
   BOOL bOsVersionInfoEx;
   DWORD dwType;

   ZeroMemory(&si, sizeof(SYSTEM_INFO));
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

   if(bOsVersionInfoEx == NULL)
        return "Os detection blocked";

   // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

   pGNSI = (PGNSI) GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")),
      "GetNativeSystemInfo");
   if(NULL != pGNSI)
      pGNSI(&si);
   else GetSystemInfo(&si);

   if(VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && osvi.dwMajorVersion>4)
   {
      if(osvi.dwMajorVersion==6)
      {
          switch(osvi.dwMinorVersion)
          {
            case 0:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+="Windows Vista ";
                else Os+="Windows Server 2008 " ;
            break;
            case 1:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+="Windows 7 ";
                else Os+="Windows Server 2008 R2 ";
            break;
            case 2:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+="Windows 8 ";
                else Os+="Windows Server 2012 ";
            break;
            default:
                 if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
                 else Os+=QStringLiteral("Windows Server (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
            break;
          }

         pGPI = (PGPI) GetProcAddress(
            GetModuleHandle(TEXT("kernel32.dll")),
            "GetProductInfo");

         pGPI(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

         switch(dwType)
         {
            case PRODUCT_ULTIMATE:
               Os+="Ultimate Edition";
               break;
            case PRODUCT_PROFESSIONAL:
               Os+="Professional";
               break;
            case PRODUCT_HOME_PREMIUM:
               Os+="Home Premium Edition";
               break;
            case PRODUCT_HOME_BASIC:
               Os+="Home Basic Edition";
               break;
            case PRODUCT_ENTERPRISE:
               Os+="Enterprise Edition";
               break;
            case PRODUCT_BUSINESS:
               Os+="Business Edition";
               break;
            case PRODUCT_STARTER:
               Os+="Starter Edition";
               break;
            case PRODUCT_CLUSTER_SERVER:
               Os+="Cluster Server Edition";
               break;
            case PRODUCT_DATACENTER_SERVER:
               Os+="Datacenter Edition";
               break;
            case PRODUCT_DATACENTER_SERVER_CORE:
               Os+="Datacenter Edition (core installation)";
               break;
            case PRODUCT_ENTERPRISE_SERVER:
               Os+="Enterprise Edition";
               break;
            case PRODUCT_ENTERPRISE_SERVER_CORE:
               Os+="Enterprise Edition (core installation)";
               break;
            case PRODUCT_ENTERPRISE_SERVER_IA64:
               Os+="Enterprise Edition for Itanium-based Systems";
               break;
            case PRODUCT_SMALLBUSINESS_SERVER:
               Os+="Small Business Server";
               break;
            case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
               Os+="Small Business Server Premium Edition";
               break;
            case PRODUCT_STANDARD_SERVER:
               Os+="Standard Edition";
               break;
            case PRODUCT_STANDARD_SERVER_CORE:
               Os+="Standard Edition (core installation)";
               break;
            case PRODUCT_WEB_SERVER:
               Os+="Web Server Edition";
               break;
         }
      }
      else if(osvi.dwMajorVersion==5)
      {
            switch(osvi.dwMinorVersion)
            {
                case 0:
                    Os+="Windows 2000 ";
                    if(osvi.wProductType==VER_NT_WORKSTATION)
                       Os+="Professional";
                    else
                    {
                       if(osvi.wSuiteMask & VER_SUITE_DATACENTER)
                          Os+="Datacenter Server";
                       else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                          Os+="Advanced Server";
                       else Os+="Server";
                    }
                break;
                case 1:
                    Os+="Windows XP ";
                    if(osvi.wSuiteMask & VER_SUITE_PERSONAL)
                       Os+="Home Edition";
                    else Os+="Professional";
                break;
                case 2:
                    if(GetSystemMetrics(SM_SERVERR2))
                        Os+="Windows Server 2003 R2, ";
                    else if(osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
                        Os+="Windows Storage Server 2003";
                    else if(osvi.wSuiteMask & VER_SUITE_WH_SERVER )
                        Os+="Windows Home Server";
                    else if(osvi.wProductType==VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
                        Os+="Windows XP Professional x64 Edition";
                    else Os+="Windows Server 2003, ";
                    // Test for the server type.
                    if(osvi.wProductType!=VER_NT_WORKSTATION )
                    {
                        if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64)
                        {
                            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                                Os+="Datacenter Edition for Itanium-based Systems";
                            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                                Os+="Enterprise Edition for Itanium-based Systems";
                        }
                        else if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
                        {
                            if(osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                Os+="Datacenter x64 Edition";
                            else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                Os+="Enterprise x64 Edition";
                            else Os+="Standard x64 Edition";
                        }
                        else
                        {
                            if(osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER)
                                Os+="Compute Cluster Edition";
                            else if( osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                Os+="Datacenter Edition";
                            else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                Os+="Enterprise Edition";
                            else if(osvi.wSuiteMask & VER_SUITE_BLADE)
                                Os+="Web Edition";
                            else Os+="Standard Edition";
                        }
                    }
                break;
            }
        }
        else
        {
            if(osvi.wProductType==VER_NT_WORKSTATION)
                Os+=QStringLiteral("Windows (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
            else Os+=QStringLiteral("Windows Server (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
        }

        // Include service pack (if any) and build number.
        QString QszCSDVersion=QString::fromUtf16((ushort*)osvi.szCSDVersion);
        if(!QszCSDVersion.isEmpty())
            Os+=QStringLiteral(" %1").arg(QszCSDVersion);
        Os+=QStringLiteral(" (build %1)").arg(osvi.dwBuildNumber);
        if(osvi.dwMajorVersion >= 6)
        {
            if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
                Os+=", 64-bit";
            else if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL)
                Os+=", 32-bit";
        }
    }
    else
    {
       if(osvi.wProductType==VER_NT_WORKSTATION)
           Os+=QStringLiteral("Windows (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
       else Os+=QStringLiteral("Windows Server (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
    }
    return Os;
}
#endif

#ifdef Q_OS_MAC
QString InternetUpdater::GetOSDisplayString()
{
        QStringList key;
    QStringList string;
    QFile xmlFile("/System/Library/CoreServices/SystemVersion.plist");
    if(xmlFile.open(QIODevice::ReadOnly))
    {
        QString content=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine;
        int errorColumn;
        QDomDocument domDocument;
        if (!domDocument.setContent(content, false, &errorStr,&errorLine,&errorColumn))
            return "Mac OS X";
        else
        {
            QDomElement root = domDocument.documentElement();
            if(root.tagName()!="plist")
                return "Mac OS X";
            else
            {
                if(root.isElement())
                {
                    QDomElement SubChild=root.firstChildElement("dict");
                    while(!SubChild.isNull())
                    {
                        if(SubChild.isElement())
                        {
                            QDomElement SubChild2=SubChild.firstChildElement("key");
                            while(!SubChild2.isNull())
                            {
                                if(SubChild2.isElement())
                                    key << SubChild2.text();
                                else
                                    return "Mac OS X";
                                SubChild2 = SubChild2.nextSiblingElement("key");
                            }
                            SubChild2=SubChild.firstChildElement("string");
                            while(!SubChild2.isNull())
                            {
                                if(SubChild2.isElement())
                                    string << SubChild2.text();
                                else
                                    return "Mac OS X";
                                SubChild2 = SubChild2.nextSiblingElement("string");
                            }
                        }
                        else
                            return "Mac OS X";
                        SubChild = SubChild.nextSiblingElement("property");
                    }
                }
                else
                    return "Mac OS X";
            }
        }
    }
    if(key.size()!=string.size())
        return "Mac OS X";
    int index=0;
    while(index<key.size())
    {
        if(key.at(index)=="ProductVersion")
            return "Mac OS X "+string.at(index);
        index++;
    }
    return "Mac OS X";
}
#endif
