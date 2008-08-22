;Include Modern UI
!include "MUI.nsh"

;The name of the installer
  Name "Cube Server Lister 0.8.1"

;The file to write
  OutFile "CSL-Installer-v0.8.1.exe"

;The default installation directory
  InstallDir "$PROGRAMFILES\CSL"

;Registry key to check for directory (so if you install again, it will 
;overwrite the old one automatically)
  InstallDirRegKey HKLM "Software\CSL" "Install_Dir"

;Compressor
  SetCompressor lzma


;Images
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP         "src\img\install_header.bmp"
  !define MUI_HEADERIMAGE_UNBITMAP       "src\img\install_header.bmp"
  !define MUI_ICON                       "src\img\csl_48.ico"
  !define MUI_UNICON                     "src\img\uninstall.ico"


;Finish page  
  !define MUI_FINISHPAGE_RUN $INSTDIR\csl.exe
  !define MUI_FINISHPAGE_RUN_TEXT $(Finish_Run)


;Pages
  !insertmacro MUI_PAGE_LICENSE "COPYING"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES  
  !insertmacro MUI_PAGE_FINISH
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES


;Languages
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "German"

  LangString Finish_Run ${LANG_ENGLISH} "Start Cube Server Lister."
  LangString Finish_Run ${LANG_GERMAN} "Cube Server Lister jetzt starten."


;The stuff to install
  Section $(TITLE_Section1) SecProgram
    SectionIn RO
    ; Set output path to the installation directory.
    SetOutPath $INSTDIR
    File release\csl.exe
    SetOutPath $INSTDIR\data
    File data\GeoIP.dat
    File data\csl_start_sb.ogz
  
    ; Write the installation path into the registry
    WriteRegStr HKLM SOFTWARE\CSL "Install_Dir" "$INSTDIR"  
    ; Write the uninstall keys for Windows
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CSL" "DisplayName" "CSL"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CSL" "UninstallString" '"$INSTDIR\csluninstall.exe"'
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CSL" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CSL" "NoRepair" 1
    WriteUninstaller "csluninstall.exe"
  SectionEnd


  Section $(TITLE_Section2) SecMapCfgTool
    SetOutPath $INSTDIR
    File release\cslmapcfgtool.exe
  SectionEnd


  Section $(TITLE_Section3) SecMapPreview
    SetOutPath $INSTDIR\data\maps
    File /r data\maps\*.cfg
    File /r data\maps\*.png
  SectionEnd


  Section $(TITLE_Section4) SecMapLang
    SetOutPath $INSTDIR\lang\cs
    File po\cs\csl.mo
    SetOutPath $INSTDIR\lang\de
    File po\de\csl.mo
    SetOutPath $INSTDIR\lang\nl
    File po\nl\csl.mo
  SectionEnd


  SectionGroup /e $(TITLE_Section5) SecShortcut

  Section $(TITLE_Section5a)
    ; Set output path to the installation directory again - necessary for shortcuts.
    SetOutPath $INSTDIR
    CreateDirectory "$SMPROGRAMS\Cube Server Lister"
    CreateShortCut "$SMPROGRAMS\Cube Server Lister\Cube Server Lister.lnk" "$INSTDIR\csl.exe" "" "$INSTDIR\csl.exe" 0
    CreateShortCut "$SMPROGRAMS\Cube Server Lister\Uninstall.lnk" "$INSTDIR\csluninstall.exe" "" "$INSTDIR\csluninstall.exe" 0
  SectionEnd

  Section  $(TITLE_Section5b)
    ; Set output path to the installation directory again - necessary for shortcuts.
    SetOutPath $INSTDIR
    CreateShortCut "$DESKTOP\Cube Server Lister.lnk" "$INSTDIR\csl.exe" "" "$INSTDIR\csl.exe" 0
  SectionEnd

  SectionGroupEnd

  ;Descriptions
    LangString TITLE_Section1  ${LANG_ENGLISH} "Program"
    LangString TITLE_Section2  ${LANG_ENGLISH} "Map config tool"
    LangString TITLE_Section3  ${LANG_ENGLISH} "Map preview files"
    LangString TITLE_Section4  ${LANG_ENGLISH} "Language files"
    LangString TITLE_Section5  ${LANG_ENGLISH} "Shortcuts"
    LangString TITLE_Section5a ${LANG_ENGLISH} "Startmenu"
    LangString TITLE_Section5b ${LANG_ENGLISH} "Desktop"

    LangString TITLE_Section1  ${LANG_GERMAN} "Hauptprogramm"
    LangString TITLE_Section2  ${LANG_GERMAN} "Mapconfig Werkzeug"
    LangString TITLE_Section3  ${LANG_GERMAN} "Mapvorschaudateien"
    LangString TITLE_Section4  ${LANG_GERMAN} "�bersetzungen"
    LangString TITLE_Section5  ${LANG_GERMAN} "Verkn�pfungen"
    LangString TITLE_Section5a ${LANG_GERMAN} "Startmen�"
    LangString TITLE_Section5b ${LANG_GERMAN} "Desktop"

    LangString DESC_Section1 ${LANG_ENGLISH} "Main progrom."
    LangString DESC_Section2 ${LANG_ENGLISH} "Tool to create map config files necessary for map preview."
    LangString DESC_Section3 ${LANG_ENGLISH} "Images and config files for map preview."
    LangString DESC_Section4 ${LANG_ENGLISH} "Some translations."
    LangString DESC_Section5 ${LANG_ENGLISH} "Program shortcuts for the startmenu and the desktop."

    LangString DESC_Section1 ${LANG_GERMAN} "Hauptprogramm."
    LangString DESC_Section2 ${LANG_GERMAN} "Ein Werkzeug, um die f�r die Mapvorschau ben�tigten Konfigurationsdateien zu erzeugen."
    LangString DESC_Section3 ${LANG_GERMAN} "Bilder und Konfigurationsdateien f�r die Mapvorschau."
    LangString DESC_Section4 ${LANG_GERMAN} "Einige �bersetzungen."
    LangString DESC_Section5 ${LANG_GERMAN} "Verkn�pfungen f�r das Startmen� und den Desktop."

    ;Assign descriptions to sections
    !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecProgram}    $(DESC_Section1)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMapCfgTool} $(DESC_Section2)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMapPreview} $(DESC_Section3)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMapLang}    $(DESC_Section4)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcut}   $(DESC_Section5)
    !insertmacro MUI_FUNCTION_DESCRIPTION_END


; Uninstaller

Section "Uninstall"
  
  ; Remove files and uninstaller
  RMDir /r $INSTDIR

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Cube Server Lister\*.*"
  Delete "$DESKTOP\Cube Server Lister.lnk"
  ; Remove directories used
  RMDir "$SMPROGRAMS\Cube Server Lister"

SectionEnd
