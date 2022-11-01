/** 
 * @file kokuachatbar.h
 * @author Chorazin Allen <chorazinallen@gmail.com>
 * @brief Chat bar floater without IMs or history
 *
 * Abstracted from llfloaterregiondebugconsole / llfloaterimnearbychat
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

#ifndef KOKUA_CHATBAR_H
#define KOKUA_CHATBAR_H

#include <boost/signals2.hpp>

#include "llchat.h"
#include "llfloater.h"

class LLChatEntry;

class KokuaChatBar : public LLFloater
{
public:
    KokuaChatBar(LLSD const & key);
    virtual ~KokuaChatBar();
    virtual BOOL handleKeyHere( KEY key, MASK mask );
    static void startChat(const char* line);
    void show();
        
    // virtual
    BOOL postBuild();

protected:
    LLChatEntry* mInputEditor;
    void sendChat( EChatType type );
    void onChatBoxKeystroke();
    void onChatBoxFocusLost();
    void onChatBoxFocusReceived();
    void onChatBoxCommit();
    void onOpen(const LLSD& key);   
    void onChatFontChange(LLFontGL* fontp);
    void reshapeChatLayoutPanel();
    S32 mInputEditorHeightPad;
    S32 mInputEditorWidthPad;
  void handleReshape(const LLRect& new_rect, bool by_user);
private:
};

#endif // KOKUA_CHATBAR_H
