/** 
 * @file kokuarlvfloaters.h
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

#ifndef KOKUARLVFLOATERS_H
#define KOKUARLVFLOATERS_H

#include "llagent.h"
#include "llcheckboxctrl.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llinventoryobserver.h"
#include "llmultifloater.h"

class LLTextEditor;
class LLUUID;
class LLInventoryObserver;

static std::map<LLUUID, std::string> mKnownIDs;     // known list of names for object IDs
static BOOL option_ignore_queries;
static BOOL option_auto_open;
static BOOL option_focus_on_latest;

// ---- Inventory Observer

class KokuaRLVInventoryObserver : public LLInventoryObserver
{
public:
    KokuaRLVInventoryObserver() {}
    virtual ~KokuaRLVInventoryObserver() {}
    virtual void changed(U32 mask);
};

// ---- Support functions

class KokuaRLVFloaterSupport
{
public:
    KokuaRLVFloaterSupport() {}
    ~KokuaRLVFloaterSupport() {}
    static BOOL addNameToLocalCache(LLUUID &obj_id, std::string &from_name);
    static std::string getNameFromUUID(const LLUUID& idObj, bool fIncludeAttachPt);
    static std::string getModifierText(F32 value, F32 ceiling);
    static void commandNotify(LLUUID& object_uuid, std::string& command);

private:
    static void checkForRefreshNeeded(LLUUID& object_uuid, std::string& command, bool* refresh_status, bool* refresh_worn);
};

// ---- Commands

class KokuaFloaterRLVDebug : public LLMultiFloater
{
    friend class LLMultiFloater;
    friend class LLFloaterReg;

public:
    KokuaFloaterRLVDebug(const LLSD& key);
    ~KokuaFloaterRLVDebug();
    BOOL postBuild();
    void setVisible(BOOL visible);
    void show(const LLUUID& object_id);
    static KokuaFloaterRLVDebug* getBase();
    /*virtual*/ void closeFloater(bool app_quitting = false);
    static void addRLVLine(const std::string &utf8mesg, const LLColor4& color, LLUUID& source_id);
    void refreshRLVDebug(LLUUID &obj_id);

protected:
    static LLFloater* addOutputWindow(const LLUUID& object_id);
};

class KokuaFloaterRLVDebugOutput : public LLFloater
{

public:
    KokuaFloaterRLVDebugOutput(const LLSD& object_id);
    ~KokuaFloaterRLVDebugOutput();

    void addLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color);
    void updateName(const std::string &user_name, const LLUUID &id);
    virtual BOOL postBuild();

protected:
    LLTextEditor* mHistoryEditor;
    LLUUID mObjectID;
};

// ---- RLV Console

class KokuaFloaterRLVConsole : public LLFloater
{
    friend class LLFloaterReg;

public:
    KokuaFloaterRLVConsole(const LLSD& sdKey);
    ~KokuaFloaterRLVConsole() override;
    BOOL postBuild() override;
    void onClose(bool fQuitting) override;
    static KokuaFloaterRLVConsole* getBase();
    void addCommandReply(const std::string& strReply);

protected:
    void onInput(LLUICtrl* ctrl, const LLSD& param);

protected:
    LLTextEditor* m_pOutputText;
};

// ---- RLV Status

class KokuaFloaterRLVStatus : public LLFloater
{
    friend class LLFloaterReg;

public:
    KokuaFloaterRLVStatus(const LLSD& sdKey);
    ~KokuaFloaterRLVStatus();
    BOOL postBuild();
    static KokuaFloaterRLVStatus* getBase();
    void refreshRLVStatus();
    static void callRefreshRLVStatus();

protected:
    void onBtnCopyToClipboard();
    void onCommitPauseUpdating();
    void onAvatarNameLookup(const LLUUID& idAgent, const LLAvatarName& avName);
    void onOpen(const LLSD& key);
    LLCheckBoxCtrl* mPauseUpdating;
    
protected:
    uuid_vec_t                  m_PendingLookup;

};

// ---- RLV Worn Items

class KokuaFloaterRLVWorn : public LLFloater
{
    friend class LLFloaterReg;

public:
    KokuaFloaterRLVWorn(const LLSD& sdKey);
    ~KokuaFloaterRLVWorn();
    BOOL postBuild();
    static KokuaFloaterRLVWorn* getBase();
    void refreshWornStatus();
    static void callRefreshWornStatus();

protected:
    void onBtnRefresh();
  void onOpen(const LLSD& key);
  void onClose(bool quitting);
    
protected:
    LLInventoryObserver* mInventoryObserver;
};
#endif // KOKUARLVFLOATERS_H
