/** 
 * @file RRInterface.cpp
 * @author Marine Kelley
 * @brief Implementation of the RLV features
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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llavatarnamecache.h"
#include "llcamera.h"
#include "lldrawpoolalpha.h"
#include "llenvadapters.h" // new include for EEP
#include "llenvironment.h" // new include for EEP
//#include "llfloaterenvsettings.h"
//#include "llfloatereditsky.h" // no longer exists with EEP
#include "llfloaterimnearbychat.h"
#include "llfloatermap.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterreg.h"
#include "llfloatersettingsdebug.h"
#include "llfloatersidepanelcontainer.h"
//#include "llfloaterwater.h"
//#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llgroupactions.h"
#include "llhudtext.h"
#include "llinventoryfunctions.h"
#include "llmoveview.h"
#include "llnavigationbar.h"
//#include "llnearbychat.h"
//#include "llnearbychatbar.h"
#include "llnotifications.h"
#include "llpanelmaininventory.h"
#include "llpaneloutfitedit.h"
#include "llpaneltopinfobar.h"
#include "llpresetsmanager.h"
#include "llregionhandle.h"
#include "llrendersphere.h"
#include "llselectmgr.h"
#include "llsettingssky.h" // new include for EEP
#include "llsettingsvo.h" // new include for EEP
#include "llspeakers.h"
#include "llstartup.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
//#include "llinventoryview.h"
#include "lltooldraganddrop.h"
#include "lltoolmgr.h"
#include "lltracker.h"
//#include "llurlsimstring.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
//#include "llwaterparammanager.h" // no longer exists with EEP
//#include "llwlparammanager.h" // no longer exists with EEP
#include "llinventorybridge.h"
#include "llviewerdisplay.h"
#include "llviewerjoystick.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h" // new include for EEP
#include "llviewermessage.h"
#include "llviewerparcelmgr.h"
#include "llvisualeffect.h" // from Catznip
#include "llwearabledata.h"
#include "llworldmapmessage.h"
#include "pipeline.h"

#include <boost/algorithm/string.hpp>

#include <cstdlib>

#include "RRInterface.h"
#include "llversioninfo.h"
//CA
#include "RRInterfaceHelper.h"
#include "RRInterfaceVersion.h"
#include "kokuarlvfloaters.h"
#include "lluicolor.h"
#include "lluicolortable.h"
#include "lloutfitslist.h"              // @showinv - "Appearance / My Outfits" panel
#include "llpaneloutfitsinventory.h"    // @showinv - "Appearance" floater
#include "llpanelwearing.h"             // @showinv - "Appearance / Current Outfit" panel
#include "kokuarlvextras.h"
#include "llteleporthistory.h"      // KKA-682 to allow another try at storing login location in TP history
#include "fsareasearch.h"         // to refresh it on changes in shownames state
//ca

// Global and static variables initialization.
BOOL gRRenabled = TRUE;
BOOL RRInterface::sRRNoSetEnv = FALSE;
BOOL RRInterface::sRestrainedLoveDebug = FALSE; // Note: not used in this file; only used in llviewermessage for 'executes/fails command'
BOOL RRInterface::sRestrainedLoveLogging = FALSE; // Note: currently only used in this file
BOOL RRInterface::sRestrainedLoveCommandLogging = FALSE; // KKA-901/KKA-914 Make some logging full time to aid crash hunting. Currently only used in this file
BOOL RRInterface::sRestrainedLoveHeadMouselookRenderRigged = FALSE;
BOOL RRInterface::sRestrainedLoveRenderInvisibleSurfaces = FALSE;
BOOL RRInterface::sCanOoc = TRUE;
std::string RRInterface::sRecvimMessage = "The Resident you messaged is prevented from reading your instant messages at the moment, please try again later.";
std::string RRInterface::sSendimMessage = "*** IM blocked by sender's viewer";
std::string RRInterface::sBlacklist = "";
F32 RRInterface::sLastOutfitChange = -1000.f;
U32 RRInterface::mCamDistNbGradients = 40;
BOOL RRInterface::sRenderLimitRenderedThisFrame = FALSE;
F32 RRInterface::previousEastAngle = 0;
F32 RRInterface::previousSunMoonPosition = 0;

#if !defined(max)
#define max(a, b)   ((a) > (b) ? (a) : (b))
#endif

//CA Helper function to be called from static libraries so we need to hide as much as possible
//   to avoid going down a rabbit warren of using newview headers in the static libraries (llui)
RRHelper::RRHelper()
{
}

RRHelper::~RRHelper()
{
}

BOOL RRHelper::preventFloater(std::string floaterName)
{
    if (!gRRenabled) return FALSE;

    if ((floaterName == "area_search" || floaterName == "floaterland") && gAgent.mRRInterface.mContainsShowloc) return TRUE;
    else if (floaterName == "map" && gAgent.mRRInterface.mContainsShowminimap) return TRUE;
    else if (floaterName == "worldmap" && gAgent.mRRInterface.mContainsShowworldmap) return TRUE;
    else if (floaterName == "Destinations" && gAgent.mRRInterface.mContainsTp) return TRUE;
    else if (floaterName == "floater_my_inventory" && gAgent.mRRInterface.mContainsShowinv) return TRUE;
    else if (floaterName == "rlv_console" && gAgent.mRRInterface.mContainsViewScript) return TRUE;

    else if (gAgent.mRRInterface.mContainsSetenv)
    {
        if (floaterName == "env_post_process" || floaterName == "env_fixed_environment_water" || 
            floaterName == "env_fixed_environment_sky" || floaterName == "env_adjust_snapshot" ||
            floaterName == "env_edit_extdaycycle" || floaterName == "my_environments") return TRUE;
    }
    return FALSE;
}

static LLUUID current_handlecommand_caller;
//ca

namespace {
        bool approx_equal(F32 f1, F32 f2)
        {
            // LL's is_approx_zero() returns incorrect results when fed values of different signs, eg +1 and -1
            // So here's our own version which also preserves the signage of nearly zero values
            return (fabs(f1 - f2) < 0.00001 && (std::signbit(f1) == std::signbit(f2)));
        }
    
        void cleanMathFloat(F32 &f)
        {
            //make nearly 1 and nearly 0 exact and make sure we're not over 1 in either direction since sin & cos shouldn't ever do that
            if (f < -1.0f) f = -1.0f;
            else if (approx_equal(-1.0f, f)) f = -1.0f;
            else if (approx_equal(-0.0f, f)) f = -0.0f;
            else if (approx_equal( 0.0f, f)) f =  0.0f;
            else if (approx_equal( 1.0f, f)) f =  1.0f;
            else if (f > 1.0f) f = 1.0f;
        }
    
        void cleanMathVector(LLVector3 &v)
        {
            cleanMathFloat(v.mV[0]);
            cleanMathFloat(v.mV[1]);
            cleanMathFloat(v.mV[2]);
        }
            
    LLVector3 convert_direction_to_azimuth_and_elevation(LLVector3 dir)
    {
        // The azimuth routine is adapted from a LL routine removed from llvosky when EEP was introduced for 
            // azimuth and from current llsky for elevation
            //
            // WARNING: due to the way that LLQuaternion::setAngleToAxis is coded, and the fact that eastangle and
            // sunmoonposition were defined as describing a whole circle rather than a semi-circle it's possible to
            // describe any position on the sphere formed by eastangle/sunmoonposition in two ways. For example,
            // eastangle=sunmoonposition=0 describes the same point as eastangle=sunmoonposition=0.5 
            // Accordingly, it's not possible to reliably to do a read and derive both / modify one / write both
            // back style implementation of setenv_eastangle/sunmoonposition. 
            // For same reason, it's now strongly recommended that callers always read both sunmoonposition and 
            // eastangle in case the returned value is part of the opposite pair to the one they expect.

            // There are some special cases where we lose the ability to return one of two postions, such as when
            // the elevation is directly up or directly down - in those situations azimuth could be anything since
            // it's irrelevant. For these we try to help by returning our last used value through the setenv commands
            
            // Reminder of the vector content
            // dir.mV[0] = cosAzimuth * cosElevation
      // dir.mV[1] = sinAzimuth * cosElevation
      // dir.mV[2] = sinElevation

            // math errors can mean that what should be 0 isn't quite and same for 1 - by the time we see them here
            // these values have been through trig functions and in and out of quaternion conversions
            cleanMathVector(dir);
            
            F32 azimuth = 0.0f;
            F32 elevation = 0.0f;
            F32 xy_component = (F32) sqrt(dir.mV[VX] * dir.mV[VX] + dir.mV[VY] * dir.mV[VY]);
            if (xy_component == 0.0f)
            {
                if (dir.mV[VZ] > 0.f)
                {
                    elevation = F_PI_BY_TWO;
                    azimuth = RRInterface::previousEastAngle * -F_TWO_PI; // it could have been anything when set, so prefer a value we're aware of (note the negation)
                }
                else
                {
                    elevation = -F_PI_BY_TWO;
                    azimuth = RRInterface::previousEastAngle * F_TWO_PI; // it could have been anything when set, so prefer a value we're aware of
                }
            }
            else if ((fabs(dir.mV[VX]) == 1.0f) && dir.mV[VY] == 0.0f && dir.mV[VZ] == 0.0f)
            {
                // it should be the case that VX comes back as -1 in some cases, which would indicate that it's a case where azimuth is
                // PI rather than zero, however it looks like we always get back 1,0,0 in this situation. We can't go any further on
                // figuring out the azimuth because cos alt being zero has wiped out the value of sin azimuth in mV[VY]. The azimuth
                // is 0 or +/PI and we can't tell which.
                //
                // We're in similar straits with the elevation. It too could be 0 or +-PI. If we could trust the sign of mV[VX] that
                // would let us decide whether it's 0,PI and PI,0 or it's 0,0 or PI,PI
                //
                // The only option I see is to return our saved values if they are consistent with this situation. If they're not
                // we'll fall back to returning one of the possibilities
                
                // at the moment elevation and azimuth are still initialised at zero; we'll return them as such if this test fails
                if (approx_equal(fabs(sin(F_TWO_PI * RRInterface::previousEastAngle)), 0.0f)  && approx_equal(fabs(sin(F_TWO_PI * RRInterface::previousSunMoonPosition)), 0.0f))                    
                {
                    elevation = RRInterface::previousSunMoonPosition * F_TWO_PI;
                    azimuth = RRInterface::previousEastAngle * -F_TWO_PI;
                }
            }               
            else
            {
                elevation = (F32) atan(dir.mV[VZ]/xy_component);
                                        
                if (dir.mV[VX] == 0.0f)
                {
                    if (dir.mV[VY] > 0.0f)
                    {
                        azimuth = F_PI_BY_TWO;
                    }
                    else if (dir.mV[VY] < 0.0f)
                    {
                        azimuth = F_TWO_PI - F_PI_BY_TWO;
                    }
                }
                else
                {
                    azimuth = (F32) atan(dir.mV[VY] / dir.mV[VX]);
                    if (dir.mV[VX] < 0.0f)
                    {
                        azimuth += F_PI;
                        elevation = F_PI - elevation;
                    }
                    else if (dir.mV[VY] < 0.0f)
                    {
                        azimuth += F_TWO_PI;
                        // no effect on elevation
                    }
                }   
            }
            while (azimuth < 0)
            {
                    azimuth += F_TWO_PI;
            }
            while (elevation < 0)
            {
                elevation += F_TWO_PI;
            }
        return (LLVector3(azimuth, elevation, 0));
    }

        // Adopted from llsettingsksy
    LLQuaternion convert_azimuth_and_elevation_to_quat(F32 azimuth, F32 elevation)
    {
            // eastangle should be negated before calling to convert rotation direction to azimuth
        F32 sinAzimuth = sin(azimuth);
        F32 cosAzimuth = cos(azimuth);
        F32 sinElevation   = sin(elevation);
        F32 cosElevation   = cos(elevation);
        
        LLVector3 dir;
        // +x right, +z up, +y at...    
        dir.mV[0] = cosAzimuth * cosElevation;
        dir.mV[1] = sinAzimuth * cosElevation;  
        dir.mV[2] = sinElevation;

        LLVector3 axis = LLVector3::x_axis % dir;
        // this is a.mV[1]*b.mV[2] - b.mV[1]*a.mV[2], a.mV[2]*b.mV[0] - b.mV[2]*a.mV[0], a.mV[0]*b.mV[1] - b.mV[0]*a.mV[1]
        // however a = <1,0,0> so this simplifies to
        // 0-0, 0-bmV[2], bmV[1]-0
        // ie axis=<0, -sinElevation, sinAzimuth * cosElevation>
        axis.normalize();

        F32 angle = acos(LLVector3::x_axis * dir); // ie x1x2+y1y2+z1z2, but y1=z1=0 and x1=1, so = dir.mV[0] = cosAzimuth * cosElevation

        LLQuaternion quat;
        quat.setAngleAxis(angle, axis);
        return quat;
    }
}
// --
// Local functions
std::string dumpList2String (std::deque<std::string> list, std::string sep, int size = -1)
{
    bool found_one = false;
    if (size < 0) size = (int)list.size();
    std::string res = "";
    for (int i = 0; i < (int)list.size() && i < size; ++i) {
        if (found_one) res += sep;
        found_one = true;
        res += list[i];
    }
    return res;
}

int match (std::deque<std::string> list, std::string str, bool& exact_match)
{
    // does str contain list[0]/list[1]/.../list[n] ?
    // yes => return the size of the list
    // no  => try again after removing the last element
    // return 0 if never found
    // exact_match is an output, set to true when strict matching is found, false otherwise.
    unsigned int size = list.size();
    std::string dump;
    exact_match = false;
    while (size > 0) {
        dump = dumpList2String (list, "/", (int)size);
        if (str == dump) {
            exact_match = true;
            return (int)size;
        }
        else if (str.find (dump) != -1) {
            return (int)size;
        }
        size--;
    }
    return 0;
}

std::deque<std::string> getSubList (std::deque<std::string> list, int min, int max = -1)
{
    if (min < 0) min = 0;
    if (max < 0) max = list.size() - 1;
    std::deque<std::string> res;
    for (int i = min; i <= max; ++i) {
        res.push_back (list[i]);
    }
    return res;
}

bool findMultiple (std::deque<std::string> list, std::string str)
{
    // returns true if all the tokens in list are contained into str
    unsigned int size = list.size();
    for (unsigned int i = 0; i < size; i++) {
        if (str.find (list[i]) == -1) return false;
    }
    return true;
}

void setVisibleAll(std::string floater_name, BOOL visible)
{
    // Use this to hide or show all floaters bearing this name
    U32 count = 0;
    LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList(floater_name);
    for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end();)
    {
        LLFloater* iv = dynamic_cast<LLFloater*>(*iter++);
        if (iv)
        {
            count++;
            iv->setVisible(visible);
        }
    }
}

void refreshCachedVariable (std::string var)
{
    // Call this function when adding/removing a restriction only, i.e. in this file
    // Test the cached variables in the code of the viewer itself
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    if (!avatar) return;

    BOOL contained = gAgent.mRRInterface.contains (var);
    if (var == "detach" || var.find ("detach:") == 0 || var.find ("addattach") == 0 || var.find ("remattach") == 0) {
        contained = gAgent.mRRInterface.contains("detach")
        || gAgent.mRRInterface.containsSubstr("detach:")
        || gAgent.mRRInterface.containsSubstr("addattach")
        || gAgent.mRRInterface.containsSubstr("remattach");
        gAgent.mRRInterface.mContainsDetach = contained;
        gAgent.mRRInterface.mHasLockedHuds = gAgent.mRRInterface.hasLockedHuds();
        if (gAgent.mRRInterface.mHasLockedHuds) {
            // To force the viewer to render the HUDs again, just in case
            LLPipeline::sShowHUDAttachments = TRUE;
        }
    }
    else if (var == "showinv")              gAgent.mRRInterface.mContainsShowinv = contained;
    else if (var == "unsit")                gAgent.mRRInterface.mContainsUnsit = contained;
    else if (var == "standtp")          gAgent.mRRInterface.mContainsStandtp = contained;
    else if (var == "interact")             gAgent.mRRInterface.mContainsInteract = contained;
    else if (var == "showworldmap")         gAgent.mRRInterface.mContainsShowworldmap = contained;
    else if (var == "showminimap")          gAgent.mRRInterface.mContainsShowminimap = contained;
    else if (var == "showloc")              gAgent.mRRInterface.mContainsShowloc = contained;
    //else if (var == "shownames")          gAgent.mRRInterface.mContainsShownames = contained;
    else if (var == "shownametags")         gAgent.mRRInterface.mContainsShownametags = contained;
    else if (var == "shownearby")           gAgent.mRRInterface.mContainsShowNearby = contained;
    else if (var == "setenv")               gAgent.mRRInterface.mContainsSetenv = contained;
    else if (var == "setdebug")             gAgent.mRRInterface.mContainsSetdebug = contained;
    else if (var == "fly")                  gAgent.mRRInterface.mContainsFly = contained;
    else if (var == "edit")                 gAgent.mRRInterface.mContainsEdit = (gAgent.mRRInterface.containsWithoutException ("edit")); // || gAgent.mRRInterface.containsSubstr ("editobj"));
    else if (var == "rez")                  gAgent.mRRInterface.mContainsRez = contained;
    else if (var == "showhovertextall")     gAgent.mRRInterface.mContainsShowhovertextall = contained;
    else if (var == "showhovertexthud")     gAgent.mRRInterface.mContainsShowhovertexthud = contained;
    else if (var == "showhovertextworld")   gAgent.mRRInterface.mContainsShowhovertextworld = contained;
    else if (var == "defaultwear")          gAgent.mRRInterface.mContainsDefaultwear = contained;
    else if (var == "permissive")           gAgent.mRRInterface.mContainsPermissive = contained;
    else if (var == "temprun")                  gAgent.mRRInterface.mContainsRun = contained;
    else if (var == "alwaysrun")                gAgent.mRRInterface.mContainsAlwaysRun = contained;
    //CA add viewscript since the console needs to check for it
    else if (var == "viewscript")               gAgent.mRRInterface.mContainsViewScript = contained;
    else if (var == "setsphere")                gAgent.mRRInterface.mContainsSetsphere = contained;
    else if (var == "touchattach")              gAgent.mRRInterface.mContainsTouchattach = contained;
    else if (var == "touchattachself")              gAgent.mRRInterface.mContainsTouchattachself = contained;
    else if (var == "touchattachother")             gAgent.mRRInterface.mContainsTouchattachother = contained;
    else if (var == "touchhud")             gAgent.mRRInterface.mContainsTouchhud = contained;
    else if (var == "touchall")             gAgent.mRRInterface.mContainsTouchall = contained;
    else if (var == "viewtexture")              gAgent.mRRInterface.mContainsViewTexture = contained;
    else if (var == "viewnote")             gAgent.mRRInterface.mContainsViewNote = contained;
    else if (var == "setcam_unlock" || var == "camunlock") {
        if (var == "camunlock") gAgent.mRRInterface.mContainsCamunlock = contained;
        else gAgent.mRRInterface.mContainsSetcamUnlock = contained;
        gAgent.mRRInterface.mContainsLockedCamera = gAgent.mRRInterface.mContainsCamunlock | gAgent.mRRInterface.mContainsSetcamUnlock;
    }
    
        
    //else if (var == "moveup")                 gAgent.mRRInterface.mContainsMoveUp = contained;
    //else if (var == "movedown")               gAgent.mRRInterface.mContainsMoveDown = contained;
    //else if (var == "moveleft")           gAgent.mRRInterface.mContainsMoveStrafeLeft = contained;
    //else if (var == "moveright")          gAgent.mRRInterface.mContainsMoveStrafeRight = contained;
    //else if (var == "moveforward")                gAgent.mRRInterface.mContainsMoveForward = contained;
    //else if (var == "movebackward")           gAgent.mRRInterface.mContainsMoveBackward = contained;
    //else if (var == "moveturnup")             gAgent.mRRInterface.mContainsMoveTurnUp = contained;
    //else if (var == "moveturndown")           gAgent.mRRInterface.mContainsMoveTurnDown = contained;
    //else if (var == "moveturnleft")           gAgent.mRRInterface.mContainsMoveTurnLeft = contained;
    //else if (var == "moveturnright")          gAgent.mRRInterface.mContainsMoveTurnRight = contained;

    gAgent.mRRInterface.mContainsCamTextures = (gAgent.mRRInterface.containsSubstr("camtextures") || gAgent.mRRInterface.containsSubstr("setcam_textures"));
    gAgent.mRRInterface.mContainsShownames = (gAgent.mRRInterface.containsSubstr("shownames")); // shownames, shownames_sec

    if (var == "showinv") {
        if (gAgent.mRRInterface.mContainsShowinv) {
//          LLSideTray::getInstance()->childSetVisible("panel_main_inventory", false);
            LLFloaterReg::hideInstance("panel_main_inventory", LLSD());
            setVisibleAll("inventory", FALSE);
            LLPanelOutfitEdit* panel_outfit_edit = dynamic_cast<LLPanelOutfitEdit*>(LLFloaterSidePanelContainer::getPanel("appearance", "panel_outfit_edit"));
            if (NULL != panel_outfit_edit) {
                panel_outfit_edit->showAddWearablesPanel(false);
            }
            //LLFloaterInventory::hideAll(); // close all the secondary inventory floaters
//          LLBottomTray::getInstance()->childSetEnabled("inventory_btn", false);
        }
        else {
//          setVisibleAll("inventory", TRUE);
//          LLSideTray::getInstance()->childSetVisible("panel_main_inventory", true);
//          LLBottomTray::getInstance()->childSetEnabled("inventory_btn", true);
        }

        //
        // Enable/disable the "My Outfits" panel on the "My Appearance" sidebar tab (lifted from RLVa and plumbed in here)
        //
        bool fHasBhvr = gAgent.mRRInterface.mContainsShowinv;
        LLPanelOutfitsInventory* pAppearancePanel = LLPanelOutfitsInventory::findInstance();
        if (pAppearancePanel)
        {
            LLTabContainer* pAppearanceTabs = pAppearancePanel->getAppearanceTabs();
            LLOutfitsList* pMyOutfitsPanel = pAppearancePanel->getMyOutfitsPanel();
            if ( (pAppearanceTabs) && (pMyOutfitsPanel) )
            {
                S32 idxTab = pAppearanceTabs->getIndexForPanel(pMyOutfitsPanel);
                if (-1 != idxTab)
                {
                    pAppearanceTabs->enableTabButton(idxTab, !fHasBhvr);

                    // When disabling, switch to the COF tab if "My Outfits" is currently active
                    if ( (fHasBhvr) && (pAppearanceTabs->getCurrentPanelIndex() == idxTab) )
                        pAppearanceTabs->selectTabPanel(pAppearancePanel->getCurrentOutfitPanel());
                }
            }
        }
    }

    else if (var == "rez") {
        bool parcel_allows_build = LLToolMgr::getInstance()->canEdit();
        if (parcel_allows_build) { // if we can't edit in this parcel, no need to refresh the button anyway
            if (gAgent.mRRInterface.mContainsRez) {
//              LLBottomTray::getInstance()->childSetEnabled("build_btn", false);
            }
            else {
//              LLBottomTray::getInstance()->childSetEnabled("build_btn", true);
            }
        }
    }
// KKA-813/RLV-147 RLV 2.9.20 added shownames:uuid so this test needs to change, the new test catches shownames_sec too
    else if (var.find ("shownames") == 0 || var == "shownametags" || var == "shownearby") {
        if (gAgent.mRRInterface.mContainsShownames || gAgent.mRRInterface.mContainsShownametags || gAgent.mRRInterface.mContainsShowNearby) {
//          LLSideTray::getInstance()->childSetVisible("nearby_panel", false);
//          LLSideTray::getInstance()->childSetVisible("recent_panel", false);
            LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
            if (panel) {
                panel->childSetVisible("avatar_list", false);
            }
            FSAreaSearch* area_search_floater = LLFloaterReg::findTypedInstance<FSAreaSearch>("area_search");
            if (area_search_floater)
            {
                area_search_floater->refreshList(false);
            }
        }
        else {
//          LLSideTray::getInstance()->childSetVisible("nearby_panel", true);
//          LLSideTray::getInstance()->childSetVisible("recent_panel", true);
            LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
            if (panel) {
                panel->childSetVisible("avatar_list", true);
            }
            FSAreaSearch* area_search_floater = LLFloaterReg::findTypedInstance<FSAreaSearch>("area_search");
            if (area_search_floater)
            {
                area_search_floater->refreshList(false);
            }
        }
        LLLocalSpeakerMgr::getInstance()->updateSpeakerList();
        //KKA-634 give the name tags a kick to reflect any changes
        LLVOAvatar::invalidateNameTags();
    }

    else if (var == "showminimap") {
        if (gAgent.mRRInterface.mContainsShowminimap) {
            LLFloaterMap::getInstance()->setVisible (false);
            LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
            if (panel) {
                panel->childSetVisible("Net Map", false);
            }
//          LLBottomTray::getInstance()->childSetEnabled("mini_map_btn", false);
//          LLSideTray::getInstance()->childSetVisible("Net Map", false);

        }
        else {
            if (!gAgent.mRRInterface.mContainsShowloc) {
//              LLBottomTray::getInstance()->childSetEnabled("mini_map_btn", true);
            }
            LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
            if (panel) {
                panel->childSetVisible("Net Map", true);
            }
//          LLSideTray::getInstance()->childSetVisible("Net Map", true);
        }
    }

    else if (var == "showworldmap") {
        if (gAgent.mRRInterface.mContainsShowworldmap) {
            LLFloaterWorldMap::getInstance()->setVisible (false);
//          LLBottomTray::getInstance()->childSetEnabled("world_map_btn", false);
        }
        else {
            if (!gAgent.mRRInterface.mContainsShowloc) {
//              LLBottomTray::getInstance()->childSetEnabled("world_map_btn", true);
            }
        }
    }
    
    //CA viewscript prevents the RLV console, unless being done from inside it
    else if (var == "viewscript") {
        if (gAgent.mRRInterface.mContainsViewScript && gAgent.getID() != current_handlecommand_caller) {
            if (KokuaFloaterRLVConsole::getBase()) KokuaFloaterRLVConsole::getBase()->closeFloater(false);
        }
    }
    
    else if (var == "showloc") {
        LLNavigationBar::getInstance()->refresh();
        if (gAgent.mRRInterface.mContainsShowloc) {
            if (LLPanelTopInfoBar::getInstance()->getVisible()) {
                toggle_show_mini_location_panel(LLSD(false));
            }
            LLFloaterWorldMap::getInstance()->setVisible (false);
            setVisibleAll("panel_places", FALSE);
            gSavedSettings.setBOOL ("ShowMiniLocationPanel", FALSE);
//CA
            //also close area search
            LLFloaterReg::hideInstance("area_search", LLSD());
//ca
        }
        else {
            LLPanelTopInfoBar::getInstance()->update();
            if (!gAgent.mRRInterface.mContainsShowworldmap) {
            }
        }
    }

    // Here we need to explicitely refresh the Stand and Stop Flying buttons because there is no Refresh function
    else if (var == "unsit" || var == "fly") {
        if (avatar->isSitting()) {
            if (gAgent.mRRInterface.mContainsUnsit) LLFloaterMove::setSittingMode(FALSE);
            else LLFloaterMove::setSittingMode(TRUE);
        }
        else if (gAgent.getFlying()) {
            if (gAgent.mRRInterface.mContainsFly) LLFloaterMove::setFlyingMode(FALSE);
            else LLFloaterMove::setFlyingMode(TRUE);
        }
    }
    else if (var == "temprun") {
        if (gAgent.mRRInterface.mContainsRun) {
            if (gAgent.getRunning()) {
                if (gAgent.getAlwaysRun()) gAgent.clearAlwaysRun();
                gAgent.clearRunning();
                gAgent.sendWalkRun(gAgent.getRunning());
            }
        }
    }
    else if (var == "alwaysrun") {
        if (gAgent.mRRInterface.mContainsAlwaysRun) {
            if (gAgent.getAlwaysRun()) {
                if (gAgent.getRunning()) gAgent.clearRunning();
                gAgent.clearAlwaysRun();
                gAgent.sendWalkRun(gAgent.getRunning());
            }
        }
    }
    else if (var.find ("camtextures") == 0 || var.find ("setcam_textures") == 0) {
        // silly hack, but we need to force all textures in world to be updated
        S32 i;
        for (i=0; i<gObjectList.getNumObjects(); ++i) {
            LLViewerObject* object = gObjectList.getObject(i);
            if (object) {
                object->setSelected(FALSE);
            }
        }

        // Is there a uuid specified ?
        int ind = var.find(":");
        if (ind != -1) {
            std::string uuid_str = var.substr(ind + 1);
            LLUUID uuid;
            if (uuid.set(uuid_str)) {
                //if (gAgent.mRRInterface.mCamTexturesCustom != LLViewerFetchedTexture::sDefaultImagep) {
                //  delete (gAgent.mRRInterface.mCamTexturesCustom);
                //}
                gAgent.mRRInterface.mCamTexturesCustom = LLViewerTextureManager::getFetchedTexture(uuid, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
            }
        }
        else {
            //if (gAgent.mRRInterface.mCamTexturesCustom != LLViewerFetchedTexture::sDefaultImagep) {
            //  delete (gAgent.mRRInterface.mCamTexturesCustom);
            //}
            gAgent.mRRInterface.mCamTexturesCustom = LLViewerFetchedTexture::sDefaultImagep;
        }
    }
    else if (var == "camunlock" || var == "setcam_unlock") {
        gAgentCamera.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
        LLPresetsManager::getInstance()->loadPreset(PRESETS_CAMERA, PRESETS_REAR_VIEW);
        gAgentCamera.resetView(TRUE, TRUE);
    }
    //else if (var.find ("camzoommax") == 0 || var.find ("camzoommin") == 0) {
    //  LLViewerCamera::getInstance()->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
    //  gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
    //}

    //else if (var.find("setcam_fovmin") == 0 || var.find("setcam_fovmax") == 0) {
    //  LLViewerCamera::getInstance()->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
    //  gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
    //}

    if (gAgent.mRRInterface.contains("tplm")
    || gAgent.mRRInterface.contains("tploc")
    || gAgent.mRRInterface.contains("tplure")
    || (gAgent.mRRInterface.mContainsUnsit && gAgentAvatarp->mIsSitting)
    ) {
        gAgent.mRRInterface.mContainsTp = TRUE;
    }
    else {
        gAgent.mRRInterface.mContainsTp = FALSE;
    }

}

void updateAllHudTexts ()
{
    LLHUDText::TextObjectIterator text_it;
    
    for (text_it = LLHUDText::sTextObjects.begin(); 
        text_it != LLHUDText::sTextObjects.end(); 
        ++text_it)
    {
        LLHUDText *hudText = *text_it;
        if (hudText && hudText->mLastMessageText != "") {
            // do not update the floating names of the avatars around
            LLViewerObject* obj = hudText->getSourceObject();
            if (obj && !obj->isAvatar()) {
                hudText->setString(hudText->mLastMessageText);
            }
        }
    }
}

void updateOneHudText (LLUUID uuid)
{
    LLViewerObject* obj = gObjectList.findObject(uuid);
    if (obj) {
        if (obj->mText.notNull()) {
            LLHUDText *hudText = obj->mText.get();
            if (hudText && hudText->mLastMessageText != "") {
                hudText->setString(hudText->mLastMessageText);
            }
        }
    }
}

// --





RRInterface::RRInterface():
    mInventoryFetched(FALSE)
    , mAllowCancelTp(TRUE)
    , mSitTargetId()
    , mLastLoadedPreset()
    , mReattaching(FALSE)
    , mReattachTimeout(FALSE)
    , mSnappingBackToLastStandingLocation(FALSE)
    , mUserUpdateAttachmentsUpdatesAll(FALSE)
    , mUserUpdateAttachmentsCalledFromScript(FALSE)
    , mScriptsEnabledOnce(FALSE)
    , mHasLockedHuds(FALSE)
    , mParcelLandingType(LLParcel::L_DIRECT)
    , mContainsDetach(FALSE)
    , mContainsShowinv(FALSE)
    , mContainsUnsit(FALSE)
    , mContainsInteract(FALSE)
    , mContainsShowworldmap(FALSE)
    , mContainsShowminimap(FALSE)
    , mContainsShowloc(FALSE)
    , mContainsShownames(FALSE)
    , mContainsShownametags(FALSE)
    , mContainsShowNearby(FALSE)
    , mContainsViewScript(FALSE)
    , mContainsSetenv(FALSE)
    , mContainsSetdebug(FALSE)
    , mContainsFly(FALSE)
    , mContainsEdit(FALSE)
    , mContainsRez(FALSE)
    , mContainsShowhovertextall(FALSE)
    , mContainsShowhovertexthud(FALSE)
    , mContainsShowhovertextworld(FALSE)
    , mContainsDefaultwear(FALSE)
    , mContainsPermissive(FALSE)
    , mContainsRun(FALSE)
    , mContainsAlwaysRun(FALSE)
    , mContainsTp(FALSE)
    , mContainsSetsphere(FALSE)
    , mHandleNoStrip(TRUE)
    , mContainsCamTextures(FALSE)
    , mUserUpdateAttachmentsFirstCall(TRUE)
    , mUserUpdateAttachmentsCalledManually(FALSE)
    , mCamDistDrawFromJoint(NULL)
    , mGarbageCollectorCalledOnce(FALSE)
    , mVisionRestricted(FALSE)
    , mSitGroundOnStandUp(FALSE)
    , mSetsphereDistMax(EXTREMUM) // KKA-835
    , mSetsphereDistMin(EXTREMUM) // KKA-835
    , mLeastDistMaxSquared(EXTREMUM) // KKA-835
    , mSetsphereValueMax(0.0) // KKA-835
    //, mContainsMoveUp(FALSE)
    //, mContainsMoveDown(FALSE)
    //, mContainsMoveForward(FALSE)
    //, mContainsMoveBackward(FALSE)
    //, mContainsMoveTurnUp(FALSE)
    //, mContainsMoveTurnDown(FALSE)
    //, mContainsMoveTurnLeft(FALSE)
    //, mContainsMoveTurnRight(FALSE)
    //, mContainsMoveStrafeLeft(FALSE)
    //, mContainsMoveStrafeRight(FALSE)
    , mLaunchTimestamp(LLDate::now().secondsSinceEpoch())
    , mCamTexturesCustom(LLViewerFetchedTexture::sDefaultImagep)
{
    mAllowedGetDebug.clear();
    mAllowedGetDebug.push_back("AvatarSex"); // 0 female, 1 male
    mAllowedGetDebug.push_back("RenderResolutionDivisor"); // simulate blur, default is 1
    mAllowedGetDebug.push_back("RestrainedLoveForbidGiveToRLV");
    mAllowedGetDebug.push_back("RestrainedLoveNoSetEnv");
    mAllowedGetDebug.push_back("WindLightUseAtmosShaders");

    mAllowedSetDebug.clear();
    mAllowedSetDebug.push_back("AvatarSex"); // 0 female, 1 male
    mAllowedSetDebug.push_back("RenderResolutionDivisor"); // simulate blur, default is 1

    mParcelName = "";

    mAssetsToReattach.clear();

    mJustDetached.uuid.setNull();
    mJustDetached.attachpt = "";
    mJustReattached.uuid.setNull();
    mJustReattached.attachpt = "";

    mLastObjectSatOn = LLUUID::null;

    sLastOutfitChange = -1000.f;
    updateCameraLimits();
    updateLimits();

    // Calling gSavedSettings here crashes the viewer when compiled with VS2005.
    // OK under Linux. Moved this initialization to llstartup.cpp as a consequence.
    // sRestrainedLoveDebug = gSavedSettings.getBOOL("RestrainedLoveDebug");
    // sRestrainedLoveLogging = gSavedSettings.getBOOL("RestrainedLoveLogging");
}

RRInterface::~RRInterface()
{
}

std::string RRInterface::getVersion ()
{
    return RR_VIEWER_NAME " viewer v" RR_VERSION " (" + LLVersionInfo::instance().getShortVersion() + ")";
}

std::string RRInterface::getVersion2 ()
{
    return RR_VIEWER_NAME_NEW " viewer v" RR_VERSION " (" + LLVersionInfo::instance().getShortVersion() + ")";
}

std::string RRInterface::getVersionNum ()
{
    std::string res = RR_VERSION_NUM;
    if (sBlacklist != "") {
        res += ","+sBlacklist;
    }
    return res;
}

std::string RRInterface::getFirstName (std::string fullName)
{
    int ind = fullName.find (" ");
    if (ind != -1) return fullName.substr (0, ind);
    ind = fullName.find (".");
    if (ind != -1) return fullName.substr (0, ind);
    else return fullName;
}

std::string RRInterface::getLastName (std::string fullName)
{
    int ind = fullName.find (" ");
    if (ind != -1) return fullName.substr (ind+1);
    ind = fullName.find (".");
    if (ind != -1) return fullName.substr (ind+1);
    else return fullName;
}

BOOL RRInterface::isAllowed (LLUUID object_uuid, std::string action, BOOL log_it)
{
    BOOL debug = sRestrainedLoveLogging && log_it;
    if (debug) {
        LL_INFOS() << object_uuid.asString() << "      " << action << LL_ENDL;
    }
    RRMAP::iterator it = mSpecialObjectBehaviours.find (object_uuid.asString());
    while (it != mSpecialObjectBehaviours.end() &&
            it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
    {
        if (debug) {
            LL_INFOS() << "  checking " << it->second << LL_ENDL;
        }
        if (it->second == action) {
            if (debug) {
                LL_INFOS() << "  => forbidden. " << LL_ENDL;
            }
            return FALSE;
        }
        it++;
    }
    if (debug) {
        LL_INFOS() << "  => allowed. " << LL_ENDL;
    }
    return TRUE;
}

BOOL RRInterface::contains (std::string action)
{
    RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
    LLStringUtil::toLower(action);
//  LL_INFOS() << "looking for " << action << LL_ENDL;
    while (it != mSpecialObjectBehaviours.end()) {
        if (it->second == action) {
//          LL_INFOS() << "found " << it->second << LL_ENDL;
            return TRUE;
        }
        it++;
    }
    return FALSE;
}

BOOL RRInterface::containsSubstr (std::string action)
{
    RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
    LLStringUtil::toLower(action);
//  LL_INFOS() << "looking for " << action << LL_ENDL;
    while (it != mSpecialObjectBehaviours.end()) {
      // KKA-829 by matching anywhere in the string this would return a false positive for 'shownames' in 'notify:100;shownames'
      // The function is now redefined to require the match to begin at index 0 which matches its usage
        //if (it->second.find (action) != -1) {
        if ( ! it->second.find (action)) {
//          LL_INFOS() << "found " << it->second << LL_ENDL;
            return TRUE;
        }
        it++;
    }
    return FALSE;
}

std::string RRInterface::get(LLUUID object_uuid, std::string action, std::string dflt /*= ""*/)
{
    // action is a single action
    LLStringUtil::toLower(action);
    std::string res = dflt;
    std::string command;
    std::string behav;
    std::string option;
    std::string param;
    LLUUID uuid;
    for (RRMAP::iterator it = mSpecialObjectBehaviours.begin(); it != mSpecialObjectBehaviours.end(); ++it) {
        uuid.set(it->first);
        if (uuid == object_uuid) {
            command = it->second;
            LLStringUtil::toLower(command);
            if (parseCommand(command + "=n", behav, option, param)) {
                if (action == behav) {
                    res = option;
                    break;
                }
            }
        }
    }
    return res;
}

F32 RRInterface::getMax (std::string action, F32 dflt /*= EXTREMUM*/)
{
    // action can be a list of behavs separated by commas
    LLStringUtil::toLower(action);
    action = "," + action + ",";
    F32 res = -EXTREMUM;
    F32 tmp;
    std::string command;
    std::string behav;
    std::string option;
    std::string param;
    BOOL found_one = FALSE;
    for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
        command = it->second;
        LLStringUtil::toLower(command);
        if (parseCommand (command+"=n", behav, option, param)) {
            if (action.find ("," + behav + ",") != -1) {
                tmp = atof (option.c_str());
                if (option == "") {
                    tmp = 1.5;
                }
                if (tmp > res) {
                    res = tmp;
                    found_one = TRUE;
                }
            }
        }
    }
    if (!found_one) {
        return dflt;
    }
    return res;
}

F32 RRInterface::getMin (std::string action, F32 dflt /*= -EXTREMUM*/)
{
    // action can be a list of behavs separated by commas
    LLStringUtil::toLower(action);
    action = "," + action + ",";
    F32 res = EXTREMUM;
    F32 tmp;
    std::string command;
    std::string behav;
    std::string option;
    std::string param;
    BOOL found_one = FALSE;
    for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
        command = it->second;
        LLStringUtil::toLower(command);
        if (parseCommand (command+"=n", behav, option, param)) {
            if (action.find("," + behav + ",") != -1) {
                tmp = atof(option.c_str());
                if (option == "") {
                    tmp = 1.5;
                }
                if (tmp < res) {
                    res = tmp;
                    found_one = TRUE;
                }
            }
        }
    }
    if (!found_one) {
        return dflt;
    }
    return res;
}

LLColor3 RRInterface::getMixedColors (std::string action, LLColor3 dflt /*= LLColor3::black*/)
{
    // action can be a list of behavs separated by commas
    LLStringUtil::toLower(action);
    action = "," + action + ",";
    LLColor3 res = LLColor3::white;
    LLColor3 tmp;
    std::string command;
    std::string behav;
    std::string option;
    std::string param;
    int nb_found = 0;
    //F32 h, s, l;
    //F32 total_h = 0.f, total_s = 0.f, total_l = 0.f;
    std::deque<std::string> tokens;
    for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
        command = it->second;
        LLStringUtil::toLower(command);
        if (parseCommand (command+"=n", behav, option, param)) {
            if (action.find("," + behav + ",") != -1) {
                tokens = parse(option, ";", 3);
                tmp.mV[0] = atof (tokens[0].c_str());
                tmp.mV[1] = atof (tokens[1].c_str());
                tmp.mV[2] = atof (tokens[2].c_str());
                res *= tmp;
                //tmp.calcHSL (&h, &s, &l);
                //total_h += h;
                //total_s += s;
                //total_l += l;
                nb_found++;
            }
        }
    }
    //if (nb_found > 0) {
    //  total_h /= (F32)nb_found;
    //  total_s /= (F32)nb_found;
    //  total_l /= (F32)nb_found;
    //  res.setHSL (total_h, total_s, total_l);
    //}
    if (nb_found == 0)  {
        return dflt;
    }
    return res;
}

BOOL RRInterface::containsWithoutException (std::string action, std::string except /* = "" */)
{
    // action is a restriction like @sendim, which can accept exceptions (@sendim:except_uuid=add)
    // action_sec is the same action, with "_sec" appended (like @sendim_sec)
    
    LLStringUtil::toLower(action);
    std::string action_sec = action + "_sec";
    LLUUID uuid;
    
    // 1. If except is empty, behave like contains(), but looking for both action and action_sec
    if (except == "") {
        return (contains (action) || contains (action_sec));
    }

    // 2. For each action_sec, if we don't find an exception tied to the same object, return TRUE
    // if @permissive is set, then even action needs the exception to be tied to the same object, not just action_sec
    // (@permissive restrains the scope of all the exceptions to their own objects)
    RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
    while (it != mSpecialObjectBehaviours.end()) {
        if (it->second == action_sec 
        || (it->second == action && mContainsPermissive)) {
            uuid.set (it->first);
            if (isAllowed (uuid, action+":"+except, FALSE) && isAllowed (uuid, action_sec+":"+except, FALSE)) { // we use isAllowed because we need to check the object, but it really means "does not contain"
                return TRUE;
            }
        }
        it++;
    }
    
    // 3. If we didn't return yet, but the map contains action, just look for except_uuid without regard to its object, if none is found return TRUE
    if (contains (action)) {
        if (!contains (action+":"+except) && !contains (action_sec+":"+except)) {
            return TRUE;
        }
    }
    
    // 4. Finally return FALSE if we didn't find anything
    return FALSE;
}

bool RRInterface::isFolderLocked(LLInventoryCategory* cat)
{
//  const LLFolderType::EType folder_type = cat->getPreferredType();
//  if (LLFolderType::lookupIsProtectedType(folder_type)) return false;

    bool shared = isUnderRlvShare(cat);
    if (!shared && contains("unsharedwear")) return true;
    if (shared && contains("sharedwear")) return true;
    if (isFolderLockedWithoutException(cat, "attach") != FolderLock_unlocked) return true;
    if (isFolderLockedWithoutException(cat, "detach") != FolderLock_unlocked) return true;
    return false;
}

FolderLock RRInterface::isFolderLockedWithoutException (LLInventoryCategory* cat, std::string attach_or_detach)
{
    if (cat == NULL) return FolderLock_unlocked;

    if (sRestrainedLoveLogging) {
        LL_INFOS() << "isFolderLockedWithoutException(" << cat->getName() << ", " << attach_or_detach << ")" << LL_ENDL;
    }
    // For each object that is locking this folder, check whether it also issues exceptions to this lock
    std::deque<std::string> commands_list;
    std::string command;
    std::string behav;
    std::string option;
    std::string param;
    std::string this_command;
    std::string this_behav;
    std::string this_option;
    std::string this_param;
    bool this_object_locks;
    FolderLock current_lock = FolderLock_unlocked;
    for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
        LLUUID uuid = LLUUID(it->first);
        command = it->second;
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "command = " << command << LL_ENDL;
        }
        // param will always be equal to "n" in this case since we added it to command, but we don't care about this here
        // Attention, an option must absolutely be specified here (there must be a ":" character), or we wouldn't be able to tell "detachthis" from "detachthis:"
        // and both have different meanings
        if (command.find (":") != -1 && parseCommand (command+"=n", behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
        {
            // find whether this object has issued a "{attach|detach}[all]this" command on a folder that is either this one, or a parent
            this_object_locks = false;
            if (behav == attach_or_detach+"this") {
                if (getCategoryUnderRlvShare(option) == cat) {
                    this_object_locks = true;
                }
            }
            else if (behav == attach_or_detach+"allthis") {
                if (isUnderFolder(getCategoryUnderRlvShare(option), cat)) {
                    this_object_locks = true;
                }
            }

            // This object has issued such a command, check whether it has issued an exception to it as well
            if (this_object_locks) {
                commands_list = getListOfRestrictions(uuid);
                FolderLock this_lock = isFolderLockedWithoutExceptionAux(cat, attach_or_detach, commands_list);
                if (this_lock == FolderLock_locked_without_except) return FolderLock_locked_without_except;
                else current_lock = this_lock;
                if (sRestrainedLoveLogging) {
                    LL_INFOS() << "this_lock=" << this_lock << LL_ENDL;
                }
            }
        }
    }

    // Finally, return unlocked since we didn't find any lock on this folder
    return current_lock;
}

FolderLock RRInterface::isFolderLockedWithoutExceptionAux (LLInventoryCategory* cat, std::string attach_or_detach, std::deque<std::string> list_of_restrictions)
{
    // list_of_restrictions contains the list of restrictions issued by one particular object, at least one is supposed to be a "{attach|detach}[all]this"
    // For each folder from cat up to the root folder, check :
    // - if we are on cat and we find "{attach|detach}this_except", there is an exception, keep looking up
    // - if we are on cat and we find "{attach|detach}this", there is no exception, return FolderLock_locked_without_except
    // - if we are on a parent and we find "{attach|detach}allthis_except", there is an exception, keep looking up
    // - if we are on a parent and we find "{attach|detach}allthis", if we found an exception return FolderLock_locked_with_except, else return FolderLock_locked_without_except
    // - finally, if we are on the root, return FolderLocked_unlocked (whether there was an exception or not)
    if (cat == NULL) {
        return FolderLock_unlocked;
    }

    if (sRestrainedLoveLogging) {
        LL_INFOS() << "isFolderLockedWithoutExceptionAux(" << cat->getName() << ", " << attach_or_detach << ", [" << dumpList2String(list_of_restrictions, ",") << "])" << LL_ENDL;
    }

    FolderLock current_lock = FolderLock_unlocked;
    std::string command;
    std::string behav;
    std::string option;
    std::string param;

    LLInventoryCategory* it = NULL;
    LLInventoryCategory* cat_option = NULL;
    LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
    
    const LLUUID& cat_id = cat->getUUID();
    it = gInventory.getCategory (cat_id);
    
    do {
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "it=" << it->getName() << LL_ENDL;
        }

        for (unsigned int i = 0; i < list_of_restrictions.size(); ++i)
        {
            command = list_of_restrictions[i];
            if (sRestrainedLoveLogging) {
                LL_INFOS() << "command2=" << command << LL_ENDL;
            }

            // param will always be equal to "n" in this case since we added it to command, but we don't care about this here
            if (parseCommand (command+"=n", behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
            {
                cat_option = getCategoryUnderRlvShare(option);
                if (cat_option == it) {
                    if (it == cat) {
                        if (behav == attach_or_detach+"this_except" || behav == attach_or_detach+"allthis_except") {
                            current_lock = FolderLock_locked_with_except;
                        }
                        else if (behav == attach_or_detach+"this"|| behav == attach_or_detach+"allthis") {
                            return FolderLock_locked_without_except;
                        }
                    }
                    else {
                        if (behav == attach_or_detach+"allthis_except") {
                            current_lock = FolderLock_locked_with_except;
                        }
                        else if (behav == attach_or_detach+"allthis") {
                            if (current_lock == FolderLock_locked_with_except) return FolderLock_locked_with_except;
                            else return FolderLock_locked_without_except;
                        }
                    }
                }
            }
        }

        const LLUUID& parent_id = it->getParentUUID();
        it = gInventory.getCategory (parent_id);
    } while (it && it->getUUID() != root_id);
    return FolderLock_unlocked; // this should never happen since list_of_commands is supposed to contain at least one "{attach|detach}[all]this" restriction
}

BOOL RRInterface::add (LLUUID object_uuid, std::string action, std::string option)
{
    if (sRestrainedLoveLogging || sRestrainedLoveCommandLogging) {
        LL_INFOS("RLV") << object_uuid.asString() << " " << action << " " << option << LL_ENDL;
    }
    
    std::string canon_action = action;
    if (option!="") action+=":"+option;
    
    if (isAllowed (object_uuid, action)) {
        // Notify if needed
        notify (object_uuid, action, "=n");

        // If this action is blacklisted, do nothing
        if (canon_action != "notify" && isBlacklisted (canon_action, false)) {
            return TRUE;
        }
        
        // Actions to do BEFORE inserting the new behav
        if (action=="fly") {
            gAgent.setFlying (FALSE);
        }
        else if (action=="edit") {
            LLPipeline::setRenderBeacons(FALSE);
            LLPipeline::setRenderScriptedBeacons(FALSE);
            LLPipeline::setRenderScriptedTouchBeacons(FALSE);
            LLPipeline::setRenderPhysicalBeacons(FALSE);
            LLPipeline::setRenderSoundBeacons(FALSE);
            LLPipeline::setRenderParticleBeacons(FALSE);
            LLPipeline::setRenderHighlights(FALSE);
            LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
        }
        else if (action=="setenv") {
            if (sRRNoSetEnv) {
                return TRUE;
            }
            // CA update this for EEP floaters
            gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
            gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
            LLFloaterReg::hideInstance("env_post_process");
            LLFloaterReg::hideInstance("env_fixed_environment_water");
            LLFloaterReg::hideInstance("env_fixed_environment_sky");
            LLFloaterReg::hideInstance("env_edit_ext_daycycle");
            LLFloaterReg::hideInstance("my_environments");
            LLFloaterReg::hideInstance("env_adjust_snapshot");
        }
        else if (action=="setdebug") {
            if (!sRRNoSetEnv) {
                gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
                gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
            }
        }

        // Insert the new behav
        mSpecialObjectBehaviours.insert(std::pair<std::string, std::string>(object_uuid.asString(), action));
        refreshCachedVariable(action);

        // Actions to do AFTER inserting the new behav
        // KKA-635 refresh nametags and also handle shownames & shownametags including exceptions
        if (action=="showhovertextall" || action=="showloc" || canon_action=="shownames" || canon_action=="shownametags"
            || action=="showhovertexthud" || action=="showhovertextworld" || action == "shownearby" ) {
            updateAllHudTexts();
            LLVOAvatar::invalidateNameTags();
        }
        else if (canon_action == "showhovertext") {
            updateOneHudText(LLUUID(option));
        }
        else if (canon_action.find("cam") == 0 || canon_action.find("setcam") == 0) {
            updateCameraLimits ();
            // Force an update of the zoom if necessary
            if (canon_action == "camzoommax" || canon_action == "camzoommin" || canon_action == "setcam_fovmin" || canon_action == "setcam_fovmax") {
                LLViewerCamera::getInstance()->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
                gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
            }
        }
        else if (canon_action.find("setsphere") == 0) {
            //KKA-831 RenderFSAASamples (Anti-aliasing) needs to be non-zero
            if (!gSavedSettings.getU32("RenderFSAASamples"))
            {
                gSavedSettings.setU32("RenderFSAASamples", 2);
            }
            gSavedSettings.setBOOL("RenderObjectBump", TRUE); // make sure to render "Bump Mapping and Shiny"
            gSavedSettings.setBOOL("RenderTransparentWater", TRUE); // make sure to render "Transparent Water"
            gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE); // make sure the atmospheric shaders are turned on
            gSavedSettings.setBOOL("RenderDeferred", TRUE); // make sure Advanced Lighting Model is on
            gSavedSettings.setBOOL("RenderDepthOfField", FALSE); // make sure DoF is off otherwise we can see through the sphere by looking through alpha-blended rigged surfaces
          //KKA-882 Advanced graphics floater forms additional tabs in Preferences/Graphics
            LLFloaterPreference* preferences = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
            if (preferences) {
                preferences->refreshEnabledState();
                preferences->refreshEnabledStateAdvanced();
            }
            //LLFloaterPreferenceGraphicsAdvanced* floater_graphics_advanced = LLFloaterReg::findTypedInstance<LLFloaterPreferenceGraphicsAdvanced>("prefs_graphics_advanced");
            //if (floater_graphics_advanced) {
            //  floater_graphics_advanced->refreshEnabledState();
            //}
            updateSetsphere();
        }
        else if (canon_action == "fartouch"
            || canon_action == "touchfar"
            || canon_action == "sittp"
            || canon_action == "tplocal"
            ) {
            updateLimits();
        }

        // Check if we can see wireframe or not, deactivate if our vision is restricted (we have locked HUDs or mCamDistDrawMax is not infinite)
        // KKA-831 setsphere should inhibit wireframe too
        if (gAgent.mRRInterface.hasLockedHuds() || gAgent.mRRInterface.mCamDistDrawMax < EXTREMUM || gAgent.mRRInterface.mContainsSetsphere)
        {
            if (gUseWireframe)
            {
                gUseWireframe = FALSE;
                // Copied from LLAdvancedToggleWireframe::handle_event() in llviewermenu.cpp
                gWindowResized = TRUE;
                LLPipeline::updateRenderDeferred();
                gPipeline.resetVertexBuffers();

                if (!gUseWireframe && !gInitialDeferredModeForWireframe && LLPipeline::sRenderDeferred != ((bool)gInitialDeferredModeForWireframe && gPipeline.isInit()))
                {
                    LLPipeline::refreshCachedSettings();
                    gPipeline.releaseGLBuffers();
                    gPipeline.createGLBuffers();
                    LLViewerShaderMgr::instance()->setShaders();
                }
            }
        }

        // Update the stored last standing location, to allow grabbers to transport a victim inside a cage while sitting, and restrict them
        // before standing up. If we didn't do this, the avatar would snap back to a safe location when being unsitted by the grabber,
        // which would be rather silly.
        if (action == "standtp") {
            gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal ());
            gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
        }
        
        //and feed it into the RLV status/worn floaters
        std::string notify = action + "=n";
        KokuaRLVFloaterSupport::commandNotify(object_uuid, notify);
        // KKA-915 fire off RLVa style callback too
        // KKA-928 change the firing to be after all other RLV state has updated
        m_OnBehaviour(action,true);
        return TRUE;
    }
    return FALSE;
}

BOOL RRInterface::remove (LLUUID object_uuid, std::string action, std::string option)
{
    if (sRestrainedLoveLogging || sRestrainedLoveCommandLogging) {
        LL_INFOS("RLV") << object_uuid.asString() << " " << action << " " << option << LL_ENDL;
    }

    std::string canon_action = action;
    if (option!="") action+=":"+option;

    BOOL must_remove_setsphere_params = FALSE; // if true, run the loop a second time afterwards to remove all the @setsphere_xxx:yyy commands associated with the same UUID
    BOOL must_update_setsphere = FALSE; // if true, call updateSetSphere() and update the preferences windows afterwards
    BOOL removed_behav = FALSE;
    
    // Notify if needed
    notify (object_uuid, action, "=y");
    
    // Actions to do BEFORE removing the behav

    // Remove the behav
    RRMAP::iterator it = mSpecialObjectBehaviours.find (object_uuid.asString());
    while (it != mSpecialObjectBehaviours.end() && it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString())) {
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "  checking " << it->second << LL_ENDL;
        }
        if (it->second == action) {
            mSpecialObjectBehaviours.erase(it);
            if (sRestrainedLoveLogging) {
                LL_INFOS() << "  => removed. " << LL_ENDL;
            }
            refreshCachedVariable(action);

            // Actions to do AFTER removing the behav
            // KKA-635 refresh nametags and also handle shownames & shownametags including exceptions
            if (action=="showhovertextall" || action=="showloc" || canon_action=="shownames" || canon_action=="shownametags"
                || action=="showhovertexthud" || action=="showhovertextworld" || action == "shownearby") {
                updateAllHudTexts();
                LLVOAvatar::invalidateNameTags();
            }
            if (canon_action == "showhovertext") {
                updateOneHudText(LLUUID(option));
            }
            else if (canon_action.find("cam") == 0 || canon_action.find("setcam") == 0) {
                updateCameraLimits ();
            }
            else if (canon_action.find ("setsphere") == 0) {
                must_update_setsphere = true; // to make sure we call this only once after we've taken care of setsphere
                // If we are removing @setsphere proper (not a param), then we must remove all the @setsphere_xxx parameters associated with this UUID.
                if (sRestrainedLoveLogging) {
                    LL_INFOS() << "canon_action: " << canon_action << LL_ENDL;
                }
                if (canon_action == "setsphere") {
                    must_remove_setsphere_params = true;
                }
            }
            else if (canon_action == "fartouch"
                || canon_action == "touchfar"
                || canon_action == "sittp"
                || canon_action == "tplocal"
                ) {
                updateLimits();
            }
            else if (action == "standtp" && gAgentAvatarp && !gAgentAvatarp->isSitting()) { // If we are not sitting, then we can remove the @standtp restriction normally
                gAgent.mRRInterface.mLastStandingLocation.clear();
                gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
            }
            //and feed it into the RLV status/worn floaters
            std::string notify = action + "=y";
            KokuaRLVFloaterSupport::commandNotify(object_uuid, notify);

            removed_behav = TRUE;
            break;
        }
        it++;
    }

    if (sRestrainedLoveLogging) {
        LL_INFOS() << "must_update_setsphere=" << must_update_setsphere << ", must_remove_setsphere_params=" << must_remove_setsphere_params << LL_ENDL;
    }

    // If we removed @setsphere, remove all the parameters associated with the same UUID as well.
    if (must_remove_setsphere_params) {
        BOOL found_one = TRUE;
        std::string str_object_uuid = object_uuid.asString();
        while (found_one) {
            found_one = FALSE;
            it = mSpecialObjectBehaviours.begin();
            while (it != mSpecialObjectBehaviours.end()) {
                // If we have found a @setsphere_xxx command associated with this UUID, remove it.
                if (it->first == str_object_uuid && it->second.find("setsphere_") == 0) {
                    // Notify if needed
                    notify(object_uuid, it->second, "=y");

                    // Remove this setsphere_xxx command.
                    std::string tmp = it->second;
                    mSpecialObjectBehaviours.erase(it);
                    if (sRestrainedLoveLogging) {
                        LL_INFOS() << "removing outstanding setsphere param: " << tmp << LL_ENDL;
                    }
                    refreshCachedVariable(tmp);
                    found_one = TRUE;
                    break; // there may be more to remove => return to the beginning of the map and keep searching
                }
                it++;
            }
        }
    }

    if (must_update_setsphere) {
        updateSetsphere();
        LLFloaterPreference* preferences = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
        //KKA-882 Advanced graphics floater forms additional tabs in Preferences/Graphics
        if (preferences) {
            preferences->refreshEnabledState();
            preferences->refreshEnabledStateAdvanced();
        }
        //LLFloaterPreferenceGraphicsAdvanced* floater_graphics_advanced = LLFloaterReg::findTypedInstance<LLFloaterPreferenceGraphicsAdvanced>("prefs_graphics_advanced");
        //if (floater_graphics_advanced) {
        //  floater_graphics_advanced->refreshEnabledState();
        //}
    }
    // KKA-915 fire off RLVa style callback too
    // KKA-928 change the firing to be after all other RLV state has updated
    m_OnBehaviour(action,false);

    return removed_behav;
}

BOOL RRInterface::clear (LLUUID object_uuid, std::string command)
{
    if (sRestrainedLoveLogging || sRestrainedLoveCommandLogging) {
        LL_INFOS("RLV") << object_uuid.asString() << " " << command << LL_ENDL;
    }

    // Notify if needed
    notify (object_uuid, "clear" + (command!=""? ":"+command : ""), "");
    
    RRMAP::iterator it;
    it = mSpecialObjectBehaviours.begin ();
    while (it != mSpecialObjectBehaviours.end()) {
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "  checking " << it->second << LL_ENDL;
        }
        if (it->first==object_uuid.asString() && (command=="" || it->second.find (command)!=-1)) {
            notify (object_uuid, it->second, "=y");
            if (sRestrainedLoveLogging) {
                LL_INFOS() << it->second << " => removed. " << LL_ENDL;
            }
            std::string tmp = it->second;
            mSpecialObjectBehaviours.erase(it);
            refreshCachedVariable(tmp);
            it = mSpecialObjectBehaviours.begin ();
            // KKA-915 fire off RLVa style callback too
            // KKA-928 change the firing to be after all other RLV state has updated
        m_OnBehaviour(tmp,false);
        }
        else {
            it++;
        }
    }
    updateAllHudTexts();
    updateCameraLimits();

    updateSetsphere();
    //KKA-882 Advanced graphics floater forms additional tabs in Preferences/Graphics
    LLFloaterPreference* preferences = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
    if (preferences) {
        preferences->refreshEnabledState();
        preferences->refreshEnabledStateAdvanced();
    }
    //LLFloaterPreferenceGraphicsAdvanced* floater_graphics_advanced = LLFloaterReg::findTypedInstance<LLFloaterPreferenceGraphicsAdvanced>("prefs_graphics_advanced");
    //if (floater_graphics_advanced) {
    //  floater_graphics_advanced->refreshEnabledState();
    //}

    updateLimits();
    if (gAgentAvatarp && !gAgentAvatarp->isSitting()) { // If we are not sitting, then we can remove the @standtp restriction normally
        gAgent.mRRInterface.mLastStandingLocation.clear();
        gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
    }
    //and feed it into the RLV status/worn floaters
    std::string notify = "clear" + (command!=""? ":"+command : "");
    KokuaRLVFloaterSupport::commandNotify(object_uuid,notify);

    return TRUE;
}

void RRInterface::replace (LLUUID what, LLUUID by)
{
    RRMAP::iterator it;
    LLUUID uuid;
    it = mSpecialObjectBehaviours.begin ();
    while (it != mSpecialObjectBehaviours.end()) {
        uuid.set (it->first);
        if (uuid == what) {
            // found the UUID to replace => add a copy of the command with the new UUID
            mSpecialObjectBehaviours.insert(std::pair<std::string, std::string>(by.asString(), it->second));
            //and feed it into the RLV status/worn floaters
            std::string notify = it->second + "=n";
            KokuaRLVFloaterSupport::commandNotify(by,notify);               
        }
        it++;
    }
    // and then clear the old UUID
    clear (what, "");
}


BOOL RRInterface::garbageCollector (BOOL all) {
    RRMAP::iterator it;
    BOOL res=FALSE;
    LLUUID uuid;
    LLViewerObject *objp=NULL;
    it = mSpecialObjectBehaviours.begin ();
    while (it != mSpecialObjectBehaviours.end()) {
        uuid.set (it->first);
        if (all || !uuid.isNull ()) {
//          if (sRestrainedLoveLogging) {
//              LL_INFOS() << "testing " << it->first << LL_ENDL;
//          }
            objp = gObjectList.findObject(uuid);
            if (!objp) {
                if (sRestrainedLoveLogging || sRestrainedLoveCommandLogging) {
                    LL_INFOS("RLV") << it->first << " not found => cleaning... " << LL_ENDL;
                }
                clear(uuid);
                {
                    LLColor4 color = LLUIColorTable::instance().getColor("ScriptErrorColor");
                    std::string released = "Restrictions released by garbage collector";
                    KokuaFloaterRLVDebug::addRLVLine(released, color, uuid);
                }
                res=TRUE;
                it=mSpecialObjectBehaviours.begin ();
            } else {
                it++;
            }
        } else {
            if (sRestrainedLoveLogging) {
                LL_INFOS() << "ignoring " << it->second << LL_ENDL;
            }
            it++;
        }
    }
    // KKA-682 There's a fair chance that handleLoginComplete gets fired before the initial restrictions
    // get lifted, which means that the login location doesn't get stored in teleport history since showloc
    // is active as part of the login restrictions
    if (!mGarbageCollectorCalledOnce)
    {
        mGarbageCollectorCalledOnce = TRUE;
        LLTeleportHistory::getInstance()->handleLoginComplete();
    }
    return res;
}

std::deque<std::string> RRInterface::parse (std::string str, std::string sep, int size_min /*= 0*/)
{
    int ind;
    int length = sep.length();
    std::string token;
    std::deque<std::string> res;
    
    do {
        ind=str.find(sep);
        if (ind!=-1) {
            token = str.substr (0, ind);
            if (token != "") {
                res.push_back (token);
            }
            str=str.substr (ind+length);
        }
        else {
            if (str != "") {
                res.push_back (str);
            }
        }
    } while (ind!=-1);
    
    while (res.size() < size_min) {
        res.push_back("");
    }

    return res;
}


void RRInterface::notify (LLUUID object_uuid, std::string action, std::string suffix)
{
    // scan the list of restrictions, when finding "notify" say the restriction on the specified channel
    RRMAP::iterator it;
    int length = 7; // size of "notify:"
    int size;
    std::deque<std::string> tokens;
    LLUUID uuid;
    std::string rule;
    it = mSpecialObjectBehaviours.begin ();

    while (it != mSpecialObjectBehaviours.end()) {
        uuid.set (it->first);
        rule = it->second; // we are looking for rules like "notify:2222;tp", if action contains "tp" then notify the scripts on channel 2222
        if (rule.find("notify:") == 0) {
            // found a possible notification to send
            rule = rule.substr(length); // keep right part only (here "2222;tp")
            tokens = parse (rule, ";");
            size = tokens.size();
            if (size == 1 || (size > 1 && action.find(tokens[1]) != -1)) {
                answerOnChat(tokens[0], "/" + action + suffix); // suffix can be "=n", "=y" or whatever else we want, "/" is needed to avoid some clever griefing
            }
        }
        it++;
    }
    // CA: also use this to trigger an update of the Worn Status floater for situations where an outfit/attach change wasn't
    // caused by a RLV command. All the messages of interest contain 'legally'. However, this is not fully reliable,
    // eg in the case of a clothing layer being detached the notification is called *before* the detach occurs
    if (suffix == "" && action.find("legally") != -1 && KokuaFloaterRLVWorn::getBase())
    {
        KokuaFloaterRLVWorn::getBase()->refreshWornStatus();
    }
}


BOOL RRInterface::parseCommand (std::string command, std::string& behaviour, std::string& option, std::string& param)
{
    int ind = command.find("=");
    behaviour=command;
    option="";
    param="";
    if (ind!=-1) {
        behaviour=command.substr(0, ind);
        param=command.substr(ind+1);
        ind=behaviour.find(":");
        if (ind!=-1) {
            option=behaviour.substr(ind+1);
            behaviour=behaviour.substr(0, ind); // keep in this order (option first, then behav) or crash
        }
        return TRUE;
    }
    return FALSE;
}

// CA: In order to have a single point of execution for commands with a clean result returned I am converting
// the real handleCommand into a private and creating a wrapper which then gives me a clean way to
// add debugging functions. The existing debug output function is in llviewermessage.cpp thus only applies
// to the calls to handleCommand that it makes, so any commands from other sources such as the blind login
// would not be recorded. The original handleCommand has multiple exits so veneering it is the easiest way.

BOOL RRInterface::handleCommand (LLUUID uuid, std::string command)
{
    // yes - this is messy ... the alternative is changing the prototype of answerOnChat everywhere
    // it occurs which is worse
    current_handlecommand_caller = uuid;
    
    BOOL res = reallyHandleCommand(uuid, command);
    LLColor4 color;

    // if all that reallyHandleCommand did was to queue it then
    // we don't want to pipe it through to debugging yet either - we'll catch
    // when it comes past again from fireCommands

    if (LLStartUp::getStartupState() < STATE_CLEANUP) return res;

    if (res)
    {
        color = LLUIColorTable::instance().getColor("SystemChatColor");
        KokuaFloaterRLVDebug::addRLVLine("Executes: "+command,color,uuid);
        
        // we no longer do this here. since add/remove/clear/replace are public and could get
        // called from anywhere the notifications have to be down in those routines rather 
        // than up here
        //  
        //and feed it into the RLV status/worn floaters
        //KokuaRLVFloaterSupport::commandNotify(uuid, command);
    }
    else
    {
        // the colours get overridden because the text window used is set as readonly but this is
        // left in just in case a different display method is used in future
        color = LLUIColorTable::instance().getColor("ScriptErrorColor");
        KokuaFloaterRLVDebug::addRLVLine("Fails: " + command, color, uuid);
    }
    
    return res;
}

BOOL RRInterface::reallyHandleCommand (LLUUID uuid, std::string command)
{
    // 1. check the command is actually a single one or a list of commands separated by ","
    if (command.find (",")!=-1) {
        BOOL res=TRUE;
        std::deque<std::string> list_of_commands=parse (command, ",");
        for (unsigned int i=0; i<list_of_commands.size (); ++i) {
            if (!reallyHandleCommand (uuid, list_of_commands.at(i))) res=FALSE;
        }
        return res;
    }
    
    // 2. this is a single command, possibly inside a 1-level recursive call (unimportant)
    // if the viewer is not fully initialized and the user does not have control of their avatar,
    // don't execute the command but retain it for later, when it is fully initialized
    // If there is another object still waiting to be automatically reattached, retain all RLV commands
    // as well to avoid an infinite loop if the one it will kick off is also locked.
    // This is valid as the object that would possibly be kicked off by the one to reattach, will have
    // its restrictions wiped out by the garbage collector
    if (LLStartUp::getStartupState() < STATE_CLEANUP
        || (!mAssetsToReattach.empty() && !mReattachTimeout)) {
        Command cmd;
        cmd.uuid=uuid;
        cmd.command=command;
        if (sRestrainedLoveLogging || sRestrainedLoveCommandLogging) {
            LLInventoryItem* item = getItem(uuid);
            std::string name = "(unknown)";
            if (item)   {
                name = item->getName();
            }
            LL_INFOS("RLV") << "Retaining " << name << " uuid " << uuid.asString() <<" : " << command << LL_ENDL;
        }
        mRetainedCommands.push_back (cmd);

        return TRUE;
    }
    else if (sRestrainedLoveCommandLogging) {
        LLInventoryItem* item = getItem(uuid);
        std::string name = "(unknown)";
        if (item)   {
            name = item->getName();
        }
        LL_INFOS("RLV") << "Processing " << name << " uuid " << uuid.asString() <<" : " << command << LL_ENDL;      
    }   
    // 3. parse the command, which is of one of these forms :
    // behav=param
    // behav:option=param
    std::string behav;
    std::string option;
    std::string param;
    LLStringUtil::toLower(command);
    if (parseCommand (command, behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
    {
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "[" << uuid.asString() << "]  [" << behav << "]  [" << option << "] [" << param << "]" << LL_ENDL;
        }
        if (behav=="version") return answerOnChat (param, getVersion ());
        else if (behav=="versionnew") return answerOnChat (param, getVersion2 ());
        else if (behav=="versionnum") return answerOnChat (param, RR_VERSION_NUM);
        else if (behav=="versionnumbl") return answerOnChat (param, getVersionNum());
        else if (behav=="getblacklist") return answerOnChat (param, dumpList2String (getBlacklist(option), ","));
        else if (behav=="getoutfit") return answerOnChat (param, getOutfit (option));
        else if (behav=="getattach") return answerOnChat (param, getAttachments (option));
        else if (behav=="getstatus") return answerOnChat (param, getStatus (uuid, option));
        else if (behav=="getstatusall") {
            uuid.setNull();
            return answerOnChat (param, getStatus (uuid, option));
        }
        else if (behav=="getinv") return answerOnChat (param, getInventoryList (option));
        else if (behav=="getinvworn") return answerOnChat (param, getInventoryList (option, TRUE));
        else if (behav=="getsitid") return answerOnChat (param, getSitTargetId ().asString());
        else if (behav=="getpath") return answerOnChat (param, getFullPath (getItem(uuid), option, false)); // option can be empty (=> find path to object) or the name of an attach pt or the name of a clothing layer
        else if (behav=="getpathnew") return answerOnChat (param, getFullPath (getItem(uuid), option)); // option can be empty (=> find path to object) or the name of an attach pt or the name of a clothing layer
        else if (behav == "findfolder") return answerOnChat(param, getFullPath(findCategoryUnderRlvShare(option)));
        else if (behav == "findfolders") {
            // KKA-865 crash fix - cope gracefully with zero options being provided
            std::string response = "";
            std::deque<std::string> options = parse(option, ";");
            if (options.size())
            {
                std::string folder_to_find = options.at(0);
                std::string separator = (options.size() > 1) ? options.at(1) : ",";
                std::deque<LLInventoryCategory*> cats = findCategoriesUnderRlvShare(folder_to_find);
                for (int i = 0; i < cats.size(); i++) {
                    if (i > 0) response += separator;
                    response += getFullPath(cats.at(i));
                }
            }
            return answerOnChat(param, response);
        }
        else if (behav.find ("getenv_") == 0) return answerOnChat (param, getEnvironment (behav));
        else if (behav.find ("getdebug_") == 0) return answerOnChat (param, getDebugSetting (behav));
        else if (behav=="getgroup") {
            std::string group_name = gAgent.getGroupName();
            if (gAgent.getGroupID() == LLUUID::null) group_name = "none"; // "none" is not localized here because a script should not have to bother about viewer language
            return answerOnChat (param, group_name);
        }
        else if (behav=="getcam_avdistmin") {
            std::stringstream str;
            str << std::fixed << mCamDistMin;
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_avdistmax") {
            std::stringstream str;
            str << std::fixed << mCamDistMax;
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_zoommin") {
            std::stringstream str;
            str << std::fixed << mCamZoomMin;
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_zoommax") {
            std::stringstream str;
            str << std::fixed << mCamZoomMax;
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_fovmin") {
            std::stringstream str;
            str << std::fixed << DEFAULT_FIELD_OF_VIEW / mCamZoomMax;
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_fovmax") {
            std::stringstream str;
            str << std::fixed << DEFAULT_FIELD_OF_VIEW / mCamZoomMin;
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_zoom") {
            std::stringstream str;
            str << std::fixed << DEFAULT_FIELD_OF_VIEW / LLViewerCamera::getInstance()->getView();
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_fov") {
            std::stringstream str;
            str << std::fixed << LLViewerCamera::getInstance()->getView();
            return answerOnChat(param, str.str());
        }
        else if (behav == "getcam_textures") {
            return answerOnChat(param, mCamTexturesCustom->getID().asString());
        }

        else {
            if (param=="n" || param=="add") add (uuid, behav, option);
            else if (param=="y" || param=="rem") remove (uuid, behav, option);
            else if (behav=="clear") clear (uuid, param);
            else if (param=="force") force (uuid, behav, option);
            else return FALSE;
        }
    }
    else // clear
    {
        if (sRestrainedLoveLogging) {
            LL_INFOS() << uuid.asString() << "       " << behav << LL_ENDL;
        }
        if (behav=="clear") clear (uuid);
        else return FALSE;
    }
    return TRUE;
}

BOOL RRInterface::fireCommands ()
{
    BOOL ok=TRUE;
    if (mRetainedCommands.size ()) {
        if (sRestrainedLoveLogging || sRestrainedLoveCommandLogging) {
            LL_INFOS("RLV") << "Firing commands : " << mRetainedCommands.size () << LL_ENDL;
        }
        Command cmd;
        while (!mRetainedCommands.empty ()) {
            cmd=mRetainedCommands[0];
            ok=ok & handleCommand (cmd.uuid, cmd.command);
            mRetainedCommands.pop_front ();
        }
    }
    return ok;
}

static void force_sit(LLUUID object_uuid)
{
    // Note : Although it would make sense that only the UUID should be needed, we actually need to send an
    // offset to the sim, therefore we need the object to be known to the viewer. In other words, issuing @sit=force
    // right after a teleport is not going to work because the object will not have had time to rez.
    LLViewerObject *object = gObjectList.findObject(object_uuid);
    if (object) {
        LLVOAvatar* avatar = gAgentAvatarp;
        if (avatar && gAgent.mRRInterface.mContainsUnsit && avatar->mIsSitting) {
            // Do not allow a script to force the avatar to sit somewhere if already forced to stay sitting here
            return;
        }

        // @interact shouldn't prevent force-sit traps etc from working, so disable this cheeck
        //if (gAgent.mRRInterface.mContainsInteract)
        //{
        //  return;
        //}

        if (gAgent.mRRInterface.contains("sit"))
        {
            return;
        }

        if (gAgentAvatarp && !gAgentAvatarp->mIsSitting)
        {
            // We are now standing, and we want to sit down => store our current location so that we can snap back here when we stand up, if under @standtp
            if (gAgent.mRRInterface.mContainsStandtp)
            {
                gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal ());
                gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
            }
        }
        gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
        gMessageSystem->nextBlockFast(_PREHASH_AgentData);
        gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
        gMessageSystem->addUUIDFast(_PREHASH_TargetID, object->mID);
        gMessageSystem->addVector3Fast(_PREHASH_Offset,
            gAgentCamera.calcFocusOffset(object, gAgent.getPositionAgent(), (S32)0.0f, (S32)0.0f));
        object->getRegion()->sendReliableMessage();
    }
}


BOOL RRInterface::force (LLUUID object_uuid, std::string command, std::string option)
{
    if (sRestrainedLoveLogging || sRestrainedLoveCommandLogging) {
        LL_INFOS("RLV") << object_uuid.asString() << " : " << command << " " << option << LL_ENDL;
    }

    // RLVa allows #RLV/ to be used at the start of the path, so support it too for compatibility
    std::string prefix = RR_RLV_REDIR_FOLDER_PREFIX;
    LLStringUtil::toLower(prefix);
    prefix = prefix.substr(0,prefix.length()-1); // we don't want to check for or strip the ~
    if (option.find(prefix) == 0)
    {
        option = option.substr(prefix.length());
    }

    // If this action is blacklisted, do nothing
    if (isBlacklisted (command, true)) {
        return TRUE;
    }
        
    if (command=="sit") { // sit:UUID
        BOOL allowed_to_sittp=TRUE;
        if (!isAllowed (object_uuid, "sittp")) {
            allowed_to_sittp=FALSE;
            remove (object_uuid, "sittp", "");
        }
        LLUUID uuid (option);
        force_sit (uuid);
        if (!allowed_to_sittp) add (object_uuid, "sittp", "");
    }
    else if (command == "sitground") { // sitground
        if (gAgentAvatarp && !(gAgentAvatarp->mIsSitting && mSitTargetId == LLUUID::null)) { // do it only if the avatar is not already sitting on the ground
            mSitGroundOnStandUp = FALSE;
            // Attention : if the avatar is currently sitting on an object, immediately sitting on the ground will not let the "Stand" button appear
            // => we need to stand up and wait until the avatar is done standing up, before sitting on the ground
            if (gAgent.isSitting ()) {
                mSitGroundOnStandUp = TRUE; // set this variable to TRUE so we don't forget to sit down on the ground once we're off the object
                gAgent.standUp();
            }
            else {
                gAgent.setFlying(FALSE); // if the avatar is flying, stop flying now
                gAgent.sitDown(); // sit on the ground
            }

        }
    }
    else if (command=="unsit") { // unsit
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "trying to unsit" << LL_ENDL;
        }
        if (gAgentAvatarp &&
            gAgentAvatarp->mIsSitting) {
            if (sRestrainedLoveLogging) {
                LL_INFOS() << "found avatar object" << LL_ENDL;
            }
            if (gAgent.mRRInterface.mContainsUnsit) {
                if (sRestrainedLoveLogging) {
                    LL_INFOS() << "prevented from unsitting" << LL_ENDL;
                }
                return TRUE;
            }
            if (sRestrainedLoveLogging) {
                LL_INFOS() << "unsitting agent" << LL_ENDL;
            }
//          LLOverlayBar::onClickStandUp(NULL);
            gAgent.standUp();
            send_agent_update(TRUE, TRUE);
        }
    }
    else if (command=="remoutfit") { // remoutfit:shoes
        if (option=="") {
//          gAgentWearables.removeWearable (WT_GLOVES, false, 0);
//          gAgentWearables.removeWearable (WT_JACKET, false, 0);
//          gAgentWearables.removeWearable (WT_PANTS, false, 0);
//          gAgentWearables.removeWearable (WT_SHIRT, false, 0);
//          gAgentWearables.removeWearable (WT_SHOES, false, 0);
//          gAgentWearables.removeWearable (WT_SKIRT, false, 0);
//          gAgentWearables.removeWearable (WT_SOCKS, false, 0);
//          gAgentWearables.removeWearable (WT_UNDERPANTS, false, 0);
//          gAgentWearables.removeWearable (WT_UNDERSHIRT, false, 0);

            for (int i = LLWearableData::MAX_CLOTHING_LAYERS - 1; i >= 0; --i) {
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_GLOVES, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_JACKET, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_PANTS, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SHIRT, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SHOES, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SKIRT, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SOCKS, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_UNDERPANTS, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_UNDERSHIRT, i)));

#if ALPHA_AND_TATTOO
//          gAgentWearables.removeWearable (WT_ALPHA, false, 0);
//          gAgentWearables.removeWearable (WT_TATTOO, false, 0);
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_ALPHA, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_TATTOO, i)));
                removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_PHYSICS, i)));
#endif
            }
        }
        else {
            LLWearableType::EType type = getOutfitLayerAsType (option);
            if (type != LLWearableType::WT_INVALID) {
                 // clothes only, not skin, eyes, hair or shape
                if (LLWearableType::getInstance()->getAssetType(type) == LLAssetType::AT_CLOTHING) {
//                  gAgentWearables.removeWearable (type, false, 0); // remove by layer
                    for (int i = LLWearableData::MAX_CLOTHING_LAYERS - 1; i >= 0; --i) {
                        removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (type, i)));
                    }
                }
            }
            else forceDetachByName (option, FALSE); // remove by category (in RLV share)
        }
    }
    else if (command=="detach" || command=="remattach") { // detach:chest=force OR detach:restraints/cuffs=force (@remattach is a synonym)
        if (LLUUID::validate (option)) { // if option is a UUID, detach this object only
            return forceDetachByUuid(option); // remove by uuid
        }
        else {
            LLViewerJointAttachment* attachpt = findAttachmentPointFromName(option, TRUE); // exact name
            if (attachpt != NULL || option == "") return forceDetach(option); // remove by attach pt
            else forceDetachByName(option, FALSE);
        }
    }
    else if (command=="detachme") { // detachme=force to detach this object specifically
        return forceDetachByUuid (object_uuid.asString()); // remove by uuid
    }
    else if (command=="detachthis") { // detachthis=force to detach the folder containing this object
        std::string pathes_str;
        if (LLUUID::validate(option)) { // if option is a UUID, we're not detaching the folder contain the calling object, but the referenced object
            pathes_str = getFullPath(getItem(LLUUID (option)));
        }
        else {
            pathes_str = getFullPath(getItem(object_uuid), option);
        }
        std::deque<std::string> pathes = parse (pathes_str, ",");
        BOOL res = TRUE;
        for (unsigned int i = 0; i < pathes.size(); ++i) {
            res &= forceDetachByName (pathes.at(i), FALSE);
        }
        return res;
    }
    else if (command=="detachall") { // detachall:cuffs=force to detach a folder and its subfolders
        BOOL res = FALSE;
        // We're now doing the same thing as "Remove From Current Outfit" in the inventory, except that we need to check for "nostrip"
        // during this action, hence the need for mUserUpdateAttachmentsCalledFromScript
        mUserUpdateAttachmentsCalledFromScript = TRUE;
        res = forceDetachByName (option, TRUE);
        mUserUpdateAttachmentsCalledFromScript = FALSE;
        return res;
    }
    else if (command=="detachallthis") { // detachallthis=force to detach the folder containing this object and also its subfolders
        std::string pathes_str;
        if (LLUUID::validate(option)) { // if option is a UUID, we're not detaching the folder contain the calling object, but the referenced object
            pathes_str = getFullPath(getItem(LLUUID(option)));
        }
        else {
            pathes_str = getFullPath(getItem(object_uuid), option);
        }
        std::deque<std::string> pathes = parse(pathes_str, ",");
        BOOL res = TRUE;
        for (unsigned int i = 0; i < pathes.size(); ++i) {
            res &= forceDetachByName (pathes.at(i), TRUE);
        }
        return res;
    }
    else if (command=="tpto") { // tpto:[region/]X/Y/Z[;lookat]=force (X, Y, Z are local or global coordinates, depending on the existence of the region)
        size_t ind = option.find(";"); // lookat present ? (FIXME : broken at the moment because gAgent.teleportViaLocationLookAt () does not take a "lookat" parameter (to add it we have to change a whole lot of calls all over the code)
        LLVector3 vecLookAt = LLVector3::zero;
        if (ind != std::string::npos && ind + 1 < option.length()) {
            F32 lookat = (F32)atof(option.substr(ind + 1).c_str());
            vecLookAt = LLVector3::x_axis;
            vecLookAt.rotVec(lookat, LLVector3::z_axis);
            vecLookAt.normalize();
            option = option.substr(0, ind); // keep the coordinates only
        }
        BOOL allowed_to_tploc = TRUE;
        BOOL allowed_to_tplocal = TRUE;
        BOOL allowed_to_unsit=TRUE;
        BOOL allowed_to_sittp=TRUE;
        BOOL res;
        if (!isAllowed (object_uuid, "tploc")) {
            allowed_to_tploc=FALSE;
            remove (object_uuid, "tploc", "");
        }
        if (!isAllowed(object_uuid, "tplocal")) {
            allowed_to_tplocal = FALSE;
            remove(object_uuid, "tplocal", "");
        }
        if (!isAllowed(object_uuid, "unsit")) {
            allowed_to_unsit=FALSE;
            remove (object_uuid, "unsit", "");
        }
        if (!isAllowed (object_uuid, "sittp")) {
            allowed_to_sittp=FALSE;
            remove (object_uuid, "sittp", "");
        }
        res = forceTeleport(option, vecLookAt);
        if (!allowed_to_tploc) add(object_uuid, "tploc", "");
        if (!allowed_to_tplocal) add(object_uuid, "tplocal", "");
        if (!allowed_to_unsit) add (object_uuid, "unsit", "");
        if (!allowed_to_sittp) add (object_uuid, "sittp", "");
        return res;
    }
    //else if (command=="offertp") { // force to offer a tp to someone (regardless of showloc restriction, but do not show a confirmation)
    //  LLSD edit_args;
    //  edit_args["REGION"] = gAgent.getRegion()->getName();

    //  LLSD payload;
    //  if (LLUUID::validate(option)) {
    //      LLUUID uuid(option);
    //      payload["ids"].append(uuid);

    //  }
    //}
    else if (command=="attach" || command == "addoutfit") { // attach:cuffs=force
        return forceAttach (option, FALSE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
    }
    else if (command=="attachover" || command == "addoutfitover") { // attachover:cuffs=force
        return forceAttach (option, FALSE, AttachHow_over);
    }
    else if (command=="attachoverorreplace" || command == "addoutfitoverorreplace") { // attachoverorreplace:cuffs=force
        return forceAttach (option, FALSE, AttachHow_over_or_replace);
    }
    else if (command=="attachthis" || command == "addoutfitthis") { // attachthis=force to attach the folder containing this object
        BOOL res = TRUE;
        std::string pathes_str = getFullPath (getItem(object_uuid), option);
        if (pathes_str != "") {
            std::deque<std::string> pathes = parse (pathes_str, ",");
            for (unsigned int i = 0; i < pathes.size(); ++i) {
                res &= forceAttach (pathes.at(i), FALSE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
            }
        }
        return res;
    }
    else if (command=="attachthisover" || command == "addoutfitthisover") { // attachthisover=force to attach the folder containing this object
        BOOL res = TRUE;
        std::string pathes_str = getFullPath (getItem(object_uuid), option);
        if (pathes_str != "") {
            std::deque<std::string> pathes = parse (pathes_str, ",");
            for (unsigned int i = 0; i < pathes.size(); ++i) {
                res &= forceAttach (pathes.at(i), FALSE, AttachHow_over);
            }
        }
        return res;
    }
    else if (command=="attachthisoverorreplace" || command == "addoutfitthisoverorreplace") { // attachthisoverorreplace=force to attach the folder containing this object
        BOOL res = TRUE;
        std::string pathes_str = getFullPath (getItem(object_uuid), option);
        if (pathes_str != "") {
            std::deque<std::string> pathes = parse (pathes_str, ",");
            for (unsigned int i = 0; i < pathes.size(); ++i) {
                res &= forceAttach (pathes.at(i), FALSE, AttachHow_over_or_replace);
            }
        }
        return res;
    }
    else if (command=="attachall" || command == "addoutfitall") { // attachall:cuffs=force to attach a folder and its subfolders
        return forceAttach (option, TRUE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
    }
    else if (command=="attachallover" || command == "addoutfitallover") { // attachallover:cuffs=force to attach a folder and its subfolders
        return forceAttach (option, TRUE, AttachHow_over);
    }
    else if (command=="attachalloverorreplace" || command == "addoutfitalloverorreplace") { // attachalloverorreplace:cuffs=force to attach a folder and its subfolders
        return forceAttach (option, TRUE, AttachHow_over_or_replace);
    }
    else if (command=="attachallthis" || command == "addoutfitallthis") { // attachallthis=force to attach the folder containing this object and its subfolders
        BOOL res = TRUE;
        std::string pathes_str = getFullPath (getItem(object_uuid), option);
        if (pathes_str != "") {
            std::deque<std::string> pathes = parse (pathes_str, ",");
            for (unsigned int i = 0; i < pathes.size(); ++i) {
                res &= forceAttach (pathes.at(i), TRUE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
            }
        }
        return res;
    }
    else if (command=="attachallthisover" || command == "addoutfitallthisover") { // attachallthisover=force to attach the folder containing this object and its subfolders
        BOOL res = TRUE;
        std::string pathes_str = getFullPath (getItem(object_uuid), option);
        if (pathes_str != "") {
            std::deque<std::string> pathes = parse (pathes_str, ",");
            for (unsigned int i = 0; i < pathes.size(); ++i) {
                res &= forceAttach (pathes.at(i), TRUE, AttachHow_over);
            }
        }
        return res;
    }
    else if (command=="attachallthisoverorreplace" || command == "addoutfitallthisoverorreplace") { // attachallthisoverorreplace=force to attach the folder containing this object and its subfolders
        BOOL res = TRUE;
        std::string pathes_str = getFullPath (getItem(object_uuid), option);
        if (pathes_str != "") {
            std::deque<std::string> pathes = parse (pathes_str, ",");
            for (unsigned int i = 0; i < pathes.size(); ++i) {
                res &= forceAttach (pathes.at(i), TRUE, AttachHow_over_or_replace);
            }
        }
        return res;
    }
    else if (command.find ("setenv_") == 0) {
        BOOL res = TRUE;
        BOOL allowed = TRUE;
        if (!sRRNoSetEnv) {
            if (!isAllowed (object_uuid, "setenv")) {
                allowed=FALSE;
                remove (object_uuid, "setenv", "");
            }
            if (!mContainsSetenv) res = forceEnvironment (command, option);
            if (!allowed) add (object_uuid, "setenv", "");
        }
        return res;
    }
    else if (command.find ("setdebug_") == 0) {
        BOOL res = TRUE;
        BOOL allowed = TRUE;
        if (!isAllowed (object_uuid, "setdebug")) {
            allowed=FALSE;
            remove (object_uuid, "setdebug", "");
        }
        if (!contains("setdebug")) res = forceDebugSetting (command, option);
        if (!allowed) add (object_uuid, "setdebug", "");
        return res;
    }
    else if (command=="setrot") { // setrot:angle_radians=force
        BOOL res = TRUE;
        LLVOAvatar* avatar = gAgentAvatarp;
        if (!avatar) return FALSE;
        F32 val = atof (option.c_str());
        gAgentCamera.startCameraAnimation();
        LLVector3 rot (0.0, 1.0, 0.0);
        rot = rot.rotVec(-val, LLVector3::z_axis);
        rot.normalize();
        gAgent.resetAxes(rot);
        return res;
    }
    else if (command == "adjustheight") { // adjustheight:adjustment_centimeters=force or adjustheight:ref_pelvis_to_foot;scalar[;delta]=force
        if (!gSavedPerAccountSettings.controlExists("AvatarHoverOffsetZ")) {
            return FALSE;
        }
        LLVOAvatar* avatar = gAgentAvatarp;
        if (avatar) {
            F32 val = (F32)atoi(option.c_str()) / 100.0f;
            size_t i = option.find(";");
            if (i != std::string::npos && i + 1 < option.length()) {
                F32 scalar = (F32)atof(option.substr(i + 1).c_str());
                if (scalar != 0.0f) {
                    if (sRestrainedLoveLogging) {
                        LL_INFOS() << "Pelvis to foot = " << avatar->getPelvisToFoot() << "m" << LL_ENDL;
                    }
                    val = (atof(option.c_str()) - avatar->getPelvisToFoot()) * scalar;
                    option = option.substr(i + 1);
                    i = option.find(";");
                    if (i != std::string::npos && i + 1 < option.length()) {
                        val += (F32)atof(option.substr(i + 1).c_str());
                    }
                }
            }
            if (val > 2.0f) {
                val = 2.0f;
            }
            else if (val < -2.0f) {
                val = -2.0f;
            }
            gSavedPerAccountSettings.setF32("AvatarHoverOffsetZ", val);
        }
    }
    else if (command == "setgroup") {
        std::string target_group_name = option;
        LLStringUtil::toLower(target_group_name);
        if (target_group_name != "none") { // "none" is not localized here because a script should not have to bother about viewer language
            S32 nb = gAgent.mGroups.size();
            for (S32 i=0; i<nb; ++i) {
                LLGroupData group = gAgent.mGroups.at(i);
                std::string this_group_name = group.mName;
                LLStringUtil::toLower(this_group_name);
                if (this_group_name == target_group_name) {
                    LLGroupActions::activate(group.mID);
                    break;
                }
            }
        }
        else {
            LLGroupActions::activate(LLUUID::null);
        }
        // not found => do nothing
    }
    else if (command == "setcam_fov") {
        F32 new_fov_rad = atof(option.c_str());
        LLViewerCamera::getInstance()->setDefaultFOV(new_fov_rad);
        gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
    }
    else if (command.find("setsphere_") == 0) { // HACK : since RLVa expects "@setsphere_xxx:yyy=force", while the API expects "@setsphere_xxx:yyy=n", make it so the former is interpreted as the latter here (but this also means we have to remove all the parameters when @setsphere=y is received)
        add(object_uuid, command, option);
    }
    return TRUE;
}

void RRInterface::removeItemFromAvatar(LLViewerInventoryItem* item)
{
    if (!item) return;
    std::string name = item->getName();
    LLStringUtil::toLower(name);
    if (!canUnwear (item)) return;
    LLAppearanceMgr::instance().removeItemFromAvatar(item->getUUID());
    //LLWearableBridge::removeItemFromAvatar (item);
}

// CA: if the command is from the avatar's own uuid and the channel is zero then it
// gets sent back to the console window

BOOL RRInterface::answerOnChat (std::string channel, std::string msg)
{
    S32 chan = (S32)atoi(channel.c_str());
    if (chan == 0 && gAgent.getID() != current_handlecommand_caller) {
        // protection against abusive "@getstatus=0" commands, or against a non-numerical channel
        return FALSE;
    }
    if (chan > 0) {
        // If the message is too long, truncate
        if (msg.length() > 1023) {
            msg = msg.substr(0, 1023);
        }
        //std::ostringstream temp;
        //temp << "/" << chan << " " << msg;
        //LLFloaterIMNearbyChat::sendChatFromViewer(temp.str(), CHAT_TYPE_SHOUT, FALSE);
        gMessageSystem->newMessageFast(_PREHASH_ChatFromViewer);
        gMessageSystem->nextBlockFast(_PREHASH_AgentData);
        gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        gMessageSystem->nextBlockFast(_PREHASH_ChatData);
        gMessageSystem->addStringFast(_PREHASH_Message, msg);
        gMessageSystem->addU8Fast(_PREHASH_Type, CHAT_TYPE_SHOUT);
        gMessageSystem->addS32("Channel", chan);

        gAgent.sendReliableMessage();
    }
    else if (chan==0)
    {
        // If the message is too long, truncate
        if (msg.length() > 1023) {
            msg = msg.substr(0, 1023);
        }
        // it's a reply for the console - all other uses of 0 got tested for earlier
        KokuaFloaterRLVConsole::getBase()->addCommandReply(msg);
    }
    else {
        // If the message is too long, truncate (254 chars instead of 255 becuase of LLTemplateMessageBuilder::addData() )
        if (msg.length() > 254) {
            msg = msg.substr(0, 254);
        }
        gMessageSystem->newMessage("ScriptDialogReply");
        gMessageSystem->nextBlock("AgentData");
        gMessageSystem->addUUID("AgentID", gAgent.getID());
        gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
        gMessageSystem->nextBlock("Data");
        gMessageSystem->addUUID("ObjectID", gAgent.getID());
        gMessageSystem->addS32("ChatChannel", chan);
        gMessageSystem->addS32("ButtonIndex", 1);
        gMessageSystem->addString("ButtonLabel", msg);
        gAgent.sendReliableMessage();
    }
    if (sRestrainedLoveLogging) {
        LL_INFOS() << "/" << chan << " " << msg << LL_ENDL;
    }
    return TRUE;
}

std::string RRInterface::crunchEmote (std::string msg, unsigned int truncateTo) {
    // Don't treat text before "/me" if there is any, we want to crunch only the emote part of the message (everything after "/me").
    int ind = msg.find("/me");
    std::string prefix = "";
    if (ind > 0) {
        prefix = msg.substr(0, ind);
        msg = msg.substr(ind);
    }

    std::string crunched = msg;

    if (msg.find ("/me ") == 0
    || msg.find ("/me'") == 0
    || (msg.find (":") == 0 && msg.length() >= 3 && isalpha(msg[1]))
    || msg.find (":'") == 0
    ) {
        // Only allow emotes without "spoken" text.
        // Forbid text containing any symbol which could be used as quotes.
        if (msg.find ("\"") != -1 || msg.find ("''") != -1
            || msg.find ("(")  != -1 || msg.find (")")  != -1
            || msg.find (" -") != -1 || msg.find ("- ") != -1
            || msg.find ("*")  != -1 || msg.find ("=")  != -1
            || msg.find ("^")  != -1 || msg.find ("_")  != -1
            || msg.find ("~")  != -1)
        {
            crunched = "...";
        }
        else if (truncateTo > 0 && !contains ("emote")) {
            // Only allow short emotes.
            int i = msg.find (".");
            if (i != -1) {
                crunched = msg.substr (0, ++i);
            }
            if (crunched.length () > truncateTo) {
                crunched = crunched.substr (0, truncateTo);
            }
        }
    }
    else if (msg.find ("/") == 0) {
        // only allow short gesture names (to avoid cheats).
        if (msg.length () > 7) { // allows things like "/ao off", "/hug X"
            crunched = "...";
        }
    }
    else if (msg.find ("((") != 0 || msg.find ("))") != msg.length () - 2 || !sCanOoc) {
        // Only allow OOC chat, starting with "((" and ending with "))".
        crunched = "...";
    }
    return crunched;
    //return prefix + crunched;
}

std::string RRInterface::getOutfitLayerAsString (LLWearableType::EType layer)
{
    switch (layer) {
        case LLWearableType::WT_SKIN: return WS_SKIN;
        case LLWearableType::WT_GLOVES: return WS_GLOVES;
        case LLWearableType::WT_JACKET: return WS_JACKET;
        case LLWearableType::WT_PANTS: return WS_PANTS;
        case LLWearableType::WT_SHIRT: return WS_SHIRT;
        case LLWearableType::WT_SHOES: return WS_SHOES;
        case LLWearableType::WT_SKIRT: return WS_SKIRT;
        case LLWearableType::WT_SOCKS: return WS_SOCKS;
        case LLWearableType::WT_UNDERPANTS: return WS_UNDERPANTS;
        case LLWearableType::WT_UNDERSHIRT: return WS_UNDERSHIRT;
#if ALPHA_AND_TATTOO
        case LLWearableType::WT_ALPHA: return WS_ALPHA;
        case LLWearableType::WT_TATTOO: return WS_TATTOO;
        case LLWearableType::WT_PHYSICS: return WS_PHYSICS;
#endif
        case LLWearableType::WT_EYES: return WS_EYES;
        case LLWearableType::WT_HAIR: return WS_HAIR;
        case LLWearableType::WT_SHAPE: return WS_SHAPE;
        default: return "";
    }
}

LLWearableType::EType RRInterface::getOutfitLayerAsType (std::string layer)
{
    if (layer==WS_SKIN) return LLWearableType::WT_SKIN;
    if (layer==WS_GLOVES) return LLWearableType::WT_GLOVES;
    if (layer==WS_JACKET) return LLWearableType::WT_JACKET;
    if (layer==WS_PANTS) return LLWearableType::WT_PANTS;
    if (layer==WS_SHIRT) return LLWearableType::WT_SHIRT;
    if (layer==WS_SHOES) return LLWearableType::WT_SHOES;
    if (layer==WS_SKIRT) return LLWearableType::WT_SKIRT;
    if (layer==WS_SOCKS) return LLWearableType::WT_SOCKS;
    if (layer==WS_UNDERPANTS) return LLWearableType::WT_UNDERPANTS;
    if (layer==WS_UNDERSHIRT) return LLWearableType::WT_UNDERSHIRT;
#if ALPHA_AND_TATTOO
    if (layer==WS_ALPHA) return LLWearableType::WT_ALPHA;
    if (layer==WS_TATTOO) return LLWearableType::WT_TATTOO;
    if (layer==WS_PHYSICS) return LLWearableType::WT_PHYSICS;
#endif
    if (layer==WS_EYES) return LLWearableType::WT_EYES;
    if (layer==WS_HAIR) return LLWearableType::WT_HAIR;
    if (layer==WS_SHAPE) return LLWearableType::WT_SHAPE;
    return LLWearableType::WT_INVALID;
}

std::string RRInterface::getOutfit (std::string layer)
{
    if (layer==WS_SKIN) return (gAgentWearables.getWearable (LLWearableType::WT_SKIN, 0) != NULL? "1" : "0");
    if (layer==WS_GLOVES) return (gAgentWearables.getWearable (LLWearableType::WT_GLOVES, 0) != NULL? "1" : "0");
    if (layer==WS_JACKET) return (gAgentWearables.getWearable (LLWearableType::WT_JACKET, 0) != NULL? "1" : "0");
    if (layer==WS_PANTS) return (gAgentWearables.getWearable (LLWearableType::WT_PANTS, 0) != NULL? "1" : "0");
    if (layer==WS_SHIRT)return (gAgentWearables.getWearable (LLWearableType::WT_SHIRT, 0) != NULL? "1" : "0");
    if (layer==WS_SHOES) return (gAgentWearables.getWearable (LLWearableType::WT_SHOES, 0) != NULL? "1" : "0");
    if (layer==WS_SKIRT) return (gAgentWearables.getWearable (LLWearableType::WT_SKIRT, 0) != NULL? "1" : "0");
    if (layer==WS_SOCKS) return (gAgentWearables.getWearable (LLWearableType::WT_SOCKS, 0) != NULL? "1" : "0");
    if (layer==WS_UNDERPANTS) return (gAgentWearables.getWearable (LLWearableType::WT_UNDERPANTS, 0) != NULL? "1" : "0");
    if (layer==WS_UNDERSHIRT) return (gAgentWearables.getWearable (LLWearableType::WT_UNDERSHIRT, 0) != NULL? "1" : "0");
#if ALPHA_AND_TATTOO
//  if (layer==WS_ALPHA) return (gAgent.getWearable (LLWearableType::WT_ALPHA, 0) != NULL? "1" : "0");
//  if (layer==WS_TATTOO) return (gAgent.getWearable (LLWearableType::WT_TATTOO, 0) != NULL? "1" : "0");
    if (layer==WS_ALPHA) return (gAgentWearables.getWearable (LLWearableType::WT_ALPHA, 0) != NULL? "1" : "0");
    if (layer==WS_TATTOO) return (gAgentWearables.getWearable (LLWearableType::WT_TATTOO, 0) != NULL? "1" : "0");
    if (layer==WS_PHYSICS) return (gAgentWearables.getWearable (LLWearableType::WT_PHYSICS, 0) != NULL? "1" : "0");
#endif
    if (layer==WS_EYES) return (gAgentWearables.getWearable (LLWearableType::WT_EYES, 0) != NULL? "1" : "0");
    if (layer==WS_HAIR) return (gAgentWearables.getWearable (LLWearableType::WT_HAIR, 0) != NULL? "1" : "0");
    if (layer==WS_SHAPE) return (gAgentWearables.getWearable (LLWearableType::WT_SHAPE, 0) != NULL? "1" : "0");
    return getOutfit (WS_GLOVES)+getOutfit (WS_JACKET)+getOutfit (WS_PANTS)
            +getOutfit (WS_SHIRT)+getOutfit (WS_SHOES)+getOutfit (WS_SKIRT)
            +getOutfit (WS_SOCKS)+getOutfit (WS_UNDERPANTS)+getOutfit (WS_UNDERSHIRT)
            +getOutfit (WS_SKIN)+getOutfit (WS_EYES)+getOutfit (WS_HAIR)+getOutfit (WS_SHAPE)
#if ALPHA_AND_TATTOO
            +getOutfit (WS_ALPHA)+getOutfit (WS_TATTOO)+getOutfit (WS_PHYSICS)
#endif
            ;
}

std::string RRInterface::getAttachments (std::string attachpt)
{
    std::string res="";
    std::string name;
    LLVOAvatar* avatar = gAgentAvatarp;
    if (!avatar) {
        LL_WARNS() << "NULL avatar pointer. Aborting." << LL_ENDL;
        return res;
    }
    if (attachpt=="") res+="0"; // to match the LSL macros
    for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
        iter != avatar->mAttachmentPoints.end(); iter++)
    {
        LLVOAvatar::attachment_map_t::iterator curiter = iter;
        LLViewerJointAttachment* attachment = curiter->second;
        name=attachment->getName ();
        LLStringUtil::toLower(name);
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "trying <" << name << ">" << LL_ENDL;
        }
        if (attachpt=="" || attachpt==name) {
            if (attachment->getNumObjects() > 0) res+="1"; //attachment->getName ();
            else res+="0";
        }
    }
    return res;
}

std::string RRInterface::getStatus (LLUUID object_uuid, std::string rule)
{
    std::string res="";
    std::string name;
    std::string separator = "/";
    // If rule contains a specification of the separator, extract it
    int ind = rule.find (";");
    if (ind != -1) {
        separator = rule.substr (ind+1);
        rule = rule.substr (0, ind);
    }
    if (separator == "") {
        separator = "/"; // Prevent a hack to force the avatar to say something
    }
    
    RRMAP::iterator it;
    if (object_uuid.isNull()) {
        it = mSpecialObjectBehaviours.begin();
    }
    else {
        it = mSpecialObjectBehaviours.find (object_uuid.asString());
    }
    bool is_first=true;
    while (it != mSpecialObjectBehaviours.end() &&
            (object_uuid.isNull() || it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
    )
    {
        if (rule=="" || it->second.find (rule)!=-1) {
            //if (!is_first) 
            res+=separator;
            res+=it->second;
            is_first=false;
        }
        it++;
    }
    return res;
}

BOOL RRInterface::forceDetach (std::string attachpt)
{
    std::string name;
    BOOL res=FALSE;
    LLVOAvatar* avatar = gAgentAvatarp;
    if (!avatar) return res;
    for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
         iter != avatar->mAttachmentPoints.end(); iter++)
    {
        LLVOAvatar::attachment_map_t::iterator curiter = iter;
        LLViewerJointAttachment* attachment = curiter->second;
        name=attachment->getName ();
        LLStringUtil::toLower(name);
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "trying <" << name << ">" << LL_ENDL;
        }
        if (attachpt=="" || attachpt==name) {
            if (sRestrainedLoveLogging) {
                LL_INFOS() << "found => detaching" << LL_ENDL;
            }
            detachAllObjectsFromAttachment (attachment);
            res=TRUE;
        }
    }
    return res;
}


BOOL RRInterface::forceDetachByUuid (std::string object_uuid)
{
    BOOL res=FALSE;
    LLVOAvatar* avatar = gAgentAvatarp;
    if (!avatar) return res;
    LLViewerObject* object = gObjectList.findObject(LLUUID (object_uuid));
    if (object) {
        object = object->getRootEdit();
        for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
             iter != avatar->mAttachmentPoints.end(); iter++)
        {
            LLVOAvatar::attachment_map_t::iterator curiter = iter;
            LLViewerJointAttachment* attachment = curiter->second;
            if (attachment && attachment->isObjectAttached (object)) {
                detachObject (object);
                res=TRUE;
            }
        }
    }
    return res;
}

BOOL RRInterface::hasLockedHuds ()
{
    LLVOAvatar* avatar = gAgentAvatarp;
    if (!avatar) return FALSE;
    for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
         iter != avatar->mAttachmentPoints.end(); iter++)
    {
        LLVOAvatar::attachment_map_t::iterator curiter = iter;
        LLViewerJointAttachment* attachment = curiter->second;
        LLViewerObject* obj;
        if (attachment) {
            for (unsigned int i = 0; i < attachment->mAttachedObjects.size(); ++i) {
                obj = attachment->mAttachedObjects.at(i);
                if (obj && obj->isHUDAttachment() && !canDetach(obj)) return TRUE;
            }
        }
    }
    return FALSE;
}


std::deque<LLInventoryItem*> RRInterface::getListOfLockedItems (LLInventoryCategory* root)
{
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    std::deque<LLInventoryItem*> res;
    std::deque<LLInventoryItem*> tmp;
    res.clear();
    
    if (root && avatar) {
        
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
        S32 count;
        S32 count_tmp;
        S32 i;
        S32 j;
        LLInventoryItem* item = NULL;
        LLInventoryCategory* cat = NULL;
        //      LLViewerObject* attached_object = NULL;
        std::string attach_point_name = "";
        
        // Try to find locked items in the current category
        count = items->size();
        for (i = 0; i < count; ++i) {
            item = items->at(i);
            // If this is an object, add it if it is worn and locked, or worn and its attach point is locked
            if (item && item->getType() == LLAssetType::AT_OBJECT) {
                LLViewerObject* attached_object = avatar->getWornAttachment (item->getLinkedUUID());
                if (attached_object) {
                    avatar->getAttachedPointName (item->getLinkedUUID(), attach_point_name);
                    if (!gAgent.mRRInterface.canDetach(attached_object)) {
                        if (sRestrainedLoveLogging) {
                            LL_INFOS() << "found a locked object : " << item->getName() << " on " << attach_point_name << LL_ENDL;
                        }
                        res.push_back (item);
                    }
                }
            }
            // If this is a piece of clothing, add it if the avatar can't unwear clothes, or if this layer itself can't be unworn
            else if (item && item->getType() == LLAssetType::AT_CLOTHING) {
                if (gAgent.mRRInterface.contains ("remoutfit")
                    || gAgent.mRRInterface.containsSubstr ("remoutfit:")
                    ) {
                    if (sRestrainedLoveLogging) {
                        LL_INFOS() << "found a locked clothing : " << item->getName() << LL_ENDL;
                    }
                    res.push_back (item);
                }
            }
        }
        
        // We have all the locked objects contained directly in this folder, now add all the ones contained in children folders recursively
        count = cats->size();
        for (i = 0; i < count; ++i) {
            cat = cats->at(i);
            tmp = getListOfLockedItems (cat);
            count_tmp = tmp.size();
            for (j = 0; j < count_tmp; ++j) {
                item = tmp[j];
                if (item) res.push_back (item);
            }
        }
        
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "number of locked objects under " << root->getName() << " =  " << res.size() << LL_ENDL;
        }
    }
    
    return res;
}

std::deque<std::string> RRInterface::getListOfRestrictions (LLUUID object_uuid, std::string rule /*= ""*/)
{
    std::deque<std::string> res;
    std::string name;
    RRMAP::iterator it;
    if (object_uuid.isNull()) {
        it = mSpecialObjectBehaviours.begin();
    }
    else {
        it = mSpecialObjectBehaviours.find (object_uuid.asString());
    }
    while (it != mSpecialObjectBehaviours.end() &&
            (object_uuid.isNull() || it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
    )
    {
        if (rule=="" || it->second.find (rule)!=-1) {
            res.push_back(it->second);
        }
        it++;
    }
    return res;
}


std::string RRInterface::getInventoryList (std::string path, BOOL withWornInfo /* = FALSE */)
{
    std::string res = "";
    LLInventoryModel::cat_array_t* cats;
    LLInventoryModel::item_array_t* items;
    LLInventoryCategory* root = NULL;
    if (path == "") root = getRlvShare();
    else root = getCategoryUnderRlvShare (path);
    
    if (root) {
        gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
        if(cats) {
            S32 count = cats->size();
            bool found_one = false;
            if (withWornInfo) {
                std::string worn_items = getWornItems (root);
                res += "|";
                found_one = true;
                if (worn_items == "n") {
                    res += "10";
                }
                else if (worn_items == "N") {
                    res += "30";
                }
                else {
                    res += worn_items;
                }
            }
            for(S32 i = 0; i < count; ++i) {
                LLInventoryCategory* cat = cats->at(i);
                std::string name = cat->getName();
                if (name != "" && name[0] !=  '.') { // hidden folders => invisible to the list
                    if (found_one) res += ",";
                    res += name.c_str();
                    if (withWornInfo) {
                        std::string worn_items = getWornItems (cat);
                        res += "|";
                        found_one = true;
                        if (worn_items == "n") {
                            res += "10";
                        }
                        else if (worn_items == "N") {
                            res += "30";
                        }
                        else {
                            res += worn_items;
                        }
                    }
                    found_one = true;
                }
            }
        }
    }

    return res;
}

std::string RRInterface::getWornItems (LLInventoryCategory* cat)
{
    // Returns a string of 2 digits according to the proportion of worn items in this folder and its children :
    // First digit is this folder, second digit is children folders
    // 0 : No item contained in the folder
    // 1 : Some items contained but none is worn
    // 2 : Some items contained and some of them are worn
    // 3 : Some items contained and all of them are worn
    std::string res_as_string = "0";
    int res         = 0;
    int subRes      = 0;
    int prevSubRes  = 0;
    int nbItems     = 0;
    int nbWorn      = 0;
    BOOL isNoMod    = FALSE;
    BOOL isRoot     = (getRlvShare() == cat);
    
    // if cat exists, scan all the items inside it
    if (cat) {
    
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        
        // retrieve all the objects contained in this folder
        gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
        if (!isRoot && items) { // do not scan the shared root
        
            // scan them one by one
            S32 count = items->size();
            for(S32 i = 0; i < count; ++i) {
            
                LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->at(i);

                if (item) {
                    LLVOAvatarSelf* avatar = gAgentAvatarp;
                    if (item->getType() == LLAssetType::AT_OBJECT
                     || item->getType() == LLAssetType::AT_CLOTHING
                     || item->getType() == LLAssetType::AT_BODYPART
                    ) {
                        nbItems++;
                    }
                    if (( avatar && avatar->isWearingAttachment( item->getLinkedUUID()) )
                        || gAgentWearables.isWearingItem (item->getLinkedUUID())) nbWorn++;

                    // special case : this item is no-mod, hence we need to check its parent folder
                    // is correctly named, and that the item is alone in its folder.
                    // If so, then the calling method will have to deal with a special character instead
                    // of a number
                    if (count == 1
                     && item->getType() == LLAssetType::AT_OBJECT
                     && !item->getPermissions().allowModifyBy(gAgent.getID())) {
                        if (findAttachmentPointFromName (cat->getName()) != NULL) {
                            isNoMod = TRUE;
                        }
                    }
                }
            }
        }
        
        // scan every subfolder of the folder we are scanning, recursively
        // note : in the case of no-mod items we shouldn't have sub-folders, so no need to check
        if (cats && !isNoMod) {
        
            S32 count = cats->size();
            for(S32 i = 0; i < count; ++i) {

                LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->at(i);

                if (cat_child) {
                    std::string tmp = getWornItems (cat_child);
                    // translate the result for no-mod items into something the upper levels can understand
                    if (tmp == "N") {
                        if (!isRoot) {
                            nbWorn++;
                            nbItems++;
                            subRes = 3;
                        }
                    }
                    else if (tmp== "n") {
                        if (!isRoot) {
                            nbItems++;
                            subRes = 1;
                        }
                    }
                    else if (cat_child->getName() != "" && cat_child->getName()[0] != '.') { // we don't want to include invisible folders, except the ones containing a no-mod item
                        // This is an actual sub-folder with several items and sub-folders inside,
                        // so retain its score to integrate it into the current one
                        // As it is a sub-folder, to integrate it we need to reduce its score first (consider "0" as "ignore")
                        // "00" = 0, "01" = 1, "10" = 1, "30" = 3, "03" = 3, "33" = 3, all the rest gives 2 (some worn, some not worn)
                        if      (tmp == "00")                               subRes = 0;
                        else if (tmp == "11" || tmp == "01" || tmp == "10") subRes = 1;
                        else if (tmp == "33" || tmp == "03" || tmp == "30") subRes = 3;
                        else subRes = 2;

                        // Then we must combine with the previous sibling sub-folders
                        // Same rule as above, set to 2 in all cases except when prevSubRes == subRes or when either == 0 (nothing present, ignore)
                        if      (prevSubRes == 0 && subRes == 0) subRes = 0;
                        else if (prevSubRes == 0 && subRes == 1) subRes = 1;
                        else if (prevSubRes == 1 && subRes == 0) subRes = 1;
                        else if (prevSubRes == 1 && subRes == 1) subRes = 1;
                        else if (prevSubRes == 0 && subRes == 3) subRes = 3;
                        else if (prevSubRes == 3 && subRes == 0) subRes = 3;
                        else if (prevSubRes == 3 && subRes == 3) subRes = 3;
                        else subRes = 2;
                        prevSubRes = subRes;
                    }
                }
            }
        }
    }

    if (isNoMod) {
        // the folder contains one no-mod object and is named from an attachment point
        // => return a special character that will be handled by the calling method
        if (nbWorn > 0) return "N";
        else return "n";
    }
    else {
        if (isRoot || nbItems == 0) res = 0; // forcibly hide all items contained directly under #RLV
        else if (nbWorn >= nbItems) res = 3;
        else if (nbWorn > 0) res = 2;
        else res = 1;
    }
    std::stringstream str;
    str << res;
    str << subRes;
    res_as_string = str.str();
    return res_as_string;
}

LLInventoryCategory* RRInterface::getRlvShare ()
{
    LLInventoryModel::cat_array_t* cats;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf (
                    gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY), cats, items
    );

    if(cats) {
        S32 count = cats->size();
        for(S32 i = 0; i < count; ++i) {
            LLInventoryCategory* cat = cats->at(i);
            std::string name = cat->getName();
            if (name == RR_SHARED_FOLDER) {
//              if (sRestrainedLoveLogging) {
//                  LL_INFOS() << "found " << name << LL_ENDL;
//              }
                return cat;
            }
        }
    }
    return NULL;
}

BOOL RRInterface::isUnderRlvShare (LLInventoryItem* item)
{
    const LLUUID& cat_id = item->getParentUUID();
    return isUnderFolder(getRlvShare(), gInventory.getCategory(cat_id));
/*
    if (item == NULL) return FALSE;
    LLInventoryCategory* res = NULL;
    LLInventoryCategory* rlv = getRlvShare();
    if (rlv == NULL) return FALSE;
    LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);

    const LLUUID& cat_id = item->getParentUUID();
    res = gInventory.getCategory (cat_id);
    
    while (res && res->getUUID() != root_id) {
        if (res == rlv) return TRUE;
        const LLUUID& parent_id = res->getParentUUID();
        res = gInventory.getCategory (parent_id);
    }
    return FALSE;
*/
}

BOOL RRInterface::isUnderRlvShare (LLInventoryCategory* cat)
{
    return isUnderFolder (getRlvShare(), cat);
/*
    if (cat == NULL) return FALSE;
    LLInventoryCategory* res = NULL;
    LLInventoryCategory* rlv = getRlvShare();
    if (rlv == NULL) return FALSE;
    LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
    
    const LLUUID& cat_id = cat->getParentUUID();
    res = gInventory.getCategory (cat_id);
    
    while (res && res->getUUID() != root_id) {
        if (res == rlv) return TRUE;
        const LLUUID& parent_id = res->getParentUUID();
        res = gInventory.getCategory (parent_id);
    }
    return FALSE;
*/
}

BOOL RRInterface::isUnderFolder (LLInventoryCategory* cat_parent, LLInventoryCategory* cat_child)
{
    if (cat_parent == NULL || cat_child == NULL) {
        return FALSE;
    }
    if (cat_child == cat_parent) {
        return TRUE;
    }
    LLInventoryCategory* res = NULL;
    LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
    
    const LLUUID& cat_id = cat_child->getParentUUID();
    res = gInventory.getCategory (cat_id);
    
    while (res && res->getUUID() != root_id) {
        if (res == cat_parent) {
            return TRUE;
        }
        const LLUUID& parent_id = res->getParentUUID();
        res = gInventory.getCategory (parent_id);
    }
    return FALSE;
}

/*
void RRInterface::renameAttachment (LLInventoryItem* item, LLViewerJointAttachment* attachment)
{
  // DEPRECATED : done directly in the viewer code
    // if item is worn and shared, check its name
    // if it doesn't contain the name of attachment, append it
    // (but truncate the name first if it's too long)
    if (!item || !attachment) return;
    LLVOAvatar* avatar = gAgentAvatarp;
    
    if( avatar && avatar->isWearingAttachment( item->getUUID() ) ) {
        if (isUnderRlvShare (item)) {
            LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
            if (attachpt == NULL) {
                
            }
        }
    }
}
*/
LLInventoryCategory* RRInterface::getCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root)
{
    if (root == NULL) root = getRlvShare();
    if (catName == "") return root;
    LLStringUtil::toLower (catName);
    std::deque<std::string> tokens = parse (catName, "/");

    // Preliminary action : remove everything after pipes ("|"), including pipes themselves
    // This way we can feed the result of a @getinvworn command directly into this method
    // without having to clean what is after the pipes
    int nb = tokens.size();
    for (int i=0; i<nb; ++i) {
        std::string tok = tokens[i];
        int ind = tok.find ("|");
        if (ind != -1) {
            tok = tok.substr (0, ind);
            tokens[i] = tok;
        }
    }
    
    if (root) {

        bool exact_match = false;
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
        
        if(cats) {
            S32 count = cats->size();
            LLInventoryCategory* cat = NULL;
            
            // we need to scan first and retain the best match
            int max_size_index = -1;
            int max_size = 0;
            
            for(S32 i = 0; i < count; ++i) {
                cat = cats->at(i);
                std::string name = cat->getName();
                if (name != "" && name[0] !=  '.') { // ignore invisible folders
                    LLStringUtil::toLower (name);
                    
                    int size = match (tokens, name, exact_match);
                    if (size > max_size || (exact_match && size == max_size)) {
                        max_size = size;
                        max_size_index = i;
                    }
                }
            }

            // only now we can grab the best match and either continue deeper or return it
            if (max_size > 0) {
                cat = cats->at(max_size_index);
                if (max_size == tokens.size()) return cat;
                else return getCategoryUnderRlvShare (
                                    dumpList2String (
                                        getSubList (tokens, max_size)
                                    , "/")
                                , cat);
            }
        }
    }

    if (sRestrainedLoveLogging) {
        LL_INFOS() << "category not found" << LL_ENDL;
    }
    return NULL;
}

LLInventoryCategory* RRInterface::findCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root)
{
    if (root == NULL) root = getRlvShare();
    LLStringUtil::toLower (catName);
    std::deque<std::string> tokens = parse (catName, "&&");
    
    if (root) {
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
        
        if(cats)
        {
            S32 count = cats->size();
            LLInventoryCategory* cat = NULL;
            
            for(S32 i = 0; i < count; ++i)
            {
                cat = cats->at(i);
                
                std::string name = cat->getName();
                LLStringUtil::toLower(name);
                // We can't find invisible folders ('.') and dropped folders ('~')
                if (name != "" && name[0] != '.' && name[0] != '~') {
                    // search deeper first
                    LLInventoryCategory* found = findCategoryUnderRlvShare(catName, cat);
                    if (found != NULL) return found;
                }
            }
        }
        // return this category if it matches
        std::string name = root->getName();
        LLStringUtil::toLower (name);
        // We can't find invisible folders ('.') and dropped folders ('~')
        if (name != "" && name[0] != '.' && name[0] != '~' && findMultiple (tokens, name)) return root;
    }
    // didn't find anything
    return NULL;
}

std::deque<LLInventoryCategory*> RRInterface::findCategoriesUnderRlvShare(std::string catName, LLInventoryCategory* root)
{
    if (root == NULL) root = getRlvShare();
    LLStringUtil::toLower(catName);
    std::deque<std::string> tokens = parse(catName, "&&");
    std::deque<LLInventoryCategory*> res;

    if (root) {
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf(root->getUUID(), cats, items);

        if (cats)
        {
            S32 count = cats->size();
            LLInventoryCategory* cat = NULL;

            for (S32 i = 0; i < count; ++i)
            {
                cat = cats->at(i);

                std::string name = cat->getName();
                LLStringUtil::toLower(name);
                // We can't find invisible folders ('.') and dropped folders ('~')
                if (name != "" && name[0] != '.' && name[0] != '~') {
                    // search deeper first
                    std::deque<LLInventoryCategory*> found = findCategoriesUnderRlvShare(catName, cat);
                    for (int i = 0; i < found.size(); i++) {
                        res.push_back(found.at(i));
                    }
                }

            }
        }

        // add this category if it matches
        std::string name = root->getName();
        LLStringUtil::toLower(name);
        // We can't find invisible folders ('.') and dropped folders ('~')
        if (name != "" && name[0] != '.' && name[0] != '~' && findMultiple(tokens, name)) res.push_back(root);
    }

    return res;
}

std::string RRInterface::findAttachmentNameFromPoint(LLViewerJointAttachment* attachpt)
{
    // return the lowercased name of the attachment, or empty if null
    if (attachpt == NULL) return "";
    std::string res = attachpt->getName();
    LLStringUtil::toLower(res);
    return res;
}

// This struct is meant to be used in RRInterface::findAttachmentPointFromName below
typedef struct
{
    int length;
    int index;
    LLViewerJointAttachment* attachment;
} Candidate;

LLViewerJointAttachment* RRInterface::findAttachmentPointFromName (std::string objectName, BOOL exactName)
{
    // for each possible attachment point, check whether its name appears in the name of
    // the item.
    // We are going to scan the whole list of attachments, but we won't decide which one to take right away.
    // Instead, for each matching point, we will store in lists the following results :
    // - length of its name
    // - right-most index where it is found in the name
    // - a pointer to that attachment point
    // When we have that list, choose the highest index, and in case of ex-aequo choose the longest length
    LLVOAvatar* avatar = gAgentAvatarp;
    if (!avatar) {
        LL_WARNS() << "NULL avatar pointer. Aborting." << LL_ENDL;
        return NULL;
    }
    LLStringUtil::toLower(objectName);
    // HACK : we replace "(avatar center)" by "(root)", to make those two equivalent
    objectName = stringReplace (objectName, "(avatar center)", "(root)");
    std::string attachName;
    int ind = -1;
    bool found_one = false;
    std::vector<Candidate> candidates;
    
    for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
         iter != avatar->mAttachmentPoints.end(); )
    {
        LLVOAvatar::attachment_map_t::iterator curiter = iter++;
        LLViewerJointAttachment* attachment = curiter->second;
        if (attachment) {
            attachName = attachment->getName();
            LLStringUtil::toLower(attachName);
//          if (sRestrainedLoveLogging) {
//              LL_INFOS() << "trying attachment " << attachName << LL_ENDL;
//          }
            if (exactName && objectName == attachName) return attachment;
            else if (!exactName && (ind = objectName.rfind ("("+attachName+")")) != -1)
            {
                Candidate new_candidate;
                new_candidate.index = ind+1;
                new_candidate.length = attachName.length();
                new_candidate.attachment = attachment;
                candidates.push_back (new_candidate);
                found_one = true;
                if (sRestrainedLoveLogging) {
                    LL_INFOS() << "new candidate '" << attachName << "' : index=" << new_candidate.index << "   length=" << new_candidate.length << LL_ENDL;
                }
            }
        }
    }
    if (!found_one) {
        if (sRestrainedLoveLogging) {
            LL_INFOS() << "no attachment found" << LL_ENDL;
        }
        return NULL;
    }
    // Now that we have at least one candidate, we have to decide which one to return
    LLViewerJointAttachment* res = NULL;
    Candidate candidate;
    unsigned int i;
    int ind_res = -1;
    int max_index = -1;
    int max_length = -1;
    // Find the highest index
    for (i=0; i<candidates.size(); ++i) {
        candidate = candidates[i];
        if (candidate.index > max_index) max_index = candidate.index;
    }
    // Find the longest match among the ones found at that index
    for (i=0; i<candidates.size(); ++i) {
        candidate = candidates[i];
        if (candidate.index == max_index) {
            if (candidate.length > max_length) {
                max_length = candidate.length;
                ind_res = i;
            }
        }
    }
    // Return this attachment point
    if (ind_res > -1) {
        candidate = candidates[ind_res];
        res = candidate.attachment;
        if (sRestrainedLoveLogging && res) {
            LL_INFOS() << "returning '" << res->getName() << "'" << LL_ENDL;
        }
    }
    return res;
}

LLViewerJointAttachment* RRInterface::findAttachmentPointFromParentName (LLInventoryItem* item)
{
    if (item) {
        // => look in parent folder (this could be a no-mod item), use its name to find the target
        // attach point
        LLViewerInventoryCategory* cat;
        const LLUUID& parent_id = item->getParentUUID();
        cat = gInventory.getCategory (parent_id);
        return findAttachmentPointFromName (cat->getName());
    }
    return NULL;
}

S32 RRInterface::findAttachmentPointNumber (LLViewerJointAttachment* attachment)
{
    LLVOAvatar* avatar = gAgentAvatarp;
    if (avatar) {
        for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin();
             iter != avatar->mAttachmentPoints.end(); ++iter)
        {
            if (iter->second == attachment) {
                return iter->first;
            }
        }
    }
    return -1;
}
/*
// This function is added here to keep retrocompatibility with 1.x code, since it has now been turned into an event,
// which cannot be called (see LLAttachmentDetachFromPoint::handleEvent() for an example)
bool RRInterface::handle_detach_from_avatar(LLViewerJointAttachment* attachment)
{
    if (attachment->getNumObjects() > 0)
    {
        if (gRRenabled && !gAgent.mRRInterface.canDetach(attachment->getObject()))
        {
            return true;
        }
        gMessageSystem->newMessage("ObjectDetach");
        gMessageSystem->nextBlockFast(_PREHASH_AgentData);
        gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
        gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        
        for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator iter = attachment->mAttachedObjects.begin();
             iter != attachment->mAttachedObjects.end();
             iter++)
        {
            LLViewerObject *attached_object = (*iter);
            gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
            gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, attached_object->getLocalID());
        }
        gMessageSystem->sendReliable( gAgent.getRegionHost() );
    }
    return true;
}
*/
void RRInterface::detachObject(LLViewerObject* object)
{
    // Handle the detach message to the sim here, after a check
    if (!object) return;
    if (gRRenabled && !gAgent.mRRInterface.canDetach(object)) return;

    LLInventoryItem* item = getItem (object->getID());
    if (item) {
        LLAppearanceMgr::getInstance()->removeItemFromAvatar (item->getUUID());
    }
    //gMessageSystem->newMessage("ObjectDetach");
    //gMessageSystem->nextBlockFast(_PREHASH_AgentData);
    //gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
    //gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    //gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
    //gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
    //gMessageSystem->sendReliable( gAgent.getRegionHost() );
}

void RRInterface::detachAllObjectsFromAttachment(LLViewerJointAttachment* attachment)
{
    if (!attachment) return;
    LLViewerObject* object;

    // We need to remove all the objects from attachment->mAttachedObjects, one by one.
    // To do this, and in order to avoid any race condition, we are going to copy the list and 
    // iterate on the copy instead of the original which changes everytime something is
    // attached and detached, asynchronously.
    LLViewerJointAttachment::attachedobjs_vec_t attachedObjectsCopy = attachment ->mAttachedObjects;

    for (unsigned int i = 0; i < attachedObjectsCopy.size(); ++i) {
        object = attachedObjectsCopy.at(i);
        detachObject (object);
    }
}

bool RRInterface::canDetachAllObjectsFromAttachment(LLViewerJointAttachment* attachment)
{
    if (!attachment) return false;
    LLViewerObject* object;

    for (unsigned int i = 0; i <  attachment ->mAttachedObjects.size(); ++i) {
        object =  attachment ->mAttachedObjects.at(i);
        if (!gAgent.mRRInterface.canDetach(object)) return false;
    }

    return true;
}

void RRInterface::fetchInventory (LLInventoryCategory* root)
{
    // do this only once on login

    if (mInventoryFetched) return;
    
    bool last_step = false;
    
    if (root == NULL) {
        root = getRlvShare();
        last_step = true;
    }
    
    if (root) {
        LLViewerInventoryCategory* viewer_root = (LLViewerInventoryCategory*) root;
        viewer_root->fetch ();

        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        
        // retrieve all the shared folders
        gInventory.getDirectDescendentsOf (viewer_root->getUUID(), cats, items);
        if (cats) {
            S32 count = cats->size();
            for(S32 i = 0; i < count; ++i) {
                LLInventoryCategory* cat = (LLInventoryCategory*)cats->at(i);
                fetchInventory (cat);
            }
        }

    }
    
    if (last_step) mInventoryFetched = TRUE;
}

BOOL RRInterface::forceAttach (std::string category, BOOL recursive, AttachHow how)
{
    // recursive is TRUE in the case of an attachall command
    // find the category under RLV shared folder
    if (category == "") return TRUE; // just a safety
    LLInventoryCategory* cat = getCategoryUnderRlvShare (category);
    BOOL isRoot = (getRlvShare() == cat);
    BOOL replacing = (how == AttachHow_replace || how == AttachHow_over_or_replace); // we're replacing for now, but the name of the category could decide otherwise
    
    // if exists, wear all the items inside it
    if (cat) {
    
        // If the name of the category begins with a special string (specified by the user, "+" by default), then we force to stack instead of replacing
        if (how == AttachHow_over_or_replace) {
            if (cat->getName().find (gSavedSettings.getString ("RestrainedLoveStackWhenFolderBeginsWith")) == 0) {
                replacing = FALSE;
            }
        }

        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        
        // retrieve all the objects contained in this folder
        gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
        if (items) {
        
            // wear them one by one
            S32 count = items->size();
            for(S32 i = 0; i < count; ++i) {
                if (!isRoot) {
                    LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->at(i);
                    if (sRestrainedLoveLogging) {
                        LL_INFOS() << "trying to attach " << item->getName() << LL_ENDL;
                    }
                    
                    // this is an object to attach somewhere
                    if (item && item->getType() == LLAssetType::AT_OBJECT) {
                        LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
                        
                        if (attachpt) {
                            if (sRestrainedLoveLogging) {
                                LL_INFOS() << "attaching item to " << attachpt->getName() << LL_ENDL;
                            }
                            if (replacing) {
                                // we're replacing => mimick rez_attachment without displaying an Xml alert to confirm
                                S32 number = findAttachmentPointNumber (attachpt);
                                if (canDetach(attachpt->getName()) && canAttach(item))
                                {
                                    LLSD payload;
                                    payload["item_id"] = item->getLinkedUUID();
                                    payload["attachment_point"] = number;
                                    LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
                                }
                            }
                            else {
                                // we're stacking => call rez_attachment directly
                                rez_attachment (item, attachpt, false);
                            }
                        }
                        else {
                            // attachment point is not in the name => stack
                            rez_attachment (item, attachpt, false);
                        }
                    }
                    // this is a piece of clothing
                    else if (item->getType() == LLAssetType::AT_CLOTHING
                             || item->getType() == LLAssetType::AT_BODYPART) {
//                      LLAppearanceMgr::instance().addCOFItemLink(item);
                        LLAppearanceMgr::instance().wearItemOnAvatar(item->getLinkedUUID(), true, replacing);
                    }
                    // this is a gesture
                    else if (item->getType() == LLAssetType::AT_GESTURE) {
                        if (!LLGestureMgr::instance().isGestureActive(item->getLinkedUUID())) {
                            LLGestureMgr::instance().activateGesture(item->getLinkedUUID());
                        }
                    }
                    // this is an environment settings (EEP) => it is not really attached but activated, replacing the current environment settings (but won't appear "attached" or "activated" in the inventory)
                    else if (item->getType() == LLAssetType::AT_SETTINGS) {
                        if (!mContainsSetenv) {
                            LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, item->getAssetUUID());
                            LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
                        }
                    }
                }
            }
        }
        
        // scan every subfolder of the folder we are attaching, in order to attach no-mod items
        if (cats) {
        
            // for each subfolder, attach the first item it contains according to its name
            S32 count = cats->size();
            for(S32 i = 0; i < count; ++i) {
                LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->at(i);
                LLViewerJointAttachment* attachpt = findAttachmentPointFromName (cat_child->getName());
                
                if (!isRoot) {
                    // this subfolder is properly named => attach the first item it contains
                    LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
                    LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
                    gInventory.getDirectDescendentsOf (cat_child->getUUID(), 
                                                        cats_grandchildren, items_grandchildren);

                    if (items_grandchildren && items_grandchildren->size() == 1) {
                        LLViewerInventoryItem* item_grandchild = 
                                (LLViewerInventoryItem*)items_grandchildren->at(0);

                        if (attachpt) {
                            if (item_grandchild && item_grandchild->getType() == LLAssetType::AT_OBJECT
                                && !item_grandchild->getPermissions().allowModifyBy(gAgent.getID())
                                && findAttachmentPointFromParentName (item_grandchild) != NULL) { // it is no-mod and its parent is named correctly
                                // we use the attach point from the name of the folder, not the no-mod item
                                if (replacing) {
                                    // we're replacing => mimick rez_attachment without displaying an Xml alert to confirm
                                    S32 number = findAttachmentPointNumber (attachpt);
                                    if (canDetach(attachpt->getName()) && canAttach(item_grandchild))
                                    {
                                        LLSD payload;
                                        payload["item_id"] = item_grandchild->getUUID();
                                        payload["attachment_point"] = number;
                                        LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
                                    }
                                }
                                else {
                                    // we're stacking => call rez_attachment directly
                                    rez_attachment (item_grandchild, attachpt, false);
                                }
                            }
                        }
                        else {
                            // attachment point is not in the name => stack
//                          rez_attachment (item_grandchild, attachpt, false);
                        }
                    }
                }

                if (recursive && cat_child->getName().find (".") != 0) { // attachall and not invisible)
                    forceAttach (getFullPath (cat_child), recursive, how);
                }
            }
        }
    }
    return TRUE;
}

BOOL RRInterface::forceDetachByName (std::string category, BOOL recursive)
{
//LLWearableBridge::removeItemFromAvatar(item);     
    // find the category under RLV shared folder
    if (category == "") return TRUE; // just a safety
    LLInventoryCategory* cat = getCategoryUnderRlvShare (category);
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    if (!avatar) return FALSE;
    BOOL isRoot = (getRlvShare() == cat);
    
    // if exists, detach/unwear all the items inside it
    if (cat) {
    
        if (mHandleNoStrip) {
            std::string name = cat->getName();
            LLStringUtil::toLower(name);
            if (name.find ("nostrip") != -1) return false;
        }

        if (recursive) {
            LLUUID cat_id = cat->getLinkedUUID();

            // The following code is almost a carbon copy of LLAppearanceMgr::takeOffOutfit()
            // (which is called when pressing "Remove from Current Outfit" in the inventory).
            // We don't want it recursive all the time though, it depends on the parameter above.
            LLInventoryModel::cat_array_t cats;
            LLInventoryModel::item_array_t items;
            LLFindWearablesEx collector(/*is_worn=*/ true, /*include_body_parts=*/ false);

            gInventory.collectDescendentsRecIf(cat_id, cats, items, recursive, FALSE, collector);

            LLInventoryModel::item_array_t::const_iterator it = items.begin();
            const LLInventoryModel::item_array_t::const_iterator it_end = items.end();
            uuid_vec_t uuids_to_remove;
            for( ; it_end != it; ++it)
            {
                LLViewerInventoryItem* item = *it;
                uuids_to_remove.push_back(item->getUUID());
            }
            LLAppearanceMgr::instance().removeItemsFromAvatar(uuids_to_remove);

            // deactivate all gestures in the outfit folder
            LLInventoryModel::item_array_t gest_items;
            LLAppearanceMgr::instance().getDescendentsOfAssetType(cat_id, gest_items, LLAssetType::AT_GESTURE);
            for(S32 i = 0; i  < gest_items.size(); ++i)
            {
                LLViewerInventoryItem *gest_item = gest_items[i];
                if ( LLGestureMgr::instance().isGestureActive( gest_item->getLinkedUUID()) )
                {
                    LLGestureMgr::instance().deactivateGesture( gest_item->getLinkedUUID() );
                }
            }

        }
        else {
            LLInventoryModel::cat_array_t* cats;
            LLInventoryModel::item_array_t* items;
        
            // retrieve all the objects contained in this folder
            gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
            if (items) {
        
                // unwear them one by one
                S32 count = items->size();
                for(S32 i = 0; i < count; ++i) {
                    if (!isRoot) {
                        LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->at(i);
                        if (sRestrainedLoveLogging) {
                            LL_INFOS() << "trying to detach " << item->getName() << LL_ENDL;
                        }
                    
                        // this is an attached object
                        if (item->getType() == LLAssetType::AT_OBJECT) {
                            // find the attachpoint from which to detach
                            for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
                                 iter != avatar->mAttachmentPoints.end(); )
                            {
                                LLVOAvatar::attachment_map_t::iterator curiter = iter++;
                                LLViewerJointAttachment* attachment = curiter->second;
                                LLViewerObject* object = avatar->getWornAttachment(item->getLinkedUUID());
                                if (attachment->isObjectAttached(object)) {
                                    detachObject (object);
                                    break;
                                }
                            }
                        
                        }
                        // this is a piece of clothing
                        else if (item->getType() == LLAssetType::AT_CLOTHING) {
    //                      const LLWearable* layer = gAgentWearables.getWearableFromItemID (item->getLinkedUUID());
    //                      if (layer != NULL) gAgentWearables.removeWearable (layer->getType(), false, 0);
                            if (canDetach (item)) removeItemFromAvatar(item);       
                        }
                        // this is a gesture
                        else if (item->getType() == LLAssetType::AT_GESTURE) {
                            if (LLGestureMgr::instance().isGestureActive(item->getLinkedUUID())) {
                                LLGestureMgr::instance().deactivateGesture(item->getLinkedUUID());
                            }
                        }
                        // this is an environment settings (EEP) => such an asset is not attached nor detached, but activated or replaced with the region settings
                        else if (item->getType() == LLAssetType::AT_SETTINGS) {
                            if (!mContainsSetenv) {
                                LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_LOCAL);
                                LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
                                LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
                            }
                        }
                    }
                }
            }

            if (cats) {
                // for each subfolder, detach the first item it contains (only for single no-mod items contained in appropriately named folders)
                S32 count = cats->size();
                for(S32 i = 0; i < count; ++i) {
                    LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->at(i);

                    if (mHandleNoStrip) {
                        std::string name = cat_child->getName();
                        LLStringUtil::toLower(name);
                        if (name.find ("nostrip") != -1) continue;
                    }

                    LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
                    LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
                    gInventory.getDirectDescendentsOf (cat_child->getUUID(), 
                                                        cats_grandchildren, items_grandchildren);

                    if (!isRoot && items_grandchildren && items_grandchildren->size() == 1) { // only one item
                        LLViewerInventoryItem* item_grandchild = 
                                (LLViewerInventoryItem*)items_grandchildren->at(0);

                        if (item_grandchild && item_grandchild->getType() == LLAssetType::AT_OBJECT
                            && !item_grandchild->getPermissions().allowModifyBy(gAgent.getID())
                            && findAttachmentPointFromParentName (item_grandchild) != NULL) { // and it is no-mod and its parent is named correctly
                            // detach this object
                            // find the attachpoint from which to detach
                            for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
                                 iter != avatar->mAttachmentPoints.end(); )
                            {
                                LLVOAvatar::attachment_map_t::iterator curiter = iter++;
                                LLViewerJointAttachment* attachment = curiter->second;
                                LLViewerObject* object = avatar->getWornAttachment(item_grandchild->getUUID());
                                if (attachment->isObjectAttached(object)) {
                                    detachObject (object);
                                    break;
                                }
                            }
                        }
                    }

                    if (recursive && cat_child->getName().find (".") != 0) { // detachall and not invisible)
                        forceDetachByName (getFullPath (cat_child), recursive);
                    }
                }
            }
        }
    }
    return TRUE;
}

BOOL RRInterface::forceTeleport(std::string location, const LLVector3& vecLookAt)
{
    // location must be X/Y/Z where X, Y and Z are ABSOLUTE coordinates => use a script in-world to translate from local to global
    // OR it can be Region/X/Y/Z where X, Y and Z are LOCAL coordinates => no need to use a script in-world
    std::string loc (location);
    std::string region_name = "";
    S64 x = 128;
    S64 y = 128;
    S64 z = 0;
    std::deque<std::string> tokens=parse (location, "/", 3);
    if (tokens.size()==3) {
        x=atoi (tokens.at(0).c_str());
        y=atoi (tokens.at(1).c_str());
        z=atoi (tokens.at(2).c_str());
    }
    else if (tokens.size() == 4) {
        region_name = tokens.at(0);
        x = atoi(tokens.at(1).c_str());
        y = atoi(tokens.at(2).c_str());
        z = atoi(tokens.at(3).c_str());
    }
    else {
        return FALSE;
    }

    if (sRestrainedLoveLogging) {
        LL_INFOS() << tokens.at(0) << "," << tokens.at(1) << "," << tokens.at(2) << "     " << x << "," << y << "," << z << LL_ENDL;
    }

    mAllowCancelTp = FALSE; // will be checked once receiving the tp order from the sim, then set to TRUE again

    if (region_name == "")
    {
        LLVector3d pos_global;
        pos_global.mdV[VX] = (F64)x;
        pos_global.mdV[VY] = (F64)y;
        pos_global.mdV[VZ] = (F64)z;

        gAgent.teleportViaLocation(pos_global);
    }
    else {
        LLVector3 pos_local;
        pos_local.mV[VX] = (F32)x;
        pos_local.mV[VY] = (F32)y;
        pos_local.mV[VZ] = (F32)z;

        LLWorldMapMessage::url_callback_t cb = boost::bind(&RRInterface::forceTeleportCallback, _1, pos_local, vecLookAt);
        LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name, cb, std::string(""), true);
    }

    return TRUE;
}

// From KB
void RRInterface::forceTeleportCallback(U64 hRegion, const LLVector3& posRegion, const LLVector3& vecLookAt)
{
    if (hRegion)
    {
        const LLVector3d posGlobal = from_region_handle(hRegion) + (LLVector3d)posRegion;
        if (vecLookAt.isExactlyZero()) {
            gAgent.teleportViaLocation(posGlobal);
        }
        else {
            gAgent.teleportViaLocationLookAt(posGlobal);
        }
    }
}
// from KB

std::string RRInterface::stringReplace(std::string s, std::string what, std::string by, BOOL caseSensitive /* = FALSE */)
{
//  LL_INFOS() << "trying to replace <" << what << "> in <" << s << "> by <" << by << ">" << LL_ENDL;
    if (what == "" || what == " ") return s; // avoid an infinite loop
    int ind;
    int old_ind = 0;
    int len_what = what.length();
    int len_by = by.length();
    if (len_by == 0) len_by = 1; // avoid an infinite loop
    
    //while ((ind = s.find ("%20")) != -1) // unescape
    //{
    //  s = s.replace (ind, 3, " ");
    //}
    
    std::string lower = s;
    if (!caseSensitive) {
        LLStringUtil::toLower (lower);
        LLStringUtil::toLower (what);
    }
    
    while ((ind = lower.find (what, old_ind)) != -1)
    {
//      LL_INFOS() << "ind=" << ind << "    old_ind=" << old_ind << LL_ENDL;
        s = s.replace (ind, len_what, by);
        old_ind = ind + len_by;
        lower = s;
        if (!caseSensitive) LLStringUtil::toLower (lower);
    }
    return s;
    
}

// same as stringReplace, but checks for neighbors of the occurrences of "what", and replace only if these neighbors are NOT alphanum characters
std::string RRInterface::stringReplaceWholeWord(std::string s, std::string what, std::string by, BOOL caseSensitive /* = FALSE */)
{
    //  LL_INFOS() << "trying to replace <" << what << "> in <" << s << "> by <" << by << ">" << LL_ENDL;
    if (what == "" || what == " ") return s; // avoid an infinite loop
    int ind;
    int old_ind = 0;
    int len_what = what.length();
    int len_by = by.length();
    int len_s = s.length();
    if (len_by == 0) len_by = 1; // avoid an infinite loop
    //bool unescaped = false;

    //while ((ind = s.find("%20")) != -1) // unescape but we will have to escape again before returning
    //{
    //  s = s.replace(ind, 3, " ");
    //  unescaped = true;
    //}

    std::string lower = s;
    if (!caseSensitive) {
        LLStringUtil::toLower(lower);
        LLStringUtil::toLower(what);
    }

    while ((ind = lower.find(what, old_ind)) != -1)
    {
        //      LL_INFOS() << "ind=" << ind << "    old_ind=" << old_ind << LL_ENDL;
        char prec = ' ';
        char succ = ' ';
        if (ind > 0) prec = s.at(ind - 1);
        if (ind < len_s - len_what - 1) succ = s.at(ind + len_what);
        if (!isalnum (prec) && prec != '\'' && !isalnum (succ) && succ != '\'')
        {
            s = s.replace(ind, len_what, by);
            len_s = s.length();
            lower = s;
            if (!caseSensitive) LLStringUtil::toLower(lower);
        }
        old_ind = ind + len_by;
    }
    //while (unescaped && (ind = s.find(" ")) != -1) // escape again
    //{
    //  s = s.replace(ind, 1, "%20");
    //}
    return s;

}

std::string RRInterface::getDummyName(std::string name, EChatAudible audible /* = CHAT_AUDIBLE_FULLY */)
{
#ifdef KOKUA_SHOWNAMES
    std::string res = KokuaRLVExtras::getInstance()->kokuaGetCensoredMessage(name, true);
#else
    int len = name.length();
    if (len == 0)
    {
        return "";
    }
    int at = 3;
    if (len <= 3) at = len-1; // just to avoid crashing in some cases
    // We use mLaunchTimestamp in order to modify the scrambling when the session restarts (it stays consistent during the session though)
    // But in crashy situations, let's not make it change at EVERY session, more like once a day or so
    // A day is 86400 seconds, the closest power of two is 65536, that's a 16-bit shift
    unsigned char hash = name.at(at) + len + (mLaunchTimestamp >> 16); // very lame hash function I know... but it should be linear enough (the old length method was way too gaussian with a peak at 11 to 16 characters)
    unsigned char mod = hash % 28;
    std::string res = "";
    switch (mod) {
        case 0:     res = "A resident";         break;
        case 1:     res = "This resident";      break;
        case 2:     res = "That resident";      break;
        case 3:     res = "An individual";      break;
        case 4:     res = "This individual";    break;
        case 5:     res = "That individual";    break;
        case 6:     res = "A person";           break;
        case 7:     res = "This person";        break;
        case 8:     res = "That person";        break;
        case 9:     res = "A stranger";         break;
        case 10:    res = "This stranger";      break;
        case 11:    res = "That stranger";      break;
        case 12:    res = "A human being";      break;
        case 13:    res = "This human being";   break;
        case 14:    res = "That human being";   break;
        case 15:    res = "An agent";           break;
        case 16:    res = "This agent";         break;
        case 17:    res = "That agent";         break;
        case 18:    res = "A soul";             break;
        case 19:    res = "This soul";          break;
        case 20:    res = "That soul";          break;
        case 21:    res = "Somebody";           break;
        case 22:    res = "Anonymous one";      break;
        case 23:    res = "Someone";            break;
        case 24:    res = "Mysterious one";     break;
        case 25:    res = "An unknown being";   break;
        case 26:    res = "Unidentified one";   break;
        default:    res = "An unknown person";  break;
    }
#endif
    if (audible == CHAT_AUDIBLE_BARELY) res += " afar";
    return res;
}

std::string RRInterface::getCensoredMessage (std::string str)
{
#ifdef KOKUA_SHOWNAMES
    return KokuaRLVExtras::getInstance()->kokuaGetCensoredMessage(str, false);
#else
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
    RRMAP::iterator it = mSpecialObjectBehaviours.begin();
    while (it != mSpecialObjectBehaviours.end()) {
        command = it->second;
        LLStringUtil::toLower(command);
        if (command.find("shownames:") == 0 || command.find("shownames_sec:") == 0 || command.find("shownametags:") == 0) {
            if (parseCommand(command, behav, option, param)) {
                LLUUID uuid;
                uuid.set(option, FALSE);
                exceptions.push_back(uuid);
            }
        }
        it++;
    }

    // Hide every occurrence of the name of anybody around (found in cache, so not completely accurate nor completely immediate)
    S32 i;
    for (i=0; i<gObjectList.getNumObjects(); ++i) {
        LLViewerObject* object = gObjectList.getObject(i);
        if (object) {
            LLAvatarName av_name;
            std::string clean_user_name;
            std::string user_name;
            std::string display_name;
            std::string dummy_name;
            
            if (object->isAvatar()) {
                if (std::find(exceptions.begin(), exceptions.end(), object->getID()) == exceptions.end()) // ignore exceptions
                {
                    if (LLAvatarNameCache::get(object->getID(), &av_name))
                    {
                        user_name = av_name.getUserName();
                        clean_user_name = LLCacheName::cleanFullName(user_name);
                        display_name = av_name.mDisplayName; // not "getDisplayName()" because we need this whether we use display names or user names

                        dummy_name = getDummyName(clean_user_name);
                        str = stringReplaceWholeWord(str, clean_user_name, dummy_name);

                        dummy_name = getDummyName(user_name);
                        str = stringReplaceWholeWord(str, user_name, dummy_name);

                        dummy_name = getDummyName(display_name);
                        str = stringReplaceWholeWord(str, display_name, dummy_name);
                    }
                }
            }
        }
    }
    return str;
#endif
}
void updateAndSave (WLColorControl* color)
{
    if (color == NULL) return;
    color->setIntensity(color->getRed());
    if (color->getGreen() > color->getIntensity()) {
        color->setIntensity(color->getGreen());
    }
    if (color->getBlue() > color->getIntensity()) {
        color->setIntensity(color->getBlue());
    }
//  if (color == NULL) return;
//  color->i = color->r;
//  if (color->g > color->i) {
//      color->i = color->g;
//  }
//  if (color->b > color->i) {
//      color->i = color->b;
//  }
//  color->update (LLWLParamManager::getInstance()->mCurParams);
}

void updateAndSave (WLFloatControl* floatControl)
{
//  if (floatControl == NULL) return;
//  floatControl->update (LLWLParamManager::getInstance()->mCurParams);
}

BOOL RRInterface::forceEnvironment (std::string command, std::string option)
{
    // command is "setenv_<something>"

    option = stringReplace(option, "/", ";"); // Catznip uses "/" for some reason, but ";" should be used for consistency ("/" is used for folders and some folder commands have multiple options separated by ";")
    
    double val = atof (option.c_str());

    int length = 7; // size of "setenv_"
    command = command.substr (length);
    //LLWLParamManager* params = LLWLParamManager::getInstance();

    //params->mAnimator.mIsRunning = false;
    //params->mAnimator.mUseLindenTime = false;
    //params->mAnimator.setTimeType(LLWLAnimator::TIME_CUSTOM);
    
    //adapted from llfloaterenvironmentadjust
    LLEnvironment &environment(LLEnvironment::instance());
    bool updatelocal(false);
    LLSettingsSky::ptr_t        psky;
        
    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        if (environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
        {   // We have a full day cycle in the local environment.  Freeze the sky
            psky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL)->buildClone();
            updatelocal = true;
        }
        else
        {   // otherwise we can just use the sky.
            psky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
        }
    }
    else
    {
        psky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL, true)->buildClone();
        updatelocal = true;
    }

    if (updatelocal)
    {
        environment.setEnvironment(LLEnvironment::ENV_LOCAL,  psky, 0);
    }
    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    environment.updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    // Clear the last loaded preset name since we're tweaking it.
    gAgent.mRRInterface.setLastLoadedPreset("");

    if (command == "daytime") {
        if (val > 1.0) val = 1.0;
        if (val >= 0.0) {
            //params->mAnimator.setDayTime(val);
            //params->mAnimator.update(params->mCurParams);
        }
        else {
            environment.clearEnvironment(LLEnvironment::ENV_LOCAL);
            environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
            environment.updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
            //LLWLParamManager::getInstance()->mAnimator.mIsRunning = true;
            ////LLWLParamManager::getInstance()->mAnimator.mUseLindenTime = true;
            //LLWLParamManager::getInstance()->mAnimator.setTimeType(LLWLAnimator::TIME_LINDEN);
            //LLEnvManagerNew::instance().useRegionSettings();
        }
    }
    else if (command == "reset") { // synonym for "daytime:-1"
        environment.clearEnvironment(LLEnvironment::ENV_LOCAL);
        environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
        environment.updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "bluehorizonr") {
        LLColor3 bluehorizon=psky->getBlueHorizon();
        bluehorizon.mV[0] = val * 2;
        psky->setBlueHorizon(bluehorizon);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueHorizon.r = val*2;
        //updateAndSave (&(params->mBlueHorizon));
    }
    else if (command == "bluehorizong") {
        LLColor3 bluehorizon=psky->getBlueHorizon();
        bluehorizon.mV[1] = val * 2;
        psky->setBlueHorizon(bluehorizon);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueHorizon.g = val*2;
        //updateAndSave (&(params->mBlueHorizon));
    }
    else if (command == "bluehorizonb") {
        LLColor3 bluehorizon=psky->getBlueHorizon();
        bluehorizon.mV[2] = val * 2;
        psky->setBlueHorizon(bluehorizon);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueHorizon.b = val*2;
        //updateAndSave (&(params->mBlueHorizon));
    }
    else if (command == "bluehorizoni") {
        LLColor3 bluehorizon=psky->getBlueHorizon();
        F32 old_intensity = llmax(bluehorizon.mV[0], bluehorizon.mV[1], bluehorizon.mV[2]);
        if (val == 0 || old_intensity == 0) {
            bluehorizon.mV[0] = bluehorizon.mV[1] = bluehorizon.mV[2] = val * 2;
        }
        else {
            bluehorizon.mV[0] *= val * 2 / old_intensity;
            bluehorizon.mV[1] *= val * 2 / old_intensity;
            bluehorizon.mV[2] *= val * 2 / old_intensity;
        }
        psky->setBlueHorizon(bluehorizon);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //F32 old_intensity = llmax(params->mBlueHorizon.r, params->mBlueHorizon.g, params->mBlueHorizon.b);
        //if (val == 0 || old_intensity == 0) {
        //  params->mBlueHorizon.r = params->mBlueHorizon.g = params->mBlueHorizon.b = val * 2;
        //}
        //else {
        //  params->mBlueHorizon.r *= val * 2 / old_intensity;
        //  params->mBlueHorizon.g *= val * 2 / old_intensity;
        //  params->mBlueHorizon.b *= val * 2 / old_intensity;
        //}
        //updateAndSave(&(params->mBlueHorizon));
    }
    else if (command == "bluehorizon") {
        LLColor3 bluehorizon = psky->getBlueHorizon();
        std::deque<std::string> tokens = parse(option, ";", 3);
        bluehorizon.mV[0] = atof(tokens.at(0).c_str()) * 2;
        bluehorizon.mV[1] = atof(tokens.at(1).c_str()) * 2;
        bluehorizon.mV[2] = atof(tokens.at(2).c_str()) * 2;
        psky->setBlueHorizon(bluehorizon);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueHorizon.r = val*2;
        //updateAndSave (&(params->mBlueHorizon));
    }

    else if (command == "bluedensityr") {
        LLColor3 bluedensity=psky->getBlueDensity();
        bluedensity.mV[0] = val * 2;
        psky->setBlueDensity(bluedensity);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueDensity.r = val*2;
        //updateAndSave (&(params->mBlueDensity));
    }
    else if (command == "bluedensityg") {
        LLColor3 bluedensity=psky->getBlueDensity();
        bluedensity.mV[1] = val * 2;
        psky->setBlueDensity(bluedensity);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueDensity.g = val*2;
        //updateAndSave (&(params->mBlueDensity));
    }
    else if (command == "bluedensityb") {
        LLColor3 bluedensity=psky->getBlueDensity();
        bluedensity.mV[2] = val * 2;
        psky->setBlueDensity(bluedensity);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueDensity.b = val*2;
        //updateAndSave (&(params->mBlueDensity));
    }
    else if (command == "bluedensityi") {
        LLColor3 bluedensity=psky->getBlueDensity();
        F32 old_intensity = llmax(bluedensity.mV[0], bluedensity.mV[1], bluedensity.mV[2]);
        if (val == 0 || old_intensity == 0) {
            bluedensity.mV[0] = bluedensity.mV[1] = bluedensity.mV[2] = val * 2;
        }
        else {
            bluedensity.mV[0] *= val * 2 / old_intensity;
            bluedensity.mV[1] *= val * 2 / old_intensity;
            bluedensity.mV[2] *= val * 2 / old_intensity;
        }
        psky->setBlueDensity(bluedensity);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //F32 old_intensity = llmax(params->mBlueDensity.r, params->mBlueDensity.g, params->mBlueDensity.b);
        //if (val == 0 || old_intensity == 0) {
        //  params->mBlueDensity.r = params->mBlueDensity.g = params->mBlueDensity.b = val * 2;
        //}
        //else {
        //  params->mBlueDensity.r *= val * 2 / old_intensity;
        //  params->mBlueDensity.g *= val * 2 / old_intensity;
        //  params->mBlueDensity.b *= val * 2 / old_intensity;
        //}
        //updateAndSave(&(params->mBlueDensity));
    }
    else if (command == "bluedensity") {
        LLColor3 bluedensity = psky->getBlueDensity();
        std::deque<std::string> tokens = parse(option, ";", 3);
        bluedensity.mV[0] = atof(tokens.at(0).c_str()) * 2;
        bluedensity.mV[1] = atof(tokens.at(1).c_str()) * 2;
        bluedensity.mV[2] = atof(tokens.at(2).c_str()) * 2;
        psky->setBlueDensity(bluedensity);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mBlueDensity.r = val*2;
        //updateAndSave (&(params->mBlueDensity));
    }

    else if (command == "hazehorizon") {
        psky->setHazeHorizon(val); // original bug, slider only goes up to 1.0
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mHazeHorizon.x = val*2;
        //updateAndSave (&(params->mHazeHorizon));
    }
    else if (command == "hazedensity") {
        psky->setHazeDensity(val * 4); // original bug, slider goes up to 4.0
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mHazeDensity.x = val*2;
        //updateAndSave (&(params->mHazeDensity));
    }

    else if (command == "densitymultiplier") {
        psky->setDensityMultiplier(val / 1000);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mDensityMult.x = val / 1000;
        //params->mDensityMult.mult = 1.0;
        //updateAndSave (&(params->mDensityMult));
//      LLWaterParamManager* water_params = LLWaterParamManager::instance();
//      water_params->mFogDensity.mExp = 5.0;
//      water_params->mFogDensity.update (water_params->mCurParams);
    }
    else if (command == "distancemultiplier") {
        psky->setDistanceMultiplier(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mDistanceMult.x = val;
        //params->mDistanceMult.mult = 1.0;
        //updateAndSave (&(params->mDistanceMult));
//      LLWaterParamManager* water_params = LLWaterParamManager::instance();
//      water_params->mUnderWaterFogMod.mX = 1.0;
//      water_params->mUnderWaterFogMod.update (water_params->mCurParams);
    }
    else if (command == "maxaltitude") {
        psky->setMaxY(val);
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mMaxAlt.x = val;
        //updateAndSave (&(params->mMaxAlt));
    }

    else if (command == "sunmooncolorr" || command == "sunlightcolorr") {
        LLColor3 suncolour=psky->getSunlightColor();
        suncolour.mV[0] = val * 3;
        psky->setSunlightColor(suncolour);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mSunlight.r = val*3;
        //updateAndSave (&(params->mSunlight));
    }
    else if (command == "sunmooncolorg" || command == "sunlightcolorg") {
        LLColor3 suncolour=psky->getSunlightColor();
        suncolour.mV[1] = val * 3;
        psky->setSunlightColor(suncolour);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mSunlight.g = val*3;
        //updateAndSave (&(params->mSunlight));
    }
    else if (command == "sunmooncolorb" || command == "sunlightcolorb") {
        LLColor3 suncolour=psky->getSunlightColor();
        suncolour.mV[2] = val * 3;
        psky->setSunlightColor(suncolour);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mSunlight.b = val*3;
        //updateAndSave (&(params->mSunlight));
    }
    else if (command == "sunmooncolori" || command == "sunlightcolori") {
        LLColor3 suncolor=psky->getSunlightColor();
        F32 old_intensity = llmax(suncolor.mV[0], suncolor.mV[1], suncolor.mV[2]);
        // original bug, *2 used here, *3 elsewhere for sunmooncolor
        if (val == 0 || old_intensity == 0) {
            suncolor.mV[0] = suncolor.mV[1] = suncolor.mV[2] = val * 3;
        }
        else {
            suncolor.mV[0] *= val * 3 / old_intensity;
            suncolor.mV[1] *= val * 3 / old_intensity;
            suncolor.mV[2] *= val * 3 / old_intensity;
        }
        psky->setSunlightColor(suncolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //F32 old_intensity = llmax(params->mSunlight.r, params->mSunlight.g, params->mSunlight.b);
        //if (val == 0 || old_intensity == 0) {
        //  params->mSunlight.r = params->mSunlight.g = params->mSunlight.b = val * 2;
        //}
        //else {
        //  params->mSunlight.r *= val * 2 / old_intensity;
        //  params->mSunlight.g *= val * 2 / old_intensity;
        //  params->mSunlight.b *= val * 2 / old_intensity;
        //}
        //updateAndSave(&(params->mSunlight));
    }
    else if (command == "sunmooncolor" || command == "sunlightcolor") {
        LLColor3 suncolour = psky->getSunlightColor();
        std::deque<std::string> tokens = parse(option, ";", 3);
        suncolour.mV[0] = atof(tokens.at(0).c_str()) * 3;
        suncolour.mV[1] = atof(tokens.at(1).c_str()) * 3;
        suncolour.mV[2] = atof(tokens.at(2).c_str()) * 3;
        psky->setSunlightColor(suncolour);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mSunlight.r = val*3;
        //updateAndSave (&(params->mSunlight));
    }

    else if (command == "ambientr") {
        LLColor3 ambientcolor=psky->getAmbientColor();
        ambientcolor.mV[0] = val * 3;
        psky->setAmbientColor(ambientcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mAmbient.r = val*3;
        //updateAndSave (&(params->mAmbient));
    }
    else if (command == "ambientg") {
        LLColor3 ambientcolor=psky->getAmbientColor();
        ambientcolor.mV[1] = val * 3;
        psky->setAmbientColor(ambientcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mAmbient.g = val*3;
        //updateAndSave (&(params->mAmbient));
    }
    else if (command == "ambientb") {
        LLColor3 ambientcolor=psky->getAmbientColor();
        ambientcolor.mV[2] = val * 3;
        psky->setAmbientColor(ambientcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mAmbient.b = val*3;
        //updateAndSave (&(params->mAmbient));
    }
    else if (command == "ambienti") {
        LLColor3 ambientcolor=psky->getAmbientColor();
        F32 old_intensity = llmax(ambientcolor.mV[0], ambientcolor.mV[1], ambientcolor.mV[2]);
        // original bug, *2 used here, *3 used in r/g/b, use *3 too
        if (val == 0 || old_intensity == 0) {
            ambientcolor.mV[0] = ambientcolor.mV[1] = ambientcolor.mV[2] = val * 3;
        }
        else {
            ambientcolor.mV[0] *= val * 3 / old_intensity;
            ambientcolor.mV[1] *= val * 3 / old_intensity;
            ambientcolor.mV[2] *= val * 3 / old_intensity;
        }
        psky->setAmbientColor(ambientcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "ambient") { // vector3 (we don't care about intensity)
        LLColor3 ambientcolor = psky->getAmbientColor();
        std::deque<std::string> tokens = parse(option, ";", 3);
        ambientcolor.mV[0] = atof(tokens.at(0).c_str()) * 3;
        ambientcolor.mV[1] = atof(tokens.at(1).c_str()) * 3;
        ambientcolor.mV[2] = atof(tokens.at(2).c_str()) * 3;
        psky->setAmbientColor(ambientcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "sunglowfocus") {
        // still needs a fudge to match UI values to internal
        LLColor3 glow=psky->getGlow();
        glow.mV[2] = val * -5; // 5 is SLIDER_SCALE_GLOW_B, negation is intended
        psky->setGlow(glow);
        psky->update();     
        //params->mGlow.b = -val*5;
        //updateAndSave (&(params->mGlow));
    }
    else if (command == "sunglowsize") {
        // still needs a fudge to match UI values to internal
        LLColor3 glow=psky->getGlow();
        glow.mV[0] = (2-val)*20; // 2 is max of UI, 20 is SLIDER_SCALE_GLOW_R
        psky->setGlow(glow);
        psky->update();     
        //params->mGlow.r = (2-val)*20;
        //updateAndSave (&(params->mGlow));
    }
    else if (command == "scenegamma") {
        psky->setGamma(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mWLGamma.x = val;
        //updateAndSave (&(params->mWLGamma));
    }
    //else if (command == "sunmoonposition") {
        // replaced by combined routine below
        //params->mCurParams.setSunAngle (F_TWO_PI * val);
    //}
    //else if (command == "eastangle") {
        // replaced by combined routine below
        //params->mCurParams.setEastAngle (F_TWO_PI * val);
    //}
    else if (command == "sunazim" || command == "sunazimuth") { // sun azimuth
        // The pitch value reported by getEulerAngles is useless here so we need to do it another way : get the yaw, substract it from the final value we want
        // (effectively cancelling the original yaw), and do a global rotate of the initial quaternion by this new value around Z.
        val -= F_TWO_PI * floor((val + F_PI) / F_TWO_PI); // wrap the angle between to -PI..PI (this is better than clamping it)
        LLQuaternion orig_quat = psky->getSunRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        LLQuaternion rotation_world;
        rotation_world.setEulerAngles(0.0, 0.0, val - yaw); // rotation of (val - yaw) radians around Z axis
        rotation_world.normalize();
        LLQuaternion new_quat = orig_quat * rotation_world; // global rotation
        psky->setSunRotation(new_quat);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "sunelev" || command == "sunelevation") { // sun elevation
        // Here it is a bit easier since the reported yaw value is useful. Create a rotation around Y for the pitch, global rotate it by the reported yaw, the result
        // is the rotation we want.
        val = (double)llclamp((F32)val, -F_PI_BY_TWO, F_PI_BY_TWO); // clamp instead of wrapping
        val = -val; // when the sun is above the horizon, the pitch is negative (pitch is a rotation around axis Y, so when the sun is east and we look south, it goes up by following a clockwise path, hence negative)
        LLQuaternion orig_quat = psky->getSunRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        LLQuaternion pitch_quat;
        pitch_quat.setAngleAxis(val, 0, 1, 0);
        LLQuaternion yaw_quat;
        yaw_quat.setAngleAxis(yaw, 0, 0, 1);
        LLQuaternion new_quat = pitch_quat * yaw_quat;
        psky->setSunRotation(new_quat);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "moonazim" || command == "moonazimuth") { // moon azimuth
        // The pitch value reported by getEulerAngles is useless here so we need to do it another way : get the yaw, substract it from the final value we want
        // (effectively cancelling the original yaw), and do a global rotate of the initial quaternion by this new value around Z.
        val -= F_TWO_PI * floor((val + F_PI) / F_TWO_PI); // wrap the angle between to -PI..PI (this is better than clamping it)
        LLQuaternion orig_quat = psky->getMoonRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        LLQuaternion rotation_world;
        rotation_world.setEulerAngles(0.0, 0.0, val - yaw); // rotation of (val - yaw) radians around Z axis
        rotation_world.normalize();
        LLQuaternion new_quat = orig_quat * rotation_world; // global rotation
        psky->setMoonRotation(new_quat);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "moonelev" || command == "moonelevation") { // moon elevation
        // Here it is a bit easier since the reported yaw value is useful. Create a rotation around Y for the pitch, global rotate it by the reported yaw, the result
        // is the rotation we want.
        val = (double)llclamp((F32)val, -F_PI_BY_TWO, F_PI_BY_TWO); // clamp instead of wrapping
        val = -val; // when the sun is above the horizon, the pitch is negative (pitch is a rotation around axis Y, so when the sun is east and we look south, it goes up by following a clockwise path, hence negative)
        LLQuaternion orig_quat = psky->getMoonRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        LLQuaternion pitch_quat;
        pitch_quat.setAngleAxis(val, 0, 1, 0);
        LLQuaternion yaw_quat;
        yaw_quat.setAngleAxis(yaw, 0, 0, 1);
        LLQuaternion new_quat = pitch_quat * yaw_quat;
        psky->setMoonRotation(new_quat);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "sunmoonposition" || command == "eastangle")
    {
        // It's not possible to reliable read back these values because any point on the eastangle/sunmoonposition sphere
        // can be described two ways and it's indeterminate whether the one you read back is the one you were expecting.
        // Instead we remember the previous value used and use it again here alongside the newly specified value.
        LLVector3 east_sun(previousEastAngle * F_TWO_PI, previousSunMoonPosition * F_TWO_PI, 0.0);
        
        // make sure val is in range (0.0-1.0 for both)
        while (val < 0.0)
        {
            val += 1.0;
        }
        while (val > 1.0)
        {
            val -= 1.0;
        }
        
        if (command == "eastangle")
        {
            east_sun.mV[0] = F_TWO_PI * val;
            previousEastAngle = val;
        }
        else // it's sunmoonposition
        {
            east_sun.mV[1] = F_TWO_PI * val;
            previousSunMoonPosition = val;
        }

        LLQuaternion sunquat  = convert_azimuth_and_elevation_to_quat(-east_sun.mV[0], east_sun.mV[1]);
        // original WL moon dir was diametrically opposed to the sun dir
        LLQuaternion moonquat = convert_azimuth_and_elevation_to_quat(-east_sun.mV[0] + F_PI, -east_sun.mV[1]);
        psky->setMoonRotation(moonquat);
        psky->setSunRotation(sunquat);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);   
    }
    else if (command == "starbrightness") {
        psky->setStarBrightness(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setStarBrightness (val);
    }

    else if (command == "cloudcolorr") {
        LLColor3 cloudcolor=psky->getCloudColor();
        cloudcolor.mV[0] = val;
        psky->setCloudColor(cloudcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudColor.r = val;
        //updateAndSave (&(params->mCloudColor));
    }
    else if (command == "cloudcolorg") {
        LLColor3 cloudcolor=psky->getCloudColor();
        cloudcolor.mV[1] = val;
        psky->setCloudColor(cloudcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudColor.g = val;
        //updateAndSave (&(params->mCloudColor));
    }
    else if (command == "cloudcolorb") {
        LLColor3 cloudcolor=psky->getCloudColor();
        cloudcolor.mV[2] = val;
        psky->setCloudColor(cloudcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudColor.b = val;
        //updateAndSave (&(params->mCloudColor));
    }
    else if (command == "cloudcolori") {
        LLColor3 cloudcolor=psky->getCloudColor();
        F32 old_intensity = llmax(cloudcolor.mV[0], cloudcolor.mV[1], cloudcolor.mV[2]);
        // original bug: *2 used here *1 for other setters, change to *1
        if (val == 0 || old_intensity == 0) {
            cloudcolor.mV[0] = cloudcolor.mV[1] = cloudcolor.mV[2] = val;
        }
        else {
            cloudcolor.mV[0] *= val / old_intensity;
            cloudcolor.mV[1] *= val / old_intensity;
            cloudcolor.mV[2] *= val / old_intensity;
        }
        psky->setCloudColor(cloudcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //F32 old_intensity = llmax(params->mCloudColor.r, params->mCloudColor.g, params->mCloudColor.b);
        //if (val == 0 || old_intensity == 0) {
        //  params->mCloudColor.r = params->mCloudColor.g = params->mCloudColor.b = val * 2;
        //}
        //else {
        //  params->mCloudColor.r *= val * 2 / old_intensity;
        //  params->mCloudColor.g *= val * 2 / old_intensity;
        //  params->mCloudColor.b *= val * 2 / old_intensity;
        //}
        //updateAndSave(&(params->mCloudColor));
    }
    else if (command == "cloudcolor") {
        LLColor3 cloudcolor = psky->getCloudColor();
        std::deque<std::string> tokens = parse(option, ";", 3);
        cloudcolor.mV[0] = atof(tokens.at(0).c_str());
        cloudcolor.mV[1] = atof(tokens.at(1).c_str());
        cloudcolor.mV[2] = atof(tokens.at(2).c_str());
        psky->setCloudColor(cloudcolor);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudColor.r = val;
        //updateAndSave (&(params->mCloudColor));
    }

    else if (command == "cloudx" || command == "clouddensityx") {
        LLColor3 clouddetail=psky->getCloudPosDensity1();
        clouddetail.mV[0] = val;
        psky->setCloudPosDensity1(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudMain.r = val;
        //updateAndSave (&(params->mCloudMain));
    }
    else if (command == "cloudy" || command == "clouddensityy") {
        LLColor3 clouddetail=psky->getCloudPosDensity1();
        clouddetail.mV[1] = val;
        psky->setCloudPosDensity1(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudMain.g = val;
        //updateAndSave (&(params->mCloudMain));
    }
    else if (command == "cloudd" || command == "clouddensityd") {
        LLColor3 clouddetail=psky->getCloudPosDensity1();
        clouddetail.mV[2] = val;
        psky->setCloudPosDensity1(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudMain.b = val;
        //updateAndSave (&(params->mCloudMain));
    }
    else if (command == "cloud" || command == "clouddensity") {
        LLColor3 clouddetail = psky->getCloudPosDensity1();
        std::deque<std::string> tokens = parse(option, ";", 3);
        clouddetail.mV[0] = atof(tokens.at(0).c_str());
        clouddetail.mV[1] = atof(tokens.at(1).c_str());
        clouddetail.mV[2] = atof(tokens.at(2).c_str());
        psky->setCloudPosDensity1(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudMain.r = val;
        //updateAndSave (&(params->mCloudMain));
    }

    else if (command == "clouddetailx") {
        LLColor3 clouddetail=psky->getCloudPosDensity2();
        clouddetail.mV[0] = val;
        psky->setCloudPosDensity2(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudDetail.r = val;
        //updateAndSave (&(params->mCloudDetail));
    }
    else if (command == "clouddetaily") {
        LLColor3 clouddetail=psky->getCloudPosDensity2();
        clouddetail.mV[1] = val;
        psky->setCloudPosDensity2(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudDetail.g = val;
        //updateAndSave (&(params->mCloudDetail));
    }
    else if (command == "clouddetaild") {
        LLColor3 clouddetail=psky->getCloudPosDensity2();
        clouddetail.mV[2] = val;
        psky->setCloudPosDensity2(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudDetail.b = val;
        //updateAndSave (&(params->mCloudDetail));
    }
    else if (command == "clouddetail") {
        LLColor3 clouddetail = psky->getCloudPosDensity2();
        std::deque<std::string> tokens = parse(option, ";", 3);
        clouddetail.mV[0] = atof(tokens.at(0).c_str());
        clouddetail.mV[1] = atof(tokens.at(1).c_str());
        clouddetail.mV[2] = atof(tokens.at(2).c_str());
        psky->setCloudPosDensity2(clouddetail);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudDetail.r = val;
        //updateAndSave (&(params->mCloudDetail));
    }

    else if (command == "cloudcoverage") {
        psky->setCloudShadow(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudCoverage.x = val;
        //updateAndSave (&(params->mCloudCoverage));
    }
    else if (command == "cloudscale") {
        psky->setCloudScale(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCloudScale.x = val;
        //updateAndSave (&(params->mCloudScale));
    }

    else if (command == "cloudvariance") {
        //val = (F32)llclamp((double)val, 0.0, 1.0);
        psky->setCloudVariance(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setCloudScrollY (val+10);
    }
    else if (command == "cloudscrollx") {
        psky->setCloudScrollRateX(val + 10);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setCloudScrollX (val+10);
    }
    else if (command == "cloudscrolly") {
        psky->setCloudScrollRateY(val + 10);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setCloudScrollY (val+10);
    }
    else if (command == "cloudscroll") {
        std::deque<std::string> tokens = parse(option, ";", 2);
        psky->setCloudScrollRateX(atof(tokens.at(0).c_str()) + 10);
        psky->setCloudScrollRateY(atof(tokens.at(1).c_str()) + 10);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setCloudScrollX (val+10);
    }

    else if (command == "moisturelevel") {
        //val = (F32)llclamp((double)val, 0.0, 1.0);
        psky->setSkyMoistureLevel(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setCloudScrollY (val+10);
    }
    else if (command == "dropletradius") {
        //val = (F32)llclamp((double)val, 5.0, 1000.0);
        psky->setSkyDropletRadius(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setCloudScrollY (val+10);
    }
    else if (command == "icelevel") {
        //val = (F32)llclamp((double)val, 0.0, 1.0);
        psky->setSkyIceLevel(val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        //params->mCurParams.setCloudScrollY (val+10);
    }
    else if (command == "sunimage" || command == "suntexture") {
        LLUUID id;
        id.set(option, FALSE);
        psky->setSunTextureId(id);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "moonimage" || command == "moontexture") {
        LLUUID id;
        id.set(option, FALSE);
        psky->setMoonTextureId(id);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "cloudimage" || command == "cloudtexture") {
        LLUUID id;
        id.set(option, FALSE);
        psky->setCloudNoiseTextureId(id);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "sunscale") {
        psky->setSunScale((F32)val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "moonscale") {
        psky->setMoonScale((F32)val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    else if (command == "moonbrightness") {
        psky->setMoonBrightness((F32)val);
        psky->update();
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    }
    // sunglowfocus 0-0.5, sunglowsize 0-2, scenegamma 0-10, starbrightness 0-2
    // cloudcolor rgb 0-1, cloudxydensity xyd 0-1, cloudcoverage 0-1, cloudscale 0-1, clouddetail xyd 0-1
    // cloudscrollx 0-1, cloudscrolly 0-1, drawclassicclouds 0/1

    else if (command == "preset" || command == "asset") {
        // Find all the environment assets contained in the Environment folder and all its sub-folders, and nowhere else.
        // If we find at least one which name matches the option, activate it, otherwise do nothing.
        // If the parameter is a UUID, apply immediately without searching in the inventory.
        if (LLUUID::validate(option))
        {
            LLUUID uuid(option);
            LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, uuid);
            LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
        }
        else
        {
    //      params->loadPreset (option);
            LLUUID env_root_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);
            if (env_root_folder_id != LLUUID::null) {
                LLViewerInventoryCategory* env_root_folder = gInventory.getCategory(env_root_folder_id);
                //LL_INFOS() << "env_root_folder : " << env_root_folder->getName() << LL_ENDL;
                LLFindSettings collector;
                LLInventoryModel::cat_array_t cats;
                LLInventoryModel::item_array_t items;
                gInventory.collectDescendentsRecIf(env_root_folder_id, cats, items, TRUE, FALSE, collector);
                LLInventoryModel::item_array_t::const_iterator it = items.begin();
                const LLInventoryModel::item_array_t::const_iterator it_end = items.end();
                LLStringUtil::toLower(option); // make sure we compare without caring about the case
                for (; it_end != it; ++it)
                {
                    LLViewerInventoryItem* item = *it;
                    if (item) {
                        std::string item_name = item->getName();
                        LLStringUtil::toLower(item_name);
                        if (item_name == option) {
                            LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, item->getAssetUUID());
                            LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
                            break;
                        }
                    }
                }
                if (it == it_end)
                {
                    LL_INFOS() << option << " not found in " << env_root_folder->getName() << LL_ENDL;
                }
            }
        }

    }

    return TRUE;
}

std::string RRInterface::getEnvironment (std::string command)
{
    int length = 7; // size of "getenv_"
    command = command.substr (length);
    //LLWLParamManager* params = LLWLParamManager::getInstance();
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
    std::stringstream str; // needs definition here so that preformatting routines can write direct

    if (command == "daytime") {
        /*** NEEDS CHANGE FOR EEP ***
        if (params->mAnimator.mIsRunning && params->mAnimator.getTimeType() == LLWLAnimator::TIME_LINDEN) str << -1;
        else str << params->mAnimator.getDayTime();
        ***/
        // This command is now deprecated so any script that asks the time of day will be given "0" or "1" as a response depending on whether the sun is below or above the horizon respectively,
        // unless the environment is set to "region", in which case it will give "-1" instead.
        if (getLastLoadedPreset() == "")
        {
            str << -1.0;
        }
        else
        {
            // The pitch value reported by getEulerAngles is useless here so we need to do it another way : get the yaw, rotate around Z by the same amount
            // to get the local pitch to become the global pitch, and then use the pitch to determine whether to return 0 or 1 as day time.
            LLQuaternion orig_quat = psky->getSunRotation();
            F32 roll;
            F32 pitch;
            F32 yaw;
            orig_quat.getEulerAngles(&roll, &pitch, &yaw);
            LLQuaternion rotation_world;
            rotation_world.setEulerAngles(0.0, 0.0, -yaw); // rotation of (-yaw) radians around Z axis
            rotation_world.normalize();
            LLQuaternion new_quat = orig_quat * rotation_world; // global rotation
            new_quat.getEulerAngles(&roll, &pitch, &yaw);
            if (roll <= -F_PI_BY_TWO || roll >= F_PI_BY_TWO){
                pitch = -pitch;
            }
            if (pitch > 0.0) // pitch > 0 means the sun is below the horizon
            {
                str << 0.0;
            }
            else
            {
                str << 1.0;
            }
        }
    }

    else if (command == "bluehorizonr") str << (psky->getBlueHorizon().mV[0])/2;
    else if (command == "bluehorizong") str << (psky->getBlueHorizon().mV[1])/2;
    else if (command == "bluehorizonb") str << (psky->getBlueHorizon().mV[2])/2;
    else if (command == "bluehorizoni") str << max(max(psky->getBlueHorizon().mV[0], psky->getBlueHorizon().mV[1]), psky->getBlueHorizon().mV[2]) / 2;
    else if (command == "bluehorizon")  str << (psky->getBlueHorizon().mV[0]) / 2 << ";" << (psky->getBlueHorizon().mV[1]) / 2 << ";" << (psky->getBlueHorizon().mV[2]) / 2;

    else if (command == "bluedensityr") str << (psky->getBlueDensity().mV[0])/2;
    else if (command == "bluedensityg") str << (psky->getBlueDensity().mV[1])/2;
    else if (command == "bluedensityb") str << (psky->getBlueDensity().mV[2])/2;
    else if (command == "bluedensityi") str << max (max (psky->getBlueDensity().mV[0], psky->getBlueDensity().mV[1]), psky->getBlueDensity().mV[2]) / 2;
    else if (command == "bluedensity")  str << (psky->getBlueDensity().mV[0]) / 2 << ";" << (psky->getBlueDensity().mV[1]) / 2 << ";" << (psky->getBlueDensity().mV[2]) / 2;

    else if (command == "hazehorizon")  str << psky->getHazeHorizon();
    else if (command == "hazedensity")  str << psky->getHazeDensity() / 4; //original bug: windlight slider goes up to 4

    else if (command == "densitymultiplier")  str << psky->getDensityMultiplier()*1000;
    else if (command == "distancemultiplier") str << psky->getDistanceMultiplier();
    else if (command == "maxaltitude")        str << psky->getMaxY();

    else if (command == "sunmooncolorr" || command == "sunlightcolorr") str << (psky->getSunlightColor().mV[0])/3;
    else if (command == "sunmooncolorg" || command == "sunlightcolorg") str << (psky->getSunlightColor().mV[1])/3;
    else if (command == "sunmooncolorb" || command == "sunlightcolorb") str << (psky->getSunlightColor().mV[2])/3;
    else if (command == "sunmooncolori" || command == "sunlightcolori") str << max (max (psky->getSunlightColor().mV[0], psky->getSunlightColor().mV[1]), psky->getSunlightColor().mV[2]) / 3;
    else if (command == "sunmooncolor" || command == "sunlightcolor")  str << (psky->getSunlightColor().mV[0]) / 3 << ";" << (psky->getSunlightColor().mV[1]) / 3 << ";" << (psky->getSunlightColor().mV[2]) / 3;

    else if (command == "ambientr") str << (psky->getAmbientColor().mV[0])/3;
    else if (command == "ambientg") str << (psky->getAmbientColor().mV[1])/3;
    else if (command == "ambientb") str << (psky->getAmbientColor().mV[2])/3;
    else if (command == "ambienti") str << max (max (psky->getAmbientColor().mV[0], psky->getAmbientColor().mV[1]), psky->getAmbientColor().mV[2]) / 3;
    else if (command == "ambient")  str << (psky->getAmbientColor().mV[0]) / 3 << ";" << (psky->getAmbientColor().mV[1]) / 3 << ";" << (psky->getAmbientColor().mV[2]) / 3;

    else if (command == "sunglowfocus") str << (-psky->getGlow().mV[2]) / 5; // negation is intended, 5 is SLIDER_SCALE_GLOW_B
    else if (command == "sunglowsize")      str << 2 - psky->getGlow().mV[0] / 20; // 2 is UI size, 20 is SLIDER_SCALE_GLOW_R
    else if (command == "scenegamma")       str << psky->getGamma();

    else if (command == "sunazim" || command == "sunazimuth") { // sun azimuth
        LLQuaternion orig_quat = psky->getSunRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        str << (F64)yaw;
    }
    else if (command == "sunelev" || command == "sunelevation") { // sun elevation
        // The pitch value reported by getEulerAngles is useless here so we need to do it another way : get the yaw, rotate around Z by the same amount
        // to get the local pitch to become the global pitch, and then return the pitch.
        LLQuaternion orig_quat = psky->getSunRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        LLQuaternion rotation_world;
        rotation_world.setEulerAngles(0.0, 0.0, -yaw); // rotation of (-yaw) radians around Z axis
        rotation_world.normalize();
        LLQuaternion new_quat = orig_quat * rotation_world; // global rotation
        new_quat.getEulerAngles(&roll, &pitch, &yaw);
        LL_INFOS() << "roll=" << roll << " pitch=" << pitch << " yaw=" << yaw << LL_ENDL;
        if (roll <= -F_PI_BY_TWO || roll >= F_PI_BY_TWO) {
            pitch = -pitch;
        }
        str << -(F64)pitch; // report the negative pitch
    }
    else if (command == "moonazim" || command == "moonazimuth") { // moon azimuth
        LLQuaternion orig_quat = psky->getMoonRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        str << (F64)yaw;
    }
    else if (command == "moonelev" || command == "moonelevation") { // moon elevation
        // The pitch value reported by getEulerAngles is useless here so we need to do it another way : get the yaw, rotate around Z by the same amount
        // to get the local pitch to become the global pitch, and then return the pitch.
        LLQuaternion orig_quat = psky->getMoonRotation();
        F32 roll;
        F32 pitch;
        F32 yaw;
        orig_quat.getEulerAngles(&roll, &pitch, &yaw);
        LLQuaternion rotation_world;
        rotation_world.setEulerAngles(0.0, 0.0, -yaw); // rotation of (-yaw) radians around Z axis
        rotation_world.normalize();
        LLQuaternion new_quat = orig_quat * rotation_world; // global rotation
        new_quat.getEulerAngles(&roll, &pitch, &yaw);
        if (roll <= -F_PI_BY_TWO || roll >= F_PI_BY_TWO) {
            pitch = -pitch;
        }
        str << -(F64)pitch; // report the negative pitch
    }
    else if (command == "eastangle" || command == "sunmoonposition")
    {
        // It's debatable whether this should return live values read back from the system or the local
        // values used to pair with a setenv command. We'll go the live value route since it's
        // quite possible that things have changed since RLV was last used to write the environment 

        LLVector3 az_alt = convert_direction_to_azimuth_and_elevation(psky->getSunDirection());

        F32 eastangle = -az_alt.mV[0] / F_TWO_PI;
        F32 sunmoonposition = az_alt.mV[1] / F_TWO_PI;
        // make sure both are in 0.0 to 1.0 range and suppress -0 return values
        if (eastangle == 0.0 && std::signbit(eastangle)) eastangle = 0.0; // eliminate -0 returns
        if (sunmoonposition == 0.0 && std::signbit(sunmoonposition)) sunmoonposition = 0.0; // eliminate -0 returns

        while (eastangle < 0.0 ) eastangle+=1.0; 
        while (eastangle > 1.0) eastangle-=1.0; // strictly should be >= however the API says 0.0 to 1.0
        while (sunmoonposition < 0.0) sunmoonposition+=1.0;
        while (sunmoonposition > 1.0) sunmoonposition-=1.0; // strictly should be >= however the API says 0.0 to 1.0

        if (command == "eastangle") str << eastangle << ";" << sunmoonposition;
        else str << sunmoonposition << ";" << eastangle;    
    }
    
    else if (command == "starbrightness")       str << psky->getStarBrightness();
    //else if (command == "starbrightness")     str << params->mCurParams.getStarBrightness();

    else if (command == "cloudcolorr") str << (psky->getCloudColor().mV[0]);
    else if (command == "cloudcolorg") str << (psky->getCloudColor().mV[1]);
    else if (command == "cloudcolorb") str << (psky->getCloudColor().mV[2]);
    else if (command == "cloudcolori") str << max (max (psky->getCloudColor().mV[0], psky->getCloudColor().mV[1]), psky->getCloudColor().mV[2]);
    else if (command == "cloudcolor")  str << (psky->getCloudColor().mV[0]) << ";" << (psky->getCloudColor().mV[1]) << ";" << (psky->getCloudColor().mV[2]);

    else if (command == "cloudx" || command == "clouddensityx")  str << (psky->getCloudPosDensity1().mV[0]);
    else if (command == "cloudy" || command == "clouddensityy")  str << (psky->getCloudPosDensity1().mV[1]);
    else if (command == "cloudd" || command == "clouddensityd")  str << (psky->getCloudPosDensity1().mV[2]);
    else if (command == "cloud" || command == "clouddensity")    str << (psky->getCloudPosDensity1().mV[0]) << ";" << (psky->getCloudPosDensity1().mV[1]) << ";" << (psky->getCloudPosDensity1().mV[2]);

    else if (command == "clouddetailx") str << (psky->getCloudPosDensity2().mV[0]);
    else if (command == "clouddetaily") str << (psky->getCloudPosDensity2().mV[1]);
    else if (command == "clouddetaild") str << (psky->getCloudPosDensity2().mV[2]);
    else if (command == "clouddetail")  str << (psky->getCloudPosDensity2().mV[0]) << ";" << (psky->getCloudPosDensity2().mV[1]) << ";" << (psky->getCloudPosDensity2().mV[2]);

    else if (command == "cloudcoverage")    str << psky->getCloudShadow();
    else if (command == "cloudscale")       str << psky->getCloudScale();
    else if (command == "cloudvariance") str << psky->getCloudVariance();

    else if (command == "cloudscrollx")  str << psky->getCloudScrollRate().mV[0] - 10;
    else if (command == "cloudscrolly")  str << psky->getCloudScrollRate().mV[1] - 10;
    else if (command == "cloudscroll")   str << psky->getCloudScrollRate().mV[0] - 10 << ";" << psky->getCloudScrollRate().mV[1] - 10;

    else if (command == "moisturelevel") str << psky->getSkyMoistureLevel();
    else if (command == "dropletradius") str << psky->getSkyDropletRadius();
    else if (command == "icelevel")      str << psky->getSkyIceLevel();
    else if (command == "sunimage" || command == "suntexture")        str << psky->getSunTextureId().asString();
    else if (command == "moonimage" || command == "moontexture")      str << psky->getMoonTextureId().asString();
    else if (command == "cloudimage" || command == "cloudtexture")    str << psky->getCloudNoiseTextureId().asString();

    else if (command == "sunscale") str << psky->getSunScale();
    else if (command == "moonscale") str << psky->getMoonScale();
    else if (command == "moonbrightness") str << psky->getMoonBrightness();

    else if (command == "preset" || command == "asset") return getLastLoadedPreset();

    return str.str();
}

BOOL RRInterface::forceDebugSetting (std::string command, std::string option)
{
    //  MK: As some debug settings are critical to the user's experience and others
    //  are just useless/not used, we are following a whitelist approach : only allow
    //  certain debug settings to be changed and not all.
    
    // command is "setdebug_<something>"
    
    int length = 9; // size of "setdebug_"
    command = command.substr (length);
    LLStringUtil::toLower(command);

    // Find the index of the command in the list of allowed commands, ignoring the case
    int ind = -1;
    std::string real_command = "";
    std::string tmp;
    int nb = mAllowedSetDebug.size();
    for (int i = 0; i < nb; i++)
    {
        tmp = mAllowedSetDebug.at(i);
        LLStringUtil::toLower(tmp);
        if (tmp == command)
        {
            real_command = mAllowedSetDebug.at(i);
            ind = i;
            break;
        }
    }

    if (ind != -1) {
        if (gSavedSettings.controlExists(real_command))
        {
            // KKA-668 turn off persistence when non-default values are set - adapted from RLVa
            LLControlVariable *pSetting = gSavedSettings.getControl(real_command);
            switch (pSetting->type())
            {
            case TYPE_U32:
                gSavedSettings.setU32(real_command, atoi(option.c_str()));
                break;

            case TYPE_S32:
                gSavedSettings.setS32(real_command, atoi(option.c_str()));
                break;

            case TYPE_F32:
                gSavedSettings.setF32(real_command, atoi(option.c_str()));
                break;

            case TYPE_BOOLEAN:
                gSavedSettings.setBOOL(real_command, atoi(option.c_str()));
                break;

            case TYPE_STRING:
                gSavedSettings.setString(real_command, option.c_str());
                break;

            case TYPE_VEC3:
                //gSavedSettings.setVector3(real_command, atoi(option.c_str()));
                break;

            case TYPE_VEC3D:
                //gSavedSettings.setVector3d(real_command, atoi(option.c_str()));
                break;

            case TYPE_RECT:
                //gSavedSettings.setRect(real_command, atoi(option.c_str()));
                break;

            case TYPE_COL4:
                //gSavedSettings.setColor4(real_command, atoi(option.c_str()));
                break;

            case TYPE_COL3:
                //gSavedSettings.setColor4(real_command, atoi(option.c_str()));
                break;

            default:
                break;
            }
            // KKA-668 turn off persistence if we're messing with RenderResolutionDivisor, logic adapted from RLVa. At the time of writing avatarsex is the only
            // other writable debug control and that's not persistent anyway so it's not worth going the whole way and caching the persistence as RLVa does.
            if (real_command == "RenderResolutionDivisor")
            {
                pSetting->setPersist( (pSetting->isDefault()) ? LLControlVariable::PERSIST_NONDFT : LLControlVariable::PERSIST_NO );
            }
        }
        return TRUE;
    }
    
    return FALSE;
}

std::string RRInterface::getDebugSetting (std::string command)
{
    std::stringstream res;
    int length = 9; // size of "setdebug_"
    command = command.substr(length);
    LLStringUtil::toLower(command);

    // Find the index of the command in the list of allowed commands, ignoring the case
    int ind = -1;
    std::string real_command = "";
    std::string tmp;
    int nb = mAllowedGetDebug.size();
    for (int i = 0; i < nb; i++)
    {
        tmp = mAllowedGetDebug.at(i);
        LLStringUtil::toLower(tmp);
        if (tmp == command)
        {
            real_command = mAllowedGetDebug.at(i);
            ind = i;
            break;
        }
    }

    if (ind != -1) {
        if (gSavedSettings.controlExists(real_command))
        {
            switch (gSavedSettings.getControl(real_command)->type())
            {
            case TYPE_U32:
                res << gSavedSettings.getU32(real_command);
                break;

            case TYPE_S32:
                res << gSavedSettings.getS32(real_command);
                break;

            case TYPE_F32:
                res << gSavedSettings.getF32(real_command);
                break;

            case TYPE_BOOLEAN:
                res << gSavedSettings.getBOOL(real_command);
                break;

            case TYPE_STRING:
                res << gSavedSettings.getString(real_command);
                break;

            case TYPE_VEC3:
                res << gSavedSettings.getVector3(real_command);
                break;

            case TYPE_VEC3D:
                res << gSavedSettings.getVector3d(real_command);
                break;

            case TYPE_RECT:
                res << gSavedSettings.getRect(real_command);
                break;

            case TYPE_COL4:
                res << gSavedSettings.getColor4(real_command);
                break;

            case TYPE_COL3:
                res << gSavedSettings.getColor3(real_command);
                break;

            default:
                break;
            }
        }
    }
    
    return res.str();
}

std::string RRInterface::getFullPath (LLInventoryCategory* cat)
{
    if (cat == NULL) return "";
    LLInventoryCategory* rlv = gAgent.mRRInterface.getRlvShare();
    if (rlv == NULL) return "";
    LLInventoryCategory* res = cat;
    std::deque<std::string> tokens;
    
    while (res && res != rlv) {
        tokens.push_front (res->getName());
        const LLUUID& parent_id = res->getParentUUID();
        res = gInventory.getCategory (parent_id);
    }
    return dumpList2String (tokens, "/");
}

std::string RRInterface::getFullPath (LLInventoryItem* item, std::string option, bool full_list /*= true*/)
{
    if (sRestrainedLoveLogging) {
        LL_INFOS() << "getFullPath(" << (item? item->getName(): "NULL") << ", " << option << ", " << full_list << ")" << LL_ENDL;
    }
    // Returns the path from the shared root to this object, or to the object worn at the attach point or clothing layer pointed by option if any
    if (option != "") {
        item = NULL; // an option is specified => we don't want to check the item that issued the command, but something else that is currently worn (object or clothing)
        LLWearableType::EType wearable_type;

        if (LLUUID::validate(option)) { // if option is a UUID, get the path of the viewer object which has this UUID
            std::deque<std::string> res;
            LLUUID id;
            id.set(option, FALSE);
            item = getItem(id); // we want the viewer object from the UUID, no the inventory object
            if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
            else {
                // We have found the inventory item => add its path to the list
                // it appears to be a recursive call but the level of recursivity is only 2, we won't execute this instruction again in the called method since "option" will be empty
                res.push_back(getFullPath(item, ""));
                if (sRestrainedLoveLogging) {
                    LL_INFOS() << "res=" << dumpList2String(res, ", ") << LL_ENDL;
                }
            }
            return dumpList2String(res, ",");
        }
        else if ((wearable_type = gAgent.mRRInterface.getOutfitLayerAsType(option)) != LLWearableType::WT_INVALID) { // this is a clothing layer => replace item with the piece clothing
            std::deque<std::string> res;
            for (unsigned int i = 0; i < LLWearableData::MAX_CLOTHING_LAYERS; ++i) {
                LLUUID id = gAgentWearables.getWearableItemID (wearable_type, i);
                if (id.notNull()) {
                    item = gInventory.getItem(id);
                    if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
                    else {
                        // We have found the inventory item => add its path to the list
                        // it appears to be a recursive call but the level of recursivity is only 2, we won't execute this instruction again in the called method since "option" will be empty
                        res.push_back (getFullPath (item, ""));
                        if (sRestrainedLoveLogging) {
                            LL_INFOS() << "res=" << dumpList2String(res, ", ") << LL_ENDL;
                        }
                        if (!full_list) break; // old behaviour : we only return the first folder, not a full list
                    }
                }
            }
            return dumpList2String (res, ",");
        }
        else { // this is not a clothing layer => it has to be an attachment point
            // Since 2.1, we need to browse through the list of attached objects, and we'll return their respective folders in a list speratated by commas
            LLViewerJointAttachment* attach_point = gAgent.mRRInterface.findAttachmentPointFromName (option, TRUE);
            if (attach_point) {
                std::deque<std::string> res;
                for (unsigned int i = 0; i < attach_point->mAttachedObjects.size(); ++i) {
                    LLViewerObject* attached_object = attach_point->mAttachedObjects.at(i);
                    if (attached_object) {
                        item = getItemAux (attached_object, gAgent.mRRInterface.getRlvShare());
                        if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
                        else {
                            // We have found the inventory item => add its path to the list
                            // it appears to be a recursive call but the level of recursivity is only 2, we won't execute this instruction again in the called method since "option" will be empty
                            res.push_back (getFullPath (item, ""));
                            if (sRestrainedLoveLogging) {
                                LL_INFOS() << "res=" << dumpList2String(res, ", ") << LL_ENDL;
                            }
                            if (!full_list) break; // old behaviour : we only return the first folder, not a full list
                        }
                    }
                }
                return dumpList2String (res, ",");
            }
        }
    }
    
    if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
    if (item == NULL) return "";
    LLUUID parent_id = item->getParentUUID();
    LLInventoryCategory* parent_cat = gInventory.getCategory (parent_id);
    
    if (item->getType() == LLAssetType::AT_OBJECT && !item->getPermissions().allowModifyBy(gAgent.getID())) {
        if (gAgent.mRRInterface.findAttachmentPointFromName(parent_cat->getName()) != NULL) {
            // this item is no-mod and its parent folder contains the name of an attach point
            // => probably we want the full path only to the containing folder of that folder
            parent_id = parent_cat->getParentUUID();
            parent_cat = gInventory.getCategory (parent_id);
            return getFullPath (parent_cat);
        }
    }
    
    return getFullPath (parent_cat);
}


LLInventoryItem* RRInterface::getItemAux (LLViewerObject* attached_object, LLInventoryCategory* root)
{
    // auxiliary function for getItem()
    if (!attached_object) return NULL;
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    if (root && avatar) {
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
        S32 count;
        S32 i;
        LLInventoryItem* item = NULL;
        LLInventoryCategory* cat = NULL;
        
        // Try to find the item in the current category
        count = items->size();
        for(i = 0; i < count; ++i) {
            item = items->at(i);
            if (item 
                && (item->getType() == LLAssetType::AT_OBJECT || item->getType() == LLAssetType::AT_CLOTHING)
                && avatar->getWornAttachment (item->getLinkedUUID()) == attached_object
                ) {
                // found the item in the current category
                return item;
            }
        }
        
        // We didn't find it here => browse the children categories
        count = cats->size();
        for(i = 0; i < count; ++i) {
            cat = cats->at(i);
            item = getItemAux (attached_object, cat);
            if (item != NULL) return item;
        }
    }
    // We didn't find it (this should not happen)
    return NULL;
}

LLInventoryItem* RRInterface::getItem (LLUUID wornObjectUuidInWorld)
{
    // return the inventory item corresponding to the viewer object which UUID is "wornObjectUuidInWorld", if any
    LLViewerObject* object = gObjectList.findObject (wornObjectUuidInWorld);
    if (object != NULL) {
        object = object->getRootEdit();
        if (object->isAttachment()) {
            return gInventory.getItem(object->getAttachmentItemID());
            //return getItemAux (object, gInventory.getCategory(gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY))); //gAgent.mRRInterface.getRlvShare());
        }
    }
    // This object is not worn => it has nothing to do with any inventory item
    return NULL;
}

void RRInterface::attachObjectByUUID (LLUUID assetUUID, int attachPtNumber)
{
    // caution : this method does NOT check that the target attach point is already used by a locked item
    LLSD payload;
    payload["item_id"] = assetUUID;
    payload["attachment_point"] = attachPtNumber;
    LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
}

bool RRInterface::canDetachAllSelectedObjects ()
{
    for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
         iter != LLSelectMgr::getInstance()->getSelection()->end(); )
    {
        LLObjectSelection::iterator curiter = iter++;
        LLViewerObject* object = (*curiter)->getObject();
        if (object && !canDetach(object))
        {
            return false;
        }
    }
    return true;
}

bool RRInterface::isSittingOnAnySelectedObject()
{
    if (gAgentAvatarp && !gAgentAvatarp->mIsSitting) {
        return false;
    }
    
    for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
         iter != LLSelectMgr::getInstance()->getSelection()->end(); )
    {
        LLObjectSelection::iterator curiter = iter++;
        LLViewerObject* object = (*curiter)->getObject();
        if (object && object->isSeat())
        {
            return true;
        }
    }
    return false;
}

bool RRInterface::canAttachCategory(LLInventoryCategory* folder, bool with_exceptions /*= true*/)
{
    // return false if :
    // - at least one object issued a @attachthis:folder restriction
    // - at least one item in this folder is to be worn on a @attachthis:attachpt restriction
    // - at least one piece of clothing in this folder is to be worn on a @attachthis:layer restriction
    // - any parent folder returns false with @attachallthis
    if (!folder) return true;
    if (IsInventoryFolderNew(folder)) return true;
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    if (!avatar) return true;
    LLInventoryCategory* rlvShare = getRlvShare();
    bool shared = isUnderRlvShare(folder);
    if (!rlvShare || !shared) {
        if (contains ("unsharedwear")) return false;
        else return true;
    }
    else {
        if (contains("sharedwear")) return false;
    }
    return canAttachCategoryAux(folder, false, false, with_exceptions);
}

bool RRInterface::canAttachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions /*= true*/)
{
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    if (!avatar) return true;
    FolderLock folder_lock = FolderLock_unlocked;
    if (folder) {
        // check @attachthis:folder in all restrictions
        //RRMAP::iterator it = mSpecialObjectBehaviours.begin (); //unused variable clang
        //LLInventoryCategory* restricted_cat;
        std::string path_to_check;
        std::string restriction = "attachthis";
        if (in_parent) restriction = "attachallthis";
        folder_lock = isFolderLockedWithoutException(folder, "attach");
        if (folder_lock == FolderLock_locked_without_except) {
            return false;
        }

        if (!with_exceptions && folder_lock == FolderLock_locked_with_except) {
            return false;
        }

        //while (it != mSpecialObjectBehaviours.end()) {
        //  if (it->second.find (restriction+":") == 0) {
        //      path_to_check = it->second.substr (restriction.length()+1); // remove ":" as well
        //      restricted_cat = getCategoryUnderRlvShare(path_to_check);
        //      if (restricted_cat == folder) return false;
        //  }
        //  it++;
        //}

        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf (folder->getUUID(), cats, items);
        S32 count;
        S32 i;
        LLInventoryItem* item = NULL;
        LLInventoryCategory* cat = NULL;
        
        // Try to find the item in the current category
        count = items->size();
        for(i = 0; i < count; ++i) {
            item = items->at(i);
            if (item) {
                if (item->getType() == LLAssetType::AT_OBJECT) {
                    LLViewerJointAttachment* attachpt = NULL;
                    if (in_no_mod) {
                        if (item->getPermissions().allowModifyBy(gAgent.getID()) || count > 1) return true;
                        LLInventoryCategory* parent = gInventory.getCategory (folder->getParentUUID());
                        attachpt = findAttachmentPointFromName(parent->getName());
                    }
                    else {
                        attachpt = findAttachmentPointFromName(item->getName());
                    }
                    if (attachpt && contains (restriction+":"+attachpt->getName())) return false;
                }
                else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
                    const LLWearable* wearable = gAgentWearables.getWearableFromItemID (item->getLinkedUUID());
                    if (wearable) {
                        if (contains(restriction+":"+getOutfitLayerAsString(wearable->getType()))) return false;
                    }
                }
            }
        }

        // now check all no-mod items => look at the sub-categories and return false if any of them returns false on a call to canAttachCategoryAux()
        count = cats->size();
        for(i = 0; i < count; ++i) {
            cat = cats->at(i);
            if (cat) {
                std::string name = cat->getName();
                if (name != "" && name[0] ==  '.' && findAttachmentPointFromName(name) != NULL) {
                    if (!canAttachCategoryAux(cat, false, true, with_exceptions)) return false;
                }
            }
        }
    }
    if (folder == getRlvShare()) return true;
    if (!in_no_mod && folder_lock == FolderLock_unlocked) {
        return canAttachCategoryAux(gInventory.getCategory (folder->getParentUUID()), true, false, with_exceptions); // check for @attachallthis in the parent
    }
    return true;
}

bool RRInterface::canDetachCategory(LLInventoryCategory* folder, bool with_exceptions)
{
    // return false if :
    // - at least one object contained in this folder issued a @detachthis restriction
    // - at least one object issued a @detachthis:folder restriction
    // - at least one worn attachment in this folder is worn on a @detachthis:attachpt restriction
    // - at least one worn piece of clothing in this folder is worn on a @detachthis:layer restriction
    // - any parent folder returns false with @detachallthis
    if (!folder) return true;
    std::string name = folder->getName();
    if (IsInventoryFolderNew(folder) && name.length() > 0 && name.at(0) != '~') return true; // if folder is new and it is not a temporary RLV folder
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    if (!avatar) return true;
    LLInventoryCategory* rlvShare = getRlvShare();
    bool shared = isUnderRlvShare(folder);
    if (!rlvShare || !shared) {
        if (contains("unsharedunwear")) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        if (contains("sharedunwear")) return false;
    }
    return canDetachCategoryAux(folder, false, false, with_exceptions);
}

bool RRInterface::canDetachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions)
{
    LLVOAvatarSelf* avatar = gAgentAvatarp;
    if (!avatar) return true;
    FolderLock folder_lock = FolderLock_unlocked;
    if (folder) {
        if (mHandleNoStrip) {
            std::string name = folder->getName();
            LLStringUtil::toLower(name);
            if (name.find ("nostrip") != -1 && (!mUserUpdateAttachmentsUpdatesAll || mUserUpdateAttachmentsCalledFromScript)) return false;
        }
        // check @detachthis:folder in all restrictions
        //RRMAP::iterator it = mSpecialObjectBehaviours.begin (); //unused variable in clang
        //LLInventoryCategory* restricted_cat;
        std::string path_to_check;
        std::string restriction = "detachthis";
        if (in_parent) restriction = "detachallthis";
        folder_lock = isFolderLockedWithoutException(folder, "detach");
        if (folder_lock == FolderLock_locked_without_except) {
            return false;
        }

        if (!with_exceptions && folder_lock == FolderLock_locked_with_except) {
            return false;
        }

        //while (it != mSpecialObjectBehaviours.end()) {
        //  if (it->second.find (restriction+":") == 0) {
        //      path_to_check = it->second.substr (restriction.length()+1); // remove ":" as well
        //      restricted_cat = getCategoryUnderRlvShare(path_to_check);
        //      if (restricted_cat == folder) return false;
        //  }
        //  it++;
        //}

        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf (folder->getUUID(), cats, items);
        S32 count;
        S32 i;
        LLInventoryItem* item = NULL;
        LLInventoryCategory* cat = NULL;
        
        // Try to find the item in the current category
        count = items->size();
        for(i = 0; i < count; ++i) {
            item = items->at(i);
            if (item) {
                if (item->getType() == LLAssetType::AT_OBJECT) {
                    if (in_no_mod) {
                        if (item->getPermissions().allowModifyBy(gAgent.getID()) || count > 1) return true;
                    }
                    LLViewerObject* attached_object = avatar->getWornAttachment (item->getLinkedUUID());
                    if (attached_object) {
                        std::string attach_point_name;
                        if (avatar->getAttachedPointName(item->getLinkedUUID(), attach_point_name))
                        {
                            if (!isAllowed(attached_object->getRootEdit()->getID(), restriction)) return false;
                            if (!in_parent && !isAllowed(attached_object->getRootEdit()->getID(), "detachallthis")) return false; // special case for objects contained into this folder and that issued a @detachallthis command without any parameter without issuing a @detachthis command along with it
                            if (contains (restriction+":"+attach_point_name)) return false;
                        }
                    }
                }
                else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
                    const LLWearable* wearable = gAgentWearables.getWearableFromItemID (item->getLinkedUUID());
                    if (wearable) {
                        if (contains(restriction+":"+getOutfitLayerAsString(wearable->getType()))) return false;
                    }
                }
            }
        }

        // now check all no-mod items => look at the sub-categories and return false if any of them returns false on a call to canDetachCategoryAux()
        count = cats->size();
        for(i = 0; i < count; ++i) {
            cat = cats->at(i);
            if (cat) {
                std::string name = cat->getName();
                if (name != "" && name[0] ==  '.' && findAttachmentPointFromName(name) != NULL) {
                    if (!canDetachCategoryAux(cat, false, true, with_exceptions)) return false;
                }
            }
        }
    }
    if (folder == getRlvShare()) return true;
    if (!in_no_mod && folder_lock == FolderLock_unlocked) {
        return canDetachCategoryAux(gInventory.getCategory (folder->getParentUUID()), true, false, with_exceptions); // check for @detachallthis in the parent
    }
    return true;
}

bool RRInterface::canUnwear(LLInventoryItem* item)
{
    if (item) {
        LLInventoryCategory* parent = gInventory.getCategory (item->getParentUUID());
        if (item->getType() ==  LLAssetType::AT_OBJECT) {
            if (!canDetach (item)) return false;
        }
        else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
            const LLViewerInventoryItem *vitem = dynamic_cast<const LLViewerInventoryItem*>(item);
            if (vitem) {
                if (!canUnwear (vitem->getWearableType())) return false;
            }
            if (!canDetachCategory (parent, true)) return false;
        }
    }
    return true;
}

bool RRInterface::canUnwear(LLWearableType::EType type)
{
    if (contains ("remoutfit")) {
        return false;
    }
    else if (contains ("remoutfit:"+getOutfitLayerAsString (type))) {
        return false;
    }
    return true;
}

bool RRInterface::canWear(LLInventoryItem* item)
{
    // CA: Apply the same defensive measures here as canWear(LLWearableType)
    if (!gAgentAvatarp) return true;

    // If we are still a cloud, allow to wear whatever the restrictions (we are probably logging on)
    if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
    {
        return true;
    }
    if (item) {
        // If the item was just received, let the user wear it
        if (isInventoryItemNew(item)) {
            return true;
        }
        LLInventoryCategory* parent = gInventory.getCategory (item->getParentUUID());
        if (item->getType() ==  LLAssetType::AT_OBJECT) {
            LLViewerJointAttachment* attachpt = findAttachmentPointFromName(item->getName());
            if (attachpt) {
                if (!canAttach (NULL, attachpt->getName())) return false;
            }
            if (!canAttachCategory (parent)) return false;
        }
        else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
            const LLViewerInventoryItem *vitem = dynamic_cast<const LLViewerInventoryItem*>(item);
            if (vitem) {
                //LLWearableType::EType type = vitem->getWearableType();
                //if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
                //{
                //  if (type == LLWearableType::WT_SHAPE
                //      || type == LLWearableType::WT_HAIR
                //      || type == LLWearableType::WT_EYES
                //      || type == LLWearableType::WT_SKIN
                //  ) {
                //      return true;
                //  }
                //}
                if (!canWear (vitem->getWearableType())) return false;
            }
            if (!canAttachCategory (parent)) return false;
        }
    }
    return true;
}

bool RRInterface::canWear(LLWearableType::EType type, bool from_server /*= false*/)
{

    // If we are still a cloud, allow to wear whatever the restrictions (we are probably logging on)
    
    // CA: I have a strong suspicion that coming through here before gAgentAvatarp is valid is the cause
    // of various cloud rezzing without a skin/shape etc issues. Even if that proves to be wrong, it's 
    // a good defensive measure since the cloud test won't have the intended effect if gAgentAvatarp
    // isn't valid yet
    if (!gAgentAvatarp) return true;
        
    if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
    {
        return true;
    }
    //// If we are still a cloud, we can always wear bodyparts because we are still logging on
    //if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
    //{
    //  if (type == LLWearableType::WT_SHAPE
    //      || type == LLWearableType::WT_HAIR
    //      || type == LLWearableType::WT_EYES
    //      || type == LLWearableType::WT_SKIN
    //  ) {
    //      return true;
    //  }
    //}
    // If from_server is true, return true because we are probably trying to wear our bodyparts while logging on
    else if (from_server) {
        return true;
    }
    else if (contains ("addoutfit")) {
        return false;
    }
    else if (contains ("addoutfit:"+getOutfitLayerAsString (type))) {
        return false;
    }
    return true;
}

bool RRInterface::canDetach(LLInventoryItem* item)
{
//  if (!scriptsEnabled() && !getScriptsEnabledOnce()) return false;
    if (item == NULL) {
        return true;
    }
    LLInventoryItem* linked_item = gInventory.getLinkedItem(item->getUUID());
    if (!linked_item) {
        return true;
    }
    LLVOAvatarSelf* avatarp = gAgentAvatarp;
    if (!avatarp) return true;
    
    if (mHandleNoStrip) {
        std::string name = item->getName();
        LLStringUtil::toLower(name);
        if (name.find ("nostrip") != -1) return false;
    }

    if (item->getType() == LLAssetType::AT_OBJECT) {
        // we'll check canDetachCategory() inside this function
        return canDetach (avatarp->getWornAttachment(linked_item->getUUID()));
    }
    else if (item->getType() == LLAssetType::AT_CLOTHING) {
        LLInventoryCategory* cat_parent = gInventory.getCategory (linked_item->getParentUUID());
        if (cat_parent && !canDetachCategory(cat_parent, true)) return false;
        const LLWearable* wearable = gAgentWearables.getWearableFromItemID(linked_item->getUUID ());
        if (wearable) return canUnwear (wearable->getType());
        return true;
    }
    return true;
}

bool RRInterface::canDetach(LLViewerObject* attached_object)
{
    std::string res=canDetachWithExplanation(attached_object);
    if (res=="removable") return true;
    return false;
}
    
    
std::string RRInterface::canDetachWithExplanation(LLViewerObject* attached_object)
{
//  if (!scriptsEnabled() && !getScriptsEnabledOnce()) return false;
    if (attached_object == NULL) return "removable";
    LLViewerObject* root = attached_object->getRootEdit();
    if (root == NULL) return "removable";

    // Check all the current restrictions, if "detach" is issued from a child prim of the root prim of
    // attached_object, then the whole object is undetachable
    RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
    while (it != mSpecialObjectBehaviours.end()) {
        if (it->second == "detach") {
            LLViewerObject* this_prim = gObjectList.findObject(LLUUID(it->first));
            //CA: root can be returned as self, so check before returning
            if (this_prim && (this_prim->getRootEdit() == root)) {
                if (this_prim == root) return "@detach";
                return "@detach (non-root)";
            }
        }
        it++;
    }

//  if (!isAllowed (attached_object->getRootEdit()->getID(), "detach", FALSE)) return false;
    if (!isAllowed (attached_object->getID(), "detach", FALSE)) return "@detach";
    if (!isAllowed (attached_object->getID(), "detachthis", FALSE)) return "@detachthis";
    if (!isAllowed (attached_object->getID(), "detachallthis", FALSE)) return "@detachallthis";

    LLInventoryItem* item = getItem (attached_object->getRootEdit()->getID());

    if (item) {
        LLInventoryCategory* cat_parent = gInventory.getCategory (item->getParentUUID());
        std::string cat_parent_name = "";
        if (cat_parent) {
            cat_parent_name = cat_parent->getName();
        }

        // If the item has just been received, let the user detach it (we know it has not issued a @detach restriction already)
        if (isInventoryItemNew(item) && cat_parent_name.length() > 0 && cat_parent_name.at(0) != '~') return "removable"; // we must check for temp RLV folders here too, not only in canDetachCategory(), because we can't know which one we will check first

        if (cat_parent && !canDetachCategory(cat_parent, true)) return "folder locked";

        if (mHandleNoStrip) {
            std::string name = item->getName();
            LLStringUtil::toLower(name);
            if (name.find ("nostrip") != -1) return "nostrip in name";
        }

        LLVOAvatarSelf* avatarp = gAgentAvatarp;
        if (avatarp) {
            std::string attachpt;
            if (avatarp->getAttachedPointName(item->getLinkedUUID(), attachpt))
            {
                if (contains("detach:"+attachpt)) return "@detach:"+attachpt;
                if (contains("remattach")) return "@remattach";
                if (contains("remattach:"+attachpt)) return "@remattach:"+attachpt;
            }
    //              if (!canDetach(attachpt)) return false;
        }
    }
    return "removable";
}

bool RRInterface::canDetach(std::string attachpt)
{
    std::string res=canDetachWithExplanation(attachpt);
    if (res=="unlocked") return true;
    return false;
}

std::string RRInterface::canDetachWithExplanation(std::string attachpt)
{
//  if (!scriptsEnabled() && !getScriptsEnabledOnce()) return false;
    LLStringUtil::toLower(attachpt);
    if (contains("detach:"+attachpt)) return "@detach:"+attachpt;
    if (contains("remattach")) return "@remattach";
    if (contains("remattach:"+attachpt)) return "@remattach:"+attachpt;
    LLViewerJointAttachment* attachment = findAttachmentPointFromName (attachpt, TRUE);
    if (!canDetachAllObjectsFromAttachment (attachment)) return "some items locked";
    return "unlocked";
}

bool RRInterface::canAttach(LLViewerObject* object_to_attach, std::string attachpt, bool from_server /* = false */)
{
        std::string res=canAttachWithExplanation(object_to_attach, attachpt, from_server);
        if (res=="unlocked") return TRUE;
        return FALSE;
}

std::string RRInterface::canAttachWithExplanation(LLViewerObject* object_to_attach, std::string attachpt, bool from_server /* = false */)
{
    // If from_server == true, the check is done while receiving an order from the server => always allow
    if (from_server) return "unlocked";

    // Attention : this function does not check if we are replacing and there is a locked object already present on the attachment point
    LLStringUtil::toLower(attachpt);
    if (contains("addattach")) return "@addattach";
    if (contains("addattach:"+attachpt)) return "@addattach:"+attachpt;
    if (object_to_attach) {
        LLInventoryItem* item = getItem(object_to_attach->getRootEdit()->getID());
        if (item) {
            // If the item has just been received, let the user attach it
            if (isInventoryItemNew(item)) return "unlocked";

            LLInventoryCategory* cat_parent = gInventory.getCategory (item->getParentUUID());
            if (cat_parent && !canAttachCategory(cat_parent)) return "folder lock";
        }
    }
    
    return "unlocked";
}

bool RRInterface::canAttach(LLInventoryItem* item, bool from_server /* = false */)
{
    // If from_server == true, the check is done while receiving an order from the server => always allow
    if (from_server) return true;

    // If the item has just been received, let the user attach it
    if (isInventoryItemNew(item)) return true;

    if (contains("addattach")) return false;
    if (!item) return true;
    LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
    if (attachpt && contains("addattach:"+attachpt->getName())) return false;
    LLInventoryCategory* cat_parent = gInventory.getCategory (item->getParentUUID());
    if (cat_parent && !canAttachCategory(cat_parent)) return false;
    return true;
}

bool RRInterface::canEdit(LLViewerObject* object)
{
    if (!object || !object->getRootEdit()) return false;
    if (containsWithoutException ("edit", object->getRootEdit()->getID().asString())) return false;
    if (!object->isAttachment() && contains("editworld")) return false;
    if (object->isAttachment() && contains("editattach")) return false;
    if (contains ("editobj:"+object->getRootEdit()->getID().asString())) return false;
    if (!object->isHUDAttachment() && mContainsInteract) return false;
    return true;
}

bool RRInterface::canTouch(LLViewerObject* object, LLVector3 pick_intersection /* = LLVector3::zero */)
{
    if (!object) return true;

    LLViewerObject* root = object->getRootEdit();
    if (!root) return true;
    LLUUID rootID = root->getID();
    std::string sRootID = rootID.asString();
    bool isHUD = root->isHUDAttachment();

    // CA turn off logging on this since it gets called a lot for the pointer hover code
    if (!isAllowed (rootID, "touchme", false)) return true; // to check the presence of "touchme" on this object, which means that we can touch it

    if (!isHUD && gAgent.mRRInterface.mContainsTouchall) return false;

    if (root->isAttachment()) {
        if (!isHUD) {
            if (gAgent.mRRInterface.mContainsTouchattach) return false;

            LLInventoryItem* inv_item = getItem (rootID);
            if (inv_item) { // this attachment is in my inv => it belongs to me
                if (gAgent.mRRInterface.mContainsTouchattachself) return false;
            }
            else { // this attachment is not in my inv => it does not belong to me
                LLVOAvatar* av = root->getAvatar();
                if (gAgent.mRRInterface.mContainsTouchattachother) return false;
                else if (av != NULL && contains ("touchattachother:" + av->getID().asString())) return false;
            }
        }
        else
        {
            if (containsWithoutException("touchhud", sRootID)) return false;
        }
    }
    else {
        if (containsWithoutException ("touchworld", sRootID)) return false;
        if (!canTouchFar (object, pick_intersection)) return false;
    }
    if (contains ("touchthis:"+sRootID)) return false;

    return true;
}

bool RRInterface::canTouchFar(LLViewerObject* object, LLVector3 pick_intersection /* = LLVector3::zero */)
{
    if (!object || object->isHUDAttachment()) return true;

    if (mContainsInteract) return false;

    LLVector3 pos = object->getPositionRegion ();
    if (pick_intersection != LLVector3::zero) pos = pick_intersection;
    pos -= gAgent.getPositionAgent ();
    F32 dist = pos.magVec();
    
    return !(dist > mFartouchMax);
}

bool RRInterface::isInventoryItemNew(LLInventoryItem* item)
{
    // Return true if the item or its parent category has been received during this session
    if (!item) return false; // just in case
    std::string name = item->getName();
    std::string parent_name = "";
    LLUUID parent_uuid = item->getParentUUID();
    LLInventoryCategory* parent = gInventory.getCategory(parent_uuid);
    if (parent) {
        parent_name = parent->getName();
    }
    size_t size = gAgent.mRRInterface.mReceivedInventoryObjects.size();
    for (size_t i = 0; i < size; i++) {
        if (gAgent.mRRInterface.mReceivedInventoryObjects[i] == name
            || gAgent.mRRInterface.mReceivedInventoryObjects[i] == parent_name) {
            return true;
        }
    }

    return false;
}

bool RRInterface::IsInventoryFolderNew(LLInventoryCategory* folder)
{
    // Return true if the folder has been received during this session
    if (!folder) return false; // just in case
    std::string name = folder->getName();
    size_t size = gAgent.mRRInterface.mReceivedInventoryObjects.size();
    for (size_t i = 0; i < size; i++) {
        if (gAgent.mRRInterface.mReceivedInventoryObjects[i] == name) {
            return true;
        }
    }

    return false;
}




bool RRInterface::scriptsEnabled()
{
    LLViewerParcelMgr* vpm = LLViewerParcelMgr::getInstance();
    LLViewerRegion* agent_region = gAgent.getRegion();
    LLParcel* agent_parcel = vpm->getAgentParcel();
    if (!agent_region || !agent_parcel)
        return false;
    return vpm->allowAgentScripts(agent_region, agent_parcel);
}

void RRInterface::printOnChat (std::string message)
{
    LLChat chat(message);
    chat.mSourceType = CHAT_SOURCE_UNKNOWN;
    LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
    if(nearby_chat)
    {
        nearby_chat->addMessage(chat);
    }
}

void RRInterface::listRlvRestrictions(std::string substr /*= ""*/)
{
    printOnChat(getRlvRestrictions(substr));
}

std::string RRInterface::getRlvRestrictions(std::string substr /*= ""*/)
{
    RRMAP::iterator it = mSpecialObjectBehaviours.begin();
    std::string object_name = "";
    std::string old_object_name = "";
    std::string res = "";
    res += ("\n### RLV RESTRICTIONS #####################");
    while (it != mSpecialObjectBehaviours.end())
    {
        old_object_name = object_name;
        //The floater name lookup routine covers more cases, so use that instead
        //
        //LLInventoryItem* item = getItem(LLUUID(it->first));
        //if (item) object_name = item->getName();
        //else object_name = "???";
        object_name = KokuaRLVFloaterSupport::getNameFromUUID(LLUUID(it->first),true);

        if (substr == "" || object_name.find(substr) != -1) {
            // print the name of the object
            if (object_name != old_object_name) {
                res += ("\n### " + object_name + " #####################");
            }
            res += ("\n-------- " + it->second);
        }
        it++;
    }
    res += ("\n##########################################");
    return res;
}

BOOL RRInterface::checkCameraLimits(BOOL and_correct /* = FALSE*/)
{
    // Check that we are within the imposed limits
    // Force the camera back into the limits when not
    // Return TRUE when the camera is ok
    if (!gAgentCamera.isInitialized()) {
        return TRUE;
    }

    if (mCamDistMax <= 0.f && !gAgentCamera.cameraMouselook())
    {
        if (and_correct) {
            gAgentCamera.changeCameraToMouselook ();
        }
        return FALSE;
    }
    else if (mCamDistMin > 0.f && gAgentCamera.cameraMouselook())
    {
        if (and_correct) {
            gAgentCamera.changeCameraToDefault ();
        }
        return FALSE;
    }
    return TRUE;
}

BOOL RRInterface::updateCameraLimits ()
{
    // Update the min and max 
    mShowavsDistMax = getMin ("camavdist", EXTREMUM);

    F32 old_mCamDistDrawMax = mCamDistDrawMax;
    F32 old_mCamDistDrawMin = mCamDistDrawMin;
    F32 old_mCamDistDrawAlphaMax = mCamDistDrawAlphaMax;
    F32 old_mCamDistDrawAlphaMin = mCamDistDrawAlphaMin;

    mCamZoomMax = getMin("camzoommax", EXTREMUM);
    if (mCamZoomMax == 0.f) mCamZoomMax = EXTREMUM;
    mCamZoomMin = getMax("camzoommin", -EXTREMUM);
    if (mCamZoomMin == 0.f) mCamZoomMin = -EXTREMUM;

    // setcam_fovmin and setcam_fovmax set the FOV, i.e. 60°/multiplier;
    // in other words, they are equivalent to camzoommin and camzoommax.
    F32 fovmin = getMax("setcam_fovmin", 0.001f);
    if (fovmin != 0.f && fovmin != 0.001f)
    {
        F32 zoommax_from_fovmin = DEFAULT_FIELD_OF_VIEW / fovmin;
        if (zoommax_from_fovmin < mCamZoomMax)
        {
            mCamZoomMax = zoommax_from_fovmin;
        }
    }
    F32 fovmax = getMin("setcam_fovmax", EXTREMUM);
    if (fovmax != 0.f && fovmax != EXTREMUM)
    {
        F32 zoommin_from_fovmax = DEFAULT_FIELD_OF_VIEW / fovmax;
        if (zoommin_from_fovmax > mCamZoomMin)
        {
            mCamZoomMin = zoommin_from_fovmax;
        }
    }

    mCamDistMax = getMin("camdistmax,setcam_avdistmax", EXTREMUM);
    mCamDistMin = getMax("camdistmin,setcam_avdistmin", -EXTREMUM);

    mCamDistDrawMax = getMin ("camdrawmax", EXTREMUM);
    mCamDistDrawMin = getMin ("camdrawmin", EXTREMUM);

    mCamDistDrawAlphaMin = getMax ("camdrawalphamin", 0.f);
    mCamDistDrawAlphaMax = getMax ("camdrawalphamax", 1.f);

    mCamDistDrawColor = getMixedColors ("camdrawcolor", LLColor3::black);

    if (mCamDistDrawMin <= 0.4f) { // So we're sure to render the spheres even when restricted to mouselook
        mCamDistDrawMin = 0.4f;
    }

    if (mCamDistDrawMax < mCamDistDrawMin) { // sort the two limits in order
        if (mCamDistDrawMin < EXTREMUM) {
            mCamDistDrawMax = mCamDistDrawMin;
        }
        else {
            mCamDistDrawMin = mCamDistDrawMax;
        }
    }

    if (mCamDistMax >= mCamDistDrawMin) { // make sure we can't move the camera outside the minimum render limit
        mCamDistMax = mCamDistDrawMin * 0.75f;
    }
    if (mCamDistMax >= mCamDistDrawMax) { // make sure we can't move the camera outside the maximum render limit
        mCamDistMax = mCamDistDrawMax * 0.75f;
    }

    if (mCamDistDrawAlphaMax < mCamDistDrawAlphaMin) { // make sure the "fog" goes in the right direction
        mCamDistDrawAlphaMax = mCamDistDrawAlphaMin;
    }

    if (mCamZoomMin > mCamZoomMax) {
        mCamZoomMin = mCamZoomMax;
    }

    if (mCamDistMin > mCamDistMax) {
        mCamDistMin = mCamDistMax;
    }

//  if (gRRenabled) // RLV_108 : don't toggle flycam if RLV is disabled since we call this method at startup
//  {
//      LLViewerJoystick::getInstance()->getOverrideCamera();
//      if (mCamDistMax < EXTREMUM * 0.75f)
//      {
//          handle_toggle_flycam();
//      }
//  }

    // silly hack, but we need to force all textures in world to be updated (code copied from camtextures above)
    // Do so only when the camera limits have changed, though, because this method is called every time a new RLV restriction
    // is received or removed, so it would slow down the viewer otherwise.
    if (old_mCamDistDrawMax != mCamDistDrawMax 
        || old_mCamDistDrawMin != mCamDistDrawMin 
        || old_mCamDistDrawAlphaMax != mCamDistDrawAlphaMax 
        || old_mCamDistDrawAlphaMin != mCamDistDrawAlphaMin) {

        // Force all the rendering types back to TRUE (and we won't be able to switch them off while the vision is restricted)
        if (mCamDistDrawMin < EXTREMUM || mCamDistDrawMax < EXTREMUM) {
            LLPipeline::setRenderBeacons(FALSE);
            LLPipeline::setRenderScriptedBeacons(FALSE);
            LLPipeline::setRenderScriptedTouchBeacons(FALSE);
            LLPipeline::setRenderPhysicalBeacons(FALSE);
            LLPipeline::setRenderSoundBeacons(FALSE);
            LLPipeline::setRenderParticleBeacons(FALSE);
            LLPipeline::setRenderHighlights(FALSE);
            //LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
            gPipeline.setAllRenderTypes();

            // Also make sure the basic shaders are enabled. On some video cards, turning them off completely hides the vision spheres.
            if (gSavedSettings.getBOOL("VertexShaderEnable") == FALSE) {
                if (gGLManager.mGLVersion >= 3.f || !gGLManager.mIsIntel) {
                    gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
                }
            }

        }

        S32 i;
        for (i=0; i<gObjectList.getNumObjects(); ++i) {
            LLViewerObject* object = gObjectList.getObject(i);
            if (object) {
                object->setSelected(FALSE);
            }
        }
    // KKA-835 update our lowest squared distance value
    if (mSetsphereDistMax < mCamDistDrawMax)
    {
      mLeastDistMaxSquared = mSetsphereDistMax * mSetsphereDistMax; 
    }
    else
    {
      mLeastDistMaxSquared = mCamDistDrawMax * mCamDistDrawMax;
    }
    }

    // Recalculate mCamDistNbGradients according to the distance between mCamDistDrawMin and mCamDistDrawMax
    // Let's say 4 gradients for every 0.1 m of difference, max 40.
    mCamDistNbGradients = ((U32)((mCamDistDrawMax - mCamDistDrawMin) * 40.f));
    if (mCamDistNbGradients > 40)
    {
        mCamDistNbGradients = 40;
    }

    mVisionRestricted = (mCamDistDrawMin < EXTREMUM || mCamDistDrawMax < EXTREMUM || mContainsSetsphere); //KKA-835 add setsphere

    // Use impostors if we use silhouettes or if the outer sphere is 99% opaque or more
    if (mShowavsDistMax < EXTREMUM || mCamDistDrawAlphaMax >= ALPHA_ALMOST_OPAQUE) {
        LLVOAvatar::sLimitNonImpostors = TRUE;
        LLVOAvatar::updateImpostorRendering(LLVOAvatar::sMaxNonImpostors); // simply refresh the value so we're sure impostors will be used
    }
    else {
        if (LLStartUp::getStartupState() >= STATE_STARTED) {
            //Control RenderUseImpostors is unreferenced elsewhere - update this to the same as LLVOAvatar
            //LLVOAvatar::sLimitNonImpostors = gSavedSettings.getBOOL("RenderUseImpostors");
            LLVOAvatar::sLimitNonImpostors = (0 != LLVOAvatar::sMaxNonImpostors);
        }

    }

    // And check the camera is still within the limits
    return checkCameraLimits (TRUE);
}

#define UPPER_ALPHA_LIMIT 0.999999f
// This function returns the effective alpha to set to each step when going from
// 0.0 to "desired_alpha", so that everything seen through the last layer will
// be obscured as if it were behind only one layer of alpha "desired_alpha", 
// regardless of the number of layers "nb_layers"
// If we have N layers and want a transparency T (T = 1-A), we want to find X so that
// X**N = T (because combined transparencies multiply), in other words, X = T**(1/N)
// The problem with this formula is that with a target transparency of 0 (alpha = 1), we would
// not get any gradient at all so we need to limit the alpha to a maximum that is lower than 1.
F32 calculateDesiredAlphaPerStep (F32 desired_alpha, int nb_layers)
{
    double trans_at_this_step;
    if (desired_alpha > UPPER_ALPHA_LIMIT) {
        desired_alpha = UPPER_ALPHA_LIMIT;
    }
    double desired_trans = (double)(1.f - desired_alpha);
    trans_at_this_step = pow (desired_trans, 1.0/(double)nb_layers);

    return (F32)(1.0 - trans_at_this_step);
}

// This method draws several big black spheres around the avatar, with various alphas
// Alpha goes from mCamDistDrawAlphaMin to mCamDistDrawAlphaMax
// Things to remember :
// - There are two render limits in RLV : min and max (min is a sphere with a variable alpha
//   and max is an opaque sphere).
// - Render limit min <= render limit max.
// - If a render limit is <= 1.0, make it 1.0 because we'll be forced into mouselook anyway,
//   so it would be better to render the sphere
// - If a render limit is unspecified (i.e. equal to EXTREMUM), don't render it.
// - If both render limits are specified and different, render both and several in-between at 
//   regular intervals, with a linear interpolation for alpha between mCamDistDrawAlphaMin and
//   mCamDistDrawAlphaMax for each sphere.
// - There are not too many spheres to render, because stacking alphas makes the video card
//   complain.
// - If force_opaque is TRUE, then the inner sphere will be opaque and no other sphere will be rendered.
void RRInterface::drawRenderLimit (BOOL force_opaque /*= FALSE*/)
{
    static LLCachedControl<bool> selectionOutlines(gSavedSettings, "RestrainedLoveSelectionOutlines", FALSE);
    //if (true) return;

    //if (sRenderLimitRenderedThisFrame) { // already rendered the vision spheres during this rendering frame ? => bail
    //  return;
    //}

  // KKA-835 Test specifically instead of using mVisionRestricted since that now includes setsphere
    if (! (mCamDistDrawMin < EXTREMUM || mCamDistDrawMax < EXTREMUM)) { // not vision sphere restricted ? => bail
        return;
    }

    gGL.setColorMask(true, false);
    gUIProgram.bind();

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gPipeline.disableLights();

    // Calculate the center of the spheres
    LLVector3 center = isAgentAvatarValid() 
        ? getCamDistDrawFromJoint()->getWorldPosition()
        : gAgent.getPositionAgent();

    // If the inner sphere is opaque, just render it and no other.
    // Also make the inner sphere opaque if we're highlighting invisible surfaces or if anything is highlighted by a selection (edit, select or drag and drop).
    if (force_opaque
    || LLDrawPoolAlpha::sShowDebugAlpha
//CA: Two options for selection outlines under camera restrictions. If selection outlines are set to on we force opaque here
    || (selectionOutlines && LLSelectMgr::getInstance()->mRenderHighlightSelections && (!LLSelectMgr::getInstance()->getSelection()->isEmpty() || LLToolDragAndDrop::getInstance()->getCargoCount() > 0))
    || mCamDistDrawAlphaMin >= UPPER_ALPHA_LIMIT)
    {
        drawSphere(center, mCamDistDrawMin, mCamDistDrawColor, 1.f);
    }
    else {
        // If the outer sphere is opaque, render it now before switching to blend mode
        if (mCamDistDrawAlphaMax >= UPPER_ALPHA_LIMIT) {
            drawSphere(center, mCamDistDrawMax, mCamDistDrawColor, 1.f);
        }

        // Switch to blend mode now
        LLGLEnable gls_blend(GL_BLEND);
        LLGLEnable gls_cull(GL_CULL_FACE);
        LLGLEnable alpha_test(GL_ALPHA_TEST);
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
        gGL.setColorMask(true, false);

        F32 alpha_step = calculateDesiredAlphaPerStep(mCamDistDrawAlphaMax, mCamDistNbGradients);

        // if the outer sphere is not opaque, render it now since we haven't before switching
        // to blend mode.
        if (mCamDistDrawAlphaMax < UPPER_ALPHA_LIMIT) {
            drawSphere(center, mCamDistDrawMax, mCamDistDrawColor, alpha_step);
        }

        // Draw from the outer sphere (excluded) to the inner sphere, knowing that we are currently in blend mode
        for (int i = mCamDistNbGradients - 1; i > 0; i--) {
            drawSphere(center
                , lerp(mCamDistDrawMin, mCamDistDrawMax, (F32)i / (F32)mCamDistNbGradients)
                , mCamDistDrawColor
                , alpha_step
            );
        }
    }

    gGL.flush();
    gGL.setColorMask(true, false);

    gUIProgram.unbind();

    sRenderLimitRenderedThisFrame = TRUE;
}

void RRInterface::drawSphere (LLVector3 center, F32 scale, LLColor3 color, F32 alpha)
{
    // Note : This method has nothing to do with @setsphere, which came much later, it is used for drawing the original vision spheres.

    // Don't bother if the sphere is invisible
    if (alpha < 0.0001f)
    {
        return;
    }
    //gGL.pushMatrix();
    //if (alpha >= 1.0)
    //{
    //  LLGLDisable blend(GL_BLEND);
    //  LLGLDisable test(GL_ALPHA_TEST);
    //}

    {
        gGL.pushMatrix();
        //gGL.loadIdentity();
        {
            gGL.translatef(center[0], center[1], center[2]);
            gGL.scalef(scale, scale, scale);

            LLColor4 color_alpha(color, alpha);
            gGL.color4fv(color_alpha.mV);
                
            // Render inside only (the camera is not supposed to go outside anyway)
            glCullFace(GL_FRONT);
            gSphere.render();
            glCullFace(GL_BACK);
        }
        gGL.popMatrix();
    }
    //gGL.popMatrix();
}

LLJoint* RRInterface::getCamDistDrawFromJoint (BOOL force_head_in_mouselook /*= TRUE*/)
{
    if (!gAgentAvatarp) {
        return NULL;
    }
    if (!mCamDistDrawFromJoint || (force_head_in_mouselook && CAMERA_MODE_MOUSELOOK == gAgentCamera.getCameraMode())) { // if we are in mouselook, force the joint to be the head (unless specified otherwise)
        return gAgentAvatarp->mHeadp;
    }
    return mCamDistDrawFromJoint;
}


BOOL RRInterface::updateSetsphere()
{
    /*
    This method exists to emulate the @setsphere feature of RLVa by reusing most of its code. Problem is, RLVa uses "setsphere_...=force" to set parameters instead of "setsphere_...=n",
    which means RLVa does not retain the parameters as restrictions unlike RLV. This difference completely changes the high level management algorithm of the "setsphere" restrictions.
    To emulate while reusing as much code as possible, the algorithm is as follows :

        Part 1 : create the preliminary lists
        Have a list "E" of exactly ESphereMode::Count (*) "RlvSphereEffect" objects with default values, each mode set to be equal to its place in the list (0 to count-1), and set them all to "inactive" for now because not all possible modes are used at any given time
        Have a map "UM" UUID->mode to retain each UUID that issued a "setsphere=n" restriction and its chosen mode (if not specified, default is 0), also set the corresponding effects to "active"
        (*) that's 5 at the time of this writing

        Part 2 : calculate the final values for each mode
        For each command in the main RLV map
          if the command is a setsphere_* one (not "setsphere" or "setsphere_mode" but any other kind)
            if the object that issued the command is part of UM
              get the corresponding mode from UM
              according to the type of command, check its value against the one stored in the corresponding effect mode in E
                => change the stored value only if necessary (ex : if distmin is less than the stored one then store the new one) or if it hasn't been changed before (so we can choose whatever default value we want without breaking the mixing)
            end if
          end if
        end for

        Part 3 : add the active effects in LLVfxManager
        clear the list of effects in LLVfxManager
        for each effect in E
          if it is active, add it to LLVfxManager (with the final values we calculated in Part 2)
        end for
    */

    // If we're not using deferred but are using Windlight shaders we need to force use of FBO and depthmap texture
    if ((!LLPipeline::RenderDeferred) && (LLPipeline::WindLightUseAtmosShaders) && (!LLPipeline::sUseDepthTexture))
    {
        LLRenderTarget::sUseFBO = true;
        LLPipeline::sUseDepthTexture = true;

        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
        gPipeline.resetVertexBuffers();
        LLViewerShaderMgr::instance()->setShaders();
    }
    
    //KKA-835 as part of extending mVisionRestricted to include setsphere, we need generally accessible min and max values
    mSetsphereDistMin = EXTREMUM;
    mSetsphereDistMax = EXTREMUM;
    mSetsphereValueMax = 0.0; // KKA-835 arguably this should only operate on mode 0, but we'll do it for all for now

    // Part 1 : create a list of ESphereMode::Count inactive effects, some of these effects will be copied to LLVfxManager later and this list and its contents will be erased
    std::deque<RlvSphereEffect*> effects;
    int i;
    for (i = 0; i < (int)RlvSphereEffect::ESphereMode::Count; i++) {
        RlvSphereEffect* effect = new RlvSphereEffect(LLUUID::null); // LLUUID::null to indicate that this effect is not active at the moment
        effect->setMode((RlvSphereEffect::ESphereMode)i);
        effects.push_back(effect);
    }

    // For each "setsphere", look for a "setsphere_mode" issued by the same object (or 0 if none).
    std::map<LLUUID, RlvSphereEffect::ESphereMode> map_uuid_to_mode;
    LLUUID uuid;
    std::string command;
    std::string behav;
    std::string option;
    std::string param;
    RlvSphereEffect::ESphereMode mode;
    RRMAP::iterator it;
    for (it = mSpecialObjectBehaviours.begin(); it != mSpecialObjectBehaviours.end(); ++it) {
        command = it->second;
        if (parseCommand(command + "=n", behav, option, param)) {
            if (behav == "setsphere") {
                uuid.set (it->first); // UUID of the object that issued this "setsphere" command
                mode = (RlvSphereEffect::ESphereMode)atoi (get(uuid, "setsphere_mode", "0").c_str()); // find the mode set by this object in the RRMap, or 0 if no mode was specified
                if ((int)mode >= 0 && (int)mode < effects.size()) { // make sure the mode is inside the list of known modes
                    map_uuid_to_mode[uuid] = mode; // add this UUID to the map with the mode we found
                    effects.at((int)mode)->setActive(TRUE); // activate this effect so it will be used by LLVfxManager 
                }
            }
        }
    }

    // Part 2 : calculate the final values for each effect, mixing them to stay as restrictive as possible (for some we retain the minimum, for others we multiply etc)
    // If a value hasn't been changed since the creation of this effect, simply replace it with the new value without mixing in any way, this allows us to set some default values to the effect without impeding the mixing afterwards.
    std::string str_setsphere_ = "setsphere_";
    int len_str_setsphere_ = str_setsphere_.length();
    RlvSphereEffect* effect; // one of the effects we've created and stored above
    for (it = mSpecialObjectBehaviours.begin(); it != mSpecialObjectBehaviours.end(); ++it) {
        command = it->second;
        if (parseCommand(command + "=n", behav, option, param)) {
            if (behav.find (str_setsphere_) == 0) { // is the command a "setsphere_*" one ?
                uuid.set (it->first); // UUID of the object that issued this "setsphere_" command
                mode = map_uuid_to_mode[uuid];
                if ((int)mode >= 0 && (int)mode < effects.size()) { // make sure the mode is inside the list of known modes
                    effect = effects.at((int)mode);
                    std::string last_part = behav.substr(len_str_setsphere_);
                    if (effect) {
                        float f_option = atof(option.c_str()); // most options are floats or ints, only "param" is a vector and has to be parsed specifically
                        int i_option = atoi(option.c_str()); // most options are floats or ints, only "param" is a vector and has to be parsed specifically
                        if (f_option < 0.f) {
                            f_option = 0.f; // also a float option cannot be negative
                        }
                        if (i_option < 0) {
                            i_option = 0; // also an int option cannot be negative
                        }
                        if (last_part == "distmin") { // mixing here means retaining the lowest value
                            if (!effect->m_nChangedDistanceMin || f_option < effect->getDistanceMin()) {
                                //KKA-835 some uses of mVisionRestricted also test camdrawmin/max so we need setsphere equivalents
                                if (f_option < mSetsphereDistMin)
                                {
                                mSetsphereDistMin = f_option;
                            }
                                effect->setDistanceMin(f_option);
                            }
                        }
                        else if (last_part == "distmax") { // mixing here means retaining the lowest value
                            if (!effect->m_nChangedDistanceMax || f_option < effect->getDistanceMax()) {
                                //KKA-835 some uses of mVisionRestricted also test camdrawmin/max so we need setsphere equivalents
                                if (f_option < mSetsphereDistMax)
                                {
                                mSetsphereDistMax = f_option;
                            }
                                effect->setDistanceMax(f_option);
                            }
                        }
                        else if (last_part == "distextend") { // mixing here means retaining the highest value
                            if (!effect->m_nChangedDistExtend || i_option < (int)(effect->getDistExtend())) {
                                effect->setDistExtend((RlvSphereEffect::ESphereDistExtend)i_option);
                            }
                        }
                        else if (last_part == "valuemin") { // mixing here means retaining the highest value
                            if (!effect->m_nChangedValueMin || f_option > effect->getValueMin()) {
                                effect->setValueMin(f_option);
                            }
                        }
                        else if (last_part == "valuemax") { // mixing here means retaining the highest value
                            if (!effect->m_nChangedValueMax || f_option > effect->getValueMax()) {
                              // KKA-835 provide an equivalent for testing alphamax (but note we don't restrict to mode 0)
                              if (f_option > mSetsphereValueMax)
                              {
                                mSetsphereValueMax = f_option;
                              }
                                effect->setValueMax(f_option);
                            }
                        }
                        else if (last_part == "tween") { // mixing here means retaining the lowest value
                            if (!effect->m_nChangedTweenDuration || f_option < effect->getTweenDuration()) {
                                effect->setTweenDuration(f_option);
                            }
                        }
                        else if (last_part == "origin") { // mixing here means retaining the lowest value (i.e. if any object centers on the avatar, we center on the avatar)
                            if (!effect->m_nChangedOrigin || i_option < (int)(effect->getOrigin())) {
                                effect->setOrigin((RlvSphereEffect::ESphereOrigin)i_option);
                            }
                        }
                        else if (last_part == "param") { // mixing here actually depends on the mode
                            LLStringUtil::replaceString(option, "/", ";"); // Catznip uses "/" for some reason, but ";" should be used for consistency ("/" is used for folders and some folder commands have multiple options separated by ";")
                            std::deque<std::string> tokens = parse(option, ";", 4); // tokens has a size of 4 or more to avoid crashing during the comparisons below
                            LLVector4 v_option (atof (tokens.at(0).c_str()), atof (tokens.at(1).c_str()), atof (tokens.at(2).c_str()), atof (tokens.at(3).c_str()));
                            LLVector4 param_final = effect->getParams();
                            switch (mode) {
                                case RlvSphereEffect::ESphereMode::Blend: { // in blend mode we mix by multiplying (each field individually) the RGB vectors
                                    if (!effect->m_nChangedParams) {
                                        param_final = v_option;
                                    }
                                    else {
                                        param_final.mV[0] *= v_option.mV[0];
                                        param_final.mV[1] *= v_option.mV[1];
                                        param_final.mV[2] *= v_option.mV[2];
                                    }
                                    break;
                                }
                                case RlvSphereEffect::ESphereMode::Blur: { // in fixed blur mode we don't mix because we don't use the params at all
                                    break;
                                }
                                case RlvSphereEffect::ESphereMode::BlurVariable: { // in kernel blur mode we mix by choosing the highest value of <kernel_size> between the params
                                    if (!effect->m_nChangedParams || v_option.mV[0] > param_final.mV[0]) param_final.mV[0] = v_option.mV[0];
                                    break;
                                }
                                case RlvSphereEffect::ESphereMode::ChromaticAberration: // in chromatic aberration and pixelate modes we mix by taking the highest values of each field
                                case RlvSphereEffect::ESphereMode::Pixelate: {
                                    if (!effect->m_nChangedParams) {
                                        param_final = v_option;
                                    }
                                    else {
                                        if (v_option.mV[0] > param_final.mV[0]) param_final.mV[0] = v_option.mV[0];
                                        if (v_option.mV[1] > param_final.mV[1]) param_final.mV[1] = v_option.mV[1];
                                        if (v_option.mV[2] > param_final.mV[2]) param_final.mV[2] = v_option.mV[2];
                                        if (v_option.mV[3] > param_final.mV[3]) param_final.mV[3] = v_option.mV[3];
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                            // Update the parameters of the effect after mixing.
                            effect->setParams(param_final);
                        }
                    }
                }
            }
        }
    }

    // Part 3 : add the active effects to LLVfxManager
    // First, clear the manager to destroy the old effects.
    LLVfxManager::instance().clearEffects();

    // Add only the active effects.
    for (i = 0; i < (int)RlvSphereEffect::ESphereMode::Count; i++) {
        RlvSphereEffect* effect = effects.at(i);
        // If the effect is active then add it to LLVfxManager, otherwise destroy it.
        if (effect->getActive()) {
            LLVfxManager::instance().addEffect(effect); // it will be destroyed later
        }
        else {
            delete effect;
        }
    }
    // All the effects that have indeed been added to LLVfxManager will be destroyed next time we call LLVfxManager::instance().clearEffects(), which means next time we call RRInterface::updateSetsphere().

  // KKA-835 mVisionRestricted now covers setsphere too
    mVisionRestricted = (mCamDistDrawMin < EXTREMUM || mCamDistDrawMax < EXTREMUM || mContainsSetsphere);
    // KKA-835 update our lowest squared distance value
    if (mSetsphereDistMax < mCamDistDrawMax)
    {
      mLeastDistMaxSquared = mSetsphereDistMax * mSetsphereDistMax; 
    }
    else
    {
      mLeastDistMaxSquared = mCamDistDrawMax * mCamDistDrawMax;
    }
    return TRUE;
}




BOOL RRInterface::isBlacklisted (std::string action, bool force)
{
    std::string blacklist;
    blacklist = ","+sBlacklist+",";
    if (force) {
        return (blacklist.find (","+action+"%f,") != -1);
    }
    else {
        return (blacklist.find (","+action+",") != -1);
    }
}

std::deque<std::string> RRInterface::getBlacklist (std::string filter /* = ""*/)
{
    std::deque<std::string> list, res;
    list = parse (sBlacklist, ",");
    res.clear();

    unsigned int size = list.size();
    for (unsigned int i = 0; i < size; i++) {
        if (filter == "" || list[i].find (filter) != -1) {
            res.push_back (list[i]);
        }
    }

    return res;
}

void RRInterface::updateLimits() {
    // Update all the min and max that are not related to camera and vision restrictions
    // For example, tplocal, fartouch, sittp etc

    // fartouch/touchfar
    // First we need to find the lowest betwen 
    float fartouch_max = getMin("fartouch", EXTREMUM);
    float touchfar_max = getMin("touchfar", EXTREMUM);
    if (fartouch_max > touchfar_max){
        fartouch_max = touchfar_max;
    }
    mFartouchMax = fartouch_max;

    // sittp
    mSittpMax = getMin("sittp", EXTREMUM);

    // tplocal
    mTplocalMax = getMin("tplocal", EXTREMUM);
}
