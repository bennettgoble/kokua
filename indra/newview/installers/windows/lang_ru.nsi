; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Russian.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_RUSSIAN} "Installer Language"
LangString SelectInstallerLanguage  ${LANG_RUSSIAN} "Please select the language of the installer"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_RUSSIAN} " Update"
LangString LicenseSubTitleSetup ${LANG_RUSSIAN} " Setup"

; installation directory text
LangString DirectoryChooseTitle ${LANG_RUSSIAN} "Installation Directory" 
LangString DirectoryChooseUpdate ${LANG_RUSSIAN} "Select the Second Life directory to update to version ${VERSION_LONG}.(XXX):"
LangString DirectoryChooseSetup ${LANG_RUSSIAN} "Select the directory to install Second Life in:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_RUSSIAN} "Could not find the program '$INSTPROG'. Silent update failed."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_RUSSIAN} "Start Second Life now?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_RUSSIAN} "Checking for old version..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_RUSSIAN} "Checking Windows version..."
LangString CheckWindowsVersionMB ${LANG_RUSSIAN} 'Second Life only supports Windows XP, Windows 2000, and Mac OS X.$\n$\nAttempting to install on Windows $R0 can result in crashes and data loss.$\n$\nInstall anyway?'

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_RUSSIAN} "Checking for permission to install..."
LangString CheckAdministratorInstMB ${LANG_RUSSIAN} 'You appear to be using a "limited" account.$\nYou must be an "administrator" to install Second Life.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_RUSSIAN} "Checking for permission to uninstall..."
LangString CheckAdministratorUnInstMB ${LANG_RUSSIAN} 'You appear to be using a "limited" account.$\nYou must be an "administrator" to uninstall Second Life.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_RUSSIAN} "It appears that Second Life ${VERSION_LONG} is already installed.$\n$\nWould you like to install it again?"

; checkcpuflags
LangString MissingSSE2 ${LANG_RUSSIAN} "This machine may not have a CPU with SSE2 support, which is required to run SecondLife ${VERSION_LONG}. Do you want to continue?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_RUSSIAN} "Waiting for Second Life to shut down..."
LangString CloseSecondLifeInstMB ${LANG_RUSSIAN} "Second Life can't be installed while it is already running.$\n$\nFinish what you're doing then select OK to close Second Life and continue.$\nSelect CANCEL to cancel installation."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_RUSSIAN} "Waiting for Second Life to shut down..."
LangString CloseSecondLifeUnInstMB ${LANG_RUSSIAN} "Second Life can't be uninstalled while it is already running.$\n$\nFinish what you're doing then select OK to close Second Life and continue.$\nSelect CANCEL to cancel."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_RUSSIAN} "Checking network connection..."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_RUSSIAN} "Deleting cache files in Documents and Settings folder"

; delete program files
LangString DeleteProgramFilesMB ${LANG_RUSSIAN} "There are still files in your SecondLife program directory.$\n$\nThese are possibly files you created or moved to:$\n$INSTDIR$\n$\nDo you want to remove them?"

; uninstall text
LangString UninstallTextMsg ${LANG_RUSSIAN} "This will uninstall Second Life ${VERSION_LONG} from your system."
