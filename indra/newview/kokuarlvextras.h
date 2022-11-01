/** 
 * @file kokuarlvextras.h
 * @brief Additional RLV code separate from RRInterface
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
 
#ifndef KOKUARLVEXTRAS_H
#define KOKUARLVEXTRAS_H

#define KOKUA_SHOWNAMES
#define KOKUA_RLV_NAMES_FILE "kokua_rlv_names.xml"

#include "llviewerobject.h"

class KokuaRLVExtras
{
public:
    KokuaRLVExtras() {}
    ~KokuaRLVExtras() {}
    static void initialise();
    static void loadFromFile(const std::string& strFilePath);
    static KokuaRLVExtras& instance();
    static KokuaRLVExtras* getInstance();
    // KKA-630 internal version with additional flag to indicate if it's called for something we know is a name and should be anonymised unless an exception applies
    std::string kokuaGetCensoredMessage (std::string str, bool anon_name); // replace names by dummy names
    // KKA-666 we need a canSit and canInteract for lltoolpie.cpp - do them here rather than make merging RRInterface harder
    bool canSit(LLViewerObject* pObj, const LLVector3& pickIntersect);  
    bool canInteract(LLViewerObject* pObj, const LLVector3& pickIntersect); 
    // KKA-801 the can send IM and can receive IM tests became more complex with the addition of group IM features so centralise the tests in here
    static bool cannotSendIM(const LLUUID& im_session_id, const std::string &other_party);
    static bool cannotRecvIM(const LLUUID& im_session_id, const std::string &other_party);
    static void writeLogSummary();
private:
    U32 mLaunchTimestamp; // timestamp of the beginning of this session
    std::string kokuaGetDummyName (std::string name); // private because this doesn't do the exception detection - call kokuaGetCensoredMessage(string, true) instead
    
protected:
    static std::string  m_StringMapPath;
};

typedef KokuaRLVExtras kokua_rlv_extras_handler_t;
extern kokua_rlv_extras_handler_t gKokuaRLVExtrasHandler;

inline KokuaRLVExtras& KokuaRLVExtras::instance()
{
    return gKokuaRLVExtrasHandler;
}

inline KokuaRLVExtras* KokuaRLVExtras::getInstance()
{
    return &gKokuaRLVExtrasHandler;
}

bool KokuaRLVExtrasMenuCanShowName();
bool KokuaRLVExtrasMenuEnableIfNot(const LLSD& sdParam);

#endif // KOKUARLVEXTRAS_H