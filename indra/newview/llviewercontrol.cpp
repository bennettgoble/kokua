/** 
 * @file llviewercontrol.cpp
 * @brief Viewer configuration
 * @author Richard Nelson
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

#include "llviewercontrol.h"

// Library includes
#include "llwindow.h"   // getGamma()

// For Listeners
#include "llaudioengine.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llconsole.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolterrain.h"
#include "llflexibleobject.h"
#include "llfeaturemanager.h"
#include "llviewershadermgr.h"

#include "llsky.h"
#include "llvieweraudio.h"
#include "llviewermenu.h"
#include "llviewertexturelist.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvoiceclient.h"
#include "llvosky.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewerjoystick.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"
#include "llkeyboard.h"
#include "llerrorcontrol.h"
#include "llappviewer.h"
#include "llvosurfacepatch.h"
#include "llvowlsky.h"
#include "llrender.h"
#include "llnavigationbar.h"
#include "llnotificationsutil.h"
#include "llfloatertools.h"
#include "llpaneloutfitsinventory.h"
#include "llpanellogin.h"
#include "llpaneltopinfobar.h"
#include "llspellcheck.h"
#include "llslurl.h"
#include "llstartup.h"
#include "fsfloaterposestand.h"
#include "llavataractions.h"
#include "llfloaterreg.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"

// Third party library includes
#include <boost/algorithm/string.hpp>

//MK
#include "llagentwearables.h"
#include "llstatusbar.h"
//mk

#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
BOOL                gHackGodmode = FALSE;
#endif

// Should you contemplate changing the name "Global", please first grep for
// that string literal. There are at least a couple other places in the C++
// code that assume the LLControlGroup named "Global" is gSavedSettings.
LLControlGroup gSavedSettings("Global");    // saved at end of session
LLControlGroup gSavedPerAccountSettings("PerAccount"); // saved at end of session
LLControlGroup gCrashSettings("CrashSettings"); // saved at end of session
LLControlGroup gWarningSettings("Warnings"); // persists ignored dialogs/warnings

std::string gLastRunVersion;
extern BOOL gResizeScreenTexture;
extern BOOL gResizeShadowTexture;
extern BOOL gDebugGL;
////////////////////////////////////////////////////////////////////////////
// Listeners

//MK
static bool handleRestrainedLoveDebugChanged(const LLSD& newvalue)
{
    RRInterface::sRestrainedLoveDebug = newvalue.asBoolean();
    return true;
}

static bool handleRestrainedLoveLoggingChanged(const LLSD& newvalue)
{
    RRInterface::sRestrainedLoveLogging = newvalue.asBoolean();
    return true;
}

static bool handleRestrainedLoveCommandLoggingChanged(const LLSD& newvalue)
{
    RRInterface::sRestrainedLoveCommandLogging = newvalue.asBoolean();
    return true;
}

static bool handleRestrainedLoveLastStandingLocationChanged(const LLSD& newvalue)
{
    // Do not let the user change these values manually (could lead to cheating through @standtp)
    //gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
    return true;
}

static bool handleRenderDeferredShowInvisiprimsChanged(const LLSD& newvalue)
{
    bool status = newvalue.asBoolean();
    LLDrawPoolBump::sRenderDeferredShowInvisiprims = status;
    return true;
}

static bool handleRestrainedLoveCamDistNbGradientsChanged(const LLSD& newvalue)
{
    //RRInterface::mCamDistNbGradients = newvalue.asInteger();
    //if (RRInterface::mCamDistNbGradients == 0)
    //{
    //  RRInterface::mCamDistNbGradients = 1;
    //}
    // Let's keep it constant for now, because there are ways to make the vision restriction less tight by playing with this setting.
    return true;
}

static bool handleRestrainedLoveHeadMouselookRenderRigged(const LLSD& newvalue)
{
    RRInterface::sRestrainedLoveHeadMouselookRenderRigged = newvalue.asBoolean();
    return true;
}

static bool handleRestrainedLoveRenderInvisibleSurfaces(const LLSD& newvalue)
{
    RRInterface::sRestrainedLoveRenderInvisibleSurfaces = newvalue.asBoolean();
    return true;
}
//mk

static bool handleRenderAvatarMouselookChanged(const LLSD& newvalue)
{
    LLVOAvatar::sVisibleInFirstPerson = newvalue.asBoolean();
    return true;
}

static bool handleRenderFarClipChanged(const LLSD& newvalue)
{
    if (LLStartUp::getStartupState() >= STATE_STARTED)
    {
        F32 draw_distance = (F32)newvalue.asReal();
    gAgentCamera.mDrawDistance = draw_distance;
    LLWorld::getInstance()->setLandFarClip(draw_distance);
    return true;
    }
    return false;
}

static bool handleTerrainDetailChanged(const LLSD& newvalue)
{
    LLDrawPoolTerrain::sDetailMode = newvalue.asInteger();
    return true;
}


static bool handleDebugAvatarJointsChanged(const LLSD& newvalue)
{
    std::string new_string = newvalue.asString();
    LLJoint::setDebugJointNames(new_string);
    return true;
}

static bool handleAvatarHoverOffsetChanged(const LLSD& newvalue)
{
    if (isAgentAvatarValid())
    {
        gAgentAvatarp->setHoverIfRegionEnabled();
    }
    return true;
}


static bool handleSetShaderChanged(const LLSD& newvalue)
{
    // changing shader level may invalidate existing cached bump maps, as the shader type determines the format of the bump map it expects - clear and repopulate the bump cache
    gBumpImageList.destroyGL();
    gBumpImageList.restoreGL();

    if (gPipeline.isInit())
    {
        // ALM depends onto atmospheric shaders, state might have changed
        bool old_state = LLPipeline::sRenderDeferred;
        LLPipeline::refreshCachedSettings();
        gPipeline.updateRenderDeferred();
        if (old_state != LLPipeline::sRenderDeferred)
        {
            gPipeline.releaseGLBuffers();
            gPipeline.createGLBuffers();
            gPipeline.resetVertexBuffers();
        }
    }

    // else, leave terrain detail as is
    LLViewerShaderMgr::instance()->setShaders();
    return true;
}

static bool handleRenderPerfTestChanged(const LLSD& newvalue)
{
       bool status = !newvalue.asBoolean();
       if (!status)
       {
               gPipeline.clearRenderTypeMask(LLPipeline::RENDER_TYPE_WL_SKY,
                                                                         LLPipeline::RENDER_TYPE_GROUND,
                                                                        LLPipeline::RENDER_TYPE_TERRAIN,
                                                                         LLPipeline::RENDER_TYPE_GRASS,
                                                                         LLPipeline::RENDER_TYPE_TREE,
                                                                         LLPipeline::RENDER_TYPE_WATER,
                                                                         LLPipeline::RENDER_TYPE_PASS_GRASS,
                                                                         LLPipeline::RENDER_TYPE_HUD,
                                                                         LLPipeline::RENDER_TYPE_CLOUDS,
                                                                         LLPipeline::RENDER_TYPE_HUD_PARTICLES,
                                                                         LLPipeline::END_RENDER_TYPES); 
               gPipeline.setRenderDebugFeatureControl(LLPipeline::RENDER_DEBUG_FEATURE_UI, false);
       }
       else 
       {
               gPipeline.setRenderTypeMask(LLPipeline::RENDER_TYPE_WL_SKY,
                                                                         LLPipeline::RENDER_TYPE_GROUND,
                                                                         LLPipeline::RENDER_TYPE_TERRAIN,
                                                                         LLPipeline::RENDER_TYPE_GRASS,
                                                                         LLPipeline::RENDER_TYPE_TREE,
                                                                         LLPipeline::RENDER_TYPE_WATER,
                                                                         LLPipeline::RENDER_TYPE_PASS_GRASS,
                                                                         LLPipeline::RENDER_TYPE_HUD,
                                                                         LLPipeline::RENDER_TYPE_CLOUDS,
                                                                         LLPipeline::RENDER_TYPE_HUD_PARTICLES,
                                                                         LLPipeline::END_RENDER_TYPES);
               gPipeline.setRenderDebugFeatureControl(LLPipeline::RENDER_DEBUG_FEATURE_UI, true);
       }

       return true;
}

bool handleRenderTransparentWaterChanged(const LLSD& newvalue)
{
//MK
    // When under @setsphere, transparent water must be rendered.
    if (gRRenabled && gAgent.mRRInterface.mContainsSetsphere && !gSavedSettings.getBOOL("RenderTransparentWater"))
    {
        gSavedSettings.setBOOL("RenderTransparentWater", TRUE);
    }
//mk
    if (gPipeline.isInit())
    {
        gPipeline.updateRenderTransparentWater();
        gPipeline.updateRenderDeferred();
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
        gPipeline.resetVertexBuffers();
        LLViewerShaderMgr::instance()->setShaders();
    }
    LLWorld::getInstance()->updateWaterObjects();
    return true;
}


static bool handleShadowsResized(const LLSD& newvalue)
{
    gPipeline.requestResizeShadowTexture();
    return true;
}

static bool handleWindowResized(const LLSD& newvalue)
{
    gPipeline.requestResizeScreenTexture();
    return true;
}

static bool handleReleaseGLBufferChanged(const LLSD& newvalue)
{
    //MK
        // When under @setsphere, don't allow DoF because it makes the alpha blended rigged surfaces win against the sphere, which lets the user see through it.
        // KKA-831 we also need to enforce RenderFSAASamples (anti-aliasing) non-zero
    if (gRRenabled && gAgent.mRRInterface.mContainsSetsphere)
    {
        if (gSavedSettings.getBOOL("RenderDepthOfField"))
        {
            gSavedSettings.setBOOL("RenderDepthOfField", FALSE);
        }
        if (!gSavedSettings.getU32("RenderFSAASamples"))
        {
            gSavedSettings.setU32("RenderFSAASamples", 2);
        }
    }
//mk
    if (gPipeline.isInit())
    {
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
    }
    return true;
}

static bool handleLUTBufferChanged(const LLSD& newvalue)
{
    if (gPipeline.isInit())
    {
        gPipeline.releaseLUTBuffers();
        gPipeline.createLUTBuffers();
    }
    return true;
}

static bool handleAnisotropicChanged(const LLSD& newvalue)
{
    LLImageGL::sGlobalUseAnisotropic = newvalue.asBoolean();
    LLImageGL::dirtyTexOptions();
    return true;
}

static bool handleVSyncChanged(const LLSD& newvalue)
{
    gViewerWindow->getWindow()->toggleVSync(newvalue.asBoolean());

    return true;
}

static bool handleVolumeLODChanged(const LLSD& newvalue)
{
    LLVOVolume::sLODFactor = llclamp((F32) newvalue.asReal(), 0.01f, MAX_LOD_FACTOR);
    LLVOVolume::sDistanceFactor = 1.f-LLVOVolume::sLODFactor * 0.1f;
    return true;
}

static bool handleAvatarLODChanged(const LLSD& newvalue)
{
    LLVOAvatar::sLODFactor = llclamp((F32) newvalue.asReal(), 0.f, MAX_AVATAR_LOD_FACTOR);
    return true;
}

static bool handleAvatarPhysicsLODChanged(const LLSD& newvalue)
{
    LLVOAvatar::sPhysicsLODFactor = llclamp((F32) newvalue.asReal(), 0.f, MAX_AVATAR_LOD_FACTOR);
    return true;
}

static bool handleTerrainLODChanged(const LLSD& newvalue)
{
        LLVOSurfacePatch::sLODFactor = (F32)newvalue.asReal();
        //sqaure lod factor to get exponential range of [0,4] and keep
        //a value of 1 in the middle of the detail slider for consistency
        //with other detail sliders (see panel_preferences_graphics1.xml)
        LLVOSurfacePatch::sLODFactor *= LLVOSurfacePatch::sLODFactor;
        return true;
}

static bool handleTreeLODChanged(const LLSD& newvalue)
{
    LLVOTree::sTreeFactor = (F32) newvalue.asReal();
    return true;
}

static bool handleFlexLODChanged(const LLSD& newvalue)
{
    LLVolumeImplFlexible::sUpdateFactor = (F32) newvalue.asReal();
    return true;
}

static bool handleGammaChanged(const LLSD& newvalue)
{
    F32 gamma = (F32) newvalue.asReal();
    if (gamma == 0.0f)
    {
        gamma = 1.0f; // restore normal gamma
    }
    if (gViewerWindow && gViewerWindow->getWindow() && gamma != gViewerWindow->getWindow()->getGamma())
    {
        // Only save it if it's changed
        if (!gViewerWindow->getWindow()->setGamma(gamma))
        {
            LL_WARNS() << "setGamma failed!" << LL_ENDL;
        }
    }

    return true;
}

const F32 MAX_USER_FOG_RATIO = 10.f;
const F32 MIN_USER_FOG_RATIO = 0.5f;

static bool handleFogRatioChanged(const LLSD& newvalue)
{
    F32 fog_ratio = llmax(MIN_USER_FOG_RATIO, llmin((F32) newvalue.asReal(), MAX_USER_FOG_RATIO));
    gSky.setFogRatio(fog_ratio);
    return true;
}

static bool handleMaxPartCountChanged(const LLSD& newvalue)
{
    LLViewerPartSim::setMaxPartCount(newvalue.asInteger());
    return true;
}

static bool handleVideoMemoryChanged(const LLSD& newvalue)
{
    gTextureList.updateMaxResidentTexMem(S32Megabytes(newvalue.asInteger()));
    return true;
}

// <FS:Ansariel> Dynamic texture memory calculation
static bool handleDynamicTextureMemoryChanged(const LLSD& newvalue)
{
    if (!newvalue.asBoolean())
    {
        gTextureList.updateMaxResidentTexMem(S32Megabytes(gSavedSettings.getS32("TextureMemory")));
    }
    return true;
}
// </FS:Ansariel>

static bool handleChatFontSizeChanged(const LLSD& newvalue)
{
    if(gConsole)
    {
        gConsole->setFontSize(newvalue.asInteger());
    }
    return true;
}

static bool handleChatPersistTimeChanged(const LLSD& newvalue)
{
    if(gConsole)
    {
        gConsole->setLinePersistTime((F32) newvalue.asReal());
    }
    return true;
}

static bool handleConsoleMaxLinesChanged(const LLSD& newvalue)
{
    if(gConsole)
    {
        gConsole->setMaxLines(newvalue.asInteger());
    }
    return true;
}

static void handleAudioVolumeChanged(const LLSD& newvalue)
{
    audio_update_volume(true);
}

static bool handleJoystickChanged(const LLSD& newvalue)
{
    LLViewerJoystick::getInstance()->setCameraNeedsUpdate(TRUE);
    return true;
}

static bool handleUseOcclusionChanged(const LLSD& newvalue)
{
    LLPipeline::sUseOcclusion = (newvalue.asBoolean() && gGLManager.mHasOcclusionQuery
        && LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion") && !gUseWireframe) ? 2 : 0;
    return true;
}

static bool handleUploadBakedTexOldChanged(const LLSD& newvalue)
{
    LLPipeline::sForceOldBakedUpload = newvalue.asBoolean();
    return true;
}


static bool handleWLSkyDetailChanged(const LLSD&)
{
    if (gSky.mVOWLSkyp.notNull())
    {
        gSky.mVOWLSkyp->updateGeometry(gSky.mVOWLSkyp->mDrawable);
    }
    return true;
}

static bool handleResetVertexBuffersChanged(const LLSD&)
{
    if (gPipeline.isInit())
    {
        gPipeline.resetVertexBuffers();
    }
    return true;
}

static bool handleRepartition(const LLSD&)
{
    if (gPipeline.isInit())
    {
        gOctreeMaxCapacity = gSavedSettings.getU32("OctreeMaxNodeCapacity");
        gOctreeMinSize = gSavedSettings.getF32("OctreeMinimumNodeSize");
        gObjectList.repartitionObjects();
    }
    return true;
}

static bool handleRenderDynamicLODChanged(const LLSD& newvalue)
{
    LLPipeline::sDynamicLOD = newvalue.asBoolean();
    return true;
}

static bool handleRenderLocalLightsChanged(const LLSD& newvalue)
{
    gPipeline.setLightingDetail(-1);
    return true;
}

// [RLVa:KB] - @setsphere
static bool handleWindLightAtmosShadersChanged(const LLSD& newvalue)
{
    LLRenderTarget::sUseFBO = newvalue.asBoolean() && LLPipeline::sUseDepthTexture;
//MK
    // When under @setsphere, WindLightUseAtmosShaders must be kept TRUE.
    if (gRRenabled && gAgent.mRRInterface.mContainsSetsphere && !gSavedSettings.getBOOL("WindLightUseAtmosShaders"))
    {
        gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
        LLRenderTarget::sUseFBO = true;
    }
//mk
    handleSetShaderChanged(LLSD());
    return true;
}
// [/RLVa:KB]

static bool handleRenderDeferredChanged(const LLSD& newvalue)
{
//  LLRenderTarget::sUseFBO = newvalue.asBoolean();
// [RLVa:KB] - @setsphere
    LLRenderTarget::sUseFBO = newvalue.asBoolean() || (gSavedSettings.getBOOL("WindLightUseAtmosShaders") && LLPipeline::sUseDepthTexture);
// [/RLVa:KB]
//MK
    // When under @setsphere, WindLightUseAtmosShaders must be kept TRUE.
    if (gRRenabled && gAgent.mRRInterface.mContainsSetsphere && !gSavedSettings.getBOOL("WindLightUseAtmosShaders"))
    {
        gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
        LLRenderTarget::sUseFBO = true;
    }
//mk
    if (gPipeline.isInit())
    {
        LLPipeline::refreshCachedSettings();
        gPipeline.updateRenderDeferred();
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
        gPipeline.resetVertexBuffers();
        if (LLPipeline::sRenderDeferred == (BOOL)LLRenderTarget::sUseFBO)
        {
            LLViewerShaderMgr::instance()->setShaders();
        }
    }
    return true;
}

// This looks a great deal like handleRenderDeferredChanged because
// Advanced Lighting (Materials) implies bumps and shiny so disabling
// bumps should further disable that feature.
//
static bool handleRenderBumpChanged(const LLSD& newval)
{
    LLRenderTarget::sUseFBO = newval.asBoolean() && gSavedSettings.getBOOL("RenderDeferred");
//  LLRenderTarget::sUseFBO = newval.asBoolean() && gSavedSettings.getBOOL("RenderDeferred");
// [RLVa:KB] - @setsphere
    LLRenderTarget::sUseFBO = LLRenderTarget::sUseFBO || (gSavedSettings.getBOOL("WindLightUseAtmosShaders") && LLPipeline::sUseDepthTexture);
// [/RLVa:KB]

//MK
    // When under @setsphere, WindLightUseAtmosShaders must be kept TRUE.
    if (gRRenabled && gAgent.mRRInterface.mContainsSetsphere && !gSavedSettings.getBOOL("WindLightUseAtmosShaders"))
    {
        gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
        LLRenderTarget::sUseFBO = true;
    }
//mk
    if (gPipeline.isInit())
    {
        gPipeline.updateRenderBump();
        gPipeline.updateRenderDeferred();
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
        gPipeline.resetVertexBuffers();
        LLViewerShaderMgr::instance()->setShaders();
    }
    return true;
}

static bool handleRenderDebugPipelineChanged(const LLSD& newvalue)
{
    gDebugPipeline = newvalue.asBoolean();
    return true;
}

static bool handleRenderResolutionDivisorChanged(const LLSD& newvalue)
{
    // KKA-668 ALM and high values of RenderResolutionDivisor can generate
    // high CPU and/or GPU usage. This is a protective measure that turns off
    // ALM when RRD goes over a threshold (default is 16) and remembers it
    // for when RRD goes below the threshold (which could be at the next login
    // if the RRD change came from RLV rather than being manually entered 
    // as a debug setting)
    U32 threshold = gSavedSettings.getU32( "KokuaDisableALMWhileRRDExceeds" );
    bool restoreALMafterRRD = gSavedSettings.getBOOL( "KokuaReinstateALM" );
    gResizeScreenTexture = TRUE;
    if (threshold > 0)
    {
        U32 new_rrd = newvalue.asInteger();

        if (new_rrd > threshold && gSavedSettings.getBOOL( "RenderDeferred" ))
        {
            gSavedSettings.setBOOL( "KokuaReinstateALM", TRUE );
            gSavedSettings.setBOOL( "RenderDeferred", FALSE );
        }
        else if (restoreALMafterRRD && new_rrd <= threshold )
        {
            gSavedSettings.setBOOL( "KokuaReinstateALM", FALSE );
            gSavedSettings.setBOOL( "RenderDeferred", TRUE );
        }
    }
    return true;
}

static bool handleDebugViewsChanged(const LLSD& newvalue)
{
    LLView::sDebugRects = newvalue.asBoolean();
    return true;
}

static bool handleLogFileChanged(const LLSD& newvalue)
{
    std::string log_filename = newvalue.asString();
    LLFile::remove(log_filename);
    LLError::logToFile(log_filename);
    return true;
}

bool handleHideGroupTitleChanged(const LLSD& newvalue)
{
    gAgent.setHideGroupTitle(newvalue);
    return true;
}

bool handleEffectColorChanged(const LLSD& newvalue)
{
    gAgent.setEffectColor(LLColor4(newvalue));
    return true;
}

bool handleHighResSnapshotChanged(const LLSD& newvalue)
{
    // High Res Snapshot active, must uncheck RenderUIInSnapshot
    if (newvalue.asBoolean())
    {
        gSavedSettings.setBOOL( "RenderUIInSnapshot", FALSE );
    }
    return true;
}

bool handleVoiceClientPrefsChanged(const LLSD& newvalue)
{
    if (LLVoiceClient::instanceExists())
    {
        LLVoiceClient::getInstance()->updateSettings();
    }
    return true;
}

bool handleVelocityInterpolate(const LLSD& newvalue)
{
    LLMessageSystem* msg = gMessageSystem;
    if ( newvalue.asBoolean() )
    {
        msg->newMessageFast(_PREHASH_VelocityInterpolateOn);
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        gAgent.sendReliableMessage();
        LL_INFOS() << "Velocity Interpolation On" << LL_ENDL;
    }
    else
    {
        msg->newMessageFast(_PREHASH_VelocityInterpolateOff);
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        gAgent.sendReliableMessage();
        LL_INFOS() << "Velocity Interpolation Off" << LL_ENDL;
    }
    return true;
}

bool handleForceShowGrid(const LLSD& newvalue)
{
    LLPanelLogin::updateLocationSelectorsVisibility();
    return true;
}

bool handleLoginLocationChanged()
{
    /*
     * This connects the default preference setting to the state of the login
     * panel if it is displayed; if you open the preferences panel before
     * logging in, and change the default login location there, the login
     * panel immediately changes to match your new preference.
     */
    std::string new_login_location = gSavedSettings.getString("LoginLocation");
    LL_DEBUGS("AppInit")<<new_login_location<<LL_ENDL;
    LLStartUp::setStartSLURL(LLSLURL(new_login_location));
    return true;
}

bool handleSpellCheckChanged()
{
    if (gSavedSettings.getBOOL("SpellCheck"))
    {
        std::list<std::string> dict_list;
        std::string dict_setting = gSavedSettings.getString("SpellCheckDictionary");
        boost::split(dict_list, dict_setting, boost::is_any_of(std::string(",")));
        if (!dict_list.empty())
        {
            LLSpellChecker::setUseSpellCheck(dict_list.front());
            dict_list.pop_front();
            LLSpellChecker::instance().setSecondaryDictionaries(dict_list);
            return true;
        }
    }
    LLSpellChecker::setUseSpellCheck(LLStringUtil::null);
    return true;
}

bool toggle_agent_pause(const LLSD& newvalue)
{
    if ( newvalue.asBoolean() )
    {
        send_agent_pause();
    }
    else
    {
        send_agent_resume();
    }
    return true;
}

bool toggle_show_navigation_panel(const LLSD& newvalue)
{
    bool value = newvalue.asBoolean();

    LLNavigationBar::getInstance()->setVisible(value);
//MK
    if (gStatusBar)
    {
        // if we show the navigation bar or the mini location bar, we don't need the parcel info and sliders on the top status bar
        // and vice-versa
        BOOL minilocation_visible = gSavedSettings.getBOOL("ShowMiniLocationPanel");
        gStatusBar->childSetVisible("parcel_info_panel", !value && !minilocation_visible);
        gStatusBar->childSetVisible("drawdistance", !value && !minilocation_visible);
        gStatusBar->childSetVisible("avatar_z_offset", !value && !minilocation_visible);
        gStatusBar->childSetVisible("avatar_z_offset_reset_btn", !value && !minilocation_visible);
    }
//  if (!gRRenabled)
    else
//mk
    gSavedSettings.setBOOL("ShowMiniLocationPanel", !value);

    return true;
}

bool toggle_show_mini_location_panel(const LLSD& newvalue)
{
    bool value = newvalue.asBoolean();

    LLPanelTopInfoBar::getInstance()->setVisible(value);
//MK
    if (gStatusBar)
    {
        // if we show the navigation bar or the mini location bar, we don't need the parcel info and sliders on the top status bar
        // and vice-versa
        BOOL navbar_visible = gSavedSettings.getBOOL("ShowNavbarNavigationPanel");
        gStatusBar->childSetVisible("parcel_info_panel", !value && !navbar_visible);
        gStatusBar->childSetVisible("drawdistance", !value && !navbar_visible);
        gStatusBar->childSetVisible("avatar_z_offset", !value && !navbar_visible);
        gStatusBar->childSetVisible("avatar_z_offset_reset_btn", !value && !navbar_visible);
    }
//  if (!gRRenabled)
    else
//mk
    gSavedSettings.setBOOL("ShowNavbarNavigationPanel", !value);

    return true;
}

bool toggle_show_object_render_cost(const LLSD& newvalue)
{
    LLFloaterTools::sShowObjectCost = newvalue.asBoolean();
    return true;
}

void handleRenderAutoMuteByteLimitChanged(const LLSD& new_value);

// <FS:Ansariel> Output device selection
// <FS:CR> Posestand Ground Lock
static void handleSetPoseStandLock(const LLSD& newvalue)
{
    FSFloaterPoseStand* pose_stand = LLFloaterReg::findTypedInstance<FSFloaterPoseStand>("fs_posestand");
    if (pose_stand)
    {
        pose_stand->setLock(newvalue);
        pose_stand->onCommitCombo();
    }
        
}
// </FS:CR> Posestand Ground Lock
void handleOutputDeviceChanged(const LLSD& newvalue)
{
   if (gAudiop)
   {
        gAudiop->setDevice(newvalue.asUUID());
   }
}
// </FS:Ansariel>
// <FS:Ansariel> FIRE-18250: Option to disable default eye movement
void handleStaticEyesChanged()
{
    if (!isAgentAvatarValid())
    {
        return;
    }

    LLUUID anim_id(gSavedSettings.getString("FSStaticEyesUUID"));
    if (gSavedPerAccountSettings.getBOOL("FSStaticEyes"))
    {
        gAgentAvatarp->startMotion(anim_id);
        gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_START);
    }
    else
    {
        gAgentAvatarp->stopMotion(anim_id);
        gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_STOP);
    }
}
// </FS:Ansariel>

// <FS:Ansariel> FIRE-20288: Option to render friends only
void handleRenderFriendsOnlyChanged(const LLSD& newvalue)
{
    if (newvalue.asBoolean() && !(gRRenabled && gAgent.mRRInterface.mContainsShownames))
    {
        for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
            iter != LLCharacter::sInstances.end(); ++iter)
        {
            LLVOAvatar* avatar = (LLVOAvatar*)*iter;

            if (avatar->getID() != gAgentID && !LLAvatarActions::isFriend(avatar->getID()) && !avatar->isControlAvatar())
            {
                gObjectList.killObject(avatar);
                if (LLViewerRegion::sVOCacheCullingEnabled && avatar->getRegion())
                {
                    avatar->getRegion()->killCacheEntry(avatar->getLocalID());
                }
            }
        }
    }
}
// </FS:Ansariel>
////////////////////////////////////////////////////////////////////////////


LLPointer<LLControlVariable> setting_get_control(LLControlGroup& group, const std::string& setting)
{
    LLPointer<LLControlVariable> cntrl_ptr = group.getControl(setting);
    if (cntrl_ptr.isNull())
    {
        LL_ERRS() << "Unable to set up setting listener for " << setting
            << ". Please reinstall viewer from  https ://secondlife.com/support/downloads/ and contact https://support.secondlife.com if issue persists after reinstall."
            << LL_ENDL;
    }
    return cntrl_ptr;
}

void setting_setup_signal_listener(LLControlGroup& group, const std::string& setting, std::function<void(const LLSD& newvalue)> callback)
{
    setting_get_control(group, setting)->getSignal()->connect([callback](LLControlVariable* control, const LLSD& new_val, const LLSD& old_val)
    {
        callback(new_val);
    });
}

void setting_setup_signal_listener(LLControlGroup& group, const std::string& setting, std::function<void()> callback)
{
    setting_get_control(group, setting)->getSignal()->connect([callback](LLControlVariable* control, const LLSD& new_val, const LLSD& old_val)
    {
        callback();
    });
}

void settings_setup_listeners()
{
//MK
    setting_setup_signal_listener(gSavedSettings, "RestrainedLoveDebug",handleRestrainedLoveDebugChanged);
    setting_setup_signal_listener(gSavedSettings, "RestrainedLoveLogging",handleRestrainedLoveLoggingChanged);
    setting_setup_signal_listener(gSavedSettings, "RestrainedLoveCommandLogging",handleRestrainedLoveCommandLoggingChanged);
    gSavedPerAccountSettings.getControl("RestrainedLoveLastStandingLocation")->getSignal()->connect(boost::bind(&handleRestrainedLoveLastStandingLocationChanged, _2));
    setting_setup_signal_listener(gSavedSettings, "RenderDeferredShowInvisiprims",handleRenderDeferredShowInvisiprimsChanged);
    setting_setup_signal_listener(gSavedSettings, "RestrainedLoveCamDistNbGradients",handleRestrainedLoveCamDistNbGradientsChanged);
    setting_setup_signal_listener(gSavedSettings, "RestrainedLoveHeadMouselookRenderRigged",handleRestrainedLoveHeadMouselookRenderRigged);
    setting_setup_signal_listener(gSavedSettings, "RestrainedLoveRenderInvisibleSurfaces",handleRestrainedLoveRenderInvisibleSurfaces);
//mk
    setting_setup_signal_listener(gSavedSettings, "FirstPersonAvatarVisible", handleRenderAvatarMouselookChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFarClip", handleRenderFarClipChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainDetail", handleTerrainDetailChanged);
    setting_setup_signal_listener(gSavedSettings, "OctreeStaticObjectSizeFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeDistanceFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeMaxNodeCapacity", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeAlphaDistanceFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeAttachmentSizeFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "RenderMaxTextureIndex", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderUseTriStrips", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderUIBuffer", handleWindowResized);
    setting_setup_signal_listener(gSavedSettings, "RenderDepthOfField", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFSAASamples", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderSpecularResX", handleLUTBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderSpecularResY", handleLUTBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderSpecularExponent", handleLUTBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAnisotropic", handleAnisotropicChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderShadowResolutionScale", handleShadowsResized);
    setting_setup_signal_listener(gSavedSettings, "RenderGlow", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGlow", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGlowResolutionPow", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAvatarCloth", handleSetShaderChanged);
//  setting_setup_signal_listener(gSavedSettings, "WindLightUseAtmosShaders",handleSetShaderChanged);
// [RLVa:KB] - @setsphere
    setting_setup_signal_listener(gSavedSettings, "WindLightUseAtmosShaders",handleWindLightAtmosShadersChanged);
// [/RLVa:KB]
    setting_setup_signal_listener(gSavedSettings, "RenderGammaFull", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderVolumeLODFactor", handleVolumeLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAvatarLODFactor", handleAvatarLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAvatarPhysicsLODFactor", handleAvatarPhysicsLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainLODFactor", handleTerrainLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTreeLODFactor", handleTreeLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFlexTimeFactor", handleFlexLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGamma", handleGammaChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFogRatio", handleFogRatioChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderMaxPartCount", handleMaxPartCountChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDynamicLOD", handleRenderDynamicLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderLocalLights", handleRenderLocalLightsChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDebugTextureBind", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAutoMaskAlphaDeferred", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAutoMaskAlphaNonDeferred", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderObjectBump", handleRenderBumpChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderMaxVBOSize", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderVSyncEnable", handleVSyncChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDeferredNoise", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDebugPipeline", handleRenderDebugPipelineChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderResolutionDivisor", handleRenderResolutionDivisorChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDeferred", handleRenderDeferredChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderShadowDetail", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDeferredSSAO", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderPerformanceTest", handleRenderPerfTestChanged);
    setting_setup_signal_listener(gSavedSettings, "TextureMemory", handleVideoMemoryChanged);
    setting_setup_signal_listener(gSavedSettings, "ChatFontSize", handleChatFontSizeChanged);
    setting_setup_signal_listener(gSavedSettings, "ChatPersistTime", handleChatPersistTimeChanged);
    setting_setup_signal_listener(gSavedSettings, "ConsoleMaxLines", handleConsoleMaxLinesChanged);
    setting_setup_signal_listener(gSavedSettings, "UploadBakedTexOld", handleUploadBakedTexOldChanged);
    setting_setup_signal_listener(gSavedSettings, "UseOcclusion", handleUseOcclusionChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMaster", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelSFX", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelUI", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelAmbient", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMusic", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMedia", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelVoice", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelDoppler", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelRolloff", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelUnderwaterRolloff", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteAudio", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteMusic", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteMedia", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteVoice", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteAmbient", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteUI", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderVBOEnable", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderUseVAO", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderVBOMappingDisable", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderUseStreamVBO", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderPreferStreamDraw", handleResetVertexBuffersChanged);
    setting_setup_signal_listener(gSavedSettings, "WLSkyDetail", handleWLSkyDetailChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis6", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale6", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone6", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "DebugViews", handleDebugViewsChanged);
    setting_setup_signal_listener(gSavedSettings, "UserLogFile", handleLogFileChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderHideGroupTitle", handleHideGroupTitleChanged);
    setting_setup_signal_listener(gSavedSettings, "HighResSnapshot", handleHighResSnapshotChanged);
    setting_setup_signal_listener(gSavedSettings, "EnableVoiceChat", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "PTTCurrentlyEnabled", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "PushToTalkButton", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "PushToTalkToggle", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceEarLocation", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceInputAudioDevice", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceOutputAudioDevice", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMic", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "LipSyncEnabled", handleVoiceClientPrefsChanged); 
    setting_setup_signal_listener(gSavedSettings, "VelocityInterpolate", handleVelocityInterpolate);
    setting_setup_signal_listener(gSavedSettings, "QAMode", show_debug_menus);
    setting_setup_signal_listener(gSavedSettings, "UseDebugMenus", show_debug_menus);
    setting_setup_signal_listener(gSavedSettings, "ShowRlvMenu",show_debug_menus);
    setting_setup_signal_listener(gSavedSettings, "KokuaClassicMainMenu", kokua_menus);
    setting_setup_signal_listener(gSavedSettings, "AgentPause", toggle_agent_pause);
    setting_setup_signal_listener(gSavedSettings, "ShowNavbarNavigationPanel", toggle_show_navigation_panel);
    setting_setup_signal_listener(gSavedSettings, "ShowMiniLocationPanel", toggle_show_mini_location_panel);
    setting_setup_signal_listener(gSavedSettings, "ShowObjectRenderingCost", toggle_show_object_render_cost);
    setting_setup_signal_listener(gSavedSettings, "ForceShowGrid", handleForceShowGrid);
    setting_setup_signal_listener(gSavedSettings, "RenderTransparentWater", handleRenderTransparentWaterChanged);
    setting_setup_signal_listener(gSavedSettings, "SpellCheck", handleSpellCheckChanged);
    setting_setup_signal_listener(gSavedSettings, "SpellCheckDictionary", handleSpellCheckChanged);
    setting_setup_signal_listener(gSavedSettings, "LoginLocation", handleLoginLocationChanged);
    setting_setup_signal_listener(gSavedSettings, "DebugAvatarJoints", handleDebugAvatarJointsChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAutoMuteByteLimit", handleRenderAutoMuteByteLimitChanged);

    setting_setup_signal_listener(gSavedPerAccountSettings, "AvatarHoverOffsetZ", handleAvatarHoverOffsetChanged);
    // <FS:Ansariel> Output device selection
    // <FS:CR> Pose stand ground lock
    gSavedSettings.getControl("FSPoseStandLock")->getSignal()->connect(boost::bind(&handleSetPoseStandLock, _2));
    gSavedSettings.getControl("FSOutputDeviceUUID")->getSignal()->connect(boost::bind(&handleOutputDeviceChanged, _2));
    // <FS:Ansariel> FIRE-18250: Option to disable default eye movement
    gSavedSettings.getControl("FSStaticEyesUUID")->getSignal()->connect(boost::bind(&handleStaticEyesChanged));
    gSavedPerAccountSettings.getControl("FSStaticEyes")->getSignal()->connect(boost::bind(&handleStaticEyesChanged));
    // <FS:Ansariel> FIRE-20288: Option to render friends only
    gSavedPerAccountSettings.getControl("FSRenderFriendsOnly")->getSignal()->connect(boost::bind(&handleRenderFriendsOnlyChanged, _2));
    // KKA-668 See if we can re-enable ALM (yes if RLV set RRD and the setting didn't persist, no if RRD was set manually and is still over threshold)
    if (gSavedSettings.getBOOL("KokuaReinstateALM"))
    {
         handleRenderResolutionDivisorChanged(gSavedSettings.getLLSD("RenderResolutionDivisor"));
    }

    // <FS:Ansariel> Dynamic texture memory calculation
    gSavedSettings.getControl("FSDynamicTextureMemory")->getSignal()->connect(boost::bind(&handleDynamicTextureMemoryChanged, _2));
}

#if TEST_CACHED_CONTROL

#define DECL_LLCC(T, V) static LLCachedControl<T> mySetting_##T("TestCachedControl"#T, V)
DECL_LLCC(U32, (U32)666);
DECL_LLCC(S32, (S32)-666);
DECL_LLCC(F32, (F32)-666.666);
DECL_LLCC(bool, true);
DECL_LLCC(BOOL, FALSE);
static LLCachedControl<std::string> mySetting_string("TestCachedControlstring", "Default String Value");
DECL_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
DECL_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
DECL_LLCC(LLRect, LLRect(0, 0, 100, 500));
DECL_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
DECL_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
DECL_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));

LLSD test_llsd = LLSD()["testing1"] = LLSD()["testing2"];
DECL_LLCC(LLSD, test_llsd);

static LLCachedControl<std::string> test_BrowserHomePage("BrowserHomePage", "hahahahahha", "Not the real comment");

void test_cached_control()
{
#define do { TEST_LLCC(T, V) if((T)mySetting_##T != V) LL_ERRS() << "Fail "#T << LL_ENDL; } while(0)
    TEST_LLCC(U32, 666);
    TEST_LLCC(S32, (S32)-666);
    TEST_LLCC(F32, (F32)-666.666);
    TEST_LLCC(bool, true);
    TEST_LLCC(BOOL, FALSE);
    if((std::string)mySetting_string != "Default String Value") LL_ERRS() << "Fail string" << LL_ENDL;
    TEST_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
    TEST_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
    TEST_LLCC(LLRect, LLRect(0, 0, 100, 500));
    TEST_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
    TEST_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
    TEST_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));
//There's no LLSD comparsion for LLCC yet. TEST_LLCC(LLSD, test_llsd); 

    if((std::string)test_BrowserHomePage != "http://www.secondlife.com") LL_ERRS() << "Fail BrowserHomePage" << LL_ENDL;
}
#endif // TEST_CACHED_CONTROL

