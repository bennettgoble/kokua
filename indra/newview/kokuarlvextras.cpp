/** 
 * @file kokuarlvextras.cpp
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

#include "llviewerprecompiledheaders.h"
#include <map>

#include "kokuarlvextras.h"
#include "kokuarlvfloaters.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llavatarnamecache.h"
#include "llerror.h"
#include "llimview.h"
#include "llsdserialize.h"
#include "llviewercontrol.h" 
#include "llworld.h"
#include "RRInterface.h"

// ============================================================================
// Forward declarations
//

// llviewermenu.cpp
LLVOAvatar* find_avatar_from_object(LLViewerObject* object);

// This file is intended for Kokua RLV features where including them in RRInterface would make it
// difficult to merge as well as making an already large source file even larger. Its initial
// purpose is to house the new routine for more intelligent choice of anonymised names however
// it's foreseen that other code will go in here over time.

kokua_rlv_extras_handler_t gKokuaRLVExtrasHandler;

std::string KokuaRLVExtras::m_StringMapPath;
 
struct embedded_anonym_t
{
    std::string mOriginalName;
    U32 mLastReferenced;
};
typedef std::map<std::string, embedded_anonym_t > anonym_map_t;
anonym_map_t mAnonEntries;

typedef std::map<std::string, LLUUID > name_map_t; 
 
// ---- all initialisation, called from llStartup just after other RLV setup

void KokuaRLVExtras::initialise()
{
    // the file loading is adapted from Kitty Barnett's RLVa code
    static bool fInitialized = false;
    if (!fInitialized)
    {
        KokuaRLVExtras::getInstance()->mLaunchTimestamp = LLDate::now().secondsSinceEpoch();
        // Load the default string values
        std::vector<std::string> files = gDirUtilp->findSkinnedFilenames(LLDir::XUI, KOKUA_RLV_NAMES_FILE, LLDir::ALL_SKINS);
        m_StringMapPath = (!files.empty()) ? files.front() : LLStringUtil::null;
        for (std::vector<std::string>::const_iterator itFile = files.begin(); itFile != files.end(); ++itFile)
        {
            loadFromFile(*itFile);
        }

        // Sanity check
        if ( (mAnonEntries.empty()) )
        {
            LL_ERRS() << "Problem parsing RLV names XML file" << LL_ENDL;
            return;
        }

        fInitialized = true;
    }   
}

// this is also adapted from Kitty's routine, however we're only using it to bring in names since
// the rest of RLV is inherently not internationalisable. We also change from vector to map
// for storage since we need added data associated with each anonym
void KokuaRLVExtras::loadFromFile(const std::string& strFilePath)
{
    llifstream fileStream(strFilePath.c_str(), std::ios::binary); LLSD sdFileData;
    if ( (!fileStream.is_open()) || (!LLSDSerialize::fromXMLDocument(sdFileData, fileStream)) )
        return;
    fileStream.close();

    if (sdFileData.has("anonyms"))
    {
        const LLSD& sdAnonyms = sdFileData["anonyms"];
        U32 initialStamp = LLDate::now().secondsSinceEpoch();
        for (LLSD::array_const_iterator itAnonym = sdAnonyms.beginArray(); itAnonym != sdAnonyms.endArray(); ++itAnonym)
        {
            struct embedded_anonym_t new_anon;
            new_anon.mOriginalName = "";
            new_anon.mLastReferenced = initialStamp;
            mAnonEntries[(*itAnonym).asString()] = new_anon;
        }
    }
}

std::string KokuaRLVExtras::kokuaGetDummyName (std::string name)
{
    // KKA-646
    // RLV and RLVa's original routines both use a hash algorithm for picking the anonym to associate with
    // a name. The repeatability is needed so that mapping is consistent during a session however it has
    // the downside that it can happen there are only two avatars around and they both get given the same
    // anonym. This routine is slightly smarter and ensures double assignment won't happen by accident.
    // The approach taken is to assign an item with the oldest access time, discarding the previous use
    // if necessary
    //
    // However, it can happen that there are too many names in close proximity. To avoid cache thrashing
    // and the likelihood of anon names changes as a consequence we fall back to a hash-based selection
    // if we find that all our timestamps are the same
    
    anonym_map_t::iterator oldest = mAnonEntries.end();
    anonym_map_t::iterator matched = mAnonEntries.end();
    U32 oldest_stamp = -1; //ie maximum value
    U32 oldest_count = 0;
    anonym_map_t::iterator itAnonym = mAnonEntries.begin();
        
    while (itAnonym != mAnonEntries.end() && matched == mAnonEntries.end())
    {
        if ((itAnonym->second).mOriginalName == name) matched = itAnonym;
        if ((itAnonym->second).mLastReferenced == oldest_stamp) oldest_count++;
        if ((itAnonym->second).mLastReferenced < oldest_stamp)
        {
            oldest = itAnonym;
            oldest_stamp = (itAnonym->second).mLastReferenced;
            oldest_count = 1;
        }
        itAnonym++;
    }
    
    if (matched != mAnonEntries.end())
    {
        // LL_INFOS() << "matched " << name << " to " << (matched->second).mOriginalName << " as " << matched->first<< LL_ENDL;
        (matched->second).mLastReferenced = LLDate::now().secondsSinceEpoch();
        return matched->first;
    }
    
    if (oldest_count == mAnonEntries.size())
    {
        // We're in a pathological situation where there are more nearby names than we have anonyms for.
        // Fall back to hash-based determination of which entry to reuse so that we can avoid the effect
        // of avatars changing anonyms as they fall out of the cache and come back in. For this we're using
        // Kitty's algorithm but with Marine's nudge based on the login time for session to session variance
        
        const char* pszName = name.c_str();
        U32 nHash = 0;
        anonym_map_t::iterator nthitem = mAnonEntries.begin();
        U32 nthcount = 0;
        
        for (int idx = 0, cnt = name.length(); idx < cnt; idx++)
        {
            nHash += pszName[idx];
        }
            
        nHash = nHash + (mLaunchTimestamp >> 16); // add some session to session variability per Marine's implementation

        nHash = nHash % mAnonEntries.size();
      
        // there doesn't seem to be an easy way to pull the Nth item of a map without walking through, so...
        
        while (nthcount < nHash)
        {
            nthitem++;
            nthcount++;
        }
        
        // LL_INFOS() << "Overflow situation, chose hash item " << nHash << LL_ENDL;

        (nthitem->second).mLastReferenced = LLDate::now().secondsSinceEpoch();
        (nthitem->second).mOriginalName = name;
        return nthitem->first;  
    }
    
    // LL_INFOS() << "Not found, reusing " << oldest->first << " occupied by " << (oldest->second).mOriginalName << LL_ENDL;
    (oldest->second).mLastReferenced = LLDate::now().secondsSinceEpoch();
    (oldest->second).mOriginalName = name;
    return oldest->first;   
}

std::string KokuaRLVExtras::kokuaGetCensoredMessage(std::string str, bool anon_name)
{
    // HACK: if the message is under the form secondlife:///app/agent/UUID/about, clear it
    // (we could just as well clear just the first part, or return a bogus message, 
    // the purpose here is to avoid showing the profile of an avatar and displaying their name on the chat)
    //if (str.find("secondlife:///app/agent/") == 0)
    //{
    //  return "";
    //}
    //
    // KKA-885 Instead of bailing out, extract the uuid, then the name, then anonymise it if necessary
    // Also note there's a bug in the commented code - it would only match if the slurl was at the string start
    // Improvement: this almost always gets a name cache hit, so also check if the avatar is nearby (in fact,
    // only do a name substitution and let the later code handle the anonymising
    size_t found;
    std::string signature="secondlife:///app/agent/";
    U32 uuid_length=36;
    found = str.find(signature);
    while (found != std::string::npos)
    {
            LLUUID uuid;
            LLAvatarName av_name;
            size_t term;
            std::string user_name;
            std::string suuid = str.substr(found+signature.length(),uuid_length);
            uuid.set(suuid, FALSE);
            if (LLAvatarNameCache::get(uuid, &av_name))
            {
                user_name = av_name.getUserName();
                //rather than use a dictionary of known operations (eg /about) we track to the first non-lower case to decide the length
                term = str.find_first_not_of("abcdefghijklmnopqrstuvwxyz",found + signature.length() + uuid_length + 1);
                if (term == std::string::npos) term = str.length();
                str = str.replace(found, term-found, user_name); // not anonymised - let the later code do that
            }
            found = str.find(signature,++found);
    }

    // First we need to build a list of all the exceptions to @shownames and @shownametags
    // Each avatar object which UUID is contained in this list should not be censored
    std::string command;
    std::string behav;
    std::string option;
    std::string param;
    std::deque<LLUUID> exceptions;
    RRMAP::iterator it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
    while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end()) {
        command = it->second;
        LLStringUtil::toLower(command);
        if (command.find("shownames:") == 0 || command.find("shownames_sec:") == 0 || command.find("shownametags:") == 0) {
            // KKA-635 this needs the =n to work
            if (gAgent.mRRInterface.parseCommand(command+"=n", behav, option, param)) {
                LLUUID uuid;
                uuid.set(option, FALSE);
                exceptions.push_back(uuid);
            }
        }
        it++;
    }
    
    // KKA-638 Previously this searched the object cache to find avatars which apart from being an intensive
    // operation had the problem that avatars within parcels with visibility turned off don't appear in the 
    // object cache. Instead, we now use the same method used by the minimap to get its avatar list
    
    std::string pre_str = str;
    uuid_vec_t avatar_ids;
    std::vector<LLVector3d> positions;
    
    LLWorld::getInstance()->getAvatars(&avatar_ids, &positions, gAgentCamera.getCameraPositionGlobal());

    // KKA-658 Having shifted this out of RRInterface to here, the way is clear to rewrite the routine
    // entirely to take advantage of maps being ordered, therefore reverse iterating will gives us
    // longest names first which is what we need to avoid the problems with names that are substrings
    // of other names

    name_map_t name_map;
    
    for (U32 i = 0; i < avatar_ids.size(); i++)
    {
        LLUUID uuid = avatar_ids[i];

        LLAvatarName av_name;
        if (LLAvatarNameCache::get(uuid, &av_name))
        {
            name_map[av_name.getUserName()] = uuid;
            name_map[av_name.getUserName()+" Resident"] = uuid;
            name_map[LLCacheName::cleanFullName(av_name.getUserName())] = uuid;
            name_map[LLCacheName::cleanFullName(av_name.getUserName()) + " Resident"] = uuid;
            name_map[av_name.mDisplayName] = uuid; // not "getDisplayName()" because we need this whether we use display names or user names
            name_map[av_name.mUsername] = uuid; // needed for minimap and maybe others now they come through here
        }
    }
                    
    name_map_t::reverse_iterator name_it = name_map.rbegin(); // important: iterate in reverse order to do longest strings first
        
    while (name_it != name_map.rend())
    {
        LLUUID uuid = name_it->second;

        LLAvatarName av_name;
        if (LLAvatarNameCache::get(uuid, &av_name))
        {
            std::string  dummy_name = kokuaGetDummyName(av_name.getUserName());
                                                        
            if (std::find(exceptions.begin(), exceptions.end(), uuid) == exceptions.end())
            {
                // no exceptions for this uuid
                if (pre_str == name_it->first)
                {
                    // exact name match, return dummy name and finish
                    str = dummy_name;
                    anon_name = false;
                    break;
                }
                else
                {
                    //no name match / a longer string than just a name, do the general substitution
                    str = gAgent.mRRInterface.stringReplaceWholeWord(str, name_it->first, dummy_name);                  
                }
            }
            else
            {
                // there is an exception for this uuid
                if (pre_str == name_it->first)
                {
                    // it matches, return original string
                    str = pre_str;
                    anon_name = false;
                    break;
                }               
            }
        }
        name_it++;
    }
    
    // if we are required to anonymise this and haven't found a match, do it here
    if (str.compare(pre_str) == 0 && anon_name) str = kokuaGetDummyName(str);
    return str;
}

// KKA-666 RLV doesn't have this, so we need our own...
bool KokuaRLVExtras::canSit(LLViewerObject* pObj, const LLVector3& pickIntersect)
{
    // logic lifted from llviewermenu sit enabling code for menus
    // fartouch test added since fartouch will prevent click sits over distance
    if (!pObj || !gRRenabled) return false;
    
    LLVOAvatar* avatar = gAgentAvatarp;
    
    if (avatar && gAgent.mRRInterface.mContainsUnsit && avatar->mIsSitting) return false;
    
    if (gAgent.mRRInterface.mContainsInteract) return false;
    
    if (gAgent.mRRInterface.contains("sit")) return false;
    
    if ((gAgent.mRRInterface.mSittpMax < EXTREMUM) || (gAgent.mRRInterface.mFartouchMax < EXTREMUM))
    {
        LLVector3 pos = pObj->getPositionRegion();
        if (pickIntersect != LLVector3::zero) pos = pickIntersect;
        pos -= gAgent.getPositionAgent();
    
        if (pos.magVec() >= gAgent.mRRInterface.mSittpMax) return false;    
        if (pos.magVec() >= gAgent.mRRInterface.mFartouchMax) return false; 
    }
    return true;
}

// KKA-666 RLV doesn't have this either, so do our own too (logic adapted from RLVa)
bool KokuaRLVExtras::canInteract(LLViewerObject* pObj, const LLVector3& pickIntersect)
{
    if (!gRRenabled) return true;

    // User can interact with the specified object if:
    //   - not interaction restricted (or the specified object is a HUD attachment)
    //   - not prevented from touching faraway objects (or the object's center + pick offset is within range)

    return
        ( (!pObj) || (pObj->isHUDAttachment()) ||
            ( !gAgent.mRRInterface.mContainsInteract && gAgent.mRRInterface.canTouchFar(pObj, pickIntersect)) );
}

// KKA-801 The can send IM and can receive IM tests became more complex with the addition of group IM features
// so I'd rather have the tests in only one place rather than duplicated around
bool KokuaRLVExtras::cannotSendIM(const LLUUID& im_session_id, const std::string& other_party)
{
    LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(im_session_id);
        
    if (gRRenabled)
    {
        // bugfix: we only have a session if it's a group chat
        if (session && session->isGroupChat())
        {
            std::string all_groups = "allgroups"; // by default, "allgroups" means "allow to send IMs to all groups", but this name may be replaced by a specific group name
            std::string group_name = session->mName;
            group_name = gAgent.mRRInterface.stringReplace(session->mName, ",", ""); // remove the "," from the group name to check against the RLV restrictions as "," is supposed to be a high-level separator
            group_name = gAgent.mRRInterface.stringReplace(session->mName, ";", ""); // ";" could also be a separator in the future so let's preemptively remove it here
            if ((gAgent.mRRInterface.containsWithoutException("sendim", group_name) && gAgent.mRRInterface.containsWithoutException("sendim", all_groups))
            || gAgent.mRRInterface.contains("sendimto:" + group_name)
            || gAgent.mRRInterface.contains("sendimto:" + all_groups)
            )
            {
                // user is forbidden to send IMs and the receiver group is no exception
                return true;
            }
        }
        else
        {
            if (gAgent.mRRInterface.containsWithoutException("sendim", other_party)
            || gAgent.mRRInterface.contains("sendimto:" + other_party)
            )
            {
                // user is forbidden to send IMs and the receiver avatar is no exception
                return true;
            }
        }
    }
    return false;
}

bool KokuaRLVExtras::cannotRecvIM(const LLUUID& im_session_id, const std::string& other_party)
{
    LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(im_session_id);
    if (gRRenabled)
    {
        // bugfix: we only have a session if it's a group chat
        if (session && session->isGroupChat())
        {
            std::string all_groups = "allgroups"; // by default, "allgroups" means "allow to send IMs to all groups", but this name may be replaced by a specific group name
            std::string group_name = session->mName;
            group_name = gAgent.mRRInterface.stringReplace(session->mName, ",", ""); // remove the "," from the group name to check against the RLV restrictions as "," is supposed to be a high-level separator
            group_name = gAgent.mRRInterface.stringReplace(session->mName, ";", ""); // ";" could also be a separator in the future so let's preemptively remove it here
            if (((gAgent.mRRInterface.containsWithoutException("recvim", other_party) || gAgent.mRRInterface.contains("recvimfrom:" + other_party))
                && gAgent.mRRInterface.containsWithoutException("recvim", group_name) && gAgent.mRRInterface.containsWithoutException("recvim", all_groups))
                || gAgent.mRRInterface.contains("recvimfrom:" + group_name)
                || gAgent.mRRInterface.contains("recvimfrom:" + all_groups)
                )
            {
                return true;
            }
        }
        else
        {
            if (gAgent.mRRInterface.containsWithoutException("recvim", other_party)
                || gAgent.mRRInterface.contains("recvimfrom:" + other_party))
            {
                return true;
            }           
        }
    }
    return false;
}

// generate a terse RLV summary in the log file
void KokuaRLVExtras::writeLogSummary()
{
    RRMAP::iterator it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
    S32 reporttype = gSavedSettings.getU32("KokuaRLVLogSummaryType");

    if (it == gAgent.mRRInterface.mSpecialObjectBehaviours.end())
    {
        if (reporttype)
        {
            LL_INFOS("RLV") << "RLV restrictions: none" << LL_ENDL;
        }
    }
    else
    {
        if (reporttype & 1)
        {
            // this is a very terse format intended to avoid bloating the log files - don't include originating
            // objects and don't include duplicates
            std::vector<std::string> output;
            std::string outputline;

            while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end())
            {
                std::string action = it->second;
                if (std::find(output.begin(), output.end(), action) == output.end())
                {
                    output.push_back(action);
                }
                it++;
            }
            for (std::vector<std::string>::const_iterator p = output.begin() ; p != output.end(); ++p)
            {
                if (p != output.begin())
                {
                    outputline += " ";
                }
                outputline += *p;
            }
            LL_INFOS("RLV") << "RLV terse: " << outputline << LL_ENDL;
        }
        
        if (reporttype & 2)
        {
            // this is screen friendly using \n but not very log friendly as a result
            LL_INFOS("RLV") << "RLV list (one-line): " << gAgent.mRRInterface.getRlvRestrictions() << LL_ENDL;
        }
        
        if (reporttype & 4)
        {
            // adapted from RRInterface for one log line per output line, more screen-friendly than log friendly
            it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
            std::string object_name = "";
            std::string old_object_name = "";
            std::string res = "";
            while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end())
            {
                old_object_name = object_name;
                object_name = KokuaRLVFloaterSupport::getNameFromUUID(LLUUID(it->first),true);

                // print the name of the object
                if (object_name != old_object_name) {
                    if (res != "") LL_INFOS("RLV") << "RLV list: " << res << LL_ENDL;
                    res = "### " + object_name + " #####################";
                }
                if (res != "") LL_INFOS("RLV") << "RLV list: " << res << LL_ENDL;
                res = "-------- " + it->second;

                it++;
            }
            if (res != "") LL_INFOS("RLV") << "RLV list: " << res << LL_ENDL;
        }

        if (reporttype & 8)
        {
            // adapted from RRInterface for log-friendly single line per item output
            it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
            std::string object_name = "";
            std::string old_object_name = "";
            std::string res = "";
            while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end())
            {
                old_object_name = object_name;
                object_name = KokuaRLVFloaterSupport::getNameFromUUID(LLUUID(it->first),true);

                // new object? output line so far if so and start a new line
                if (object_name != old_object_name) {
                    if (res != "") LL_INFOS("RLV") << "RLV source: " << res << LL_ENDL;
                    res = object_name + ":";
                }
                res += " " + it->second;
                it++;
            }
            if (res != "") LL_INFOS("RLV") << "RLV source: " << res << LL_ENDL;
        }
    }
}

// These two are adapted from RLVa's equivalents to make porting easier

bool KokuaRLVExtrasMenuCanShowName()
{
    bool fEnable = true;
    if (gRRenabled)
    {
      const LLVOAvatar* pAvatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
      if (pAvatar)
      {
          //return (pAvatar) && (RlvActions::canShowName(RlvActions::SNC_DEFAULT, pAvatar->getID()));
          fEnable = !gAgent.mRRInterface.containsWithoutException("shownames",pAvatar->getID().asString());
        }
    }
    return fEnable;
}

bool KokuaRLVExtrasMenuEnableIfNot(const LLSD& sdParam)
{
    bool fEnable = true;
    if (gRRenabled)
    {
        fEnable = !gAgent.mRRInterface.containsWithoutException(sdParam.asString());
    }
    return fEnable;
}