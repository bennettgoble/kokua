/** 
 * @file RRInterface.h
 * @author Marine Kelley
 * @brief The header for all RLV features
 *
 * RLV Source Code
 * The source code in this file ("Source Code") is provided by Marine Kelley
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Marine Kelley.  Terms of
 * the GPL can be found in doc/GPL-license.txt in the distribution of the
 * original source of the Second Life Viewer, or online at 
 * http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL SOURCE CODE FROM MARINE KELLEY IS PROVIDED "AS IS." MARINE KELLEY 
 * MAKES NO WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING 
 * ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
 */

#ifndef LL_RRINTERFACE_H
#define LL_RRINTERFACE_H

#define RR_VIEWER_NAME "RestrainedLife"
#define RR_VIEWER_NAME_NEW "RestrainedLove"
#define RR_VERSION_NUM "2080304"
#define RR_VERSION "2.08.03.04"
#define RR_SLV_VERSION "3.3.4.23851"

#define RR_PREFIX "@"
#define RR_SHARED_FOLDER "#RLV"
#define RR_RLV_REDIR_FOLDER_PREFIX "#RLV/~"
// Length of the "#RLV/" string constant in characters.
#define RR_HRLVS_LENGTH 5

// Set to 1 for Tattoo and Alpha wearables support
#define ALPHA_AND_TATTOO 1

// wearable types as strings
#define WS_ALL "all"
#define WS_EYES "eyes"
#define WS_SKIN "skin"
#define WS_SHAPE "shape"
#define WS_HAIR "hair"
#define WS_GLOVES "gloves"
#define WS_JACKET "jacket"
#define WS_PANTS "pants"
#define WS_SHIRT "shirt"
#define WS_SHOES "shoes"
#define WS_SKIRT "skirt"
#define WS_SOCKS "socks"
#define WS_UNDERPANTS "underpants"
#define WS_UNDERSHIRT "undershirt"
#if ALPHA_AND_TATTOO
#define WS_ALPHA "alpha"
#define WS_TATTOO "tattoo"
#define WS_PHYSICS "physics"
#endif

//#include <set>
#include <deque>
#include <map>
#include <string>

#include "lluuid.h"
#include "llframetimer.h"

#include "llchat.h"
#include "llchatbar.h"
#include "llinventorymodel.h"
#include "llviewermenu.h"
#include "llviewerjointattachment.h"
#include "llwearable.h"
#include "llwearabletype.h"

extern BOOL gRRenabled;

// RLV retains its restrictions in a multimap (i.e. several entries per key), linking the restrictions to the UUIDs of the objects
typedef std::multimap<std::string, std::string> RRMAP;
typedef struct Command {
	LLUUID uuid;
	std::string command;
} Command;

// Where to attach what, when automatically reattaching an object
typedef struct AssetAndTarget {
	LLUUID uuid;
	std::string attachpt;
} AssetAndTarget;

// How to call @attach:outfit=force (useful for multi-attachments and multi-wearables
typedef enum AttachHow {
	AttachHow_replace			= 0, // default behavior
	AttachHow_under				= 1, // unusued for now
	AttachHow_over				= 2, // add on top
	AttachHow_over_or_replace	= 3, // stack if the name of the outfit begins with a special sign, otherwise replace
	AttachHow_count				= 4
} AttachHow;

// Type of the lock of a folder
typedef enum FolderLock {
	FolderLock_unlocked					= 0, // not locked
	FolderLock_locked_with_except		= 1, // locked but with exception (i.e. will be treated as unlocked)
	FolderLock_locked_without_except	= 2, // locked without exception
	FolderLock_count					= 3
} FolderLock;

class RRInterface
{
public:
	
	RRInterface ();
	~RRInterface ();

	std::string getVersion (); // returns "RestrainedLife Viewer blah blah"
	std::string getVersion2 (); // returns "RestrainedLove Viewer blah blah"
	std::string getVersionNum (); // returns "RR_VERSION_NUM[,blacklist]"
	std::string getFirstName (std::string fullName);
	std::string getLastName (std::string fullName);
	BOOL isAllowed (LLUUID object_uuid, std::string action, BOOL log_it = TRUE);
	BOOL contains (std::string action); // return TRUE if the action is contained
	BOOL containsSubstr (std::string action); // return TRUE if the action, or an action which name contains the specified name, is contained
	BOOL containsWithoutException (std::string action, std::string except = ""); // return TRUE if the action or action+"_sec" is contained, and either there is no global exception, or there is no local exception in the case of action+"_sec"
	bool isFolderLocked(LLInventoryCategory* cat); // return true if cat has a lock specified for it or one of its parents, or not shared and @unshared is active
	FolderLock isFolderLockedWithoutException (LLInventoryCategory* cat, std::string attach_or_detach); // attach_or_detach must be equal to either "attach" or "detach"
	FolderLock isFolderLockedWithoutExceptionAux (LLInventoryCategory* cat, std::string attach_or_detach, std::deque<std::string> list_of_restrictions); // auxiliary function to isFolderLockedWithoutException
	BOOL isBlacklisted (std::string action, bool force); // return TRUE if the command is blacklisted, with %f in the case of a "=force" command
	std::deque<std::string> getBlacklist (std::string filter = ""); // return the list of blacklisted commands which contain the substring specified by filter

	BOOL add (LLUUID object_uuid, std::string action, std::string option); // add a restriction
	BOOL remove (LLUUID object_uuid, std::string action, std::string option); // remove a restriction
	BOOL clear (LLUUID object_uuid, std::string command=""); // clear restrictions
	void replace (LLUUID what, LLUUID by); // replace a list of restrictions by restrictions linked to another UUID
	BOOL garbageCollector (BOOL all=TRUE); // if false, don't clear rules attached to NULL_KEY as they are issued from external objects (only cleared when changing parcel)
	std::deque<std::string> parse (std::string str, std::string sep); // utility function
	void notify (LLUUID object_uuid, std::string action, std::string suffix); // scan the list of restrictions, when finding "notify" say the action on the specified channel

	BOOL parseCommand (std::string command, std::string& behaviour, std::string& option, std::string& param);
	BOOL handleCommand (LLUUID uuid, std::string command);
	BOOL fireCommands (); // execute commands buffered while the viewer was initializing (mostly useful for force-sit as when the command is sent the object is not necessarily rezzed yet)
	BOOL force (LLUUID object_uuid, std::string command, std::string option);
	void removeItemFromAvatar (LLViewerInventoryItem* item);

	BOOL answerOnChat (std::string channel, std::string msg);
	std::string crunchEmote (std::string msg, unsigned int truncateTo = 0);

	std::string getOutfitLayerAsString (LLWearableType::EType layer);
	LLWearableType::EType getOutfitLayerAsType (std::string layer);
	std::string getOutfit (std::string layer);
	std::string getAttachments (std::string attachpt);

	std::string getStatus (LLUUID object_uuid, std::string rule); // if object_uuid is null, return all
	BOOL forceDetach (std::string attachpt);
	BOOL forceDetachByUuid (std::string object_uuid);

	BOOL hasLockedHuds ();
	std::deque<LLInventoryItem*> getListOfLockedItems (LLInventoryCategory* root);
	std::deque<std::string> getListOfRestrictions (LLUUID object_uuid, std::string rule = "");
	std::string getInventoryList (std::string path, BOOL withWornInfo = FALSE);
	std::string getWornItems (LLInventoryCategory* cat);
	LLInventoryCategory* getRlvShare (); // return pointer to #RLV folder or null if does not exist
	BOOL isUnderRlvShare (LLInventoryItem* item);
	BOOL isUnderRlvShare (LLInventoryCategory* cat);
	BOOL isUnderFolder (LLInventoryCategory* cat_parent, LLInventoryCategory* cat_child); // true if cat_child is a child of cat_parent
//	void renameAttachment (LLInventoryItem* item, LLViewerJointAttachment* attachment); // DEPRECATED
	LLInventoryCategory* getCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root = NULL);
	LLInventoryCategory* findCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root = NULL);
	std::string findAttachmentNameFromPoint (LLViewerJointAttachment* attachpt);
	LLViewerJointAttachment* findAttachmentPointFromName (std::string objectName, BOOL exactName = FALSE);
	LLViewerJointAttachment* findAttachmentPointFromParentName (LLInventoryItem* item);
	S32 findAttachmentPointNumber (LLViewerJointAttachment* attachment);
//	bool handle_detach_from_avatar(LLViewerJointAttachment* attachment);
	void detachObject(LLViewerObject* object);
	void detachAllObjectsFromAttachment(LLViewerJointAttachment* attachment);
	bool canDetachAllObjectsFromAttachment(LLViewerJointAttachment* attachment);
	void fetchInventory (LLInventoryCategory* root = NULL);

	BOOL forceAttach (std::string category, BOOL recursive, AttachHow how);
	BOOL forceDetachByName (std::string category, BOOL recursive);

	BOOL getAllowCancelTp() { return mAllowCancelTp; }
	void setAllowCancelTp(BOOL newval) { mAllowCancelTp = newval; }

	std::string getParcelName () { return mParcelName; }
	void setParcelName (std::string newval) { mParcelName = newval; }

	bool getScriptsEnabledOnce () { return mScriptsEnabledOnce; }
	void setScriptsEnabledOnce (bool newval) { mScriptsEnabledOnce = newval; }

	BOOL forceTeleport (std::string location);

	std::string stringReplace (std::string s, std::string what, std::string by, BOOL caseSensitive = FALSE);

	std::string getDummyName (std::string name, EChatAudible audible = CHAT_AUDIBLE_FULLY); // return "someone", "unknown" etc according to the length of the name (when shownames is on)
	std::string getCensoredMessage (std::string str); // replace names by dummy names

	LLUUID getSitTargetId () { return mSitTargetId; }
	void setSitTargetId (LLUUID newval) { mSitTargetId = newval; }

	BOOL forceEnvironment (std::string command, std::string option); // command is "setenv_<something>", option is a list of floats (separated by "/")
	std::string getEnvironment (std::string command); // command is "getenv_<something>"
	
	std::string getLastLoadedPreset () { return mLastLoadedPreset; }
	void setLastLoadedPreset (std::string newval) { mLastLoadedPreset = newval; }

	BOOL forceDebugSetting (std::string command, std::string option); // command is "setdebug_<something>", option is a list of values (separated by "/")
	std::string getDebugSetting (std::string command); // command is "getdebug_<something>"

	std::string getFullPath (LLInventoryCategory* cat);
	std::string getFullPath (LLInventoryItem* item, std::string option = "", bool full_list = true);
	LLInventoryItem* getItemAux (LLViewerObject* attached_object, LLInventoryCategory* root);
	LLInventoryItem* getItem (LLUUID wornObjectUuidInWorld);
	void attachObjectByUUID (LLUUID assetUUID, int attachPtNumber = 0);
	
	bool canDetachAllSelectedObjects();
	bool isSittingOnAnySelectedObject();
	
	bool canAttachCategory(LLInventoryCategory* folder, bool with_exceptions = true);
	bool canAttachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions = true);
	bool canDetachCategory(LLInventoryCategory* folder, bool with_exceptions);
	bool canDetachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions);
	bool canUnwear(LLInventoryItem* item);
	bool canUnwear(LLWearableType::EType type);
	bool canWear(LLInventoryItem* item);
	bool canWear(LLWearableType::EType type, bool from_server = false);
	bool canDetach(LLInventoryItem* item);
	bool canDetach(LLViewerObject* attached_object);
	bool canDetach(std::string attachpt);
	bool canAttach(LLViewerObject* object_to_attach, std::string attachpt, bool from_server = false);
	bool canAttach(LLInventoryItem* item, bool from_server = false);
	bool canEdit(LLViewerObject* object);
	bool canTouch(LLViewerObject* object, LLVector3 pick_intersection = LLVector3::zero); // set pick_intersection to force the check on this position
	bool canTouchFar(LLViewerObject* object, LLVector3 pick_intersection = LLVector3::zero); // set pick_intersection to force the check on this position
	
	bool scriptsEnabled(); // returns true if scripts are enabled for us in this parcel

	void listRlvRestrictions(std::string substr = "");

	// Some cache variables to accelerate common checks
	BOOL mHasLockedHuds;
	BOOL mContainsDetach;
	BOOL mContainsShowinv;
	BOOL mContainsUnsit;
	BOOL mContainsFartouch;
	BOOL mContainsShowworldmap;
	BOOL mContainsShowminimap;
	BOOL mContainsShowloc;
	BOOL mContainsShownames;
	BOOL mContainsSetenv;
	BOOL mContainsSetdebug;
	BOOL mContainsFly;
	BOOL mContainsEdit;
	BOOL mContainsRez;
	BOOL mContainsShowhovertextall;
	BOOL mContainsShowhovertexthud;
	BOOL mContainsShowhovertextworld;
	BOOL mContainsDefaultwear;
	BOOL mContainsPermissive;
	BOOL mContainsRun;
	BOOL mContainsAlwaysRun;
	BOOL mContainsTp;
	BOOL mHandleNoStrip;
	//BOOL mContainsMoveUp;
	//BOOL mContainsMoveDown;
	//BOOL mContainsMoveForward;
	//BOOL mContainsMoveBackward;
	//BOOL mContainsMoveTurnUp;
	//BOOL mContainsMoveTurnDown;
	//BOOL mContainsMoveTurnLeft;
	//BOOL mContainsMoveTurnRight;
	//BOOL mContainsMoveStrafeLeft;
	//BOOL mContainsMoveStrafeRight;

	static BOOL sRRNoSetEnv;
	static BOOL sRestrainedLoveDebug;
	static BOOL sCanOoc; // when TRUE, the user can bypass a sendchat restriction by surrounding with (( and ))
	static std::string sRecvimMessage; // message to replace an incoming IM, when under recvim
	static std::string sSendimMessage; // message to replace an outgoing IM, when under sendim
	static std::string sBlacklist; // comma-separated list of RLV commands, add "%f" after a token to indicate it is the "=force" variant

	// Allowed debug settings (initialized in the ctor)
	std::string mAllowedU32;
	std::string mAllowedS32;
	std::string mAllowedF32;
	std::string mAllowedBOOLEAN;
	std::string mAllowedSTRING;
	std::string mAllowedVEC3;
	std::string mAllowedVEC3D;
	std::string mAllowedRECT;
	std::string mAllowedCOL4;
	std::string mAllowedCOL3;
	std::string mAllowedCOL4U;

	// These should be private but we may want to browse them from the outside world, so let's keep them public
	RRMAP mSpecialObjectBehaviours;
	std::deque<Command> mRetainedCommands;

	// When a locked attachment is kicked off by another one with llAttachToAvatar() in a script, retain its UUID here, to reattach it later 
	std::deque<AssetAndTarget> mAssetsToReattach;
	LLFrameTimer mReattachTimer; // Reset each time a locked attachment is kicked by a "Wear", and on auto-reattachment timeout.
	BOOL mReattaching; // TRUE when llappviewer.cpp asked for a reattachment. FALSE when llviewerjointattachment.cpp detected a reattachment.
	BOOL mReattachTimeout; // TRUE when llappviewer.cpp detects a reattachment timeout, FALSE when llviewerjointattachment.cpp detected a reattachment.
	AssetAndTarget mJustDetached; // we need this to inhibit the removeObject event that occurs right after addObject in the case of a replacement
	AssetAndTarget mJustReattached; // we need this to inhibit the removeObject event that occurs right after addObject in the case of a replacement
	LLVector3d mLastStandingLocation; // this is the global position we had when we sat down on something, and we will be teleported back there when we stand up if we are prevented from "sit-tp by rezzing stuff"
	BOOL mSnappingBackToLastStandingLocation; // TRUE when we are teleporting back to the last standing location, in order to bypass the usual checks
	BOOL mUserUpdateAttachmentsUpdatesAll; // TRUE when we've just called "Replace CurrentOutfit" and "Remove From Current Outfit" commands, FALSE otherwise

private:
	std::string mParcelName; // for convenience (gAgent does not retain the name of the current parcel)
	bool mScriptsEnabledOnce; // to know if we have been in a script enabled area at least once (so that no-script areas prevent detaching only when we have logged in there)
	BOOL mInventoryFetched; // FALSE at first, used to fetch RL Share inventory once upon login
	BOOL mAllowCancelTp; // TRUE unless forced to TP with @tpto (=> receive TP order from server, act like it is a lure from a Linden => don't show the cancel button)
	LLUUID mSitTargetId;
	std::string mLastLoadedPreset; // contains the name of the latest loaded Windlight preset
	int mLaunchTimestamp; // timestamp of the beginning of this session
	
};


#endif
