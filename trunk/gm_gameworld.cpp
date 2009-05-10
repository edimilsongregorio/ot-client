//////////////////////////////////////////////////////////////////////
// Yet Another Tibia Client
//////////////////////////////////////////////////////////////////////
// Gameworld gamemode
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include <GLICT/globals.h>
#include <sstream>
#include <time.h>
#include <iomanip>
#include "product.h"
#include "gm_gameworld.h"
#include "gm_mainmenu.h"
#include "engine.h"
#include "options.h"
#include "objects.h"
#include "gamecontent/globalvars.h"
#include "gamecontent/map.h"
#include "gamecontent/creature.h"
#include "net/connection.h"
#include "net/protocolgame.h"
#include "console.h"
#include "gamecontent/inventory.h"
#include "util.h"
#include "popup.h"
#include "objects.h"

#include "ui/tutorialhints.h"

#ifdef _MSC_VER
#define time _time64
#endif

extern Connection* g_connection;
extern uint32_t g_frameTime;
int g_lastmousebutton=SDL_BUTTON_LEFT;
extern uint32_t g_frameDiff;

void resetDefaultCursor();

//void tmp(){((GM_Gameworld*)g_game)->msgBox("You made a choice", "Wat?");}


void winItemMove_t::moveItem(glictPos* pos, glictContainer *caller)
{
	GM_Gameworld *gw = ((GM_Gameworld*)g_game);

	winItemMove_t* win = (winItemMove_t*)caller->GetCustomData();
	if(win)
	{
		gw->m_protocol->sendThrow(win->fromPos, win->itemid, win->fromStack,
									win->toPos, win->sbCt.GetValue());
		win->window.SetVisible(false);
	}
}

void pnlInventory_t::onClick_Logout(glictPos* relmousepos, glictContainer* callerclass)
{
	GM_Gameworld* gameclass = (GM_Gameworld*)g_game;
    gameclass->getActiveConsole()->insertEntry(ConsoleEntry(PRODUCTSHORT ": Logging out...", TEXTCOLOR_WHITE));
	gameclass->m_protocol->sendLogout();
}

void pnlInventory_t::onClick_Options(glictPos* relmousepos, glictContainer* callerclass)
{
	GM_Gameworld* gameclass = (GM_Gameworld*)g_game;
	gameclass->winOptions.window.SetVisible(true);
    /*gameclass->msgBox(gettext("This functionality is not yet finished"),"TODO");*/
}

GM_Gameworld::GM_Gameworld() : pnlMap(&m_automap)
{
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Starting gameworld...\n");

	m_protocol = (ProtocolGame*)g_connection->getProtocol();

	#ifndef WINCE
	// first, let's construct right side panel
	desktop.AddObject(&pnlRightSide);
	pnlRightSide.SetPos(750 - (172 + 4), 0 );
	pnlRightSide.SetWidth(172 + 4); // 4 is for border
	pnlRightSide.SetHeight(600); // dynamic and updated later
	pnlRightSide.SetSkin(&g_skin.consoletabpassive);

	pnlRightSide.AddObject(&yspRightSide);
	yspRightSide.SetPos(0,0);
	yspRightSide.SetBGActiveness(false);
	yspRightSide.SetWidth(172);
	yspRightSide.SetHeight(600); // dynamic and updated later

	#if (GLICT_APIREV>=95)
	#define RIGHTSIDE yspRightSide
	#define AUTOSETPOS true
	#else
	#define RIGHTSIDE pnlRightSide
	#define AUTOSETPOS false
	#warning You are using too old GLICT; right side bar will be messed up.
	#endif

    // objects which didnt get set up in constructors and require initial setup...
    pnlTraffic.SetWidth(172);
	pnlTraffic.SetHeight(40);
    pnlTraffic.SetSkin(&g_skin.inv);
	pnlTraffic.SetFont("gamefont", 10);
	pnlTraffic.SetCaption("...");
	#if (GLICT_APIREV>=98)
	pnlTraffic.SetDraggable(true);
    pnlTraffic.SetFocusable(true);
    #endif

    // now let's get crackin

    /*RIGHTSIDE.AddObject(&pnlTraffic);
	if (!AUTOSETPOS) pnlTraffic.SetPos(0,0);*/
	RIGHTSIDE.AddObject(&pnlMap.panel);
	if (!AUTOSETPOS) pnlMap.panel.SetPos(600, 20);
	RIGHTSIDE.AddObject(&pnlHealth.panel);
	if (!AUTOSETPOS) pnlHealth.panel.SetPos(600, 478);
	RIGHTSIDE.AddObject(&pnlInventory.panel);
	if (!AUTOSETPOS) pnlInventory.panel.SetPos(600, 148);
	RIGHTSIDE.AddObject(&winSkills.window);
	if (!AUTOSETPOS) winSkills.window.SetPos(600, 308);

	//TODO (nfries88): AUTOSETPOS crap?
	RIGHTSIDE.AddObject(&winShop.window);
	winShop.window.SetVisible(false);
	RIGHTSIDE.AddObject(&winTrade.window);
	winTrade.window.SetVisible(false);

	#if (GLICT_APIREV>=95)
    yspRightSide.RebuildList();
    #endif

	#endif

	desktop.AddObject(&winOptions.window);
	winOptions.window.SetPos(600, 450);
	winOptions.window.SetVisible(false);

	desktop.AddObject(&winOutfit.window);
	winOutfit.window.SetPos(200,200);
	winOutfit.window.SetVisible(false);

	desktop.AddObject(&txtConsoleEntry);
	txtConsoleEntry.SetHeight(12);
	txtConsoleEntry.SetCaption("");

	desktop.AddObject(&winMove.window);
	winMove.window.SetVisible(false);

    desktop.AddObject(&pnlConsoleButtonContainer);
    pnlConsoleButtonContainer.SetSkin(&g_skin.consoletabbg);
    //pnlConsoleButtonContainer.SetBGActiveness(false);

    createConsole(); // add default console
	m_activeconsole = getDefaultConsole();
	pnlConsoleButtons[0]->SetSkin(&g_skin.consoletabactive);
	#if (GLICT_APIREV>=85)
	pnlConsoleButtons[0]->SetCaptionColor(0.7,0.7,0.7,1.0);
	#else
	#warning No support for setcaptioncolor before glict apirev 85
	#endif

	m_startTime = time(NULL);

    m_draggingPrep = false;
    m_dragging = false;
    m_dragThingId = 0;
    m_dragThingCount = 0;
    m_dragStackPos = 0;

    m_popup = NULL;
    m_showPopup = true;


	m_extendedThingId = 0;
    m_extendedstackpos = 0;

    m_tradeItemId = 0;
    m_tradestackpos = 0;

	doResize(glictGlobals.w, glictGlobals.h);

    SDL_SetCursor(g_engine->m_cursorBasic);
}

GM_Gameworld::~GM_Gameworld ()
{
    //m_automap.flushTiles();
    DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Terminating protocol connection from gameworld...\n");
	delete g_connection;
	g_connection = NULL;
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Destroying map...\n");
	Map::getInstance().clear();
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Destroying creature cache...\n");
	Creatures::getInstance().clear();
	Creatures::destroyInstance();
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Destroying inventory...\n");
	Inventory::getInstance().clear();


    DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Unloading data...\n");
	Objects::getInstance()->unloadDat();
	Objects::destroyInstance();


	if(!Objects::getInstance()->loadDat("Tibia.dat")){
        NativeGUIError("Tibia.dat suddenly disappeared during client's runtime.", "What?!");
    }

}


void GM_Gameworld::doResize(float w, float h)
{
    GameModeOptions::doResize(w,h);

    int wi=(int)w, hi=(int)h; // just to avoid warnings
	desktop.SetWidth(w);
	desktop.SetHeight(h);
	desktop.ResetTransformations();

	m_mapui.setSize(wi-172-4,hi-150-18);

	pnlRightSide.SetHeight(h);
	yspRightSide.SetHeight(h);
	pnlRightSide.SetPos(w-172-4,0); // ysp is always on 0,0

	txtConsoleEntry.SetWidth(w-172-4);
	txtConsoleEntry.SetPos(0,h-12);

	pnlTraffic.SetPos(w-200, 0);

    pnlConsoleButtonContainer.SetPos(0,glictGlobals.h-150-18);
    pnlConsoleButtonContainer.SetWidth(glictGlobals.w-172-4);
    pnlConsoleButtonContainer.SetHeight(18);

	DEBUGPRINT(0,0,"Updating scene\n");
	updateScene(); // FIXME (ivucica#2#) potentially dangerous during call inside constructor (map possibly not loaded yet) -- gotta check with mips if we may draw map while it still isn't received from server via initial 0x64 packet
	DEBUGPRINT(0,0,"Scene updated\n");


}


void GM_Gameworld::updateScene()
{
	// TODO (ivucica#2#) test on edge of map
	#if 0
	#ifdef WINCE
	{
		Position pos = GlobalVariables::getPlayerPosition();

		for(uint32_t i = 0; i < 7; ++i){
			for(uint32_t j = 0; j < 7; ++j){
				const Tile* tile = Map::getInstance().getTile(pos.x + i - 3, pos.y + j - 3, pos.z);
				int screenx = (int)(i*32);
				int screeny = (int)(j*32);
				const Item* ground = tile->getGround();
				if(ground){
					ground->Blit(screenx, screeny, 1.);
				}
			}
		}
		return;
	}

	#endif
	#endif

	#ifndef WINCE
	for(int i = 0; i < options.w; i += 96){
		for(int j = 0; j < options.h; j += 96){
			g_engine->getUISprite()->Blit(i, j, 0, 0, 96, 96);
		}
	}
	#endif

	m_mapui.renderMap();
	//m_automap.renderSelf(options.w-128 -170, 0, 128,128, GlobalVariables::getPlayerPosition());

	if (time(NULL)-m_startTime) {
		double txbph, rxbph, trbph;
		std::stringstream s;
		std::string txs, rxs, trs;

		txbph = (g_connection->getSent() * 3600. / (time(NULL)-m_startTime));
		rxbph = (g_connection->getRecv() * 3600. / (time(NULL)-m_startTime));
		trbph = (g_connection->getTraffic() * 3600. / (time(NULL)-m_startTime));

		txs = (txbph < 1024 ? "B/h" : (txbph < 1024*1024 ? "kiB/h" : "MiB/h"));
		rxs = (rxbph < 1024 ? "B/h" : (rxbph < 1024*1024 ? "kiB/h" : "MiB/h"));
		trs = (trbph < 1024 ? "B/h" : (trbph < 1024*1024 ? "kiB/h" : "MiB/h"));

		txbph /= (txbph < 1024 ? 1 : (txbph < 1024*1024 ? 1024 : 1024*1024));
		rxbph /= (rxbph < 1024 ? 1: (rxbph < 1024*1024 ? 1024 : 1024*1024));
		trbph /= (trbph < 1024 ? 1: (trbph < 1024*1024 ? 1024 : 1024*1024));


		s << "TX: " << g_connection->getSent() << " bytes (" << std::setprecision(3) << txbph << txs << ")\n" <<
			 "RX: " << g_connection->getRecv()<< " bytes (" << std::setprecision(3) << rxbph << rxs << ")\n" <<
			 "++: " << g_connection->getTraffic() << " bytes (" << std::setprecision(3) << trbph << trs << ")\n" <<
			 "";
		pnlTraffic.SetCaption(s.str());
	}

    if (m_popup)
    {
        if (m_popup->wantsDeath())
        {
            printf("Removing popup\n");
            desktop.RemoveObject(m_popup->getGlictList());
            desktop.DelayedRemove();
            delete m_popup;
            m_popup = NULL;

        }
    }




    // status messages
    m_statusStatMsg.paintSelf(0,0,glictGlobals.w-172-4, glictGlobals.h - 150 - 20);
    m_statusStatMsg.updateSelf(g_frameDiff / 1000.);
    m_lookatStatMsg.paintSelf(0,0,glictGlobals.w-172-4, glictGlobals.h - 150 - 20);
    m_lookatStatMsg.updateSelf(g_frameDiff / 1000.);

	desktop.Paint();
	g_engine->resetClipping();


	getActiveConsole()->paintConsole(0, glictGlobals.h-150, glictGlobals.w-172-4, glictGlobals.h-12);

}



void GM_Gameworld::keyPress (char key)
{

	txtConsoleEntry.Focus(NULL);
	if (key==13) {
		if (txtConsoleEntry.GetCaption().size()) {
		    bool sent = false;
		    std::string msg = txtConsoleEntry.GetCaption();
		    if (msg[0] == '@') {
		        printf("Local client command\n");
		        int i = msg.find(' ',1);
		        std::string command = msg.substr(1,i-1);
                std::string params = "";
                if (i < msg.size())
                    params = msg.substr(i+1);

		        if (command == "help" && !sent)
		        {
                    getActiveConsole()->insertEntry(ConsoleEntry(PRODUCTSHORT ": Supported commands:", TEXTCOLOR_WHITE));
                    getActiveConsole()->insertEntry(ConsoleEntry(PRODUCTSHORT ": @reloadgfx - reloads graphics", TEXTCOLOR_WHITE));
                    getActiveConsole()->insertEntry(ConsoleEntry(PRODUCTSHORT ": @mmzoom FACTOR - ", TEXTCOLOR_WHITE));
                    sent=true;
                }
                else if (command == "reloadgfx" && !sent)
		        {
                    getActiveConsole()->insertEntry(ConsoleEntry(PRODUCTSHORT ": Unloading graphics", TEXTCOLOR_WHITE));
                    printf("Unloading graphics\n");
                    Objects::getInstance()->unloadGfx();
                    Creatures::getInstance().unloadGfx(); // unload colored outfit for each creature instance
                    getActiveConsole()->insertEntry(ConsoleEntry(PRODUCTSHORT ": Reloading graphics", TEXTCOLOR_WHITE));
                    printf("Reloading graphics\n");
                    Creatures::getInstance().loadGfx();
                    g_engine->reloadGlobalGfx();
                    sent = true;
		        }
		        if (!sent)
                    getActiveConsole()->insertEntry(ConsoleEntry(PRODUCTSHORT": Unknown command", TEXTCOLOR_RED));
		        sent = true;
		    } else
		    if (msg[0] == '#')
		    {
		        if (tolower(msg[1]) == 'y' and msg[2] == ' ')
		        {
                    m_protocol->sendSay(SPEAK_YELL, msg.substr(3));
                    sent = true;
		        }
		    } else {
		        if (msg[0] == '*') {
                    int i = msg.find('*',1);
                    if (i!=-1) {
                        std::string recipient = msg.substr(1,i-1);
                        std::string newmsg = msg.substr(i+1);
                        m_protocol->sendSay(SPEAK_PRIVATE, recipient,newmsg);
                        Console*c = findConsole(recipient);
                        if (c) {
                            c->insertEntry(ConsoleEntry(newmsg, Creatures::getInstance().getCreature(GlobalVariables::getPlayerID())->getName() ,TEXTCOLOR_LIGHTBLUE));
                        }
                        sent = true;
                    }
		        }
		    }


            if (!sent)
            {
                if (getActiveConsole() == getDefaultConsole()) {
                    // currently nothing happens ONLY on default console
                } else {
                    if (getActiveConsole()->getSpeakerName() == "NPCs") { // FIXME (ivucica#1#) Incorrect way; what if there really is a player called NPCs?
                        getActiveConsole()->insertEntry(ConsoleEntry(msg, Creatures::getInstance().getCreature(GlobalVariables::getPlayerID())->getName() , TEXTCOLOR_LIGHTBLUE));
                        m_protocol->sendSay(SPEAK_PRIVATE_PN, msg);
                        sent = true;
                    } else
                    if (getActiveConsole()->getSpeakerName().size()) {
                        getActiveConsole()->insertEntry(ConsoleEntry(msg, Creatures::getInstance().getCreature(GlobalVariables::getPlayerID())->getName() , TEXTCOLOR_LIGHTBLUE));
                        m_protocol->sendSay(SPEAK_PRIVATE, getActiveConsole()->getSpeakerName(), msg);
                        sent = true;
                    }
                }
                if (!sent) // if we havent sent yet then use the default ...
                    m_protocol->sendSay(SPEAK_SAY, msg);
            }
		}

		txtConsoleEntry.SetCaption("");
	} else if(key != 0) {
		// ALT and CTRL are 0.
		// pressing ALT or CTRL will otherwise cause the text console to lose append with (char)0
		// which is bad because it causes the string to terminate the console textbox to lose focus.
		// It should always have focus
		desktop.CastEvent(GLICT_KEYPRESS, &key, 0);
	}

}

bool GM_Gameworld::specKeyPress (const SDL_keysym& key)
{
	Direction dir = DIRECTION_NORTH;
	int action = 0;
	bool ret = false;

	switch (key.sym) {
	case SDLK_LEFT:
		action = 1; dir = DIRECTION_WEST;
		ret = true;
		break;
	case SDLK_RIGHT:
		action = 1; dir = DIRECTION_EAST;
		ret = true;
		break;
	case SDLK_UP:
		action = 1; dir = DIRECTION_NORTH;
		ret = true;
		break;
	case SDLK_DOWN:
		action = 1; dir = DIRECTION_SOUTH;
		ret = true;
		break;
	case SDLK_KP1:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_SW;
			ret = true;
		}
		break;
	case SDLK_KP2:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_SOUTH;
			ret = true;
		}
		break;
	case SDLK_KP3:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_SE;
			ret = true;
		}
		break;
	case SDLK_KP4:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_WEST;
			ret = true;
		}
		break;
	case SDLK_KP6:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_EAST;
			ret = true;
		}
		break;
	case SDLK_KP7:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_NW;
			ret = true;
		}
		break;
	case SDLK_KP8:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_NORTH;
			ret = true;
		}
		break;
	case SDLK_KP9:
		if(!(key.mod & KMOD_NUM)) {
			action = 1; dir = DIRECTION_NE;
			ret = true;
		}
		break;

    case SDLK_ESCAPE:
    	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Terminating protocol connection from gameworld by esc...\n");
        delete g_connection;
        g_connection = NULL;
        DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Destroying map...\n");
        Map::getInstance().clear();
        DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Destroying creature cache...\n");
        Creatures::getInstance().clear();
        Creatures::destroyInstance();
        DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Destroying inventory...\n");
        Inventory::getInstance().clear();
        DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Switch to main menu\n");
        delete g_game;
        g_game = new GM_MainMenu;
        ret = true;
		break;
	case SDLK_F1: case SDLK_F2: case SDLK_F3: case SDLK_F4: case SDLK_F5: case SDLK_F6:
	case SDLK_F7: case SDLK_F8: case SDLK_F9: case SDLK_F10: case SDLK_F11: case SDLK_F12:
		action = 2;
		ret = true;
		break;
	default:
		action = 0;
        ret = false;
		break;
	}

	switch (action) {
	case 0: // invalid action / ignore
		break;
	case 1: // do move or turn
		if (!Creatures::getInstance().getPlayer()->isPreWalking()) {
			Direction lookdir = dir;
			if(lookdir == DIRECTION_NW || lookdir == DIRECTION_SW) {
				lookdir = DIRECTION_WEST;
			}
			if(lookdir == DIRECTION_NE || lookdir == DIRECTION_SE) {
				lookdir = DIRECTION_EAST;
			}
			Creatures::getInstance().getPlayer()->setLookDir(lookdir);

			if (key.mod & KMOD_CTRL)
				m_protocol->sendTurn(dir);
			else {
				Creatures::getInstance().getPlayer()->startWalk();
				m_protocol->sendMove(dir);
			}

		}
		break;
	case 2: // hotkeys
	{
		//NOTE (nfries88) IDK if key-SDLK_F1 works like i think it will in all cases... hoping it will :P
		int hkkey = key.sym - SDLK_F1;
		if(key.mod & KMOD_SHIFT) hkkey += 12;
		else if(key.mod & KMOD_CTRL) hkkey += 24;
		Hotkey hk = options.hotkeys[hkkey];

		if(hk.isText)
		{
			if(hk.text.length())
			{
				txtConsoleEntry.SetCaption(hk.text);
				if(hk.sendAuto)
					keyPress(13); // let keypress() take care of all the dirty work ;)
			}
			else
				txtConsoleEntry.SetCaption("");
		}
		else
		{
			// TODO (nfries88): item hotkeys
		}
		break;
	}
	case 255: // debugging action
		// FIXME (ivucica#1#) debugging action, just for testing (ctrl+a)
		getDefaultConsole()->insertEntry(std::string("No debugging action"));
	}
	return ret;
}

void GM_Gameworld::actionLook(const glictPos& pos)
{
	printf("It's a look\n");
	const Thing* thing;
	uint32_t x,y,z;
	int stackpos;

	m_mapui.lookAtItem((int)pos.x, (int)pos.y, thing, x, y, z, stackpos);
	if(stackpos != -1){
		m_protocol->sendLookItem(Position(x,y,z), thing->getID(), stackpos);
	}
}

void GM_Gameworld::actionUse(const glictPos& pos)
{
	const Thing* thing;
	uint32_t x,y,z;
	int stackpos;
	bool isextended;

	m_mapui.useItem((int)pos.x, (int)pos.y, thing, x, y, z, stackpos, isextended);

	if(stackpos != -1){
		if(!isextended){
			m_protocol->sendUseItem(Position(x,y,z), thing->getID(), stackpos );
		}
		else{
			beginExtendedUse(thing, stackpos, Position(x,y,z));
		}
	}
}

void GM_Gameworld::beginExtendedUse(const Thing* thing, int stackpos, const Position& pos)
{
	m_extendedThingId = thing->getID();
	m_extendedstackpos = stackpos;
	m_extendedpos = pos;
	SDL_SetCursor(g_engine->m_cursorUse);
}

void GM_Gameworld::performExtendedUse(const Position& destpos, const Thing* destthing, int deststackpos)
{
	if(deststackpos != -1)
		m_protocol->sendUseItemWith(m_extendedpos, m_extendedThingId, m_extendedstackpos,
				destpos, destthing->getID(), deststackpos);

	SDL_SetCursor(g_engine->m_cursorBasic);
	m_extendedThingId = 0;
}


void GM_Gameworld::actionUseWith(const glictPos& pos)
{
	const Thing* thing;
	uint32_t x,y,z;
	int stackpos;
	bool isextended;

	m_mapui.useItem((int)pos.x, (int)pos.y, thing, x, y, z, stackpos, isextended);
	performExtendedUse(Position(x,y,z), thing, stackpos);
}

void GM_Gameworld::actionAttack(const glictPos& pos)
{
	const Creature* creature = NULL;
	m_mapui.attackCreature((int)pos.x, (int)pos.y, creature);
	if(creature != NULL) {
		if(creature->getID() != GlobalVariables::getPlayerID()) {
			if(creature->getID() == GlobalVariables::getAttackID()){
				m_protocol->sendAttackCreature(0);
			}
			else{
				m_protocol->sendAttackCreature(creature->getID());
				GlobalVariables::setAttackID(creature->getID());
			}
		}
	}
}

void GM_Gameworld::beginTrade(const Item* item, int stackpos, const Position& pos)
{
	m_tradeItemId = item->getID();
	m_tradestackpos = stackpos;
	m_tradepos = pos;
	SDL_SetCursor(g_engine->m_cursorUse);
}

void GM_Gameworld::actionTrade(const glictPos& pos)
{
	const Creature* creature = NULL;
	m_mapui.attackCreature((int)pos.x, (int)pos.y, creature);
	if(creature != NULL) {
		if(creature->getID() != GlobalVariables::getPlayerID()) {
			m_protocol->sendRequestTrade(m_tradepos, m_tradeItemId, m_tradestackpos,
				creature->getID());
		}
	}

	SDL_SetCursor(g_engine->m_cursorBasic);
	m_tradeItemId = 0;
}

void GM_Gameworld::actionWalk(const glictPos& pos)
{
	//
}

void GM_Gameworld::mouseEvent(SDL_Event& event)
{
	glictPos pos;
	// FIXME (ivucica#3#) this is incorrect, we should be refreshing ptrx and ptry here as well, not just read the old versions ...
	// who knows how the platforms with a different pointing device (e.g. touchscreen?) would behave!
	pos.x = ptrx;
	pos.y = ptry;

	desktop.TransformScreenCoords(&pos);

    if (event.type == SDL_MOUSEMOTION) {
        #if (GLICT_APIREV >= 67)
        desktop.CastEvent(GLICT_MOUSEMOVE, &pos, 0);
        #else
        #warning We need GLICT apirev 67 or greater to support basic movable windows.
        #endif
        if(m_draggingPrep && !m_dragging){
            if(abs(int(pos.x - m_dragBegin.x)) > 2 || abs(int(pos.y - m_dragBegin.y)) > 2) {
                m_dragging = true;
                SDL_SetCursor(g_engine->m_cursorUse);
            }
        }
        if(m_popup){
            m_popup->mouseOver(pos.x,pos.y);
        }
    }
    else if(event.button.button == SDL_BUTTON_LEFT) {
        g_lastmousebutton = event.button.button;
        if(m_popup && !m_popup->wantsDeath()) { // handle popup menu before attempting anything else
            printf("Handling left mousedn on popup\n");

            if(event.button.state == SDL_RELEASED)
                m_popup->mouseClick(pos.x, pos.y);
        }
        else if(event.button.state == SDL_PRESSED){
            if(desktop.CastEvent(GLICT_MOUSEDOWN, &pos, 0)){ // if event got handled by glict
                // just ignore
            }
            else{
                // just remember where we began the drag, for later comparison
                uint32_t x, y, z;
                int stackpos;
                const Thing* thing;
                m_mapui.dragThing((int)pos.x, (int)pos.y, thing, x, y, z, stackpos);

                if(thing && thing->getItem()){
                	const Item* item = thing->getItem();
					startDrag(Position(x, y, z), stackpos, item->getID(), item->getCount());
                }
            }
        }
        else if(event.button.state == SDL_RELEASED){
            if (desktop.CastEvent(GLICT_MOUSEUP, &pos, 0)){
                // ignore if it was handled by glict
            }
            else if(m_dragging){
				uint32_t dx, dy, dz;
				m_mapui.translateClickToTile((int)pos.x, (int)pos.y, dx, dy, dz);
				Position dest(dx, dy, dz);
				dragComplete(dest);
			}
			else if(isExtendedUsing()){ // otherwise handle as appropriate
				actionUseWith(pos);
            }
			else if(isTrade()){ // otherwise handle as appropriate
				actionTrade(pos);
            }
            else if(options.classiccontrol == 0 && SDL_GetModState() & KMOD_ALT){
				actionAttack(pos);
            }
			else if(options.classiccontrol == 0 && SDL_GetModState() & KMOD_CTRL){
				actionUse(pos);
            }
            else if(options.classiccontrol == 1 && SDL_GetModState() & KMOD_CTRL){
				m_mapui.handlePopup((int)pos.x, (int)pos.y);
            }
            else if (SDL_GetModState() & KMOD_SHIFT){
                actionLook(pos);
            }
            else{
				actionWalk(pos);
            }
            dismissDrag();
        }
        else{
            printf("unknown event - %d\n", event.button.state);
        }
    }
    else if (event.button.button == SDL_BUTTON_RIGHT) {
        g_lastmousebutton = event.button.button;
        if(event.button.state == SDL_PRESSED){
			if(desktop.CastEvent(GLICT_MOUSEDOWN, &pos, 0)){
                // just ignore
            }
            else if(options.classiccontrol == 1 &&
				!((SDL_GetModState() & KMOD_CTRL) || (SDL_GetModState() & KMOD_SHIFT))){
            	uint32_t retx, rety, retz;
				Tile* tile = m_mapui.translateClickToTile((int)pos.x, (int)pos.y, retx, rety, retz);
				if(tile){
					if(tile->getTopCreature()){
						actionAttack(pos);
					}
					else{
						actionUse(pos);
					}
					m_showPopup = false;
				}
            }
        }
        else if (event.button.state == SDL_RELEASED) {
            bool hadpopup = (m_popup && !m_popup->wantsDeath());
            if(desktop.CastEvent(GLICT_MOUSECLICK, &pos, 0)){
            	//ignore
			}
			else if(options.classiccontrol == 0 && SDL_GetModState() & KMOD_ALT){
				actionAttack(pos);
			}
			else if(options.classiccontrol == 0 && SDL_GetModState() & KMOD_CTRL){
				actionUse(pos);
            }
            else if(SDL_GetModState() & KMOD_SHIFT){
                actionLook(pos);
            }
			else{
                m_mapui.handlePopup((int)pos.x, (int)pos.y);
            }

            if(hadpopup){ // if we have a popup AND it existed even before casting of click
                performPopup(NULL, NULL, NULL); // destroy it please
            }
        }
    }
	// Scene();
}

void GM_Gameworld::performPopup(PopupProducerCallback cb, void*owner, void*arg) {
    if(!m_popup){
        if(!cb)
            return;
		if(!m_showPopup){
			m_showPopup = true;
			return;
		}
        m_popup = new Popup();
        desktop.AddObject(m_popup->getGlictList());
        if(cb)
            cb(m_popup,owner,arg);

        m_popup->getGlictList()->SetPos(ptrx,ptry);
    }
    else{
        m_popup->prepareToDie();
    }
}

////////////// CONNECTION EVENTS //////////////////////
void GM_Gameworld::onConnectionClosed()
{
	delete g_game;
	g_game = new GM_MainMenu();
	//todo: return to character list
}

/////////////// PROTOCOL EVENTS ///////////////////////

void GM_Gameworld::onWalk()
{
	Creatures::getInstance().getPlayer()->confirmWalk();
}

void GM_Gameworld::onCancelWalk()
{
	Creatures::getInstance().getPlayer()->cancelWalk();
}

void GM_Gameworld::onTextMessage(MessageType_t type, const std::string& message)
{
	switch (type){

	case MSG_INFO_DESCR:
		getDefaultConsole()->insertEntry(ConsoleEntry(message, TEXTCOLOR_LIGHTGREEN));
		m_lookatStatMsg = StatusMsg(TEXTCOLOR_LIGHTGREEN, message, 3, 0, -30);
		break;
    case MSG_EVENT_ADVANCE:
    case MSG_EVENT_DEFAULT:
    case MSG_STATUS_DEFAULT:
    case MSG_STATUS_SMALL:
        if (type != MSG_STATUS_SMALL)
            getDefaultConsole()->insertEntry(ConsoleEntry(message, TEXTCOLOR_WHITE));
        m_statusStatMsg = StatusMsg(TEXTCOLOR_WHITE, message, 3);
        break;
	default:
		getDefaultConsole()->insertEntry(ConsoleEntry(message));
	}
}


//SAY,WHISPER, YELL, MONSTER_SAY, MONSTER_YELL; from 8.2 on, NPC PMs
void GM_Gameworld::onCreatureSpeak(SpeakClasses_t type, int n, const std::string& name, int level, const Position& pos, const std::string& message)
{
    switch (type){
    case SPEAK_PRIVATE_NP:
    case SPEAK_PRIVATE_PN:
        findConsole("NPCs")->insertEntry(ConsoleEntry(message, name, TEXTCOLOR_LIGHTBLUE)); // this is bad; this way we disallow potential player called "NPCs"
        break;
    case SPEAK_MONSTER_SAY:
    case SPEAK_MONSTER_YELL:
         getDefaultConsole()->insertEntry(ConsoleEntry(message, name, TEXTCOLOR_ORANGE));
         break;
    default:
        getDefaultConsole()->insertEntry(ConsoleEntry(message, name, TEXTCOLOR_YELLOW));
    }

}
//SPEAK_CHANNEL_Y, SPEAK_CHANNEL_R1, SPEAK_CHANNEL_R2, SPEAK_CHANNEL_O
void GM_Gameworld::onCreatureSpeak(SpeakClasses_t type, int n, const std::string& name, int level, int channel, const std::string& message)
{
    TextColor_t col;
    switch(type){
        case SPEAK_CHANNEL_O:
            col = TEXTCOLOR_ORANGE;
            break;
        case SPEAK_CHANNEL_R1:
        case SPEAK_CHANNEL_R2:
            col = TEXTCOLOR_RED;
            break;
		case SPEAK_CHANNEL_W:
			col = TEXTCOLOR_WHITE;
			break;
        default:
            col = TEXTCOLOR_YELLOW;
    }
	getDefaultConsole()->insertEntry(ConsoleEntry(message, name, col));
}
//SPEAK_PRIVATE, SPEAK_PRIVATE_RED, SPEAK_BROADCAST
void GM_Gameworld::onCreatureSpeak(SpeakClasses_t type, int n, const std::string& name, int level, const std::string& message)
{
    TextColor_t col;
    switch(type){
        case SPEAK_CHANNEL_O:
            col = TEXTCOLOR_ORANGE;
            break;
        case SPEAK_CHANNEL_R1:
        case SPEAK_CHANNEL_R2:
            col = TEXTCOLOR_RED;
            break;
        default:
            col = TEXTCOLOR_YELLOW;
    }
    switch (type) {
        case SPEAK_PRIVATE:
            findConsole(name)->insertEntry(ConsoleEntry(message, name, TEXTCOLOR_LIGHTBLUE));
            break;
        default: {
            std::stringstream s;
            s << "(private msg of unknown speakclass " << type << ")";
            getDefaultConsole()->insertEntry(ConsoleEntry(message + s.str(), name, TEXTCOLOR_YELLOW));
        }
    }

}

void GM_Gameworld::onCreatureMove(uint32_t id, const Position& oldPos, const Position& newPos)
{
    Creature* c = Creatures::getInstance().getCreature(id);
	if (id != GlobalVariables::getPlayerID() || !Creatures::getInstance().getPlayer()->isPreWalking() )
		c->startWalk();
	c->confirmWalk();

}

void GM_Gameworld::onChangeSkills()
{
	winSkills.updateSelf();
}
void GM_Gameworld::onChangeStats()
{
    pnlHealth.updateSelf();
	winSkills.updateSelf();
}
void GM_Gameworld::onTileUpdate(const Position& pos)
{
    Tile* t = Map::getInstance().getTile(pos);
    m_automap.setTileColor(pos.x,pos.y,pos.z, t->getMinimapColor(), t->getSpeedIndex());
}


///////////////////////////////////////////////
std::vector<Console*>::iterator GM_Gameworld::findConsole_it(uint32_t channelid)
{
    for(std::vector<Console*>::iterator it=m_consoles.begin(); it != m_consoles.end(); it++) {
        if ((*it)->getChannelId() == channelid) {
            return it;
        }
    }
    createConsole(channelid);
    return m_consoles.end()-1;
}
std::vector<Console*>::iterator GM_Gameworld::findConsole_it(const std::string& speaker)
{
    for(std::vector<Console*>::iterator it=m_consoles.begin(); it != m_consoles.end(); it++) {
        if ((*it)->getSpeakerName() == speaker) {
            return it;
        }
    }
    printf("Creating console for speaker\n");
    createConsole(speaker);
    return m_consoles.end()-1;
}

std::vector<Console*>::iterator GM_Gameworld::findConsole_it(const Console* c)
{
    for(std::vector<Console*>::iterator it=m_consoles.begin(); it != m_consoles.end(); it++) {
        if ((*it) == c) {
            return it;
        }
    }
    ASSERTFRIENDLY(c, "Code attempted to access a non-existing console")
    return m_consoles.end();
}
void GM_Gameworld::pnlConsoleButton_OnClick(glictPos* relmousepos, glictContainer* caller)
{
    if (g_lastmousebutton != SDL_BUTTON_LEFT)
        return;
    Console* c = (Console*)caller->GetCustomData();
    GM_Gameworld* gw = (GM_Gameworld*)g_game;
    //std::vector<Console*>::iterator cit = gw->findConsole_it(c);
    gw->setActiveConsole(c);

}
void GM_Gameworld::createConsole(uint32_t channelid,const std::string& speaker)
{
    Console* nc;
    std::stringstream s;
    if (channelid) {
        nc = new Console(channelid);
        s << channelid;
    } else if (speaker.size()) {
        nc = new Console(speaker);
        s << speaker;
    } else {
        nc = new Console();
        s << gettext("Console");
    }


    glictPanel* p = new glictPanel;
    nc->setAssignedButton(p);
    p->SetCustomData(nc);
    p->SetHeight(18);
    p->SetCaption(s.str().c_str());
    p->SetWidth(96); //g_engine->sizeText(s.str().c_str(),"system"));
    p->SetBGColor(.2,.2,.2,1.);
    #if (GLICT_APIREV >= 85)
    p->SetTextOffset(int(96 / 2 - g_engine->sizeText(s.str().c_str(),"gamefont") / 2), 4);
    p->SetCaptionColor(0.5,0.5,0.5);
    #else
	#warning No support for setcaptioncolor before glict apirev 85
	#endif
    int sum=0;
    for (std::vector<glictPanel*>::iterator it = pnlConsoleButtons.begin(); it != pnlConsoleButtons.end(); it++) {
        (*it)->SetPos(sum,0);
        sum += (int)(*it)->GetWidth();
    }
    p->SetPos(sum,0);
    p->SetOnClick(pnlConsoleButton_OnClick);
    p->SetSkin(&g_skin.consoletabpassive);
    p->SetFont("gamefont");
    pnlConsoleButtonContainer.AddObject(p);
    pnlConsoleButtons.push_back(p);

    m_consoles.push_back(nc);
}

void GM_Gameworld::openContainer(uint32_t cid)
{
	Container* container = Containers::getInstance().getContainer(cid);
	winContainer_t* window = new winContainer_t(container, cid);

	yspRightSide.AddObject(&window->window);
	containers.push_back(window);
}

void GM_Gameworld::closeContainer(uint32_t cid)
{
	winContainer_t* window = NULL;

	std::vector<winContainer_t*>::iterator it = containers.begin();
	for(;it != containers.end(); ++it)
	{
		if((*it)->containerId == cid)
		{
			window = (*it);
			break;
		}
	}

	if(window)
	{
		yspRightSide.RemoveObject(&window->window);
		yspRightSide.DelayedRemove();
		yspRightSide.RebuildList();
		containers.erase(it);
		delete window;
	}
}

void GM_Gameworld::openShopWindow(const std::list<ShopItem>& itemlist)
{
	winShop.generateList(itemlist);
    winShop.window.SetVisible(true);
}

void GM_Gameworld::closeShopWindow() {
    winShop.window.SetVisible(false);
    winShop.destroyList();
}

void GM_Gameworld::openTradeWindow(bool ack)
{
	winTrade.onTradeUpdate(ack);
	winTrade.window.SetVisible(true);
}

void GM_Gameworld::closeTradeWindow()
{
	winTrade.window.SetVisible(false);
	winTrade.onTradeCompleted();
}

void GM_Gameworld::onUpdatePlayerCash(uint32_t newcash) {
    winShop.setCash(newcash);
}


void GM_Gameworld::startDrag(const Position& pos, int stackPos, uint32_t itemid, uint32_t count)
{
	m_dragging = false;
	m_draggingPrep = true;
	m_dragBegin.x = ptrx;
	m_dragBegin.y = ptry;
	//
	m_dragPos = pos;
	m_dragStackPos = stackPos;
	m_dragThingId = itemid;
	m_dragThingCount = count;
}

void GM_Gameworld::dragComplete(const Position& toPos)
{
	const ObjectType* type = Objects::getInstance()->getItemType(m_dragThingId);
	if(type){
		if(type->stackable && !(SDL_GetModState() & KMOD_CTRL) && (m_dragThingCount > 1)){
			winMove.open(m_dragThingId, m_dragThingCount, m_dragPos, toPos, m_dragStackPos);
			centerWindow(&winMove.window);
		}
		else{
		    printf("m_dragPos: %d %d %d\n", m_dragPos.x, m_dragPos.y, m_dragPos.z);
			m_protocol->sendThrow(m_dragPos, m_dragThingId, m_dragStackPos, toPos, m_dragThingCount);
		}
	}
	dismissDrag();
}

void GM_Gameworld::showTutorial(uint8_t id) {
    if (UITutorialHints::getInstance().showHint(id))
        this->msgBox(UITutorialHints::getInstance().getTutorialHint(id).c_str(), gettext("Tutorial Hint"));
}




void GM_Gameworld::onSetOutfit(Popup::Item *parent) {
    // happens when user clicks on "Set Outfit" in right click popup menu
    GM_Gameworld *g = (GM_Gameworld*)g_game;
    g->m_protocol->sendRequestOutfit();
}
void GM_Gameworld::openOutfitWindow(const Outfit_t& current, const std::list<AvailOutfit_t>& available){
    GM_Gameworld *g = (GM_Gameworld*)g_game;

    g->winOutfit.openSelf(current, available);
}

void GM_Gameworld::setActiveConsole(Console* i){
    m_activeconsole->getAssignedButton()->SetSkin(&g_skin.consoletabpassive);
    #if (GLICT_APIREV >= 85)
    m_activeconsole->getAssignedButton()->SetCaptionColor(0.5,0.5,0.5);
    #else
	#warning No support for setcaptioncolor before glict apirev 85
	#endif
    m_activeconsole = i;
    m_activeconsole->getAssignedButton()->SetSkin(&g_skin.consoletabactive);
    #if (GLICT_APIREV >= 85)
    m_activeconsole->getAssignedButton()->SetCaptionColor(0.7,0.7,0.7);
    #else
	#warning No support for setcaptioncolor before glict apirev 85
	#endif
}
