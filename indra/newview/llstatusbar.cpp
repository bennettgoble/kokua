/** 
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llstatusbar.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llaudioengine.h"
#include "llbutton.h"
#include "llcommandhandler.h"
#include "llfirstuse.h"
#include "llviewercontrol.h"
#include "llfloaterbuycurrency.h"
#include "llbuycurrencyhtml.h"
#include "llpanelnearbymedia.h"
#include "llpanelpresetscamerapulldown.h"
#include "llpanelpresetspulldown.h"
#include "llpanelvolumepulldown.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llhints.h"
#include "llhudicon.h"
#include "llnavigationbar.h"
#include "llkeyboard.h"
#include "lllayoutstack.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llrootview.h"
#include "llsd.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerparcelmedia.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatarself.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llstatgraph.h"
#include "llvieweraudio.h"
#include "llviewermedia.h"
#include "llviewermenu.h"   // for gMenuBarView
#include "llviewerparcelmgr.h"
#include "llviewerthrottle.h"
#include "lluictrlfactory.h"

//MK
#include "llagentui.h"
#include "llclipboard.h"
#include "llfloatersidepanelcontainer.h"
#include "lllandmarkactions.h"
#include "lllocationinputctrl.h"
#include "llparcel.h"
#include "llslurl.h"
#include "llviewerinventory.h"
//mk
#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "llappviewer.h"
#include "lltrans.h"

// library includes
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"
#include "llsearchableui.h"
#include "llsearcheditor.h"

// system includes
#include <iomanip>

//
// Globals
//
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 26;
extern S32 MENU_BAR_HEIGHT;


// TODO: these values ought to be in the XML too
const LLColor4 SIM_OK_COLOR(0.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_WARN_COLOR(1.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_FULL_COLOR(1.f, 0.f, 0.f, 1.f);
const F32 ICON_TIMER_EXPIRY     = 3.f; // How long the balance and health icons should flash after a change.

static void onClickVolume(void*);

//MK
const S32 MENU_PARCEL_SPACING = 1;

class LLStatusBar::LLParcelChangeObserver : public LLParcelObserver
{
public:
    LLParcelChangeObserver(LLStatusBar* topInfoBar) : mTopInfoBar(topInfoBar) {}

private:
    /*virtual*/ void changed()
    {
        if (mTopInfoBar)
        {
            mTopInfoBar->updateParcelIcons();
        }
    }

    LLStatusBar* mTopInfoBar;
};
//mk

LLStatusBar::LLStatusBar(const LLRect& rect)
:   LLPanel(),
    mTextTime(NULL),
    mSGBandwidth(NULL),
    mSGPacketLoss(NULL),
    mFPSText(NULL),
    mDrawDistancePanel(NULL),
    mStatisticsPanel(NULL),
    mFPSPanel(NULL),
    mBtnVolume(NULL),
    mBoxBalance(NULL),
    mInMouselookMode(false),
    mBalance(0),
    mHealth(100),
    mSquareMetersCredit(0),
    mSquareMetersCommitted(0),
    mAudioStreamEnabled(FALSE), // ## Zi: Media/Stream separation
    mMediaToggle(NULL),
    mMouseEnterVolumeConnection(),
    mMouseEnterNearbyMediaConnection(),    
    mFilterEdit(NULL),          // Edit for filtering
    mSearchPanel(NULL)          // Panel for filtering
{
    setRect(rect);
    
    // status bar can possible overlay menus?
    setMouseOpaque(FALSE);

    mBalanceTimer = new LLFrameTimer();
    mHealthTimer = new LLFrameTimer();

//MK
    LLUICtrl::CommitCallbackRegistry::currentRegistrar()
            .add("TopInfoBar.Action", boost::bind(&LLStatusBar::onContextMenuItemClicked, this, _2));
//mk

    buildFromFile("panel_status_bar.xml");
}

LLStatusBar::~LLStatusBar()
{
    delete mBalanceTimer;
    mBalanceTimer = NULL;

    delete mHealthTimer;
    mHealthTimer = NULL;
    // <FS:Ansariel> FIRE-19697: Add setting to disable graphics preset menu popup on mouse over
    //NP graphics presets no longer disabled
    if (mMouseEnterVolumeConnection.connected())
    {
        mMouseEnterVolumeConnection.disconnect();
    }
    if (mMouseEnterNearbyMediaConnection.connected())
    {
        mMouseEnterNearbyMediaConnection.disconnect();
    }
    // </FS:Ansariel>    

//MK
    if (mParcelChangedObserver)
    {
        LLViewerParcelMgr::getInstance()->removeObserver(mParcelChangedObserver);
        delete mParcelChangedObserver;
    }

    if (mParcelPropsCtrlConnection.connected())
    {
        mParcelPropsCtrlConnection.disconnect();
    }

    if (mParcelMgrConnection.connected())
    {
        mParcelMgrConnection.disconnect();
    }

    if (mShowCoordsCtrlConnection.connected())
    {
        mShowCoordsCtrlConnection.disconnect();
    }
//mk

    // LLView destructor cleans up children
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
    refresh();
//MK
    updateParcelInfoText();
    updateHealth();
//mk

    LLPanel::draw();
}

BOOL LLStatusBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    show_navbar_context_menu(this,x,y);
    return TRUE;
}

BOOL LLStatusBar::postBuild()
{
    gMenuBarView->setRightMouseDownCallback(boost::bind(&show_navbar_context_menu, _1, _2, _3));

    mTextTime = getChild<LLTextBox>("TimeText" );
    mPurchasePanel = getChild<LLLayoutPanel>("purchase_panel");
    
    getChild<LLUICtrl>("buyL")->setCommitCallback(
        boost::bind(&LLStatusBar::onClickBuyCurrency, this));

    getChild<LLUICtrl>("goShop")->setCommitCallback(boost::bind(&LLWeb::loadURL, gSavedSettings.getString("MarketplaceURL"), LLStringUtil::null, LLStringUtil::null));

    mBoxBalance = getChild<LLTextBox>("balance");
    mBoxBalance->setClickedCallback( &LLStatusBar::onClickBalance, this );

    mIconPresetsCamera = getChild<LLIconCtrl>( "presets_icon_camera" );
    mIconPresetsCamera->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterPresetsCamera, this));

    mIconPresetsGraphic = getChild<LLIconCtrl>( "presets_icon_graphic" );
    mIconPresetsGraphic->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterPresets, this));

    mBtnVolume = getChild<LLButton>( "volume_btn" );
    mBtnVolume->setClickedCallback( onClickVolume, this );
 
    if (gSavedSettings.getBOOL("ShowMediaPopupsOnRollover"))
    {
        mMouseEnterVolumeConnection = mBtnVolume->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterVolume, this));
    }
    // </FS: KC> FIRE-19697: Add setting to disable status bar icon menu popup on mouseover

    // <FS:Zi> Media/Stream separation
    mStreamToggle = getChild<LLButton>("stream_toggle_btn");
    mStreamToggle->setClickedCallback(&LLStatusBar::onClickStreamToggle, this);
    // </FS:Zi> Media/Stream separation

    mMediaToggle = getChild<LLButton>("media_toggle_btn");
    mMediaToggle->setClickedCallback( &LLStatusBar::onClickMediaToggle, this );
    // <FS: KC> FIRE-19697: Add setting to disable status bar icon menu popup on mouseover
    // mMediaToggle->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterNearbyMedia, this));
    if (gSavedSettings.getBOOL("ShowMediaPopupsOnRollover"))
    {
        mMouseEnterNearbyMediaConnection = mMediaToggle->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterNearbyMedia, this));
    }
    // </FS: KC> FIRE-19697: Add setting to disable status bar icon menu popup on mouseover

    LLHints::getInstance()->registerHintTarget("linden_balance", getChild<LLView>("balance_bg")->getHandle());

    gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&LLStatusBar::onVolumeChanged, this, _2));
    // <FS:Ansariel> FIRE-19697: Add setting to disable graphics preset menu popup on mouse over
    //NP graphics presets no longer disabled
    gSavedSettings.getControl("ShowMediaPopupsOnRollover")->getSignal()->connect(boost::bind(&LLStatusBar::onPopupRolloverChanged, this, _2));    

    mSGBandwidth = getChild<LLStatGraph>("bandwidth_graph");
    if (mSGBandwidth) {
        mSGBandwidth->setStat(&LLStatViewer::ACTIVE_MESSAGE_DATA_RECEIVED);
        mSGBandwidth->setClickedCallback(boost::bind(&LLStatusBar::onClickStatistics, this));
    }
    mSGPacketLoss = getChild<LLStatGraph>("packet_loss_graph");
    if (mSGPacketLoss) {
        mSGPacketLoss->setStat(&LLStatViewer::PACKETS_LOST_PERCENT);
        mSGPacketLoss->setClickedCallback(boost::bind(&LLStatusBar::onClickStatistics, this));
    }
    // KKA-821 Add script percent run, script time and spare time. The cast is necessary to get from derived class SimMeasurement to its parent which llui understands
    mSGScriptPctRun = getChild<LLStatGraph>("script_pct_run_graph");
    if (mSGScriptPctRun) {
      //inverse, so the more bar is visible the worse (lower) the script percent run figure is
        mSGScriptPctRun->setStat((LLTrace::SampleStatHandle<LLUnit<F64, LLUnits::Percent> > *)&LLStatViewer::SIM_PERCENTAGE_SCRIPTS_RUN);
        mSGScriptPctRun->setClickedCallback(boost::bind(&LLStatusBar::onClickStatistics, this));
    }
    mSGScriptTime = getChild<LLStatGraph>("script_time_graph");
    if (mSGScriptTime ) {
      // not inverse, so the more bar, the more time scripts are using
        mSGScriptTime->setStat((LLTrace::SampleStatHandle<F64Milliseconds > *)&LLStatViewer::SIM_SCRIPTS_TIME);
        mSGScriptTime->setClickedCallback(boost::bind(&LLStatusBar::onClickStatistics, this));
    }
    mSGSpareTime = getChild<LLStatGraph>("spare_time_graph");
    if (mSGSpareTime ) {
      // inverse, so no free frame time results in a full bar
        mSGSpareTime->setStat((LLTrace::SampleStatHandle<F64Milliseconds >  *)&LLStatViewer::SIM_SPARE_TIME);
        mSGSpareTime->setClickedCallback(boost::bind(&LLStatusBar::onClickStatistics, this));
    }

    mFPSText = getChild<LLTextBox>("fps_text");
    if (mFPSText) {
        mFPSText->setClickedCallback(boost::bind(&LLStatusBar::onClickStatistics, this));
    }

    mDrawDistancePanel = getChild<LLLayoutPanel>("draw_distance_panel");
    mStatisticsPanel = getChild<LLLayoutPanel>("statistics_panel");
    mFPSPanel = getChild<LLLayoutPanel>("fps_panel");

    mPanelPresetsCameraPulldown = new LLPanelPresetsCameraPulldown();
    addChild(mPanelPresetsCameraPulldown);
    mPanelPresetsCameraPulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelPresetsCameraPulldown->setVisible(FALSE);

    mPanelPresetsPulldown = new LLPanelPresetsPulldown();
    addChild(mPanelPresetsPulldown);
    mPanelPresetsPulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelPresetsPulldown->setVisible(FALSE);

    mPanelVolumePulldown = new LLPanelVolumePulldown();
    addChild(mPanelVolumePulldown);
    mPanelVolumePulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelVolumePulldown->setVisible(FALSE);

    mPanelNearByMedia = new LLPanelNearByMedia();
    addChild(mPanelNearByMedia);
    mPanelNearByMedia->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
    mPanelNearByMedia->setVisible(FALSE);

    mScriptOut = getChildView("scriptout");
    updateBalancePanelPosition();

//MK
    mParcelInfoPanel = getChild<LLPanel>("parcel_info_panel");
    mParcelInfoText = getChild<LLTextBox>("parcel_info_text");
    mDamageText = getChild<LLTextBox>("damage_text");

    mInfoBtn = getChild<LLButton>("place_info_btn");
    mInfoBtn->setClickedCallback(boost::bind(&LLStatusBar::onInfoButtonClicked, this));
    mInfoBtn->setToolTip(LLTrans::getString("LocationCtrlInfoBtnTooltip"));

    mAvatarHeightOffsetResetBtn = getChild<LLButton>("avatar_z_offset_reset_btn");
    mAvatarHeightOffsetResetBtn->setClickedCallback(boost::bind(&LLStatusBar::onAvatarHeightOffsetResetButtonClicked, this));

    initParcelIcons();

    mParcelChangedObserver = new LLParcelChangeObserver(this);
    LLViewerParcelMgr::getInstance()->addObserver(mParcelChangedObserver);

    // Connecting signal for updating parcel icons on "Show Parcel Properties" setting change.
    LLControlVariable* ctrl = gSavedSettings.getControl("NavBarShowParcelProperties").get();
    if (ctrl)
    {
        mParcelPropsCtrlConnection = ctrl->getSignal()->connect(boost::bind(&LLStatusBar::updateParcelIcons, this));
    }

    // Connecting signal for updating parcel text on "Show Coordinates" setting change.
    ctrl = gSavedSettings.getControl("NavBarShowCoordinates").get();
    if (ctrl)
    {
        mShowCoordsCtrlConnection = ctrl->getSignal()->connect(boost::bind(&LLStatusBar::onNavBarShowParcelPropertiesCtrlChanged, this));
    }

    mParcelMgrConnection = gAgent.addParcelChangedCallback(
            boost::bind(&LLStatusBar::onAgentParcelChange, this));
//mk

    // Hook up and init for filtering
    mFilterEdit = getChild<LLSearchEditor>( "search_menu_edit" );
    mSearchPanel = getChild<LLPanel>( "menu_search_panel" );

    BOOL search_panel_visible = gSavedSettings.getBOOL("MenuSearch");
    mSearchPanel->setVisible(search_panel_visible);
    mFilterEdit->setKeystrokeCallback(boost::bind(&LLStatusBar::onUpdateFilterTerm, this));
    mFilterEdit->setCommitCallback(boost::bind(&LLStatusBar::onUpdateFilterTerm, this));
    collectSearchableItems();
    gSavedSettings.getControl("MenuSearch")->getCommitSignal()->connect(boost::bind(&LLStatusBar::updateMenuSearchVisibility, this, _2));

    if (search_panel_visible)
    {
        updateMenuSearchPosition();
    }

    return TRUE;
}

// Per-frame updates of visibility
void LLStatusBar::refresh()
{
    if (mStatusBarUpdateTimer.getElapsedTimeF32() < 0.5f || mInMouselookMode) {
        //
        //  only update the status bar twice per second rather
        //  than on every frame
        //
        //  also, if we are in mouselook mode then there's no
        //  need to update the status bar at all
        //
        return;
    }

    mStatusBarUpdateTimer.reset();

    static LLCachedControl<bool> net_stats_visible(gSavedSettings, "ShowNetStats", true);
    static LLCachedControl<bool> fps_stats_visible(gSavedSettings, "ShowFPSStats", true);
    static LLCachedControl<bool> show_draw_distance(gSavedSettings, "ShowDDSlider", false);
    static LLCachedControl<bool> show_media_popups(gSavedSettings, "ShowMediaPopupsOnRollover", true);
    //KKA-821 allow for easy changing of the crossover points
    static LLCachedControl<F32> pctrun_threshold1(gSavedSettings, "KokuaStatGraphPctRunThreshold1", 33.f);
    static LLCachedControl<F32> pctrun_threshold2(gSavedSettings, "KokuaStatGraphPctRunThreshold2", 66.f);
    static LLCachedControl<F32> scripttime_threshold1(gSavedSettings, "KokuaStatGraphScriptTimeThreshold1", 12.f);
    static LLCachedControl<F32> scripttime_threshold2(gSavedSettings, "KokuaStatGraphScriptTimeThreshold2", 18.f);
    static LLCachedControl<F32> sparetime_threshold1(gSavedSettings, "KokuaStatGraphSpareTimeThreshold1", 2.f);
    static LLCachedControl<F32> sparetime_threshold2(gSavedSettings, "KokuaStatGraphSpareTimeThreshold2", 4.f);
    //
    //  update the netstat graph and FPS counter text
    //
    if (net_stats_visible) {
        F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1024.f;

        mSGBandwidth->setMax(bwtotal * 1.25f);
        mSGBandwidth->setThreshold(1, bwtotal * 0.60f);
        mSGBandwidth->setThreshold(2, bwtotal);
        //KKA-821 update the crossover points
        mSGScriptPctRun->setThreshold(1, pctrun_threshold1);
        mSGScriptPctRun->setThreshold(2, pctrun_threshold2);
        mSGScriptTime->setThreshold(1, scripttime_threshold1);
        mSGScriptTime->setThreshold(2, scripttime_threshold2);
        mSGSpareTime->setThreshold(1, sparetime_threshold1);
        mSGSpareTime->setThreshold(2, sparetime_threshold2);
    }
    
    if (fps_stats_visible) {
        F32 fps = LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::FPS, 200);

        if (fps >= 100.f) {
            mFPSText->setValue(llformat("%.0f", fps));
        }
        else {
            mFPSText->setValue(llformat("%.1f", fps));
        }
    }

    mDrawDistancePanel->setVisible(show_draw_distance);
    mStatisticsPanel->setVisible(net_stats_visible);
    mFPSPanel->setVisible(fps_stats_visible);

    // update clock every 10 seconds
    if(mClockUpdateTimer.getElapsedTimeF32() > 10.f)
    {
        mClockUpdateTimer.reset();

        // Get current UTC time, adjusted for the user's clock
        // being off.
        time_t utc_time;
        utc_time = time_corrected();

        std::string timeStr = getString("time");
        LLSD substitution;
        substitution["datetime"] = (S32) utc_time;
        LLStringUtil::format (timeStr, substitution);
        mTextTime->setText(timeStr);

        // set the tooltip to have the date
        std::string dtStr = getString("timeTooltip");
        LLStringUtil::format(dtStr, substitution);
//MK
        if (gRRenabled)
        {
            LLViewerRegion* region = gAgent.getRegion();
            if (region)
            {
                dtStr = dtStr + " (" + region->getSimAccessString() + ")";
            }
        }
//mk
        mTextTime->setToolTip(dtStr);
    }

    LLRect r;
    const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();

    // reshape menu bar to its content's width
    if (MENU_RIGHT != gMenuBarView->getRect().getWidth())
    {
        gMenuBarView->reshape(MENU_RIGHT, gMenuBarView->getRect().getHeight());
    }
//MK
    // also update the parcel info panel pos -KC
    if ((MENU_RIGHT + MENU_PARCEL_SPACING) != mParcelInfoPanel->getRect().mLeft)
    {
        updateParcelPanel();
    }
//mk

    // update the master volume button state
    bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
    mBtnVolume->setToggleState(mute_audio);

    LLViewerMedia* media_inst = LLViewerMedia::getInstance();

    // Disable media toggle if there's no media, parcel media, and no parcel audio
    // (or if media is disabled)
    bool button_enabled = (gSavedSettings.getBOOL("AudioStreamingMedia")) &&    // ## Zi: Media/Stream separation
                          (media_inst->hasInWorldMedia() || media_inst->hasParcelMedia()    // || media_inst->hasParcelAudio()  // ## Zi: Media/Stream separation
                          );
    mMediaToggle->setEnabled(button_enabled);
    // Note the "sense" of the toggle is opposite whether media is playing or not
    bool any_media_playing = (media_inst->isAnyMediaPlaying() || 
                              media_inst->isParcelMediaPlaying());
    mMediaToggle->setValue(!any_media_playing);

    // ## Zi: Media/Stream separation
    button_enabled = (gSavedSettings.getBOOL("AudioStreamingMusic") && media_inst->hasParcelAudio());

    mStreamToggle->setEnabled(button_enabled);
    mStreamToggle->setValue(!media_inst->isParcelAudioPlaying());
    // ## Zi: Media/Stream separation
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
    static LLCachedControl<bool> net_stats_visible(gSavedSettings, "ShowNetStats", true);
    static LLCachedControl<bool> fps_stats_visible(gSavedSettings, "ShowFPSStats", true);
    static LLCachedControl<bool> show_draw_distance(gSavedSettings, "ShowDDSlider", true);
    static LLCachedControl<bool> show_media_popups(gSavedSettings, "ShowMediaPopupsOnRollover", true);
    mInMouselookMode = !visible;

    mTextTime->setVisible(visible);
    mBoxBalance->setVisible(visible);
    mPurchasePanel->setVisible(visible);
    mBtnVolume->setVisible(visible);
    mStreamToggle->setVisible(visible);     // ## Zi: Media/Stream separation
    mMediaToggle->setVisible(visible);
    mDrawDistancePanel->setVisible(visible && show_draw_distance);
    mStatisticsPanel->setVisible(visible && net_stats_visible);
    mFPSPanel->setVisible(visible && fps_stats_visible);
    mSearchPanel->setVisible(visible && gSavedSettings.getBOOL("MenuSearch"));
    setBackgroundVisible(visible);
    mIconPresetsCamera->setVisible(visible);
    mIconPresetsGraphic->setVisible(visible);
}

void LLStatusBar::debitBalance(S32 debit)
{
    setBalance(getBalance() - debit);
}

void LLStatusBar::creditBalance(S32 credit)
{
    setBalance(getBalance() + credit);
}

void LLStatusBar::setBalance(S32 balance)
{
    if (balance > getBalance() && getBalance() != 0)
    {
        LLFirstUse::receiveLindens();
    }

    std::string money_str = (balance < 0) ? "--" : LLResMgr::getInstance()->getMonetaryString(balance);

    LLStringUtil::format_map_t string_args;
    string_args["[AMT]"] = llformat("%s", money_str.c_str());
    std::string label_str = getString("buycurrencylabel", string_args);
    mBoxBalance->setValue(label_str);

    updateBalancePanelPosition();

    // If the search panel is shown, move this according to the new balance width. Parcel text will reshape itself in setParcelInfoText
    if (mSearchPanel && mSearchPanel->getVisible())
    {
        updateMenuSearchPosition();
    }

    if (mBalance && (fabs((F32)(mBalance - balance)) > gSavedSettings.getF32("UISndMoneyChangeThreshold")))
    {
        if (mBalance > balance)
            make_ui_sound("UISndMoneyChangeDown");
        else
            make_ui_sound("UISndMoneyChangeUp");
    }

    if( balance != mBalance )
    {
        mBalanceTimer->reset();
        mBalanceTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
        mBalance = balance;
    }
}


// static
void LLStatusBar::sendMoneyBalanceRequest()
{
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_MoneyData);
    msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );

    if (gDisconnected)
    {
        LL_DEBUGS() << "Trying to send message when disconnected, skipping balance request!" << LL_ENDL;
        return;
    }
    if (!gAgent.getRegion())
    {
        LL_DEBUGS() << "LLAgent::sendReliableMessage No region for agent yet, skipping balance request!" << LL_ENDL;
        return;
    }
    // Double amount of retries due to this request initially happening during busy stage
    // Ideally this should be turned into a capability
    gMessageSystem->sendReliable(gAgent.getRegionHost(), LL_DEFAULT_RELIABLE_RETRIES * 2, TRUE, LL_PING_BASED_TIMEOUT_DUMMY, NULL, NULL);
}


void LLStatusBar::setHealth(S32 health)
{
    //LL_INFOS() << "Setting health to: " << buffer << LL_ENDL;
    if( mHealth > health )
    {
        if (mHealth > (health + gSavedSettings.getF32("UISndHealthReductionThreshold")))
        {
            if (isAgentAvatarValid())
            {
                if (gAgentAvatarp->getSex() == SEX_FEMALE)
                {
                    make_ui_sound("UISndHealthReductionF");
                }
                else
                {
                    make_ui_sound("UISndHealthReductionM");
                }
            }
        }

        mHealthTimer->reset();
        mHealthTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
    }

    mHealth = health;
}

void LLStatusBar::hideBalance(bool hide)
{
    mBoxBalance->setVisible(!hide);
}

S32 LLStatusBar::getBalance() const
{
    return mBalance;
}


S32 LLStatusBar::getHealth() const
{
    return mHealth;
}

void LLStatusBar::setLandCredit(S32 credit)
{
    mSquareMetersCredit = credit;
}
void LLStatusBar::setLandCommitted(S32 committed)
{
    mSquareMetersCommitted = committed;
}

BOOL LLStatusBar::isUserTiered() const
{
    return (mSquareMetersCredit > 0);
}

S32 LLStatusBar::getSquareMetersCredit() const
{
    return mSquareMetersCredit;
}

S32 LLStatusBar::getSquareMetersCommitted() const
{
    return mSquareMetersCommitted;
}

S32 LLStatusBar::getSquareMetersLeft() const
{
    return mSquareMetersCredit - mSquareMetersCommitted;
}

void LLStatusBar::onClickBuyCurrency()
{
    // open a currency floater - actual one open depends on 
    // value specified in settings.xml
    LLBuyCurrencyHTML::openCurrencyFloater();
    LLFirstUse::receiveLindens(false);
}

void LLStatusBar::onMouseEnterPresetsCamera()
{
    LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
    LLIconCtrl* icon =  getChild<LLIconCtrl>( "presets_icon_camera" );
    LLRect icon_rect = icon->getRect();
    LLRect pulldown_rect = mPanelPresetsCameraPulldown->getRect();
    pulldown_rect.setLeftTopAndSize(icon_rect.mLeft -
         (pulldown_rect.getWidth() - icon_rect.getWidth()),
                   icon_rect.mBottom,
                   pulldown_rect.getWidth(),
                   pulldown_rect.getHeight());

    pulldown_rect.translate(popup_holder->getRect().getWidth() - pulldown_rect.mRight, 0);
    mPanelPresetsCameraPulldown->setShape(pulldown_rect);

    // show the master presets pull-down
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelPresetsCameraPulldown);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelPresetsCameraPulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterPresets()
{
    LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
    LLIconCtrl* icon =  getChild<LLIconCtrl>( "presets_icon_graphic" );
    LLRect icon_rect = icon->getRect();
    LLRect pulldown_rect = mPanelPresetsPulldown->getRect();
    pulldown_rect.setLeftTopAndSize(icon_rect.mLeft -
         (pulldown_rect.getWidth() - icon_rect.getWidth()),
                   icon_rect.mBottom,
                   pulldown_rect.getWidth(),
                   pulldown_rect.getHeight());

    pulldown_rect.translate(popup_holder->getRect().getWidth() - pulldown_rect.mRight, 0);
    mPanelPresetsPulldown->setShape(pulldown_rect);

    // show the master presets pull-down
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelPresetsPulldown);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterVolume()
{
    LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
    LLButton* volbtn =  getChild<LLButton>( "volume_btn" );
    LLRect volume_pulldown_rect = mPanelVolumePulldown->getRect();
    LLRect vol_btn_rect;

    volbtn->localRectToOtherView(volbtn->getLocalRect(), &vol_btn_rect, this);

    volume_pulldown_rect.setLeftTopAndSize(vol_btn_rect.mLeft -
         (volume_pulldown_rect.getWidth() - vol_btn_rect.getWidth()),
                   vol_btn_rect.mBottom,
                   volume_pulldown_rect.getWidth(),
                   volume_pulldown_rect.getHeight());

    volume_pulldown_rect.translate(popup_holder->getRect().getWidth() - volume_pulldown_rect.mRight, 0);
    mPanelVolumePulldown->setShape(volume_pulldown_rect);


    // show the master volume pull-down
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelVolumePulldown);
    mPanelPresetsCameraPulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelNearByMedia->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterNearbyMedia()
{
    LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
    LLRect nearby_media_rect = mPanelNearByMedia->getRect();
    LLButton* nearby_media_btn =  getChild<LLButton>( "media_toggle_btn" );
    LLRect nearby_media_btn_rect;

    nearby_media_btn->localRectToOtherView(nearby_media_btn->getLocalRect(), &nearby_media_btn_rect, this);

    nearby_media_rect.setLeftTopAndSize(nearby_media_btn_rect.mLeft - 
                                        (nearby_media_rect.getWidth() - nearby_media_btn_rect.getWidth()/2),
                                        nearby_media_btn_rect.mBottom,
                                        nearby_media_rect.getWidth(),
                                        nearby_media_rect.getHeight());
    // force onscreen
    nearby_media_rect.translate(popup_holder->getRect().getWidth() - nearby_media_rect.mRight, 0);
    
    // show the master volume pull-down
    mPanelNearByMedia->setShape(nearby_media_rect);
    LLUI::getInstance()->clearPopups();
    LLUI::getInstance()->addPopup(mPanelNearByMedia);

    mPanelPresetsCameraPulldown->setVisible(FALSE);
    mPanelPresetsPulldown->setVisible(FALSE);
    mPanelVolumePulldown->setVisible(FALSE);
    mPanelNearByMedia->setVisible(TRUE);
}


static void onClickVolume(void* data)
{
    // toggle the master mute setting
    bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
    LLAppViewer::instance()->setMasterSystemAudioMute(!mute_audio); 
}

//static 
void LLStatusBar::onClickBalance(void* )
{
    // Force a balance request message:
    LLStatusBar::sendMoneyBalanceRequest();
    // The refresh of the display (call to setBalance()) will be done by process_money_balance_reply()
}

//static 
void LLStatusBar::onClickMediaToggle(void* data)
{
    LLStatusBar *status_bar = (LLStatusBar*)data;
    // "Selected" means it was showing the "play" icon (so media was playing), and now it shows "pause", so turn off media
    bool pause = status_bar->mMediaToggle->getValue();
    LLViewerMedia::getInstance()->setAllMediaPaused(pause);
}
void LLStatusBar::toggleMedia(bool enable)
{
// </FS:Zi>
    LLViewerMedia::getInstance()->setAllMediaEnabled(enable);
}
void LLStatusBar::toggleStream(bool enable)
{
    if (!gAudiop)
    {
        return;
    }

    if(enable)
    {
        if (LLAudioEngine::AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
        {
            // 'false' means unpause
            LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLViewerMedia::getInstance()->getParcelAudioURL());
        }
        else
        {
            LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLViewerMedia::getInstance()->getParcelAudioURL());
        }
    }
    else
    {
        LLViewerAudio::getInstance()->stopInternetStreamWithAutoFade();
    }

    mAudioStreamEnabled = enable;
}

// ## Zi: Media/Stream separation
// static
void LLStatusBar::onClickStreamToggle(void* data)
{
    if (!gAudiop)
        return;

    LLStatusBar *status_bar = (LLStatusBar*)data;
    bool enable = ! status_bar->mStreamToggle->getValue();

    if(enable)
    {
        if (LLAudioEngine::AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
        {
            // 'false' means unpause
            gAudiop->pauseInternetStream(false);
        }
        else
        {
            gAudiop->startInternetStream(LLViewerMedia::getInstance()->getParcelAudioURL());
        }
    }
    else
    {
        gAudiop->stopInternetStream();
    }

    status_bar->mAudioStreamEnabled = enable;
}

BOOL LLStatusBar::getAudioStreamEnabled() const
{
    return mAudioStreamEnabled;
}
// ## Zi: Media/Stream separation

BOOL can_afford_transaction(S32 cost)
{
    return((cost <= 0)||((gStatusBar) && (gStatusBar->getBalance() >=cost)));
}

void LLStatusBar::onVolumeChanged(const LLSD& newvalue)
{
    refresh();
}

void LLStatusBar::onClickStatistics()
{
    LLFloaterReg::toggleInstance("stats");
}

// <FS:Ansariel> FIRE-19697: Add setting to disable graphics preset menu popup on mouse over
//NP graphics presets no longer disabled
void LLStatusBar::onPopupRolloverChanged(const LLSD& newvalue)
{
    bool new_value = newvalue.asBoolean();

    if (new_value)
    {
        mMouseEnterVolumeConnection = mBtnVolume->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterVolume, this));
        mMouseEnterNearbyMediaConnection = mMediaToggle->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterNearbyMedia, this));
    }
    else
    {
        if (mMouseEnterVolumeConnection.connected())
        {
            mMouseEnterVolumeConnection.disconnect();
        }
        if (mMouseEnterNearbyMediaConnection.connected())
        {
            mMouseEnterNearbyMediaConnection.disconnect();
        }
    }
}

void LLStatusBar::onUpdateFilterTerm()
{
    LLWString searchValue = utf8str_to_wstring( mFilterEdit->getValue() );
    LLWStringUtil::toLower( searchValue );

    if( !mSearchData || mSearchData->mLastFilter == searchValue )
        return;

    mSearchData->mLastFilter = searchValue;

    mSearchData->mRootMenu->hightlightAndHide( searchValue );
    gMenuBarView->needsArrange();
}

void collectChildren( LLMenuGL *aMenu, ll::statusbar::SearchableItemPtr aParentMenu )
{
    for( U32 i = 0; i < aMenu->getItemCount(); ++i )
    {
        LLMenuItemGL *pMenu = aMenu->getItem( i );

        ll::statusbar::SearchableItemPtr pItem( new ll::statusbar::SearchableItem );
        pItem->mCtrl = pMenu;
        pItem->mMenu = pMenu;
        pItem->mLabel = utf8str_to_wstring( pMenu->ll::ui::SearchableControl::getSearchText() );
        LLWStringUtil::toLower( pItem->mLabel );
        aParentMenu->mChildren.push_back( pItem );

        LLMenuItemBranchGL *pBranch = dynamic_cast< LLMenuItemBranchGL* >( pMenu );
        if( pBranch )
            collectChildren( pBranch->getBranch(), pItem );
    }

}

void LLStatusBar::collectSearchableItems()
{
    mSearchData.reset( new ll::statusbar::SearchData );
    ll::statusbar::SearchableItemPtr pItem( new ll::statusbar::SearchableItem );
    mSearchData->mRootMenu = pItem;
    collectChildren( gMenuBarView, pItem );
}

void LLStatusBar::updateMenuSearchVisibility(const LLSD& data)
{
    bool visible = data.asBoolean();
    mSearchPanel->setVisible(visible);
    if (!visible)
    {
        mFilterEdit->setText(LLStringUtil::null);
        onUpdateFilterTerm();
    }
    else
    {
        updateMenuSearchPosition();
    }
}

void LLStatusBar::updateMenuSearchPosition()
{
    const S32 HPAD = 12;
    LLRect balanceRect = getChildView("balance_bg")->getRect();
    LLRect searchRect = mSearchPanel->getRect();
    S32 w = searchRect.getWidth();
    searchRect.mLeft = balanceRect.mLeft - w - HPAD;
    searchRect.mRight = searchRect.mLeft + w;
    mSearchPanel->setShape( searchRect );
}

void LLStatusBar::updateBalancePanelPosition()
{
    // Resize the L$ balance background to be wide enough for your balance plus the buy button
    const S32 HPAD = 24;
    LLRect balance_rect = mBoxBalance->getTextBoundingRect();
    LLRect buy_rect = getChildView("buyL")->getRect();
    LLRect shop_rect = getChildView("goShop")->getRect();
    LLView* balance_bg_view = getChildView("balance_bg");
    LLRect balance_bg_rect = balance_bg_view->getRect();
    balance_bg_rect.mLeft = balance_bg_rect.mRight - (buy_rect.getWidth() + shop_rect.getWidth() + balance_rect.getWidth() + HPAD);
    balance_bg_view->setShape(balance_bg_rect);
}


// Implements secondlife:///app/balance/request to request a L$ balance
// update via UDP message system. JC
class LLBalanceHandler : public LLCommandHandler
{
public:
    // Requires "trusted" browser/URL source
    LLBalanceHandler() : LLCommandHandler("balance", UNTRUSTED_BLOCK) { }
    bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
    {
        if (tokens.size() == 1
            && tokens[0].asString() == "request")
        {
            LLStatusBar::sendMoneyBalanceRequest();
            return true;
        }
        return false;
    }
};
// register with command dispatch system
LLBalanceHandler gBalanceHandler;

//MK
void LLStatusBar::initParcelIcons()
{
    mParcelIcon[VOICE_ICON] = getChild<LLIconCtrl>("voice_icon");
    mParcelIcon[FLY_ICON] = getChild<LLIconCtrl>("fly_icon");
    mParcelIcon[PUSH_ICON] = getChild<LLIconCtrl>("push_icon");
    mParcelIcon[BUILD_ICON] = getChild<LLIconCtrl>("build_icon");
    mParcelIcon[SCRIPTS_ICON] = getChild<LLIconCtrl>("scripts_icon");
    mParcelIcon[DAMAGE_ICON] = getChild<LLIconCtrl>("damage_icon");

    mParcelIcon[VOICE_ICON]->setMouseDownCallback(boost::bind(&LLStatusBar::onParcelIconClick, this, VOICE_ICON));
    mParcelIcon[FLY_ICON]->setMouseDownCallback(boost::bind(&LLStatusBar::onParcelIconClick, this, FLY_ICON));
    mParcelIcon[PUSH_ICON]->setMouseDownCallback(boost::bind(&LLStatusBar::onParcelIconClick, this, PUSH_ICON));
    mParcelIcon[BUILD_ICON]->setMouseDownCallback(boost::bind(&LLStatusBar::onParcelIconClick, this, BUILD_ICON));
    mParcelIcon[SCRIPTS_ICON]->setMouseDownCallback(boost::bind(&LLStatusBar::onParcelIconClick, this, SCRIPTS_ICON));
    mParcelIcon[DAMAGE_ICON]->setMouseDownCallback(boost::bind(&LLStatusBar::onParcelIconClick, this, DAMAGE_ICON));

    mDamageText->setText(LLStringExplicit("100%"));
}

void LLStatusBar::handleLoginComplete()
{
    // An agent parcel update hasn't occurred yet, so
    // we have to manually set location and the icons.
    update();
}

void LLStatusBar::onNavBarShowParcelPropertiesCtrlChanged()
{
    std::string new_text;

    // don't need to have separate show_coords variable; if user requested the coords to be shown
    // they will be added during the next call to the draw() method.
    buildLocationString(new_text, false);
    setParcelInfoText(new_text);
}

void LLStatusBar::buildLocationString(std::string& loc_str, bool show_coords)
{
    LLAgentUI::ELocationFormat format =
        (show_coords ? LLAgentUI::LOCATION_FORMAT_FULL : LLAgentUI::LOCATION_FORMAT_NO_COORDS);

    if (!LLAgentUI::buildLocationString(loc_str, format))
    {
        loc_str = "???";
    }
}

void LLStatusBar::setParcelInfoText(const std::string& new_text)
{
    const LLFontGL* font = mParcelInfoText->getFont();
    S32 new_text_width = font->getWidth(new_text);

    mParcelInfoText->setText(new_text);

    LLRect rect = mParcelInfoText->getRect();
    rect.setOriginAndSize(rect.mLeft, rect.mBottom, new_text_width, rect.getHeight());

    mParcelInfoText->reshape(rect.getWidth(), rect.getHeight(), TRUE);
    mParcelInfoText->setRect(rect);
    layoutParcelIcons();
}

void LLStatusBar::update()
{
    std::string new_text;

    // don't need to have separate show_coords variable; if user requested the coords to be shown
    // they will be added during the next call to the draw() method.
    buildLocationString(new_text, false);
    setParcelInfoText(new_text);

    updateParcelIcons();
    updateParcelPanel();
}

void LLStatusBar::updateParcelPanel()
{
    const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();
    S32 left = MENU_RIGHT + MENU_PARCEL_SPACING;
    LLRect rect = mParcelInfoPanel->getRect();
    rect.mRight = left + rect.getWidth();
    rect.mLeft = left;
    
    mParcelInfoPanel->setRect(rect);
}

void LLStatusBar::updateParcelInfoText()
{
    static LLUICachedControl<bool> show_coords("NavBarShowCoordinates", false);

//MK
    // Update the location whether the coordinates are shown or not, because
    // buildLocationString() is where the parcel, region and coords are hidden
    // when under @showloc
////    if (show_coords)
//mk
    {
        std::string new_text;

        buildLocationString(new_text, show_coords);
        setParcelInfoText(new_text);
    }
}

void LLStatusBar::updateParcelIcons()
{
    LLViewerParcelMgr* vpm = LLViewerParcelMgr::getInstance();

    LLViewerRegion* agent_region = gAgent.getRegion();
    LLParcel* agent_parcel = vpm->getAgentParcel();
    if (!agent_region || !agent_parcel)
        return;

    if (gSavedSettings.getBOOL("NavBarShowParcelProperties"))
    {
        LLParcel* current_parcel;
        LLViewerRegion* selection_region = vpm->getSelectionRegion();
        LLParcel* selected_parcel = vpm->getParcelSelection()->getParcel();

        // If agent is in selected parcel we use its properties because
        // they are updated more often by LLViewerParcelMgr than agent parcel properties.
        // See LLViewerParcelMgr::processParcelProperties().
        // This is needed to reflect parcel restrictions changes without having to leave
        // the parcel and then enter it again. See EXT-2987
        if (selected_parcel && selected_parcel->getLocalID() == agent_parcel->getLocalID()
                && selection_region == agent_region)
        {
            current_parcel = selected_parcel;
        }
        else
        {
            current_parcel = agent_parcel;
        }

        bool allow_voice    = vpm->allowAgentVoice(agent_region, current_parcel);
        bool allow_fly      = vpm->allowAgentFly(agent_region, current_parcel);
        bool allow_push     = vpm->allowAgentPush(agent_region, current_parcel);
        bool allow_build    = vpm->allowAgentBuild(current_parcel); // true when anyone is allowed to build. See EXT-4610.
        bool allow_scripts  = vpm->allowAgentScripts(agent_region, current_parcel);
        bool allow_damage   = vpm->allowAgentDamage(agent_region, current_parcel);
        //bool has_pwl      = KCWindlightInterface::instance().WLset;

        // Most icons are "block this ability"
        mParcelIcon[VOICE_ICON]->setVisible(   !allow_voice );
        mParcelIcon[FLY_ICON]->setVisible(     !allow_fly );
        mParcelIcon[PUSH_ICON]->setVisible(    !allow_push );
        mParcelIcon[BUILD_ICON]->setVisible(   !allow_build );
        mParcelIcon[SCRIPTS_ICON]->setVisible( !allow_scripts );
        mParcelIcon[DAMAGE_ICON]->setVisible(  allow_damage );
        mDamageText->setVisible(allow_damage);
        //mPWLBtn->setVisible(has_pwl);
        //mPWLBtn->setEnabled(has_pwl);

        layoutParcelIcons();
    }
    else
    {
        for (S32 i = 0; i < ICON_COUNT; ++i)
        {
            mParcelIcon[i]->setVisible(false);
        }
        mDamageText->setVisible(false);
    }
}

void LLStatusBar::updateHealth()
{
    static LLUICachedControl<bool> show_icons("NavBarShowParcelProperties", false);

    // *FIXME: Status bar owns health information, should be in agent
    if (show_icons && gStatusBar)
    {
        static S32 last_health = -1;
        S32 health = gStatusBar->getHealth();
        if (health != last_health)
        {
            std::string text = llformat("%d%%", health);
            mDamageText->setText(text);
            last_health = health;
        }
    }
}

void LLStatusBar::layoutParcelIcons()
{
    // TODO: remove hard-coded values and read them as xml parameters
    static const int FIRST_ICON_HPAD = 16;
    // Kadah - not needed static const int LAST_ICON_HPAD = 11;

    S32 left = mParcelInfoText->getRect().mRight + FIRST_ICON_HPAD;

    left = layoutWidget(mDamageText, left);

    for (int i = ICON_COUNT - 1; i >= 0; --i)
    {
        left = layoutWidget(mParcelIcon[i], left);
    }

    //layoutWidget(mPWLBtn, left);
}

S32 LLStatusBar::layoutWidget(LLUICtrl* ctrl, S32 left)
{
    // TODO: remove hard-coded values and read them as xml parameters
    static const int ICON_HPAD = 2;

    if (ctrl->getVisible())
    {
        LLRect rect = ctrl->getRect();
        rect.mRight = left + rect.getWidth();
        rect.mLeft = left;

        ctrl->setRect(rect);
        left += rect.getWidth() + ICON_HPAD;
    }

    return left;
}

void LLStatusBar::onParcelIconClick(EParcelIcon icon)
{
    switch (icon)
    {
    case VOICE_ICON:
        LLNotificationsUtil::add("NoVoice");
        break;
    case FLY_ICON:
        LLNotificationsUtil::add("NoFly");
        break;
    case PUSH_ICON:
        LLNotificationsUtil::add("PushRestricted");
        break;
    case BUILD_ICON:
        LLNotificationsUtil::add("NoBuild");
        break;
    case SCRIPTS_ICON:
    {
        LLViewerRegion* region = gAgent.getRegion();
        if(region && region->getRegionFlags() & REGION_FLAGS_ESTATE_SKIP_SCRIPTS)
        {
            LLNotificationsUtil::add("ScriptsStopped");
        }
        else if(region && region->getRegionFlags() & REGION_FLAGS_SKIP_SCRIPTS)
        {
            LLNotificationsUtil::add("ScriptsNotRunning");
        }
        else
        {
            LLNotificationsUtil::add("NoOutsideScripts");
        }
        break;
    }
    case DAMAGE_ICON:
        LLNotificationsUtil::add("NotSafe");
        break;
    case ICON_COUNT:
        break;
    // no default to get compiler warning when a new icon gets added
    }
}

void LLStatusBar::onAgentParcelChange()
{
    update();
}

void LLStatusBar::onContextMenuItemClicked(const LLSD::String& item)
{
    if (item == "landmark")
    {
        LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();

        if(landmark == NULL)
        {
//          LLSideTray::getInstance()->showPanel("panel_places", LLSD().with("type", "create_landmark"));
        }
        else
        {
//          LLSideTray::getInstance()->showPanel("panel_places",
//                  LLSD().with("type", "landmark").with("id",landmark->getUUID()));
        }
    }
    else if (item == "copy")
    {
        LLSLURL slurl;
        LLAgentUI::buildSLURL(slurl, false);
        LLUIString location_str(slurl.getSLURLString());

        LLClipboard::instance().copyToClipboard (utf8str_to_wstring(location_str), 0, location_str.length());
    }
}

void LLStatusBar::onInfoButtonClicked()
{
    //LLSideTray::getInstance()->showPanel("panel_places", LLSD().with("type", "agent"));
//MK
////    LLFloaterReg::showInstance("about_land");
    if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
    {
        return;
    }
    LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "agent"));
//mk
}

void LLStatusBar::onParcelWLClicked()
{
    //KCWindlightInterface::instance().onClickWLStatusButton();
}

// hack -KC
void LLStatusBar::setBackgroundColor( const LLColor4& color )
{
    LLPanel::setBackgroundColor(color);
    getChild<LLPanel>("balance_bg")->setBackgroundColor(color);
    getChild<LLPanel>("time_and_media_bg")->setBackgroundColor(color);
}

//MK
void LLStatusBar::onAvatarHeightOffsetResetButtonClicked()
{
    gSavedPerAccountSettings.setF32 ("AvatarHoverOffsetZ", 0.0);
}
//mk
