!include Library.nsh

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "CatchChallenger"
!define PRODUCT_VERSION "X.X.X.X"
!define PRODUCT_PUBLISHER "CatchChallenger"
!define PRODUCT_WEB_SITE "http://catchchallenger.first-world.info/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

RequestExecutionLevel admin

SetCompressor /FINAL /SOLID lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
; !insertmacro MUI_PAGE_LICENSE "COPYING.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "setup.exe"
InstallDir "$PROGRAMFILES\CatchChallenger"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "SectionPrincipale" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on
  File "catchchallenger-CATCHCHALLENGER_SUBVERSION.exe"
  CreateDirectory "$SMPROGRAMS\CatchChallenger"
  CreateShortCut "$SMPROGRAMS\CatchChallenger\CatchChallenger.lnk" "$INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe"
  File /r /x *.nsi /x setup.exe *
SectionEnd

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\CatchChallenger\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstFailed
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "To remove $(^Name) from the computer, close the application and remove manualy the folder"
FunctionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) have been uninstall from the computer."
FunctionEnd

Function .onInit
 
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
  "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort
 
;Run the uninstaller
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file
 
  IfErrors no_remove_uninstaller done
    ;You can either use Delete /REBOOTOK in the uninstaller or add some code
    ;here to remove the uninstaller. Use a registry key to check
    ;whether the user has chosen to uninstall. If you are using an uninstaller
    ;components page, make sure all sections are uninstalled.
  no_remove_uninstaller:
 
done:
 
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Êtes-vous certains de vouloir désinstaller totalement $(^Name) et tous ses composants ?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  IfFileExists "$INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe" CloseProgram
  Abort "The original application $INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe is not found"
  Goto NotLaunched
  CloseProgram:
  ExecWait '"$INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe" quit' $0
  IntCmp $0 0 NotLaunched
  DetailPrint "Waiting Close..."
  CloseLoop:
    Sleep 200
    ExecWait '"$INSTDIR\catchchallenger-CATCHCHALLENGER_SUBVERSION.exe" quit' $0
    IntCmp $0 0 NotLaunched
    Goto CloseLoop

  NotLaunched:

  DeleteRegKey HKCU "Software\CatchChallenger"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "catchchallenger"
  Delete "$SMPROGRAMS\CatchChallenger\Uninstall.lnk"
  Delete "$SMPROGRAMS\CatchChallenger\CatchChallenger.lnk"
  RMDir /REBOOTOK /r "$SMPROGRAMS\CatchChallenger"
  RMDir /REBOOTOK /r "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
