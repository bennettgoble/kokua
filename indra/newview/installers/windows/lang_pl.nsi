; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Polish.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_POLISH} "Język instalatora"
LangString SelectInstallerLanguage  ${LANG_POLISH} "Proszę wybrać język instalatora"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_POLISH} " Aktualizacja"
LangString LicenseSubTitleSetup ${LANG_POLISH} " Instalacja"

LangString MULTIUSER_TEXT_INSTALLMODE_TITLE ${LANG_POLISH} "Installation Mode"
LangString MULTIUSER_TEXT_INSTALLMODE_SUBTITLE ${LANG_POLISH} "Install for all users (requires Admin) or only for the current user?"
LangString MULTIUSER_INNERTEXT_INSTALLMODE_TOP ${LANG_POLISH} "When you run this installer with Admin privileges, you can choose whether to install in (e.g.) c:\Program Files or the current user's AppData\Local folder."
LangString MULTIUSER_INNERTEXT_INSTALLMODE_ALLUSERS ${LANG_POLISH} "Install for all users"
LangString MULTIUSER_INNERTEXT_INSTALLMODE_CURRENTUSER ${LANG_POLISH} "Install for current user only"

; installation directory text
LangString DirectoryChooseTitle ${LANG_POLISH} "Katalog instalacji" 
LangString DirectoryChooseUpdate ${LANG_POLISH} "Wybierz katalog instalacji Kokua w celu aktualizacji wersji ${VERSION_LONG}.(XXX):"
LangString DirectoryChooseSetup ${LANG_POLISH} "Wybierz katalog instalacji Kokua w:"

LangString MUI_TEXT_DIRECTORY_TITLE ${LANG_POLISH} "Installation Directory"
LangString MUI_TEXT_DIRECTORY_SUBTITLE ${LANG_POLISH} "Select the directory into which to install Kokua:"

LangString MUI_TEXT_INSTALLING_TITLE ${LANG_POLISH} "Installing Second Life..."
LangString MUI_TEXT_INSTALLING_SUBTITLE ${LANG_POLISH} "Installing the Kokua viewer to $INSTDIR"

LangString MUI_TEXT_FINISH_TITLE ${LANG_POLISH} "Installing Second Life"
LangString MUI_TEXT_FINISH_SUBTITLE ${LANG_POLISH} "Installed the Kokua viewer to $INSTDIR."

LangString MUI_TEXT_ABORT_TITLE ${LANG_POLISH} "Installation Aborted"
LangString MUI_TEXT_ABORT_SUBTITLE ${LANG_POLISH} "Not installing the Kokua viewer to $INSTDIR."

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_POLISH} "Nie można odnaleźć programu '$INSTNAME'. Cicha aktualizacja zakończyła się niepowodzeniem."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_POLISH} "Czy uruchomić Kokua teraz?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_POLISH} "Poszukiwanie starszej wersji..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_POLISH} "Sprawdzanie wersji Windows..."
LangString CheckWindowsVersionMB ${LANG_POLISH} 'Kokua obsługuje tylko Windows Vista.$\n$\nPróba zainstalowania na Windows $R0 może spowodować awarie i utratę danych.$\n$\n'
LangString CheckWindowsServPackMB ${LANG_POLISH} "Zalecane jest uruchamianie Kokua z najnowszym dostępnym Service Packiem zainstalowanym w systemie.$\nPomaga on w podniesieniu wydajności i stabilności programu."
LangString UseLatestServPackDP ${LANG_POLISH} "Użyj usługi Windows Update, aby zainstalować najnowszy Service Pack."

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_POLISH} "Sprawdzanie zezwolenia na instalację..."
LangString CheckAdministratorInstMB ${LANG_POLISH} 'Używasz "ograniczonego" konta.$\nMusisz być zalogowany jako "administrator" aby zainstalować Kokua.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_POLISH} "Sprawdzanie zezwolenia na odinstalowanie..."
LangString CheckAdministratorUnInstMB ${LANG_POLISH} 'Używasz "ograniczonego" konta.$\nMusisz być być zalogowany jako "administrator" aby zainstalować Kokua.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_POLISH} "Kokua ${VERSION_LONG} jest już zainstalowane.$\n$\nCzy chcesz zainstalować Kokua ponownie?"

; checkcpuflags
LangString MissingSSE2 ${LANG_POLISH} "Ten komputer może nie mieć procesora z obsługą SSE2, który jest wymagany aby uruchomić Firestorma w wersji ${VERSION_LONG}. Chcesz kontynuować?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_POLISH} "Oczekiwanie na zamknięcie Kokua..."
LangString CloseSecondLifeInstMB ${LANG_POLISH} "Kokua nie może zostać zainstalowane, ponieważ jest już włączone.$\n$\nZakończ swoje działania i wybierz OK aby zamknąć Kokua i kontynuować.$\nWybierz CANCEL aby anulować instalację."
LangString CloseSecondLifeInstRM ${LANG_POLISH} "Kokua failed to remove some files from a previous install.$\n$\nSelect OK to continue.$\nSelect CANCEL to cancel installation."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_POLISH} "Oczekiwanie na zamknięcie Kokua..."
LangString CloseSecondLifeUnInstMB ${LANG_POLISH} "Kokua nie może zostać zainstalowane, ponieważ jest już włączone.$\n$\nZakończ swoje działania i wybierz OK aby zamknąć Kokua i kontynuować.$\nWybierz CANCEL aby anulować."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_POLISH} "Sprawdzanie połączenia sieciowego..."

; error during installation
LangString ErrorSecondLifeInstallRetry ${LANG_POLISH} "Second Life installer encountered issues during installation. Some files may not have been copied correctly."
LangString ErrorSecondLifeInstallSupport ${LANG_POLISH} "Please reinstall viewer from https://secondlife.com/support/downloads/ and contact https://support.secondlife.com if issue persists after reinstall."
; removecachefiles
LangString RemoveCacheFilesDP ${LANG_POLISH} "Kasowanie plików pamięci podręcznej (cache) w folderze Documents and Settings"

; ask to remove user's data files
LangString RemoveDataFilesMB ${Lang_POLISH} "Do you want to REMOVE all other files related to Kokua as well?$\n$\nIt is recomended to keep the settings and cache files if you have other versions of Kokua installed or are uninstalling to upgrade to a newer version."

; delete program files
LangString DeleteProgramFilesMB ${LANG_POLISH} "Nadal istnieją pliki w katalogu instalacyjnym Kokua.$\n$\nMożliwe, że są to pliki, które stworzyłeś/stworzyłaś lub przeniosłeś/przeniosłaś do:$\n$INSTDIR$\n$\nCzy chcesz je usunąć?"

; uninstall text
LangString UninstallTextMsg ${LANG_POLISH} "To spowoduje odinstalowanie Kokua ${VERSION_LONG} z Twojego systemu."

; ask to remove registry keys that still might be needed by other viewers that are installed
LangString DeleteRegistryKeysMB ${LANG_POLISH} "Do you want to remove application registry keys?$\n$\nIt is recomended to keep registry keys if you have other versions of Second Life installed."
