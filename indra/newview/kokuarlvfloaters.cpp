/** 
 * @file kokuarlvfloaters.cpp
 * @brief RLV floaters (adapted from llfloaterscriptdebug.cpp and Firestorm)
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

// Clear credit: This source file draws upon the Script Debug Floater for the
// RLV Commands window and RLVa/Firestorm for the Console and Status floaters.
// Routines that manipulate RLV data are typically abstracted from code in
// RLV's RRInterface.cpp.
// The Worn floater is a new derivative of the Status Floater. There are heavy
// modifications to integrate with RLV and my own preferences about operation.

#include "llviewerprecompiledheaders.h"

#include "kokuarlvfloaters.h"

#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llavatarnamecache.h"
#include "llcallbacklist.h"
#include "llclipboard.h"
#include "llerror.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "lllineeditor.h"
#include "llrect.h"
#include "llscrolllistctrl.h"
#include "llstartup.h"
#include "llstring.h"
#include "lltexteditor.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewertexteditor.h"
#include "llvoavatarself.h"
#include "llwearabledata.h"

#include "RRInterface.h"

#include <boost/foreach.hpp>

// Marine doesn't export it so we need it here too
#define EXTREMUM 1000000.f

#define UNLIMITED "unlimited"

static bool awaiting_idle_for_worn = false;
static bool awaiting_idle_for_status = false;
static bool pause_updating = false;

// ---- inventory observer

void KokuaRLVInventoryObserver::changed(U32 mask)
{
    if (KokuaFloaterRLVWorn::getBase())
    {
        KokuaFloaterRLVWorn::getBase()->refreshWornStatus();
    }
}

// ---- floater support

std::string KokuaRLVFloaterSupport::getModifierText(F32 value, F32 ceiling)
{
    if (fabs(value) == ceiling) return UNLIMITED;
    return llformat("%.1f",value);
}

BOOL KokuaRLVFloaterSupport::addNameToLocalCache(LLUUID &obj_id, std::string &from_name)
{
    // returns TRUE if name exists in viewer's cache

    // is it currently in object cache?
    LLViewerObject* pobject = gObjectList.findObject(obj_id);

    // is it currently in local cache?
    std::map<LLUUID, std::string>::iterator iter = mKnownIDs.find(obj_id);
    BOOL incache = !(iter == mKnownIDs.end());

    // is it an attachment or an avatar? if neither (eg standalone object or locally made UUID) keep it in this cache
    BOOL canberesolved = FALSE;
    if (pobject && incache)
    {
        if ((pobject->isAvatar()))
        {
            canberesolved = TRUE;
        }
        else
        {
            LLViewerObject* pObjRoot = pobject->getRootEdit();

            LLViewerInventoryItem* pItem = NULL;
            if ((pObjRoot) && (pObjRoot->isAttachment()))
            {
                pItem = gInventory.getItem(pObjRoot->getAttachmentItemID());
            }
            if (pItem)
            {
                canberesolved = TRUE;
            }
        }
        if (canberesolved)
        {
            mKnownIDs.erase(iter);
            incache = FALSE;
        }
    }

    if ((!pobject || (pobject && !canberesolved)) && !incache)
    {
        // not in either, so starts in local until we know attachment cache has it and can handle it
        mKnownIDs[obj_id] = from_name;
    }

    if (KokuaFloaterRLVDebug::getBase()) KokuaFloaterRLVDebug::getBase()->refreshRLVDebug(obj_id);
    if (KokuaFloaterRLVStatus::getBase()) KokuaFloaterRLVStatus::getBase()->refreshRLVStatus();

    return (pobject != NULL);
}

std::string KokuaRLVFloaterSupport::getNameFromUUID(const LLUUID& idObj, bool fIncludeAttachPt = true)
{
    LLViewerObject* pObj = gObjectList.findObject(idObj);
    std::map<LLUUID, std::string>::iterator iter = mKnownIDs.find(idObj);

    // if we're not in object cache but do have it in local, return that and don't
    // try analysing it further - this will protect us at startup during which the object
    // cache is loading and also allow the local uuid for the blinding effect to
    // be reported sensibly. However, if we've been called for a non-attached object
    // this algorithm will fail and we fall back on the local cache (rather than
    // querying the server for it and generating additional traffic)

    if (!pObj && !(iter == mKnownIDs.end())) return mKnownIDs[idObj];

    if ((pObj) && (pObj->isAvatar()))
    {
        LLAvatarName avName;
        if (LLAvatarNameCache::get(pObj->getID(), &avName))
        {
            return avName.getCompleteName();
        }
        return ((LLVOAvatar*)pObj)->getFullname();
    }

    LLViewerObject* pObjRoot = NULL;
    if (pObj)
    {
        pObjRoot = pObj->getRootEdit();
    }

    LLViewerInventoryItem* pItem = NULL;
    if ((pObjRoot) && (pObjRoot->isAttachment()))
    {
        pItem = gInventory.getItem(pObjRoot->getAttachmentItemID());
    }

    std::string strItemName = idObj.asString();
    if (pItem)
    {
        strItemName = pItem->getName();
    }
    else if (iter != mKnownIDs.end())
    {
        //at this point we've failed to get it as a worn item, however we've allowed for
        //this and it should still be in our local cache

        return mKnownIDs[idObj];
    }
    // if we didn't have it in the local cache either, continue through with uuid asString

    if ((!fIncludeAttachPt) || (!pObj) || (!pObj->isAttachment()) || (!isAgentAvatarValid()))
    {
        //satisfy from local cache if we can
        if (iter != mKnownIDs.end())
        {
            return mKnownIDs[idObj];
        }
        else
        {
            return strItemName;
        }
    }

    S32 attachptnum = 0;
    if (pObjRoot)
    {
        attachptnum = ATTACHMENT_ID_FROM_STATE(pObjRoot->getAttachmentState());
    }
    
    LLViewerJointAttachment* pAttachPt = get_if_there(gAgentAvatarp->mAttachmentPoints, attachptnum, (LLViewerJointAttachment*)NULL);
    std::string strAttachPtName = "Unknown";
    if (pAttachPt)
    {
        strAttachPtName = pAttachPt->getName();
    }

    std::string schild = "";
    if (pObj != pObjRoot)
    {
        schild = ", child";
    }

    return llformat("%s (%s%s)", strItemName.c_str(), strAttachPtName.c_str(), schild.c_str());
}

void KokuaRLVFloaterSupport::checkForRefreshNeeded(LLUUID& object_uuid, std::string& command, bool* refresh_status, bool* refresh_worn)
{
    std::string behav = "";
    std::string option = "";
    std::string param = "";

    LLStringUtil::toLower(behav);

    // null pointer check
    if (!refresh_status || !refresh_worn) return;
    
    // the actual test is here so that we can recurse if needed but only trigger a maximum of one refresh at the end

    if (command.find(",") != -1) {
        std::deque<std::string> list_of_commands = gAgent.mRRInterface.parse(command, ",");
        for (unsigned int i = 0; i<list_of_commands.size(); ++i)
        {
            checkForRefreshNeeded(object_uuid, list_of_commands.at(i),refresh_status,refresh_worn);
            //if we know that we need to refresh both, break out early
            if (*refresh_status && *refresh_worn) return;
        }
        return;
    }

    if (command.find("clear") == 0)
    {
        *refresh_status = true;
        *refresh_worn = true;
    }
    else
    {
        int ind = command.find("=");
        if (ind != -1)
        {
            param = command.substr(ind + 1);
            behav = command.substr(0, ind);
            ind = behav.find(":");
            if (ind != -1)
            {
                option = behav.substr(ind + 1);
                behav = behav.substr(0, ind);
            }
        }
        else
        {
            behav = command; // fallback, since we already covered @clear above
        }

        // status window - refresh on clear and any lasting effect (ie =n/=y/=add/=rem)
        if (param == "n" ||
            param == "y" ||
            param == "add" ||
            param == "rem")
        {
            *refresh_status = true;
        }

        // worn window - refresh on clear and shared/unshared/*attach*/*detach*/*outfit*
        if (behav.find("shared") != -1 ||
            behav.find("attach") != -1 ||
            behav.find("detach") != -1 ||
            behav.find("outfit") != -1)
        {
            *refresh_worn = true;
        }
    }
}

void KokuaRLVFloaterSupport::commandNotify(LLUUID& object_uuid, std::string& command)
{
    // called when handleCommand returns success - not any more...
    // since add / remove / clear and replace are public, they could get called internally
    // by anything (the garbage collector being one example) we now get called from
    // these routines instead. This means that the routine below to deal with combination
    // commands actually isn't going to ever get fed combinations, just singles, however
    // we leave the capability in place for possible future use

    // to avoid too much redrawing and processing, we filter down on what triggers an update

    bool refresh_status = false;
    bool refresh_worn = false;
    
    // optimisation - if neither floater is open, they obviously don't need refreshing...
    if (!(KokuaFloaterRLVStatus::getBase() && KokuaFloaterRLVStatus::getBase()->isShown()) &&
          !(KokuaFloaterRLVWorn::getBase() && KokuaFloaterRLVWorn::getBase()->isShown())) return;   

    // we could get called with stacked commands, so this handles the possible
    // recursive call to break down the command stack and returns as early as
    // possible once it decides both floaters need to refresh
    checkForRefreshNeeded(object_uuid, command, &refresh_status, &refresh_worn);
    
    // since we can no longer get the benefit of processing a whole command line and only
    // doing one update, we change update strategy instead and defer it until the viewer
    // does an idle callback

    if (refresh_status && KokuaFloaterRLVStatus::getBase() && !awaiting_idle_for_status)
    {
        doOnIdleOneTime(&KokuaFloaterRLVStatus::callRefreshRLVStatus);
    }

    if (refresh_worn && KokuaFloaterRLVWorn::getBase() && !awaiting_idle_for_worn)
    {
        doOnIdleOneTime(&KokuaFloaterRLVWorn::callRefreshWornStatus);
    }
}

// ---- RLV Commands

KokuaFloaterRLVDebug::KokuaFloaterRLVDebug(const LLSD& key)
  : LLMultiFloater(key)
{
    // avoid resizing of the window to match 
    // the initial size of the tabbed-childs, whenever a tab is opened or closed
    mAutoResize = FALSE;
    // enabled autofocus blocks controling focus via  LLFloaterReg::showInstance
    setAutoFocus(FALSE);
}

KokuaFloaterRLVDebug::~KokuaFloaterRLVDebug()
{
}

void KokuaFloaterRLVDebug::show(const LLUUID& object_id)
{
    addOutputWindow(object_id);
}

BOOL KokuaFloaterRLVDebug::postBuild()
{
    LLMultiFloater::postBuild();

    if (mTabContainer)
    {
        return TRUE;
    }

    return FALSE;
}

void KokuaFloaterRLVDebug::setVisible(BOOL visible)
{
    if(visible)
    {
        KokuaFloaterRLVDebugOutput* floater_output = LLFloaterReg::findTypedInstance<KokuaFloaterRLVDebugOutput>("rlv_debug_output", LLUUID::null);
        if (floater_output == NULL)
        {
            floater_output = dynamic_cast<KokuaFloaterRLVDebugOutput*>(LLFloaterReg::showInstance("rlv_debug_output", LLUUID::null, FALSE));
            if (floater_output)
            {
                addFloater(floater_output, false);
            }
        }

    }
    LLMultiFloater::setVisible(visible);
}

void KokuaFloaterRLVDebug::closeFloater(bool app_quitting/* = false*/)
{
    if(app_quitting)
    {
        LLMultiFloater::closeFloater(app_quitting);
    }
    else
    {
        setVisible(false);
    }
}

LLFloater* KokuaFloaterRLVDebug::addOutputWindow(const LLUUID &object_id)
{
    LLFloater* floaterp;

    LLMultiFloater* host = LLFloaterReg::showTypedInstance<LLMultiFloater>("rlv_debug", LLSD());
    if (!host)
        return NULL;

    LLFloater::setFloaterHost(host);
    // prevent stealing focus
    floaterp = LLFloaterReg::showInstance("rlv_debug_output", object_id, FALSE);
    LLFloater::setFloaterHost(NULL);

    return floaterp;
}

// Although using a colour is supported in the code, the colour will get suppressed in the text display
// because it's set as read-only
void KokuaFloaterRLVDebug::addRLVLine(const std::string &utf8mesg, const LLColor4& color, LLUUID& source_id)
{
    std::string source_name;
    std::string floater_label;

    KokuaFloaterRLVDebug* self = KokuaFloaterRLVDebug::getBase(); //used to check if the floater gets created on this call
    LLFloater* restoreprevious = (LLFloater*)&LLUUID::null;
        
    static LLCachedControl<bool> option_ignore_queries(gSavedSettings, "KokuaRLVDebugFloaterIgnoreQueries");
    static LLCachedControl<bool> option_auto_open(gSavedSettings, "KokuaRLVDebugFloaterAutoOpen");
    static LLCachedControl<bool> option_focus_on_latest(gSavedSettings, "KokuaRLVDebugFloaterFocusOnLatest");

    if (!option_auto_open)
    {
        // when this is turned off we discard everything until the floater is manually opened
        if (!LLFloaterReg::instanceVisible("rlv_debug", LLSD())) return;
    }

    if (option_ignore_queries)
    {
        // supress any command with "=number"
        int ind = utf8mesg.find("=");
        if (ind != -1) {
            S32 channel = 0; // must be initialised - convertToS32 will not modify if conversion fails
            std::string param = utf8mesg.substr(ind + 1);
            LLStringUtil::convertToS32(param, channel);
            if (channel != 0) return;
        }
    }

    if (getBase()) 
    {
        restoreprevious = getBase()->getActiveFloater();
    }

    // this changes the active floater
    addOutputWindow(source_id);

    source_name = KokuaRLVFloaterSupport::getNameFromUUID(source_id);

    // add to "All" floater
    KokuaFloaterRLVDebugOutput* floaterp = LLFloaterReg::getTypedInstance<KokuaFloaterRLVDebugOutput>("rlv_debug_output", LLUUID::null);
    if (floaterp)
    {
        floaterp->addLine(source_name + ": " + utf8mesg, source_name, color);

        //special case - when focus on latest is off we want the default active tab to be the all items tab
        if (!option_focus_on_latest && !self)
        {
            // only here if this is the first time through and the floater is newly created
            restoreprevious = floaterp;
        }
    }   

    // add to specific RLV instance floater
    floaterp = LLFloaterReg::getTypedInstance<KokuaFloaterRLVDebugOutput>("rlv_debug_output", source_id);
    if (floaterp)
    {
        floaterp->addLine(utf8mesg, source_name, color);
    }

    if (!option_focus_on_latest && getBase() && restoreprevious)
    {
        // usual behaviour is for focus to follow where lines where last added
        // if this option is off we have to save the incoming context and restore it
        getBase()->selectFloater(restoreprevious);      
    }
}

void KokuaFloaterRLVDebug::refreshRLVDebug(LLUUID& source_id)
{
    // called when we detect that an item previously in local cache has now arrived in viewer object cache

    KokuaFloaterRLVDebugOutput* floaterp = LLFloaterReg::getTypedInstance<KokuaFloaterRLVDebugOutput>("rlv_debug_output", source_id);
    if (floaterp)
    {
        floaterp->updateName(KokuaRLVFloaterSupport::getNameFromUUID(source_id, TRUE), source_id);
    }
}

//static
KokuaFloaterRLVDebug* KokuaFloaterRLVDebug::getBase()
{
    return LLFloaterReg::findTypedInstance<KokuaFloaterRLVDebug>("rlv_debug");
}

//
// ---- The embedded texteditor panes
//

KokuaFloaterRLVDebugOutput::KokuaFloaterRLVDebugOutput(const LLSD& object_id)
  : LLFloater(LLSD(object_id)),
    mObjectID(object_id.asUUID())
{
    // enabled autofocus blocks controling focus via  LLFloaterReg::showInstance
    setAutoFocus(FALSE);
}

BOOL KokuaFloaterRLVDebugOutput::postBuild()
{
    LLFloater::postBuild();
    mHistoryEditor = getChild<LLViewerTextEditor>("rlv_debug_output");
    return TRUE;
}

KokuaFloaterRLVDebugOutput::~KokuaFloaterRLVDebugOutput()
{
}

void KokuaFloaterRLVDebugOutput::addLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color)
{
    if (mObjectID.isNull())
    {
        setCanTearOff(FALSE);
        setCanClose(FALSE);
    }
    else
    {
        setTitle(user_name);
        setShortTitle(user_name);
    }
    mHistoryEditor->appendText(utf8mesg, true, LLStyle::Params().color(color));
    mHistoryEditor->blockUndo();
    mHistoryEditor->setCursorAndScrollToEnd();
}

void KokuaFloaterRLVDebugOutput::updateName(const std::string &user_name, const LLUUID &id)
{
    if (mObjectID.isNull())
    {
        setCanTearOff(FALSE);
        setCanClose(FALSE);
    }
    else
    {
        setTitle(user_name);
        setShortTitle(user_name);
        
        //we don't put the uuid into the actual text, so no need to replace there
        //mHistoryEditor->replaceTextAll(id.asString(), user_name, FALSE);
        
        //and update the main window too
        KokuaFloaterRLVDebugOutput* floaterp = LLFloaterReg::getTypedInstance<KokuaFloaterRLVDebugOutput>("rlv_debug_output", LLUUID::null);
        if (floaterp)
        {
             floaterp->mHistoryEditor->replaceTextAll(id.asString(), user_name, TRUE);
             floaterp->mHistoryEditor->setReadOnly(TRUE);
        }
    }
}

// ---- RLV Console

std::string sRLVprompt = "RLV>";

KokuaFloaterRLVConsole::KokuaFloaterRLVConsole(const LLSD& sdKey)
    : LLFloater(sdKey), m_pOutputText(nullptr)
{
}

KokuaFloaterRLVConsole::~KokuaFloaterRLVConsole()
{
}

BOOL KokuaFloaterRLVConsole::postBuild()
{
    LLLineEditor* pInputEdit = getChild<LLLineEditor>("rlv_console_input");
    pInputEdit->setEnableLineHistory(true);
    pInputEdit->setCommitCallback(boost::bind(&KokuaFloaterRLVConsole::onInput, this, _1, _2));
    pInputEdit->setFocus(true);
    pInputEdit->setCommitOnFocusLost(false);

    m_pOutputText = getChild<LLTextEditor>("rlv_console_output");
    m_pOutputText->appendText(sRLVprompt, false);

    return TRUE;
}

void KokuaFloaterRLVConsole::onClose(bool fQuitting)
{
    gAgent.mRRInterface.handleCommand(gAgent.getID(),"clear");
}

void KokuaFloaterRLVConsole::addCommandReply(const std::string& reply)
{
    // this has to be static to be called from rrinterface, so we need
    // to make sure we've got a floater to write to
    if (m_pOutputText) m_pOutputText->appendText('\n'+reply, false);
}

void KokuaFloaterRLVConsole::onInput(LLUICtrl* pCtrl, const LLSD& sdParam)
{
    LLLineEditor* pInputEdit = static_cast<LLLineEditor*>(pCtrl);
    std::string strInput = pInputEdit->getText();
    LLStringUtil::trim(strInput);

    m_pOutputText->appendText(strInput, false);
    pInputEdit->clear();

    if (!gRRenabled)
    {
        m_pOutputText->appendText("RLV is currently disabled", true);
    }
    else if ('@' != strInput[0])
    {
        m_pOutputText->appendText("Commands must begin with @", true);
    }
    else
    {
        strInput.erase(0, 1);
        LLStringUtil::toLower(strInput);
            
        BOOL result = gAgent.mRRInterface.handleCommand(gAgent.getID(), strInput);
        
        // only put feedback in the window if the command was rejected (output from =channel commands will arrive by another route)
        
        if (!result) m_pOutputText->appendText("Command failed", true);
    }

    m_pOutputText->appendText(sRLVprompt, true);
}

//static
KokuaFloaterRLVConsole* KokuaFloaterRLVConsole::getBase()
{
    return LLFloaterReg::findTypedInstance<KokuaFloaterRLVConsole>("rlv_console");
}

// ---- RLV Status

KokuaFloaterRLVStatus::KokuaFloaterRLVStatus(const LLSD& sdKey)
: LLFloater(LLSD(sdKey)),
    mPauseUpdating(NULL)
{
}

KokuaFloaterRLVStatus::~KokuaFloaterRLVStatus()
{
}

BOOL KokuaFloaterRLVStatus::postBuild()
{
    getChild<LLUICtrl>("copy_btn")->setCommitCallback(boost::bind(&KokuaFloaterRLVStatus::onBtnCopyToClipboard, this));
    mPauseUpdating = getChild<LLCheckBoxCtrl>( "pause_updating");
    mPauseUpdating->setCommitCallback(boost::bind(&KokuaFloaterRLVStatus::onCommitPauseUpdating, this));
    return TRUE;
}

void KokuaFloaterRLVStatus::onOpen(const LLSD& key)
{
  refreshRLVStatus();
}

void KokuaFloaterRLVStatus::onBtnCopyToClipboard()
{
    LLWString res = utf8str_to_wstring(gAgent.mRRInterface.getRlvRestrictions());
    LLClipboard::instance().copyToClipboard(res, 0, res.length());
}

void KokuaFloaterRLVStatus::onCommitPauseUpdating()
{
    pause_updating = KokuaFloaterRLVStatus::getBase()->mPauseUpdating->get();
    if (!pause_updating) callRefreshRLVStatus();
}

void KokuaFloaterRLVStatus::callRefreshRLVStatus()
{
    if (KokuaFloaterRLVStatus::getBase())
    {
        KokuaFloaterRLVStatus::getBase()->refreshRLVStatus();
    }
}

void KokuaFloaterRLVStatus::refreshRLVStatus()
{
    if (!(KokuaFloaterRLVStatus::getBase() && KokuaFloaterRLVStatus::getBase()->isShown())) return; 
    if (pause_updating) return;
    
    awaiting_idle_for_status = false;
    LLCtrlListInterface* pBhvrList = childGetListInterface("behaviour_list");
    LLCtrlListInterface* pExceptList = childGetListInterface("exception_list");
    LLCtrlListInterface* pModifierList = childGetListInterface("modifier_list");
    LLCtrlListInterface* pNotifyList = childGetListInterface("notify_list");
    if ((!pBhvrList) || (!pExceptList) || (!pModifierList) || (!pNotifyList))
    {
        return;
    }
    
    pBhvrList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pExceptList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pModifierList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pNotifyList->operateOnAll(LLCtrlListInterface::OP_DELETE);

    if (!isAgentAvatarValid())
    {
        return;
    }

    //
    // Set-up a row we can just reuse
    //
    LLSD sdBhvrRow; LLSD& sdBhvrColumns = sdBhvrRow["columns"];
    sdBhvrColumns[0] = LLSD().with("column", "behaviour").with("type", "text");
    sdBhvrColumns[1] = LLSD().with("column", "issuer").with("type", "text");

    LLSD sdNotifyRow; LLSD& sdNotifyColumns = sdNotifyRow["columns"];
    sdNotifyColumns[0] = LLSD().with("column", "behaviour").with("type", "text");
    sdNotifyColumns[1] = LLSD().with("column", "issuer").with("type", "text");

    LLSD sdExceptRow; LLSD& sdExceptColumns = sdExceptRow["columns"];
    sdExceptColumns[0] = LLSD().with("column", "behaviour").with("type", "text");
    sdExceptColumns[1] = LLSD().with("column", "option").with("type", "text");
    sdExceptColumns[2] = LLSD().with("column", "issuer").with("type", "text");

    LLSD sdModifierRow; LLSD& sdModifierColumns = sdModifierRow["columns"];
    sdModifierColumns[0] = LLSD().with("column", "modifier").with("type", "text");
    sdModifierColumns[1] = LLSD().with("column", "minimum").with("type", "text");
    sdModifierColumns[2] = LLSD().with("column", "maximum").with("type", "text");

    RRMAP::iterator it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
    std::string object_name = "";

    while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end())
    {

        std::string option;
        BOOL is_exception = FALSE;
        //BOOL option_is_uuid = FALSE;
        std::string behav = it->second;
        LLStringUtil::toLower(behav);

        if (behav.find("recvchat:") == 0
            || behav.find("recvemote:") == 0
            || behav.find("sendim:") == 0
            || behav.find("startim:") == 0
            || behav.find("recvim:") == 0
            || behav.find("tplure:") == 0
            || behav.find("accepttp:") == 0
            || behav.find("accepttprequest:") == 0
            || behav.find("tprequest:") == 0
            || behav.find("edit:") == 0
            || behav.find("touchworld:") == 0
            || behav.find("shownames:") == 0
            || behav.find("shownametags:") == 0) is_exception = TRUE;

        int ind = behav.find(":");
        if (ind != -1) {
            option = behav.substr(ind + 1);
            behav = behav.substr(0, ind);
        }

        //option may be a uuid, but could be a bunch of other things too

        object_name = KokuaRLVFloaterSupport::getNameFromUUID(LLUUID(it->first), true);

        LLUUID idOption;

        if (option != "" && (idOption.set(option, FALSE)) && (idOption.notNull()))
        {
            LLAvatarName avName;
            //KKA-823 add exception for camtextures/setcam_textures - a texture uuid isn't going to resolve to a name
            //It's ok that we don't test for the colon - we wouldn't be here unless we'd already found it there and set up 'option'
            if (behav.find("camtextures") != 0 && behav.find("setcam_textures") != 0)
            {
                if (gObjectList.findObject(idOption))
                {
                    option = KokuaRLVFloaterSupport::getNameFromUUID(idOption, false);
                }
                else if (LLAvatarNameCache::get(idOption, &avName))
                {
                    option = (!avName.getAccountName().empty()) ? avName.getAccountName() : avName.getDisplayName();
                }
                else if (!gCacheName->getGroupName(idOption, option))
                {
                    if (m_PendingLookup.end() == std::find(m_PendingLookup.begin(), m_PendingLookup.end(), idOption))
                    {
                        LLAvatarNameCache::get(idOption, boost::bind(&KokuaFloaterRLVStatus::onAvatarNameLookup, this, _1, _2));
                        m_PendingLookup.push_back(idOption);
                    }
                }
            }
        }

        if (is_exception)
        {
            sdExceptColumns[0]["value"] = behav;
            sdExceptColumns[1]["value"] = option;
            sdExceptColumns[2]["value"] = object_name;
            pExceptList->addElement(sdExceptRow, ADD_BOTTOM);
        }
        else if (behav == "notify")
        {
            sdNotifyColumns[0]["value"] = it->second;
            sdNotifyColumns[1]["value"] = object_name;
            pNotifyList->addElement(sdNotifyRow, ADD_BOTTOM);
        }
        else
        {
            if (option != "") behav = behav + ":" + option; // so that we pick up any uuid lookup for eg editobj:uuid
            sdBhvrColumns[0]["value"] = behav;
            sdBhvrColumns[1]["value"] = object_name;
            pBhvrList->addElement(sdBhvrRow, ADD_BOTTOM);
        }

        it++;
    }
    
    // now for the modifiers ... here we're going to rely on the computed values shared by the RRInterface class
    // as at RLV 2.9.23.2 these are
    // F32 mCamZoomMax;
    // F32 mCamZoomMin;
    // F32 mCamDistMax;
    // F32 mCamDistMin;
    // F32 mCamDistDrawMax;
    // F32 mCamDistDrawMin;
    // LLColor3 mCamDistDrawColor;
    // F32 mCamDistDrawAlphaMin;
    // F32 mCamDistDrawAlphaMax;
    // F32 mShowavsDistMax;
    // F32 mTplocalMax;
    // F32 mSittpMax;
    // F32 mFartouchMax;
    //
    // Extracting information about which object is responsible for them would involve crawling through the behaviour
    // list - maybe something for a future enhancement, however I don't want to do too much processing in here since
    // we get called on every RLV Command
    
    //camavdist
    sdModifierColumns[0]["value"] = "camavdist";
    sdModifierColumns[1]["value"] = "";
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mShowavsDistMax,EXTREMUM);
    if (sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }

    //camdist
    sdModifierColumns[0]["value"] = "camdistmin/max";
    sdModifierColumns[1]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamDistMin,EXTREMUM);
    // this gets limited by 'make sure we can't move the camera outside the minimum render limit' at ~line 5105 in RRInterface.cpp
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamDistMax,750000.0); 
    if (sdModifierColumns[1]["value"] != UNLIMITED || sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }

    //camdrawalpha
    sdModifierColumns[0]["value"] = "camdrawalpha";
    sdModifierColumns[1]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamDistDrawAlphaMin,0.0);
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamDistDrawAlphaMax,1.0);
    if (sdModifierColumns[1]["value"] != UNLIMITED || sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }

    //camdrawcolor
    LLSD getcolor = gAgent.mRRInterface.mCamDistDrawColor.getValue();
    sdModifierColumns[0]["value"] = "camdrawcolor";
    sdModifierColumns[1]["value"] = "";
    sdModifierColumns[2]["value"] = llformat("<%.1f, %.1f, %.1f>",getcolor[0].asReal(),getcolor[1].asReal(),getcolor[2].asReal());
    if (getcolor[0].asReal() != 0.0 || getcolor[1].asReal() != 0.0 || getcolor[2].asReal() != 0.0)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }
    
    //camdraw
    sdModifierColumns[0]["value"] = "camdrawmin/max";
    sdModifierColumns[1]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamDistDrawMin,EXTREMUM);
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamDistDrawMax,EXTREMUM);
    if (sdModifierColumns[1]["value"] != UNLIMITED || sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }
    
    // camzoom
    sdModifierColumns[0]["value"] = "camzoommin/max";
    sdModifierColumns[1]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamZoomMin,EXTREMUM);
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mCamZoomMax,EXTREMUM);
    if (sdModifierColumns[1]["value"] != UNLIMITED || sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }

    //fartouch
    sdModifierColumns[0]["value"] = "fartouch/touchfar";
    sdModifierColumns[1]["value"] = "";
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mFartouchMax,EXTREMUM);
    if (sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }

    //sittp
    sdModifierColumns[0]["value"] = "sittp";
    sdModifierColumns[1]["value"] = "";
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mSittpMax,EXTREMUM);
    if (sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }
    
    //tplocal
    sdModifierColumns[0]["value"] = "tplocal";
    sdModifierColumns[1]["value"] = "";
    sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(gAgent.mRRInterface.mTplocalMax,EXTREMUM);
    if (sdModifierColumns[2]["value"] != UNLIMITED)
    {
        pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
    }
    
    // Setsphere effects : We check all the stored effects and display all their aggregated values on a per-mode basis.
    // Each effect in the set has a unique mode so we can take each one of them individually and print its values directly (but we can't guarantee the order of the modes)
    std::set<LLVisualEffect*> effects = LLVfxManager::instance().getEffects();
    std::string label_mode = "";
    std::stringstream str; // for constructing the labels
    std::string str_modes[(int)RlvSphereEffect::ESphereMode::Count] { "blend", "blur", "kblur", "chroma", "pix" };
    BOOST_FOREACH(LLVisualEffect* visual_effect, effects)
    {
        if (visual_effect)
        {
            RlvSphereEffect* sphere_effect = dynamic_cast<RlvSphereEffect*> (visual_effect);
            if (sphere_effect)
            {
                RlvSphereEffect::ESphereMode mode = sphere_effect->getMode();
                if ((int)mode >= 0 && (int)mode < (int)RlvSphereEffect::ESphereMode::Count)
                {
                    str << "ss-" << str_modes[(int)mode] << "-";
                    label_mode = str.str();
                    str.str(std::string());
                }
                else
                {
                    label_mode = "ss-unknown-";
                }

                // Distances
                str << label_mode << "distmin/max";
                sdModifierColumns[0]["value"] = str.str();
                sdModifierColumns[1]["value"] = KokuaRLVFloaterSupport::getModifierText(sphere_effect->getDistanceMin(), EXTREMUM);
                sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(sphere_effect->getDistanceMax(), EXTREMUM);
                pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
                str.str(std::string());

                // Values
                str << label_mode << "valuemin/max";
                sdModifierColumns[0]["value"] = str.str();
                sdModifierColumns[1]["value"] = KokuaRLVFloaterSupport::getModifierText(sphere_effect->getValueMin(), EXTREMUM);
                sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(sphere_effect->getValueMax(), EXTREMUM);
                pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
                str.str(std::string());

                // Distextend
                str << label_mode << "distextend";
                sdModifierColumns[0]["value"] = str.str();
                sdModifierColumns[1]["value"] = "";
                sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText((int)sphere_effect->getDistExtend(), EXTREMUM);
                pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
                str.str(std::string());

                // Origin
                str << label_mode << "origin";
                sdModifierColumns[0]["value"] = str.str();
                sdModifierColumns[1]["value"] = "";
                sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText((int)sphere_effect->getOrigin(), EXTREMUM);
                pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
                str.str(std::string());

                // Tween
                str << label_mode << "tween";
                sdModifierColumns[0]["value"] = str.str();
                sdModifierColumns[1]["value"] = "";
                sdModifierColumns[2]["value"] = KokuaRLVFloaterSupport::getModifierText(sphere_effect->getTweenDuration(), EXTREMUM);
                pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
                str.str(std::string());

                // Params
                str << label_mode << "params";
                LLVector4 params = sphere_effect->getParams();
                sdModifierColumns[0]["value"] = str.str();
                sdModifierColumns[1]["value"] = "";
                sdModifierColumns[2]["value"] = llformat("%.1f, %.1f, %.1f, %.1f", params.mV[0], params.mV[1], params.mV[2], params.mV[3]);
                pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
                str.str(std::string());
            }
        }

    }
}


void KokuaFloaterRLVStatus::onAvatarNameLookup(const LLUUID& idAgent, const LLAvatarName& avName)
{
    uuid_vec_t::iterator itLookup = std::find(m_PendingLookup.begin(), m_PendingLookup.end(), idAgent);
    if (itLookup != m_PendingLookup.end())
    {
        m_PendingLookup.erase(itLookup);
    }
    if (getVisible())
    {
        refreshRLVStatus();
    }
}

//static
KokuaFloaterRLVStatus* KokuaFloaterRLVStatus::getBase()
{
    return LLFloaterReg::findTypedInstance<KokuaFloaterRLVStatus>("rlv_status");
}

// ---- RLV Worn

KokuaFloaterRLVWorn::KokuaFloaterRLVWorn(const LLSD& sdKey) : LLFloater(LLSD(sdKey))
{
}

KokuaFloaterRLVWorn::~KokuaFloaterRLVWorn()
{
    gInventory.removeObserver(mInventoryObserver);
}

BOOL KokuaFloaterRLVWorn::postBuild()
{
    getChild<LLUICtrl>("refresh_btn")->setCommitCallback(boost::bind(&KokuaFloaterRLVWorn::onBtnRefresh, this));
    return TRUE;
}

void KokuaFloaterRLVWorn::onOpen(const LLSD& key)
{
    //Although we also get refresh notifications by looking for RLV's potential
    //wear/object attach/detach notifications they aren't reliable. In particular,
    //some are sent before performing the action to which they refer so we end up
    //doing a floater status refresh before anything has changed. Therefore, we
    //also use an Inventory Observer to be sure we independently get advised of
    //changes to worn/unworn status
    mInventoryObserver = new KokuaRLVInventoryObserver;
    gInventory.addObserver(mInventoryObserver);

  refreshWornStatus();
}

void KokuaFloaterRLVWorn::onClose(bool quitting)
{
    gInventory.removeObserver(mInventoryObserver);
}

void KokuaFloaterRLVWorn::onBtnRefresh()
{
    // Hopefully never needed with the notify hook and the inventory observer
    refreshWornStatus();
}

void KokuaFloaterRLVWorn::callRefreshWornStatus()
{
    if (KokuaFloaterRLVWorn::getBase())
    {
        KokuaFloaterRLVWorn::getBase()->refreshWornStatus();
    }
}

void KokuaFloaterRLVWorn::refreshWornStatus()
{
    if (!(KokuaFloaterRLVWorn::getBase() && KokuaFloaterRLVWorn::getBase()->isShown())) return;
    
    awaiting_idle_for_worn = false;
    LLCtrlListInterface* pAttachedList = childGetListInterface("attached_list");
    LLCtrlListInterface* pAttachPtList = childGetListInterface("attachpt_list");
    LLCtrlListInterface* pFolderList = childGetListInterface("folder_list");
    LLCtrlListInterface* pClothingList = childGetListInterface("clothing_list");
    if ((!pAttachedList) || (!pAttachPtList) || (!pFolderList) || (!pClothingList) || !isAgentAvatarValid())
    {
        return;
    }

    pAttachedList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pAttachPtList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pFolderList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pClothingList->operateOnAll(LLCtrlListInterface::OP_DELETE);

    //
    // Set-up a row we can just reuse
    //
    LLSD sdAttachedRow; LLSD& sdAttachedColumns = sdAttachedRow["columns"];
    sdAttachedColumns[0] = LLSD().with("column", "item").with("type", "text");
    sdAttachedColumns[1] = LLSD().with("column", "wearpt").with("type", "text");
    sdAttachedColumns[2] = LLSD().with("column", "lock").with("type", "text");

    LLSD sdAttachPtRow; LLSD& sdAttachPtColumns = sdAttachPtRow["columns"];
    sdAttachPtColumns[0] = LLSD().with("column", "attachpt").with("type", "text");
    sdAttachPtColumns[1] = LLSD().with("column", "items").with("type", "text");
    sdAttachPtColumns[2] = LLSD().with("column", "canadd").with("type", "text");
    sdAttachPtColumns[3] = LLSD().with("column", "candetach").with("type", "text");

    LLSD sdFolderRow; LLSD& sdFolderColumns = sdFolderRow["columns"];
    sdFolderColumns[0] = LLSD().with("column", "behaviour").with("type", "text");
    sdFolderColumns[1] = LLSD().with("column", "option").with("type", "text");
    sdFolderColumns[2] = LLSD().with("column", "source").with("type", "text");

    LLSD sdWearRow; LLSD& sdWearColumns = sdWearRow["columns"];
    sdWearColumns[0] = LLSD().with("column", "wearpoint").with("type", "text");
    sdWearColumns[1] = LLSD().with("column", "item").with("type", "text");
    sdWearColumns[2] = LLSD().with("column", "addblock").with("type", "text");
    sdWearColumns[3] = LLSD().with("column", "remblock").with("type", "text");

    // this is adapted from RRInterface::getAttachments 
    std::string attachpt = "";
    std::string res = "0";
    std::string name;

    LLVOAvatar* avatar = gAgentAvatarp;
    if (avatar)
    {
        for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin();
            iter != avatar->mAttachmentPoints.end(); iter++)
        {
            LLVOAvatar::attachment_map_t::iterator curiter = iter;
            LLViewerJointAttachment* attachment = curiter->second;

            // first panel - worn attachments
            if (attachment->getNumObjects() > 0)
            {
                for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator iter = attachment->mAttachedObjects.begin();
                    iter != attachment->mAttachedObjects.end();
                    iter++)
                {
                    LLViewerObject *attached_object = (*iter);

                    sdAttachedColumns[0]["value"] = attached_object->getAttachmentItemName();
                    sdAttachedColumns[1]["value"] = attachment->getName();
                    sdAttachedColumns[2]["value"] = gAgent.mRRInterface.canDetachWithExplanation(attached_object);
                    pAttachedList->addElement(sdAttachedRow, ADD_BOTTOM);
                }
            }

            // second panel - the attach points
            sdAttachPtColumns[0]["value"] = attachment->getName();
            sdAttachPtColumns[1]["value"] = llformat("%d", attachment->getNumObjects());
            sdAttachPtColumns[2]["value"] = gAgent.mRRInterface.canAttachWithExplanation((LLViewerObject*)NULL, attachment->getName(), false);
            sdAttachPtColumns[3]["value"] = gAgent.mRRInterface.canDetachWithExplanation(attachment->getName());

            pAttachPtList->addElement(sdAttachPtRow, ADD_BOTTOM);
        }
    }

    // panel 3 summarises everything affecting inventory, ie (un)shared(un)wear, detach*, *attach*
    // we won't exclude commands operating on attachpts or clothing to present a complete picture
    // since a locked item in a folder prevents a folder detach being performed

    RRMAP::iterator it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
    std::string object_name = "";

    while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end())
    {
        std::string option = "";
        std::string behav = it->second;
        LLStringUtil::toLower(behav);

        // Odd compiler behaviour (Windows, VS2013) - if this test is > -1 rather than != -1 it'll never return true
        // Exclude touchattach(self) so we can then include everything else that matches *attach*
        if (behav.find("touchattach") == -1
            && (behav.find("shared") != -1
            || behav.find("attach") != -1
            || behav.find("detach") != -1))
        {
            int ind = behav.find(":");
            if (ind != -1)
            {
                option = behav.substr(ind + 1);
                behav = behav.substr(0, ind);
            }
            sdFolderColumns[0]["value"] = behav;
            sdFolderColumns[1]["value"] = option;
            sdFolderColumns[2]["value"] = KokuaRLVFloaterSupport::getNameFromUUID(LLUUID(it->first), true);

            pFolderList->addElement(sdFolderRow, ADD_BOTTOM);
        }
        it++;
    }

    // panel 4 iterates through the clothing layers reporting worn items and the attach/detach status
    // for that clothing layer

    int ind_wt = 0;
    while (ind_wt < LLWearableType::WT_COUNT)
    {
        // do this as a while to make the early exit more readable, unfortunately getWearableCount
        // isn't public although it exists
        int ind_layers = 0;
        while (ind_layers < LLWearableData::MAX_CLOTHING_LAYERS)
        {
            LLViewerInventoryItem* vi_item = gInventory.getItem(gAgentWearables.getWearableItemID((LLWearableType::EType)ind_wt, ind_layers));

            if (!vi_item)
            {
                // even if it's empty we want to give it a row in the floater
                sdWearColumns[0]["value"] = gAgent.mRRInterface.getOutfitLayerAsString((LLWearableType::EType)ind_wt);
                sdWearColumns[1]["value"] = "(empty)";
                BOOL can_wear = gAgent.mRRInterface.canWear((LLWearableType::EType)ind_wt, false);
                if (can_wear)
                {
                    sdWearColumns[2]["value"] = "unlocked";
                }
                else
                {
                    sdWearColumns[2]["value"] = "locked";
                }
                // we don't need to think about unwearing from the must-be-one-or-more spots at this point
                // since it's empty (which it shouldn't remain as) right now
                BOOL can_unwear = gAgent.mRRInterface.canUnwear((LLWearableType::EType)ind_wt);
                if (can_unwear)
                {
                    sdWearColumns[3]["value"] = "unlocked";
                }
                else
                {
                    sdWearColumns[3]["value"] = "locked";
                }
                pClothingList->addElement(sdWearRow, ADD_BOTTOM);
                ind_layers = LLWearableData::MAX_CLOTHING_LAYERS;
            }
            else
            {
                // one or more items worn, so go through each one and add a line of output
                do
                {
                    sdWearColumns[0]["value"] = gAgent.mRRInterface.getOutfitLayerAsString((LLWearableType::EType)ind_wt);
                    sdWearColumns[1]["value"] = vi_item->getName();

                    BOOL can_wear = gAgent.mRRInterface.canWear((LLWearableType::EType)ind_wt, false);
                    // ideally this would return false if LLWearableData::MAX_CLOTHING_LAYERS is reached, but we won't try to
                    // allow for that here
                    if (can_wear)
                    {
                        sdWearColumns[2]["value"] = "unlocked";
                    }
                    else
                    {
                        sdWearColumns[2]["value"] = "locked";
                    }
                    // this time we check detach for the item itself, not the attach point
                    // however we also need to add a warning if it's one of the must-be-there points
                    BOOL can_unwear = gAgent.mRRInterface.canUnwear(vi_item);
                    if (can_unwear)
                    {
                        LLViewerWearable* vw_item = gAgentWearables.getViewerWearable((LLWearableType::EType)ind_wt, ind_layers);
                        if (gAgentWearables.canWearableBeRemoved(vw_item))
                        {
                            sdWearColumns[3]["value"] = "unlocked";
                        }
                        else
                        {
                            sdWearColumns[3]["value"] = "replace only";
                        }
                    }
                    else
                    {
                        sdWearColumns[3]["value"] = "locked";
                    }
                    pClothingList->addElement(sdWearRow, ADD_BOTTOM);

                    ind_layers++;
                    vi_item = gInventory.getItem(gAgentWearables.getWearableItemID((LLWearableType::EType)ind_wt, ind_layers));
                } while (vi_item);
                ind_layers = LLWearableData::MAX_CLOTHING_LAYERS;
            }
        }
        ind_wt++;
    }
}

//static
KokuaFloaterRLVWorn* KokuaFloaterRLVWorn::getBase()
{
    return LLFloaterReg::findTypedInstance<KokuaFloaterRLVWorn>("rlv_worn");
}