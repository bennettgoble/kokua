/** 
 * @file RRInterfaceHelper.h
 * @author Chorazin Allen
 * @brief Isolated header for llui library llfloater.cpp to call
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

// RRinterface needs this, but it's no longer provided by llwearabledata.h
#define MAX_CLOTHING_PER_TYPE 10

// Switch for whether to operate as always-on RLV
#define RLV_ALWAYS_ON 0 

class RRHelper
{
public:
	RRHelper ();
	~RRHelper ();
	static BOOL preventFloater(std::string floaterName); //used to check if a floater should not be made visible due to a restriction
};
