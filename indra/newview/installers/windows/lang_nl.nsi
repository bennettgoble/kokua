; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Dutch.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_DUTCH} "Installatietaal"
LangString SelectInstallerLanguage  ${LANG_DUTCH} "Selecteer alstublieft de taal van de installatie"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_DUTCH} " Bijwerken"
LangString LicenseSubTitleSetup ${LANG_DUTCH} " Installeren"

; installation directory text
LangString DirectoryChooseTitle ${LANG_DUTCH} "Installatiemap" 
LangString DirectoryChooseUpdate ${LANG_DUTCH} "Selecteer de Kokua map om bij te werken naar versie ${VERSION_LONG}.(XXX):"
LangString DirectoryChooseSetup ${LANG_DUTCH} "Selecteer de map waar u Kokua wilt installeren:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_DUTCH} "Kan het programma '$INSTPROG' niet vinden. Geruisloos bijwerken mislukt."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_DUTCH} "Kokua nu starten?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_DUTCH} "Zoeken naar oude versie..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_DUTCH} "Windowsversie controleren..."
LangString CheckWindowsVersionMB ${LANG_DUTCH} 'Kokua ondersteunt alleen Windows Vista, Windows 2000 en Max OS X.$\n$\nProberen te installeren op Windows $R0 kan leiden tot storingen en gegevensverlies.$\n$\nToch installeren?'

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_DUTCH} "Permissie om te installeren controleren..."
LangString CheckAdministratorInstMB ${LANG_DUTCH} 'U lijkt gebruik te maken van een "beperkt" account.$\nOm Kokua te installeren dient u een "beheerder" te zijn.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_DUTCH} "Permissie om te verwijderen controleren..."
LangString CheckAdministratorUnInstMB ${LANG_DUTCH} 'U lijkt gebruik te maken van een van een "beperkt" account.$\nOm Kokua te verwijderen dient u een "beheerder" te zijn.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_DUTCH} "Kokua ${VERSION_LONG} lijkt reeds geïnstalleerd te zijn.$\n$\nWilt u het nogmaals installeren?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_DUTCH} "Wachten tot Kokua is afgesloten..."
LangString CloseSecondLifeInstMB ${LANG_DUTCH} "Kokua kan niet worden geïnstalleerd terwijl het uitgevoerd wordt.$\n$\nRond uw bezigheden af en selecteer OK om Kokua af te sluiten en verder te gaan.$\nSelecteer ANNULEREN om de installatie te annuleren."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_DUTCH} "Wachten tot Kokua is afgesloten..."
LangString CloseSecondLifeUnInstMB ${LANG_DUTCH} "Kokua kan niet worden verwijderd terwijl het uitgevoerd wordt.$\n$\nRond uw bezigheden af en selecteer OK om Kokua af te sluiten en verder te gaan.$\nSelecteer ANNULEREN om te annuleren."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_DUTCH} "Netwerkverbinding controleren..."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_DUTCH} "Cachebestanden verwijderen uit de map Documents and Settings"

; delete program files
LangString DeleteProgramFilesMB ${LANG_DUTCH} "Er bevinden zich nog steeds bestanden in uw Kokua-programmamap.$\n$\nEr zijn mogelijk bestanden gecreëerd in of verplaatst naar:$\n$INSTDIR$\n$\nWilt u deze verwijderen?"

; uninstall text
LangString UninstallTextMsg ${LANG_DUTCH} "Hiermee wordt Kokua ${VERSION_LONG} van uw systeem verwijderd."
