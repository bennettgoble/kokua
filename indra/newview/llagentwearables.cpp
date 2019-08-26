/** 
 * @file llagentwearables.cpp
 * @brief LLAgentWearables class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llagentwearables.h"

#include "llattachmentsmgr.h"
#include "llaccordionctrltab.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llappearancemgr.h"
#include "llcallbacklist.h"
#include "llfloatersidepanelcontainer.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "lllocaltextureobject.h"
#include "llmd5.h"
#include "llnotificationsutil.h"
#include "lloutfitobserver.h"
#include "llsidepanelappearance.h"
#include "lltexlayer.h"
#include "lltooldraganddrop.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llviewerwearable.h"
#include "llwearablelist.h"
#include "llfloaterperms.h"

//CA we need MAX_CLOTHING_PER_TYPE here for MK
#include "RRInterfaceHelper.h"
//ca
#include <boost/scoped_ptr.hpp>

LLAgentWearables gAgentWearables;

BOOL LLAgentWearables::mInitialWearablesUpdateReceived = FALSE;
// [SL:KB] - Patch: Appearance-InitialWearablesLoadedCallback | Checked: 2010-08-14 (Catznip-2.1)
bool LLAgentWearables::mInitialWearablesLoaded = false;
// [/SL:KB]

using namespace LLAvatarAppearanceDefines;

//MK
const F32 OFFSET_FACTOR = 0.66f; // This factor is arbitrary and is supposed to align the baked avatar Hover setting with the offset we wanted to apply in the first place.
//mk

///////////////////////////////////////////////////////////////////////////////

void set_default_permissions(LLViewerInventoryItem* item)
{
	llassert(item);
	LLPermissions perm = item->getPermissions();
	if (perm.getMaskNextOwner() != LLFloaterPerms::getNextOwnerPerms("Wearables")
		|| perm.getMaskEveryone() != LLFloaterPerms::getEveryonePerms("Wearables")
		|| perm.getMaskGroup() != LLFloaterPerms::getGroupPerms("Wearables"))
	{
		perm.setMaskNext(LLFloaterPerms::getNextOwnerPerms("Wearables"));
		perm.setMaskEveryone(LLFloaterPerms::getEveryonePerms("Wearables"));
		perm.setMaskGroup(LLFloaterPerms::getGroupPerms("Wearables"));

		item->setPermissions(perm);

		item->updateServer(FALSE);
	}
}

// Callback to wear and start editing an item that has just been created.
void wear_and_edit_cb(const LLUUID& inv_item)
{
	if (inv_item.isNull()) return;
	
	LLViewerInventoryItem* item = gInventory.getItem(inv_item);
	if (!item) return;

	set_default_permissions(item);

	// item was just created, update even if permissions did not changed
	gInventory.updateItem(item);
	gInventory.notifyObservers();

	// Request editing the item after it gets worn.
	gAgentWearables.requestEditingWearable(inv_item);
	
	// Wear it.
	LLAppearanceMgr::instance().wearItemOnAvatar(inv_item,true);
}

void wear_cb(const LLUUID& inv_item)
{
	if (!inv_item.isNull())
	{
		LLViewerInventoryItem* item = gInventory.getItem(inv_item);
		if (item)
		{
			set_default_permissions(item);

			gInventory.updateItem(item);
			gInventory.notifyObservers();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

// HACK: For EXT-3923: Pants item shows in inventory with skin icon and messes with "current look"
// Some db items are corrupted, have inventory flags = 0, implying wearable type = shape, even though
// wearable type stored in asset is some other value.
// Calling this function whenever a wearable is added to increase visibility if this problem
// turns up in other inventories.
void checkWearableAgainstInventory(LLViewerWearable *wearable)
{
	if (wearable->getItemID().isNull())
		return;
	
	// Check for wearable type consistent with inventory item wearable type.
	LLViewerInventoryItem *item = gInventory.getItem(wearable->getItemID());
	if (item)
	{
		if (!item->isWearableType())
		{
			LL_WARNS() << "wearable associated with non-wearable item" << LL_ENDL;
		}
		if (item->getWearableType() != wearable->getType())
		{
			LL_WARNS() << "type mismatch: wearable " << wearable->getName()
					<< " has type " << wearable->getType()
					<< " but inventory item " << item->getName()
					<< " has type "  << item->getWearableType() << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS() << "wearable inventory item not found" << wearable->getName()
				<< " itemID " << wearable->getItemID().asString() << LL_ENDL;
	}
}

void LLAgentWearables::dump()
{
	LL_INFOS() << "LLAgentWearablesDump" << LL_ENDL;
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		U32 count = getWearableCount((LLWearableType::EType)i);
		LL_INFOS() << "Type: " << i << " count " << count << LL_ENDL;
		for (U32 j=0; j<count; j++)
		{
			LLViewerWearable* wearable = getViewerWearable((LLWearableType::EType)i,j);
			if (wearable == NULL)
			{
				LL_INFOS() << "    " << j << " NULL wearable" << LL_ENDL;
			}
			LL_INFOS() << "    " << j << " Name " << wearable->getName()
					<< " description " << wearable->getDescription() << LL_ENDL;
			
		}
	}
}

struct LLAgentDumper
{
	LLAgentDumper(std::string name):
		mName(name)
	{
		LL_INFOS() << LL_ENDL;
		LL_INFOS() << "LLAgentDumper " << mName << LL_ENDL;
		gAgentWearables.dump();
	}

	~LLAgentDumper()
	{
		LL_INFOS() << LL_ENDL;
		LL_INFOS() << "~LLAgentDumper " << mName << LL_ENDL;
		gAgentWearables.dump();
	}

	std::string mName;
};

LLAgentWearables::LLAgentWearables() :
	LLWearableData(),
	mWearablesLoaded(FALSE)
,	mCOFChangeInProgress(false)
//MK from HB
,	mHasModifiableShape(false)
,	mLastWornShape(NULL)
,	mSavedOffset(0.0f)
//mk from HB
{
}

LLAgentWearables::~LLAgentWearables()
{
	cleanup();
}

void LLAgentWearables::cleanup()
{
}

// static
void LLAgentWearables::initClass()
{
	// this can not be called from constructor because its instance is global and is created too early.
	// Subscribe to "COF is Saved" signal to notify observers about this (Loading indicator for ex.).
	LLOutfitObserver::instance().addCOFSavedCallback(boost::bind(&LLAgentWearables::notifyLoadingFinished, &gAgentWearables));
}

void LLAgentWearables::setAvatarObject(LLVOAvatarSelf *avatar)
{
	llassert(avatar);
	setAvatarAppearance(avatar);
}

/**
 * @brief Construct a callback for dealing with the wearables.
 *
 * Would like to pass the agent in here, but we can't safely
 * count on it being around later.  Just use gAgent directly.
 * @param cb callback to execute on completion (? unused ?)
 * @param type Type for the wearable in the agent
 * @param wearable The wearable data.
 * @param todo Bitmask of actions to take on completion.
 */
LLAgentWearables::AddWearableToAgentInventoryCallback::AddWearableToAgentInventoryCallback(
	LLPointer<LLRefCount> cb, LLWearableType::EType type, U32 index, LLViewerWearable* wearable, U32 todo, const std::string description) :
	mType(type),
	mIndex(index),	
	mWearable(wearable),
	mTodo(todo),
	mCB(cb),
	mDescription(description)
{
	LL_INFOS() << "constructor" << LL_ENDL;
}

void LLAgentWearables::AddWearableToAgentInventoryCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;

	gAgentWearables.addWearabletoAgentInventoryDone(mType, mIndex, inv_item, mWearable);

	/*
	 * Do this for every one in the loop
	 */
	if (mTodo & CALL_MAKENEWOUTFITDONE)
	{
		gAgentWearables.makeNewOutfitDone(mType, mIndex);
	}
	if (mTodo & CALL_WEARITEM)
	{
		LLAppearanceMgr::instance().addCOFItemLink(inv_item, 
			new LLUpdateAppearanceAndEditWearableOnDestroy(inv_item), mDescription);
		editWearable(inv_item);
	}
}
void LLAgentWearables::addWearabletoAgentInventoryDone(const LLWearableType::EType type,
													   const U32 index,
													   const LLUUID& item_id,
													   LLViewerWearable* wearable)
{
	LL_INFOS() << "type " << type << " index " << index << " item " << item_id.asString() << LL_ENDL;

	if (item_id.isNull())
		return;

	LLUUID old_item_id = getWearableItemID(type,index);

	if (wearable)
	{
		wearable->setItemID(item_id);

		if (old_item_id.notNull())
		{	
			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
			setWearable(type,index,wearable);
		}
		else
		{
			pushWearable(type,wearable);
		}
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (item && wearable)
	{
		// We're changing the asset id, so we both need to set it
		// locally via setAssetUUID() and via setTransactionID() which
		// will be decoded on the server. JC
		item->setAssetUUID(wearable->getAssetID());
		item->setTransactionID(wearable->getTransactionID());
		gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		item->updateServer(FALSE);
	}
	gInventory.notifyObservers();
}

void LLAgentWearables::saveWearable(const LLWearableType::EType type, const U32 index,
									const std::string new_name)
{
	LLViewerWearable* old_wearable = getViewerWearable(type, index);
	if(!old_wearable) return;
	bool name_changed = !new_name.empty() && (new_name != old_wearable->getName());
	if (name_changed || old_wearable->isDirty() || old_wearable->isOldVersion())
	{
		LLUUID old_item_id = old_wearable->getItemID();
		LLViewerWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
		new_wearable->setItemID(old_item_id); // should this be in LLViewerWearable::copyDataFrom()?
		setWearable(type,index,new_wearable);

		// old_wearable may still be referred to by other inventory items. Revert
		// unsaved changes so other inventory items aren't affected by the changes
		// that were just saved.
		old_wearable->revertValues();

		LLInventoryItem* item = gInventory.getItem(old_item_id);
		if (item)
		{
			std::string item_name = item->getName();
			if (name_changed)
			{
				LL_INFOS() << "saveWearable changing name from "  << item->getName() << " to " << new_name << LL_ENDL;
				item_name = new_name;
			}
			// Update existing inventory item
			LLPointer<LLViewerInventoryItem> template_item =
				new LLViewerInventoryItem(item->getUUID(),
										  item->getParentUUID(),
										  item->getPermissions(),
										  new_wearable->getAssetID(),
										  new_wearable->getAssetType(),
										  item->getInventoryType(),
										  item_name,
										  item->getDescription(),
										  item->getSaleInfo(),
										  item->getFlags(),
										  item->getCreationDate());
			template_item->setTransactionID(new_wearable->getTransactionID());
			update_inventory_item(template_item, gAgentAvatarp->mEndCustomizeCallback);
		}
		else
		{
			// Add a new inventory item (shouldn't ever happen here)
			U32 todo = AddWearableToAgentInventoryCallback::CALL_NONE;
			LLPointer<LLInventoryCallback> cb =
				new AddWearableToAgentInventoryCallback(
					LLPointer<LLRefCount>(NULL),
					type,
					index,
					new_wearable,
					todo);
			addWearableToAgentInventory(cb, new_wearable);
			return;
		}

		gAgentAvatarp->wearableUpdated(type);
	}
}

void LLAgentWearables::saveWearableAs(const LLWearableType::EType type,
									  const U32 index,
									  const std::string& new_name,
									  const std::string& description,
									  BOOL save_in_lost_and_found)
{
	if (!isWearableCopyable(type, index))
	{
		LL_WARNS() << "LLAgent::saveWearableAs() not copyable." << LL_ENDL;
		return;
	}
	LLViewerWearable* old_wearable = getViewerWearable(type, index);
	if (!old_wearable)
	{
		LL_WARNS() << "LLAgent::saveWearableAs() no old wearable." << LL_ENDL;
		return;
	}

	LLInventoryItem* item = gInventory.getItem(getWearableItemID(type,index));
	if (!item)
	{
		LL_WARNS() << "LLAgent::saveWearableAs() no inventory item." << LL_ENDL;
		return;
	}
	std::string trunc_name(new_name);
	LLStringUtil::truncate(trunc_name, DB_INV_ITEM_NAME_STR_LEN);
	LLViewerWearable* new_wearable = LLWearableList::instance().createCopy(
		old_wearable,
		trunc_name);

	LLPointer<LLInventoryCallback> cb =
		new AddWearableToAgentInventoryCallback(
			LLPointer<LLRefCount>(NULL),
			type,
			index,
			new_wearable,
			AddWearableToAgentInventoryCallback::CALL_WEARITEM,
			description
			);
	LLUUID category_id;
	if (save_in_lost_and_found)
	{
		category_id = gInventory.findCategoryUUIDForType(
			LLFolderType::FT_LOST_AND_FOUND);
	}
	else
	{
		// put in same folder as original
		category_id = item->getParentUUID();
	}

	copy_inventory_item(
		gAgent.getID(),
		item->getPermissions().getOwner(),
		item->getUUID(),
		category_id,
		new_name,
		cb);

	// old_wearable may still be referred to by other inventory items. Revert
	// unsaved changes so other inventory items aren't affected by the changes
	// that were just saved.
	old_wearable->revertValuesWithoutUpdate();
}

void LLAgentWearables::revertWearable(const LLWearableType::EType type, const U32 index)
{
	LLViewerWearable* wearable = getViewerWearable(type, index);
	llassert(wearable);
	if (wearable)
	{
		wearable->revertValues();
	}
}

void LLAgentWearables::saveAllWearables()
{
	//if (!gInventory.isLoaded())
	//{
	//	return;
	//}

	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
			saveWearable((LLWearableType::EType)i, j);
	}
}

// Called when the user changes the name of a wearable inventory item that is currently being worn.
void LLAgentWearables::setWearableName(const LLUUID& item_id, const std::string& new_name)
{
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLUUID curr_item_id = getWearableItemID((LLWearableType::EType)i,j);
			if (curr_item_id == item_id)
			{
				LLViewerWearable* old_wearable = getViewerWearable((LLWearableType::EType)i,j);
				llassert(old_wearable);
				if (!old_wearable) continue;

				std::string old_name = old_wearable->getName();
				old_wearable->setName(new_name);
				LLViewerWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
				new_wearable->setItemID(item_id);
				LLInventoryItem* item = gInventory.getItem(item_id);
				if (item)
				{
					new_wearable->setPermissions(item->getPermissions());
				}
				old_wearable->setName(old_name);

				setWearable((LLWearableType::EType)i,j,new_wearable);
				break;
			}
		}
	}
}


BOOL LLAgentWearables::isWearableModifiable(LLWearableType::EType type, U32 index) const
{
	LLUUID item_id = getWearableItemID(type, index);
	return item_id.notNull() ? isWearableModifiable(item_id) : FALSE;
}

BOOL LLAgentWearables::isWearableModifiable(const LLUUID& item_id) const
{
	const LLUUID& linked_id = gInventory.getLinkedItemID(item_id);
	if (linked_id.notNull())
	{
		LLInventoryItem* item = gInventory.getItem(linked_id);
		if (item && item->getPermissions().allowModifyBy(gAgent.getID(),
														 gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLAgentWearables::isWearableCopyable(LLWearableType::EType type, U32 index) const
{
	LLUUID item_id = getWearableItemID(type, index);
	if (!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if (item && item->getPermissions().allowCopyBy(gAgent.getID(),
													   gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
  U32 LLAgentWearables::getWearablePermMask(LLWearableType::EType type)
  {
  LLUUID item_id = getWearableItemID(type);
  if (!item_id.isNull())
  {
  LLInventoryItem* item = gInventory.getItem(item_id);
  if (item)
  {
  return item->getPermissions().getMaskOwner();
  }
  }
  return PERM_NONE;
  }
*/

LLInventoryItem* LLAgentWearables::getWearableInventoryItem(LLWearableType::EType type, U32 index)
{
	LLUUID item_id = getWearableItemID(type,index);
	LLInventoryItem* item = NULL;
	if (item_id.notNull())
	{
		item = gInventory.getItem(item_id);
	}
	return item;
}

const LLViewerWearable* LLAgentWearables::getWearableFromItemID(const LLUUID& item_id) const
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			const LLViewerWearable * curr_wearable = getViewerWearable((LLWearableType::EType)i, j);
			if (curr_wearable && (curr_wearable->getItemID() == base_item_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}

LLViewerWearable* LLAgentWearables::getWearableFromItemID(const LLUUID& item_id)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLViewerWearable * curr_wearable = getViewerWearable((LLWearableType::EType)i, j);
			if (curr_wearable && (curr_wearable->getItemID() == base_item_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}

LLViewerWearable*	LLAgentWearables::getWearableFromAssetID(const LLUUID& asset_id) 
{
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLViewerWearable * curr_wearable = getViewerWearable((LLWearableType::EType)i, j);
			if (curr_wearable && (curr_wearable->getAssetID() == asset_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}

LLViewerWearable* LLAgentWearables::getViewerWearable(const LLWearableType::EType type, U32 index /*= 0*/)
{
	return dynamic_cast<LLViewerWearable*> (getWearable(type, index));
}

const LLViewerWearable* LLAgentWearables::getViewerWearable(const LLWearableType::EType type, U32 index /*= 0*/) const
{
	return dynamic_cast<const LLViewerWearable*> (getWearable(type, index));
}

// static
BOOL LLAgentWearables::selfHasWearable(LLWearableType::EType type)
{
	return (gAgentWearables.getWearableCount(type) > 0);
}

// virtual
void LLAgentWearables::wearableUpdated(LLWearable *wearable, BOOL removed)
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->wearableUpdated(wearable->getType());
	}

	LLWearableData::wearableUpdated(wearable, removed);

	if (!removed)
	{
		LLViewerWearable* viewer_wearable = dynamic_cast<LLViewerWearable*>(wearable);
		viewer_wearable->refreshName();

		// Hack pt 2. If the wearable we just loaded has definition version 24,
		// then force a re-save of this wearable after slamming the version number to 22.
		// This number was incorrectly incremented for internal builds before release, and
		// this fix will ensure that the affected wearables are re-saved with the right version number.
		// the versions themselves are compatible. This code can be removed before release.
		if( wearable->getDefinitionVersion() == 24 )
		{
			U32 index;
			if (getWearableIndex(wearable,index))
			{
				LL_INFOS() << "forcing wearable type " << wearable->getType() << " to version 22 from 24" << LL_ENDL;
				wearable->setDefinitionVersion(22);
				saveWearable(wearable->getType(),index);
			}
		}

		checkWearableAgainstInventory(viewer_wearable);
	}
//MK
	if (mLastWornShape)
	{
		mSavedOffset = mLastWornShape->getVisualParamWeight(AVATAR_HOVER);
		LL_INFOS() << "mSavedOffset is now " << mSavedOffset << LL_ENDL;
	}
//mk
}


const LLUUID LLAgentWearables::getWearableItemID(LLWearableType::EType type, U32 index) const
{
	const LLViewerWearable *wearable = getViewerWearable(type,index);
	if (wearable)
		return wearable->getItemID();
	else
		return LLUUID();
}

const LLUUID LLAgentWearables::getWearableAssetID(LLWearableType::EType type, U32 index) const
{
	const LLViewerWearable *wearable = getViewerWearable(type,index);
	if (wearable)
		return wearable->getAssetID();
	else
		return LLUUID();
}

BOOL LLAgentWearables::isWearingItem(const LLUUID& item_id) const
{
	return getWearableFromItemID(item_id) != NULL;
}

void LLAgentWearables::addLocalTextureObject(const LLWearableType::EType wearable_type, const LLAvatarAppearanceDefines::ETextureIndex texture_type, U32 wearable_index)
{
	LLViewerWearable* wearable = getViewerWearable((LLWearableType::EType)wearable_type, wearable_index);
	if (!wearable)
	{
		LL_ERRS() << "Tried to add local texture object to invalid wearable with type " << wearable_type << " and index " << wearable_index << LL_ENDL;
		return;
	}
	LLLocalTextureObject lto;
	wearable->setLocalTextureObject(texture_type, lto);
}

class OnWearableItemCreatedCB: public LLInventoryCallback
{
public:
	OnWearableItemCreatedCB():
		mWearablesAwaitingItems(LLWearableType::WT_COUNT,NULL)
	{
		LL_INFOS() << "created callback" << LL_ENDL;
	}
	/* virtual */ void fire(const LLUUID& inv_item)
	{
		LL_INFOS() << "One item created " << inv_item.asString() << LL_ENDL;
		LLConstPointer<LLInventoryObject> item = gInventory.getItem(inv_item);
		mItemsToLink.push_back(item);
		updatePendingWearable(inv_item);
	}
	~OnWearableItemCreatedCB()
	{
		LL_INFOS() << "All items created" << LL_ENDL;
		LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;
		link_inventory_array(LLAppearanceMgr::instance().getCOF(),
							 mItemsToLink,
							 link_waiter);
	}
	void addPendingWearable(LLViewerWearable *wearable)
	{
		if (!wearable)
		{
			LL_WARNS() << "no wearable" << LL_ENDL;
			return;
		}
		LLWearableType::EType type = wearable->getType();
		if (type<LLWearableType::WT_COUNT)
		{
			mWearablesAwaitingItems[type] = wearable;
		}
		else
		{
			LL_WARNS() << "invalid type " << type << LL_ENDL;
		}
	}
	void updatePendingWearable(const LLUUID& inv_item)
	{
		LLViewerInventoryItem *item = gInventory.getItem(inv_item);
		if (!item)
		{
			LL_WARNS() << "no item found" << LL_ENDL;
			return;
		}
		if (!item->isWearableType())
		{
			LL_WARNS() << "non-wearable item found" << LL_ENDL;
			return;
		}
		if (item && item->isWearableType())
		{
			LLWearableType::EType type = item->getWearableType();
			if (type < LLWearableType::WT_COUNT)
			{
				LLViewerWearable *wearable = mWearablesAwaitingItems[type];
				if (wearable)
					wearable->setItemID(inv_item);
			}
			else
			{
				LL_WARNS() << "invalid wearable type " << type << LL_ENDL;
			}
		}
	}
	
private:
	LLInventoryObject::const_object_list_t mItemsToLink;
	std::vector<LLViewerWearable*> mWearablesAwaitingItems;
};

void LLAgentWearables::createStandardWearables()
{
	LL_WARNS() << "Creating standard wearables" << LL_ENDL;

	if (!isAgentAvatarValid()) return;

	const BOOL create[LLWearableType::WT_COUNT] = 
		{
			TRUE,  //LLWearableType::WT_SHAPE
			TRUE,  //LLWearableType::WT_SKIN
			TRUE,  //LLWearableType::WT_HAIR
			TRUE,  //LLWearableType::WT_EYES
			TRUE,  //LLWearableType::WT_SHIRT
			TRUE,  //LLWearableType::WT_PANTS
			TRUE,  //LLWearableType::WT_SHOES
			TRUE,  //LLWearableType::WT_SOCKS
			FALSE, //LLWearableType::WT_JACKET
			FALSE, //LLWearableType::WT_GLOVES
			TRUE,  //LLWearableType::WT_UNDERSHIRT
			TRUE,  //LLWearableType::WT_UNDERPANTS
			FALSE  //LLWearableType::WT_SKIRT
		};

	LLPointer<LLInventoryCallback> cb = new OnWearableItemCreatedCB;
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		if (create[i])
		{
			llassert(getWearableCount((LLWearableType::EType)i) == 0);
			LLViewerWearable* wearable = LLWearableList::instance().createNewWearable((LLWearableType::EType)i, gAgentAvatarp);
			((OnWearableItemCreatedCB*)(&(*cb)))->addPendingWearable(wearable);
			// no need to update here...
			LLUUID category_id = LLUUID::null;
			create_inventory_item(gAgent.getID(),
								  gAgent.getSessionID(),
								  category_id,
								  wearable->getTransactionID(),
								  wearable->getName(),
								  wearable->getDescription(),
								  wearable->getAssetType(),
								  LLInventoryType::IT_WEARABLE,
								  wearable->getType(),
								  wearable->getPermissions().getMaskNextOwner(),
								  cb);
		}
	}
}

// We no longer need this message in the current viewer, but send
// it for now to maintain compatibility with release viewers. Can
// remove this function once the SH-3455 changesets are universally deployed.
void LLAgentWearables::sendDummyAgentWearablesUpdate()
{
	LL_DEBUGS("Avatar") << "sendAgentWearablesUpdate()" << LL_ENDL;

	// Send the AgentIsNowWearing 
	gMessageSystem->newMessageFast(_PREHASH_AgentIsNowWearing);
	
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	// Send 4 standardized nonsense item ids (same as returned by the modified sim, not that it especially matters).
	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(1));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("db5a4e5f-9da3-44c8-992d-1181c5795498"));			

	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(2));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("6969c7cc-f72f-4a76-a19b-c293cce8ce4f"));			

	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(3));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("7999702b-b291-48f9-8903-c91dfb828408"));			

	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(4));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("566cb59e-ef60-41d7-bfa6-e0f293fbea40"));			

	gAgent.sendReliableMessage();
}

void LLAgentWearables::makeNewOutfitDone(S32 type, U32 index)
{
	LLUUID first_item_id = getWearableItemID((LLWearableType::EType)type, index);
	// Open the inventory and select the first item we added.
	if (first_item_id.notNull())
	{
		LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
		if (active_panel)
		{
			active_panel->setSelection(first_item_id, TAKE_FOCUS_NO);
		}
	}
}


void LLAgentWearables::addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												   LLViewerWearable* wearable,
												   const LLUUID& category_id,
												   BOOL notify)
{
	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  category_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  wearable->getAssetType(),
						  LLInventoryType::IT_WEARABLE,
						  wearable->getType(),
						  wearable->getPermissions().getMaskNextOwner(),
						  cb);
}

void LLAgentWearables::removeWearable(const LLWearableType::EType type, bool do_remove_all, U32 index)
{
//MK
	if (gRRenabled)
	{
		if (!gAgent.mRRInterface.canUnwear (type))
		{
			return;
		}
	}
//mk
	if (gAgent.isTeen() &&
		(type == LLWearableType::WT_UNDERSHIRT || type == LLWearableType::WT_UNDERPANTS))
	{
		// Can't take off underclothing in simple UI mode or on PG accounts
		// TODO: enable the removing of a single undershirt/underpants if multiple are worn. - Nyx
		return;
	}
	if (getWearableCount(type) == 0)
	{
		// no wearables to remove
		return;
	}

	if (do_remove_all)
	{
		removeWearableFinal(type, do_remove_all, index);
	}
	else
	{
		LLViewerWearable* old_wearable = getViewerWearable(type,index);
		
		if (old_wearable)
		{
			if (old_wearable->isDirty())
			{
				LLSD payload;
				payload["wearable_type"] = (S32)type;
				payload["wearable_index"] = (S32)index;
				// Bring up view-modal dialog: Save changes? Yes, No, Cancel
				LLNotificationsUtil::add("WearableSave", LLSD(), payload, &LLAgentWearables::onRemoveWearableDialog);
				return;
			}
			else
			{
				removeWearableFinal(type, do_remove_all, index);
			}
		}
	}
}


// static 
bool LLAgentWearables::onRemoveWearableDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLWearableType::EType type = (LLWearableType::EType)notification["payload"]["wearable_type"].asInteger();
	S32 index = (S32)notification["payload"]["wearable_index"].asInteger();
	switch(option)
	{
		case 0:  // "Save"
			gAgentWearables.saveWearable(type, index);
			gAgentWearables.removeWearableFinal(type, false, index);
			break;

		case 1:  // "Don't Save"
			gAgentWearables.removeWearableFinal(type, false, index);
			break;

		case 2: // "Cancel"
			break;

		default:
			llassert(0);
			break;
	}
	return false;
}

// Called by removeWearable() and onRemoveWearableDialog() to actually do the removal.
void LLAgentWearables::removeWearableFinal(const LLWearableType::EType type, bool do_remove_all, U32 index)
{
//MK
	// At this point, all the RLV checks have been done and passed, the wearable has been removed legally => notify
	if (gRRenabled)
	{
		std::string layer = gAgent.mRRInterface.getOutfitLayerAsString(type);
		gAgent.mRRInterface.notify(LLUUID::null, "unworn legally " + layer, "");
	}
//mk

	//LLAgentDumper dumper("removeWearable");
	if (do_remove_all)
	{
		S32 max_entry = getWearableCount(type)-1;
		for (S32 i=max_entry; i>=0; i--)
		{
			LLViewerWearable* old_wearable = getViewerWearable(type,i);
			if (old_wearable)
			{
				eraseWearable(old_wearable);
				old_wearable->removeFromAvatar();
			}
		}
//		clearWearableType(type);
// [RLVa:KB] - Checked: 2010-05-14 (RLVa-1.2.0)
		// The line above shouldn't be needed
// [/RLVa:KB]
	}
	else
	{
		LLViewerWearable* old_wearable = getViewerWearable(type, index);

		if (old_wearable)
		{
			eraseWearable(old_wearable);
			old_wearable->removeFromAvatar();
		}
	}

	gInventory.notifyObservers();
}

// Assumes existing wearables are not dirty.
void LLAgentWearables::setWearableOutfit(const LLInventoryItem::item_array_t& items,
										 const std::vector< LLViewerWearable* >& wearables)
{
	LL_INFOS() << "setWearableOutfit() start" << LL_ENDL;

	S32 count = wearables.size();
	llassert(items.size() == count);

	// Check for whether outfit already matches the one requested
	S32 matched = 0, mismatched = 0;
	const S32 arr_size = LLWearableType::WT_COUNT;
	S32 type_counts[arr_size];
	BOOL update_inventory = FALSE;
	std::fill(type_counts,type_counts+arr_size,0);
	for (S32 i = 0; i < count; i++)
	{
		LLViewerWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];

		const LLWearableType::EType type = new_wearable->getType();
		if (type < 0 || type>=LLWearableType::WT_COUNT)
		{
			LL_WARNS() << "invalid type " << type << LL_ENDL;
			mismatched++;
			continue;
		}
		S32 index = type_counts[type];
		type_counts[type]++;

		LLViewerWearable *curr_wearable = dynamic_cast<LLViewerWearable*>(getWearable(type,index));
		if (!new_wearable || !curr_wearable ||
			new_wearable->getAssetID() != curr_wearable->getAssetID())
		{
			LL_DEBUGS("Avatar") << "mismatch, type " << type << " index " << index
								<< " names " << (curr_wearable ? curr_wearable->getName() : "NONE")  << ","
								<< " names " << (new_wearable ? new_wearable->getName() : "NONE")  << LL_ENDL;
			mismatched++;
			continue;
		}

		// Update only inventory in this case - ordering of wearables with the same asset id has no effect.
		// Updating wearables in this case causes the two-alphas error in MAINT-4158.
		// We should actually disallow wearing two wearables with the same asset id.
		if (curr_wearable->getName() != new_item->getName() ||
			curr_wearable->getItemID() != new_item->getUUID())
		{
			LL_DEBUGS("Avatar") << "mismatch on name or inventory id, names "
								<< curr_wearable->getName() << " vs " << new_item->getName()
								<< " item ids " << curr_wearable->getItemID() << " vs " << new_item->getUUID()
								<< LL_ENDL;
			update_inventory = TRUE;
			continue;
		}
		// If we got here, everything matches.
		matched++;
	}
	LL_DEBUGS("Avatar") << "matched " << matched << " mismatched " << mismatched << LL_ENDL;
	for (S32 j=0; j<LLWearableType::WT_COUNT; j++)
	{
		LLWearableType::EType type = (LLWearableType::EType) j;
		if (getWearableCount(type) != type_counts[j])
		{
			LL_DEBUGS("Avatar") << "count mismatch for type " << j << " current " << getWearableCount(j) << " requested " << type_counts[j] << LL_ENDL; 
			mismatched++;
		}
	}
	if (mismatched == 0 && !update_inventory)
	{
		LL_DEBUGS("Avatar") << "no changes, bailing out" << LL_ENDL;
		notifyLoadingFinished();
		return;
	}

	// updating inventory

	// TODO: Removed check for ensuring that teens don't remove undershirt and underwear. Handle later
	// note: shirt is the first non-body part wearable item. Update if wearable order changes.
	// This loop should remove all clothing, but not any body parts
	for (S32 j = 0; j < (S32)LLWearableType::WT_COUNT; j++)
	{
		if (LLWearableType::getAssetType((LLWearableType::EType)j) == LLAssetType::AT_CLOTHING)
		{
			removeWearable((LLWearableType::EType)j, true, 0);
		}
	}

	for (S32 i = 0; i < count; i++)
	{
		LLViewerWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];

		llassert(new_wearable);
		if (new_wearable)
		{
			const LLWearableType::EType type = new_wearable->getType();

			LLUUID old_wearable_id = new_wearable->getItemID();
			new_wearable->setName(new_item->getName());
			new_wearable->setItemID(new_item->getUUID());

			if (LLWearableType::getAssetType(type) == LLAssetType::AT_BODYPART)
			{
				// exactly one wearable per body part
				setWearable(type,0,new_wearable);
				if (old_wearable_id.notNull())
				{
					// we changed id before setting wearable, update old item manually
					// to complete the swap.
					gInventory.addChangedMask(LLInventoryObserver::LABEL, old_wearable_id);
				}
			}
			else
			{
				pushWearable(type,new_wearable);
//MK
					// Notify that this layer has been worn
					gAgent.mRRInterface.notify (LLUUID::null, "worn legally " + gAgent.mRRInterface.getOutfitLayerAsString(type), "");
//mk
			}

			const BOOL removed = FALSE;
			wearableUpdated(new_wearable, removed);
		}
	}

	gInventory.notifyObservers();

	if (mismatched == 0)
	{
		LL_DEBUGS("Avatar") << "inventory updated, wearable assets not changed, bailing out" << LL_ENDL;
		notifyLoadingFinished();
		return;
	}

	// updating agent avatar

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);

		// If we have not yet declouded, we may want to use
		// baked texture UUIDs sent from the first objectUpdate message
		// don't overwrite these. If we have already declouded, we've saved
		// these ids as the last known good textures and can invalidate without
		// re-clouding.
		if (!gAgentAvatarp->getIsCloud())
		{
			gAgentAvatarp->invalidateAll();
		}
	}

	// Start rendering & update the server
	mWearablesLoaded = TRUE; 

// [SL:KB] - Patch: Appearance-InitialWearablesLoadedCallback | Checked: 2010-09-22 (Catznip-2.2)
	if (!mInitialWearablesLoaded)
	{
		mInitialWearablesLoaded = true;
		mInitialWearablesLoadedSignal();
	}
// [/SL:KB]
	notifyLoadingFinished();

	// Copy wearable params to avatar.
	gAgentAvatarp->writeWearablesToAvatar();

	// Then update the avatar based on the copied params.
	gAgentAvatarp->updateVisualParams();

	gAgentAvatarp->dumpAvatarTEs("setWearableOutfit");

	LL_DEBUGS("Avatar") << "setWearableOutfit() end" << LL_ENDL;
}


// User has picked "wear on avatar" from a menu.
//void LLAgentWearables::setWearableItem(LLInventoryItem* new_item, LLViewerWearable* new_wearable, bool do_append)
//{
//	//LLAgentDumper dumper("setWearableItem");
//	if (isWearingItem(new_item->getUUID()))
//	{
//		LL_WARNS() << "wearable " << new_item->getUUID() << " is already worn" << LL_ENDL;
//		return;
//	}
//	
//	const LLWearableType::EType type = new_wearable->getType();
//
//	if (!do_append)
//	{
//		// Remove old wearable, if any
//		// MULTI_WEARABLE: hardwired to 0
//		LLViewerWearable* old_wearable = getViewerWearable(type,0);
//		if (old_wearable)
//		{
//			const LLUUID& old_item_id = old_wearable->getItemID();
//			if ((old_wearable->getAssetID() == new_wearable->getAssetID()) &&
//				(old_item_id == new_item->getUUID()))
//			{
//				LL_DEBUGS() << "No change to wearable asset and item: " << LLWearableType::getTypeName(type) << LL_ENDL;
//				return;
//			}
//			
//			if (old_wearable->isDirty())
//			{
//				// Bring up modal dialog: Save changes? Yes, No, Cancel
//				LLSD payload;
//				payload["item_id"] = new_item->getUUID();
//				LLNotificationsUtil::add("WearableSave", LLSD(), payload, boost::bind(onSetWearableDialog, _1, _2, new_wearable));
//				return;
//			}
//		}
//	}
//
//	setWearableFinal(new_item, new_wearable, do_append);
//}

// static 
//bool LLAgentWearables::onSetWearableDialog(const LLSD& notification, const LLSD& response, LLViewerWearable* wearable)
//{
//	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
//	LLInventoryItem* new_item = gInventory.getItem(notification["payload"]["item_id"].asUUID());
//	U32 index;
//	if (!gAgentWearables.getWearableIndex(wearable,index))
//	{
//		LL_WARNS() << "Wearable not found" << LL_ENDL;
//		delete wearable;
//		return false;
//	}
//	switch(option)
//	{
//		case 0:  // "Save"
//			gAgentWearables.saveWearable(wearable->getType(),index);
//			gAgentWearables.setWearableFinal(new_item, wearable);
//			break;
//
//		case 1:  // "Don't Save"
//			gAgentWearables.setWearableFinal(new_item, wearable);
//			break;
//
//		case 2: // "Cancel"
//			break;
//
//		default:
//			llassert(0);
//			break;
//	}
//
//	delete wearable;
//	return false;
//}

// Called from setWearableItem() and onSetWearableDialog() to actually set the wearable.
// MULTI_WEARABLE: unify code after null objects are gone.
//void LLAgentWearables::setWearableFinal(LLInventoryItem* new_item, LLViewerWearable* new_wearable, bool do_append)
//{
//	const LLWearableType::EType type = new_wearable->getType();
//
//	if (do_append && getWearableItemID(type,0).notNull())
//	{
//		new_wearable->setItemID(new_item->getUUID());
//		const bool trigger_updated = false;
//		pushWearable(type, new_wearable, trigger_updated);
//		LL_INFOS() << "Added additional wearable for type " << type
//				<< " size is now " << getWearableCount(type) << LL_ENDL;
//		checkWearableAgainstInventory(new_wearable);
//	}
//	else
//	{
//		// Replace the old wearable with a new one.
//		llassert(new_item->getAssetUUID() == new_wearable->getAssetID());
//
//		LLViewerWearable *old_wearable = getViewerWearable(type,0);
//		LLUUID old_item_id;
//		if (old_wearable)
//		{
//			old_item_id = old_wearable->getItemID();
//		}
//		new_wearable->setItemID(new_item->getUUID());
//		setWearable(type,0,new_wearable);
//
//		if (old_item_id.notNull())
//		{
//			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
//			gInventory.notifyObservers();
//		}
//		LL_INFOS() << "Replaced current element 0 for type " << type
//				<< " size is now " << getWearableCount(type) << LL_ENDL;
//	}
//}

// User has picked "remove from avatar" from a menu.
// static
//void LLAgentWearables::userRemoveWearable(const LLWearableType::EType &type, const U32 &index)
//{
//	if (!(type==LLWearableType::WT_SHAPE || type==LLWearableType::WT_SKIN || type==LLWearableType::WT_HAIR || type==LLWearableType::WT_EYES)) //&&
//		//!((!gAgent.isTeen()) && (type==LLWearableType::WT_UNDERPANTS || type==LLWearableType::WT_UNDERSHIRT)))
//	{
//		gAgentWearables.removeWearable(type,false,index);
//	}
//}

//static 
//void LLAgentWearables::userRemoveWearablesOfType(const LLWearableType::EType &type)
//{
//	if (!(type==LLWearableType::WT_SHAPE || type==LLWearableType::WT_SKIN || type==LLWearableType::WT_HAIR || type==LLWearableType::WT_EYES)) //&&
//		//!((!gAgent.isTeen()) && (type==LLWearableType::WT_UNDERPANTS || type==LLWearableType::WT_UNDERSHIRT)))
//	{
//		gAgentWearables.removeWearable(type,true,0);
//	}
//}

// Given a desired set of attachments, find what objects need to be
// removed, and what additional inventory items need to be added.
void LLAgentWearables::findAttachmentsAddRemoveInfo(LLInventoryModel::item_array_t& obj_item_array,
													llvo_vec_t& objects_to_remove,
													llvo_vec_t& objects_to_retain,
													LLInventoryModel::item_array_t& items_to_add)

{

//CA I don't think we need this any more after the wholesale import of Firestorm COF handling improvements.
// However, for ease of future merging (and in case I'm proved wrong) we'll leave the code here but disabled
//
//MK
	// When calling this function, one of two purposes are expected :
	// - If this is the first time (i.e. immediately after logging on), look at all the links in the COF, request to wear the items that are not worn
	// (since normally an item which has a link in the COF must necessarily be worn, this is a good way to make things straight)
	// - If this is not the first time (i.e. immediately after wearing and unwearing items and outfits), then there might be a problem : links are created slowly
	// and the user may unwear those items before all the links are done being created, which makes those belated links be worn again. In practice, you wear a folder
	// then unwear it before all the links appear, and the belated items are automatically worn again. We don't want that, so we need to DELETE those links
	// instead of automatically wearing them.
	// To distinguish between these two cases is the purpose of the boolean gAgent.mRRInterface.mUserUpdateAttachmentsFirstCall
	// Attention : we need to call the regular part of the function if we did a "Add to Current Outfit" or "Replace Current Outfit" in the inventory
	// CA disable this - see comment above
    if (gRRenabled && false)
    {
        if (gAgentAvatarp && !gAgentAvatarp->getIsCloud() && !gAgent.mRRInterface.mUserUpdateAttachmentsFirstCall && !gAgent.mRRInterface.mUserUpdateAttachmentsCalledManually)
        {
            LLInventoryModel::cat_array_t cat_array;
            LLInventoryModel::item_array_t item_array;
            gInventory.collectDescendents(LLAppearanceMgr::instance().getCOF(), cat_array, item_array, LLInventoryModel::EXCLUDE_TRASH);
            for (S32 i = 0; i < item_array.size(); i++)
            {
                const LLViewerInventoryItem* inv_item = item_array.at(i).get();
                if (inv_item)
                {
                    if (LLAssetType::AT_LINK == inv_item->getActualType())
                    {
                        const LLViewerInventoryItem* linked_item = inv_item->getLinkedItem();
                        if (NULL == linked_item)
                        {
                            // Broken link => remove
                        }
                        else
                        {
                            if (LLAssetType::AT_OBJECT == linked_item->getType())
                            {
                                std::string attachment_point_name;
                                if (!gAgentAvatarp->getAttachedPointName(linked_item->getUUID(), attachment_point_name))
                                {
                                    LLAppearanceMgr::instance().removeCOFItemLinks(linked_item->getUUID());
                                }
                            }
                        }
                    }
                }
                LLUUID item_id(inv_item->getUUID());
            }
            return;
        }
    }
//mk

	// Possible cases:
	// already wearing but not in request set -> take off.
	// already wearing and in request set -> leave alone.
	// not wearing and in request set -> put on.

	if (!isAgentAvatarValid()) return;

	std::set<LLUUID> requested_item_ids;
	std::set<LLUUID> current_item_ids;
	for (S32 i=0; i<obj_item_array.size(); i++)
	{
		const LLUUID & requested_id = obj_item_array[i].get()->getLinkedUUID();
		//LL_INFOS() << "Requested attachment id " << requested_id << LL_ENDL;
		requested_item_ids.insert(requested_id);
	}

	// Build up list of objects to be removed and items currently attached.
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end();)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *objectp = (*attachment_iter);
			if (objectp)
			{
				LLUUID object_item_id = objectp->getAttachmentItemID();

				bool remove_attachment = true;
				if (requested_item_ids.find(object_item_id) != requested_item_ids.end())
				{	// Object currently worn, was requested to keep it
					// Flag as currently worn so we won't have to add it again.
					remove_attachment = false;
				}
				else if (objectp->isTempAttachment())
				{	// Check if we should keep this temp attachment
					remove_attachment = LLAppearanceMgr::instance().shouldRemoveTempAttachment(objectp->getID());
				}

				if (remove_attachment)
				{
					// LL_INFOS() << "found object to remove, id " << objectp->getID() << ", item " << objectp->getAttachmentItemID() << LL_ENDL;
					objects_to_remove.push_back(objectp);
				}
				else
				{
					// LL_INFOS() << "found object to keep, id " << objectp->getID() << ", item " << objectp->getAttachmentItemID() << LL_ENDL;
					current_item_ids.insert(object_item_id);
					objects_to_retain.push_back(objectp);
				}
			}
		}
	}

	for (LLInventoryModel::item_array_t::iterator it = obj_item_array.begin();
		 it != obj_item_array.end();
		 ++it)
	{
		LLUUID linked_id = (*it).get()->getLinkedUUID();
		if (current_item_ids.find(linked_id) != current_item_ids.end())
		{
			// Requested attachment is already worn.
		}
		else
		{
			// Requested attachment is not worn yet.
			items_to_add.push_back(*it);
		}
	}
	// S32 remove_count = objects_to_remove.size();
	// S32 add_count = items_to_add.size();
	// LL_INFOS() << "remove " << remove_count << " add " << add_count << LL_ENDL;

//MK
    if (gRRenabled)
    {
        gAgent.mRRInterface.mUserUpdateAttachmentsUpdatesAll = FALSE;
        gAgent.mRRInterface.mUserUpdateAttachmentsFirstCall = FALSE;
        gAgent.mRRInterface.mUserUpdateAttachmentsCalledManually = FALSE;
    }
//mk
}

std::vector<LLViewerObject*> LLAgentWearables::getTempAttachments()
{
	llvo_vec_t temp_attachs;
	if (isAgentAvatarValid())
	{
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); iter != gAgentAvatarp->mAttachmentPoints.end();)
		{
			LLVOAvatar::attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachment = curiter->second;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				attachment_iter != attachment->mAttachedObjects.end();
				++attachment_iter)
			{
				LLViewerObject *objectp = (*attachment_iter);
				if (objectp && objectp->isTempAttachment())
				{
					temp_attachs.push_back(objectp);
				}
			}
		}
	}
	return temp_attachs;
}

void LLAgentWearables::userRemoveMultipleAttachments(llvo_vec_t& objects_to_remove)
{
	if (!isAgentAvatarValid()) return;

	if (objects_to_remove.empty())
		return;

	LL_DEBUGS("Avatar") << "ATT [ObjectDetach] removing " << objects_to_remove.size() << " objects" << LL_ENDL;
	gMessageSystem->newMessage("ObjectDetach");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	
	for (llvo_vec_t::iterator it = objects_to_remove.begin();
		 it != objects_to_remove.end();
		 ++it)
	{
		LLViewerObject *objectp = *it;
//MK
		if (gRRenabled && !gAgent.mRRInterface.canDetach (objectp))
		{
			continue;
		}
//mk
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, objectp->getLocalID());
		const LLUUID& item_id = objectp->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_DEBUGS("Avatar") << "ATT removing object, item is " << (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
        LLAttachmentsMgr::instance().onDetachRequested(item_id);
	}
	gMessageSystem->sendReliable(gAgent.getRegionHost());
}

void LLAgentWearables::userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array)
{
	// Build a compound message to send all the objects that need to be rezzed.
	S32 obj_count = obj_item_array.size();
	if (obj_count > 0)
	{
		LL_DEBUGS("Avatar") << "ATT attaching multiple, total obj_count " << obj_count << LL_ENDL;
	}

    for(LLInventoryModel::item_array_t::const_iterator it = obj_item_array.begin();
        it != obj_item_array.end();
        ++it)
    {
		const LLInventoryItem* item = *it;
        LLAttachmentsMgr::instance().addAttachmentRequest(item->getLinkedUUID(), 0, TRUE);
    }
}

// Returns false if the given wearable is already topmost/bottommost
// (depending on closer_to_body parameter).
bool LLAgentWearables::canMoveWearable(const LLUUID& item_id, bool closer_to_body) const
{
	const LLWearable* wearable = getWearableFromItemID(item_id);
	if (!wearable) return false;

	LLWearableType::EType wtype = wearable->getType();
	const LLWearable* marginal_wearable = closer_to_body ? getBottomWearable(wtype) : getTopWearable(wtype);
	if (!marginal_wearable) return false;

	return wearable != marginal_wearable;
}

BOOL LLAgentWearables::areWearablesLoaded() const
{
	return mWearablesLoaded;
}

bool LLAgentWearables::canWearableBeRemoved(const LLViewerWearable* wearable) const
{
	if (!wearable) return false;
	
	LLWearableType::EType type = wearable->getType();
	// Make sure the user always has at least one shape, skin, eyes, and hair type currently worn.
	return !(((type == LLWearableType::WT_SHAPE) || (type == LLWearableType::WT_SKIN) || (type == LLWearableType::WT_HAIR) || (type == LLWearableType::WT_EYES))
			 && (getWearableCount(type) <= 1) );		  
}
void LLAgentWearables::animateAllWearableParams(F32 delta)
{
	for( S32 type = 0; type < LLWearableType::WT_COUNT; ++type )
	{
		for (S32 count = 0; count < (S32)getWearableCount((LLWearableType::EType)type); ++count)
		{
			LLViewerWearable *wearable = getViewerWearable((LLWearableType::EType)type,count);
			llassert(wearable);
			if (wearable)
			{
				wearable->animateParams(delta);
			}
		}
	}
}

bool LLAgentWearables::moveWearable(const LLViewerInventoryItem* item, bool closer_to_body)
{
	if (!item) return false;
	if (!item->isWearableType()) return false;

	LLWearableType::EType type = item->getWearableType();
	U32 wearable_count = getWearableCount(type);
	if (0 == wearable_count) return false;

	const LLUUID& asset_id = item->getAssetUUID();

	//nowhere to move if the wearable is already on any boundary (closest to the body/furthest from the body)
	if (closer_to_body)
	{
		LLViewerWearable* bottom_wearable = dynamic_cast<LLViewerWearable*>( getBottomWearable(type) );
		if (bottom_wearable->getAssetID() == asset_id)
		{
			return false;
		}
	}
	else // !closer_to_body
	{
		LLViewerWearable* top_wearable = dynamic_cast<LLViewerWearable*>( getTopWearable(type) );
		if (top_wearable->getAssetID() == asset_id)
		{
			return false;
		}
	}

	for (U32 i = 0; i < wearable_count; ++i)
	{
		LLViewerWearable* wearable = getViewerWearable(type, i);
		if (!wearable) continue;
		if (wearable->getAssetID() != asset_id) continue;
		
		//swapping wearables
		U32 swap_i = closer_to_body ? i-1 : i+1;
		swapWearables(type, i, swap_i);
		return true;
	}

	return false;
}

// static
void LLAgentWearables::createWearable(LLWearableType::EType type, bool wear, const LLUUID& parent_id)
{
	if (type == LLWearableType::WT_INVALID || type == LLWearableType::WT_NONE) return;

//MK
	// We can't create a wearable under #RLV if at least one folder is locked
	if (gRRenabled)
	{
		if (gAgent.mRRInterface.isUnderRlvShare(gInventory.getCategory(parent_id)))
		{
			if (gAgent.mRRInterface.containsSubstr("attachthis:")
			|| gAgent.mRRInterface.containsSubstr("attachallthis:"))
			{
				return;
			}
		}
	}
//mk
	if (type == LLWearableType::WT_UNIVERSAL && !gAgent.getRegion()->bakesOnMeshEnabled())
	{
		LL_WARNS("Inventory") << "Can't create WT_UNIVERSAL type " << LL_ENDL;
		return;
	}

	LLViewerWearable* wearable = LLWearableList::instance().createNewWearable(type, gAgentAvatarp);
	LLAssetType::EType asset_type = wearable->getAssetType();
	LLInventoryType::EType inv_type = LLInventoryType::IT_WEARABLE;
	LLPointer<LLInventoryCallback> cb;
	if(wear)
	{
		cb = new LLBoostFuncInventoryCallback(wear_and_edit_cb);
	}
	else
	{
		cb = new LLBoostFuncInventoryCallback(wear_cb);
	}

	LLUUID folder_id;

	if (parent_id.notNull())
	{
		folder_id = parent_id;
	}
	else
	{
		LLFolderType::EType folder_type = LLFolderType::assetTypeToFolderType(asset_type);
		folder_id = gInventory.findCategoryUUIDForType(folder_type);
	}

	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  folder_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  asset_type, inv_type,
						  wearable->getType(),
						  LLFloaterPerms::getNextOwnerPerms("Wearables"),
						  cb);
}

// static
void LLAgentWearables::editWearable(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = gInventory.getLinkedItem(item_id);
	if (!item)
	{
		LL_WARNS() << "Failed to get linked item" << LL_ENDL;
		return;
	}

	LLViewerWearable* wearable = gAgentWearables.getWearableFromItemID(item_id);
	if (!wearable)
	{
		LL_WARNS() << "Cannot get wearable" << LL_ENDL;
		return;
	}

	if (!gAgentWearables.isWearableModifiable(item->getUUID()))
	{
		LL_WARNS() << "Cannot modify wearable" << LL_ENDL;
		return;
	}

	const BOOL disable_camera_switch = LLWearableType::getDisableCameraSwitch(wearable->getType());
	LLPanel* panel = LLFloaterSidePanelContainer::getPanel("appearance");
	LLSidepanelAppearance::editWearable(wearable, panel, disable_camera_switch);
}

// Request editing the item after it gets worn.
void LLAgentWearables::requestEditingWearable(const LLUUID& item_id)
{
	mItemToEdit = gInventory.getLinkedItemID(item_id);
}

// Start editing the item if previously requested.
void LLAgentWearables::editWearableIfRequested(const LLUUID& item_id)
{
	if (mItemToEdit.notNull() &&
		mItemToEdit == gInventory.getLinkedItemID(item_id))
	{
		LLAgentWearables::editWearable(item_id);
		mItemToEdit.setNull();
	}
}

//MK from HB
void LLAgentWearables::checkModifiableShape()
{
	mLastWornShape = getViewerWearable(LLWearableType::WT_SHAPE, 0);

	LLViewerInventoryItem* item;
	item = (LLViewerInventoryItem*)getWearableInventoryItem(LLWearableType::WT_SHAPE, 0);
	if (item)
	{
		const LLPermissions& perm = item->getPermissions();
		mHasModifiableShape = perm.allowModifyBy(gAgentID,
												 gAgent.getGroupID());
	}
	else
	{
		mHasModifiableShape = false;
	}
}

void LLAgentWearables::setShapeAvatarOffset(bool send_update)
{
	// MK : I took most of the code from Henri Beauchamp's viewer (thanks Henri), but I tweaked it rather deeply.
	// In this viewer, I want the user to be able to modify their Z offset live, and then have it applied to the
	// shape after a second or so, so it propagates and other users see it too. But it is very important that the
	// user can move the Z offset slider manually and see the change in real time before deciding on an offset.

	// Problem is, the offset we see in local and the offset applied to the shape are different ! When the user leaves
	// the slider alone for a second, the offset could be sent as is to the server, but if we do that the offset will
	// look off. After a few hours of trial and error, it seems the offset returns as 1.5 times the offset we have sent it.

	// Worse, once we use the slider again, the offset suddenly "jumps" and even moving the slider will look off. For example,
	// setting the offset to 0.5 and waiting will make the avatar float above the ground. Then setting it straight back to 0.0
	// should put it straight back to the ground, except it doesn't. It just moves the avatar down half way and stops there.
	// Then after a second, the offset is back to a correct value of 0.0 and the avatar is back to the ground. But it doesn't
	// help setting the offset in local and just confuses the user.

	// Once again after some trial and error, it seems that substracting half of the apparent offset does the trick, that's why
	// you'll see "mSavedOffset * 0.5" later in the code.

	// This is all very hacky and not at all the way I like to code, but this makes the thing work and that's all that matters.

	checkModifiableShape();

	if (gAgent.getRegion() && gAgent.getRegion()->getCentralBakeVersion())
	{
		if (mHasModifiableShape && mLastWornShape)
		{
			F32 offset = gSavedPerAccountSettings.getF32("RestrainedLoveOffsetAvatarZ");
			F32 old_offset = mLastWornShape->getVisualParamWeight(AVATAR_HOVER);
//			LL_INFOS() << "old_offset = " << old_offset << " new offset = " << offset << " saved offset = " << mSavedOffset << LL_ENDL;

			if (old_offset != offset)
			{
				//mLastWornShape->setVisualParamWeight(AVATAR_HOVER, offset - mSavedOffset * OFFSET_FACTOR);
				//mLastWornShape->setVisualParamWeight(AVATAR_HOVER, offset);
				mLastWornShape->setVisualParamWeight(AVATAR_HOVER, offset * 0.88);
				mLastWornShape->writeToAvatar(gAgentAvatarp);
				gAgentAvatarp->updateVisualParams();

				// We've updated the Hover value locally, now we must update the server.
				// But we don't want to hammer the sim with requests, so we're just going to
				// wait for a little while after the last change before triggering the update.
				RRInterface::sLastAvatarZOffsetCommit = gFrameTimeSeconds;
			}
		}
	}
	else
	{
		if (mHasModifiableShape && mLastWornShape &&
			mLastWornShape->getVisualParamWeight(AVATAR_HOVER) != 0.f)
		{
			mLastWornShape->setVisualParamWeight(AVATAR_HOVER, 0.f);
			saveWearable(LLWearableType::WT_SHAPE, 0);
		}
	}
}
//mk from HB

//MK
void LLAgentWearables::forceUpdateShape (void)
{
	checkModifiableShape();
	if (!mHasModifiableShape || !mLastWornShape)
	{
		return;
	}

	RRInterface::sLastOutfitChange = gFrameTimeSeconds;

	F32 offset = gSavedPerAccountSettings.getF32("RestrainedLoveOffsetAvatarZ");
	mSavedOffset = offset;
	mLastWornShape->setVisualParamWeight(AVATAR_HOVER, offset * OFFSET_FACTOR);
	//mLastWornShape->writeToAvatar(gAgentAvatarp);
	//gAgentAvatarp->updateVisualParams();

	U32 index;
	if (!gAgentWearables.getWearableIndex(mLastWornShape, index))
	{
		return;
	}

    std::string new_name = mLastWornShape->getName();

	// Find an existing link to this wearable's inventory item, if any, and its description field.
	LLInventoryItem *link_item = NULL;
	std::string description;
	LLInventoryModel::item_array_t links =
		LLAppearanceMgr::instance().findCOFItemLinks(mLastWornShape->getItemID());
	if (links.size()>0)
	{
		link_item = links.at(0).get();
		if (link_item && link_item->getIsLinkType())
		{
			description = link_item->getActualDescription();
		}
	}

	// Make another copy of this link, with the same
	// description.  This is needed to bump the COF
	// version so texture baking service knows appearance has changed.
	if (link_item)
	{
		// Create new link
		LL_DEBUGS("Avatar") << "link refresh, creating new link to " << link_item->getLinkedUUID()
							<< " removing old link at " << link_item->getUUID()
							<< " wearable item id " << mLastWornShape->getItemID() << LL_ENDL;

		LLInventoryObject::const_object_list_t obj_array;
		obj_array.push_back(LLConstPointer<LLInventoryObject>(link_item));
		link_inventory_array(LLAppearanceMgr::instance().getCOF(),
								obj_array, 
								gAgentAvatarp->mEndCustomizeCallback);
		//// Remove old link
		LLPointer<LLInventoryCallback> cb = new LLUpdateAppearanceOnDestroy;
		//LLAppearanceMgr::instance().removeCOFItemLinks(link_item->getUUID(), cb);

		gAgentWearables.saveWearable(mLastWornShape->getType(), index, new_name);
		//LLAppearanceMgr::instance().updateAppearanceFromCOF();
		LLAppearanceMgr::instance().enforceCOFItemRestrictions(cb);
	}

	//// To force the update of the shape, we need to remove the link to it from the COF
	//// and then immediately add a new link to it.

	//checkModifiableShape();

	//F32 offset = gSavedPerAccountSettings.getF32("RestrainedLoveOffsetAvatarZ");

	//if (gAgent.getRegion() && gAgent.getRegion()->getCentralBakeVersion())
	//{
	//	if (mHasModifiableShape && mLastWornShape)
	//	{
	//		// For some reason, setting X to RestrainedLoveOffsetAvatarZ will set the Hover to X * 1.5, but only
	//		// after the bake. DON'T ASK ME WHY !
	//		mSavedOffset = offset * OFFSET_FACTOR;
	//		mLastWornShape->setVisualParamWeight(AVATAR_HOVER, mSavedOffset);
	//	}
	//}

	//saveWearable(LLWearableType::WT_SHAPE, 0);

	//if (gAgent.getRegion() && gAgent.getRegion()->getCentralBakeVersion())
	//{
	//	if (mHasModifiableShape && mLastWornShape)
	//	{
	//		mLastWornShape->setVisualParamWeight(AVATAR_HOVER, offset);
	//	}
	//}

	////LLAppearanceMgr::instance().setOutfitDirty( true );		

	//// HACK : Force an update server-side by removing the link to the shape, then adding a new one
	//LLViewerWearable* shape = getViewerWearable(LLWearableType::WT_SHAPE, 0);

	//LLUUID uuid = shape->getItemID();
	//LLViewerInventoryItem* item = gInventory.getItem (uuid);

	//LLInventoryModel::cat_array_t cat_array;
	//LLInventoryModel::item_array_t item_array;
	//gInventory.collectDescendents(LLAppearanceMgr::instance().getCOF(),
	//								cat_array,
	//								item_array,
	//								LLInventoryModel::EXCLUDE_TRASH);
	//for (S32 i=0; i<item_array.size(); i++)
	//{
	//	const LLInventoryItem* item = item_array.at(i).get();
	//	if (item->getIsLinkType() && item->getLinkedUUID() == uuid)
	//	{
	//		remove_inventory_item(item->getUUID(), NULL);
	//		break;
	//	}
	//}

	//// Now create a new link to the shape
	//LLAppearanceMgr::instance().addCOFItemLink (item);

	////LLAppearanceMgr::instance().incrementCofVersion();
}
//mk


boost::signals2::connection LLAgentWearables::addLoadingStartedCallback(loading_started_callback_t cb)
{
	return mLoadingStartedSignal.connect(cb);
}

boost::signals2::connection LLAgentWearables::addLoadedCallback(loaded_callback_t cb)
{
	return mLoadedSignal.connect(cb);
}

bool LLAgentWearables::changeInProgress() const
{
	return mCOFChangeInProgress;
}

// [SL:KB] - Patch: Appearance-InitialWearablesLoadedCallback | Checked: 2010-08-14 (Catznip-2.1)
boost::signals2::connection LLAgentWearables::addInitialWearablesLoadedCallback(const loaded_callback_t& cb)
{
	return mInitialWearablesLoadedSignal.connect(cb);
}
// [/SL:KB]

void LLAgentWearables::notifyLoadingStarted()
{
	mCOFChangeInProgress = true;
	mCOFChangeTimer.reset();
	mLoadingStartedSignal();
}

void LLAgentWearables::notifyLoadingFinished()
{
	mCOFChangeInProgress = false;
	mLoadedSignal();
}
// EOF
