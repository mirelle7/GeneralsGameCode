/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: ControlBarObserver.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Electronic Arts Pacific.
//
//                       Confidential Information
//                Copyright (C) 2002 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
//	created:	Aug 2002
//
//	Filename: 	ControlBarObserver.cpp
//
//	author:		Chris Huybregts
//
//	purpose:	All things related to the Observer Control bar, are in here.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/GameUtility.h"
#include "Common/NameKeyGenerator.h"
#include "Common/PlayerList.h"
#include "Common/Player.h"
#include "Common/PlayerTemplate.h"
#include "Common/KindOf.h"
#include "Common/Recorder.h"
#include "GameClient/ControlBar.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GadgetPushButton.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/GameText.h"
#include "GameNetwork/NetworkDefs.h"
//-----------------------------------------------------------------------------
// DEFINES ////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
enum { MAX_BUTTONS = ControlBar::MAX_OBSERVER_PLAYER_BUTTONS };
static NameKeyType buttonPlayerID[MAX_BUTTONS] = { NAMEKEY_INVALID,NAMEKEY_INVALID,
																										NAMEKEY_INVALID,NAMEKEY_INVALID,
																										NAMEKEY_INVALID,NAMEKEY_INVALID,
																										NAMEKEY_INVALID,NAMEKEY_INVALID };
static NameKeyType staticTextPlayerID[MAX_BUTTONS] = { NAMEKEY_INVALID,NAMEKEY_INVALID,
																										NAMEKEY_INVALID,NAMEKEY_INVALID,
																										NAMEKEY_INVALID,NAMEKEY_INVALID,
																										NAMEKEY_INVALID,NAMEKEY_INVALID };

// Splitscreen: the observer panel's windows are ControlBar members, not file statics. Every
// bar has its own copy of ControlBar.wnd, so one set of statics was overwritten by each seat
// bar as it was created and left pointing at that bar's windows after it was destroyed - the
// classic bar then wrote through freed windows the next time it showed the observer list.
// The name keys above are the same in every copy and stay shared.

static NameKeyType buttonCancelID = NAMEKEY_INVALID;

static NameKeyType s_replayObserverNameKey = NAMEKEY_INVALID;

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------


void ControlBar::initObserverControls()
{
	// Splitscreen: resolve inside THIS bar's own layout copy. The global lookup returns whichever
	// copy of a name was created last, which is a seat bar's once one exists.
	m_observerPlayerInfoWindow = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:ObserverPlayerInfoWindow"));
	m_observerPlayerListWindow = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:ObserverPlayerListWindow"));

	for (Int i = 0; i < MAX_BUTTONS; i++)
	{
		AsciiString tmpString;
		tmpString.format("ControlBar.wnd:ButtonPlayer%d", i);
		buttonPlayerID[i] = TheNameKeyGenerator->nameToKey( tmpString );
		m_observerButtonPlayer[i] = findBarWindowById( buttonPlayerID[i] );
		tmpString.format("ControlBar.wnd:StaticTextPlayer%d", i);
		staticTextPlayerID[i] = TheNameKeyGenerator->nameToKey( tmpString );
		m_observerStaticTextPlayer[i] = findBarWindowById( staticTextPlayerID[i] );
	}

	m_observerStaticTextNumberOfUnits = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:StaticTextNumberOfUnits"));
	m_observerStaticTextNumberOfBuildings = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:StaticTextNumberOfBuildings"));
	m_observerStaticTextNumberOfUnitsKilled = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:StaticTextNumberOfUnitsKilled"));
	m_observerStaticTextNumberOfUnitsLost = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:StaticTextNumberOfUnitsLost"));
	m_observerStaticTextPlayerName = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:StaticTextPlayerName"));
	m_observerWinFlag = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:WinFlag"));
	m_observerWinGeneralPortrait = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:WinGeneralPortrait"));
	m_observerButtonIdleWorker = findBarWindowById(TheNameKeyGenerator->nameToKey("ControlBar.wnd:ButtonIdleWorker"));

	buttonCancelID = TheNameKeyGenerator->nameToKey("ControlBar.wnd:ButtonCancel");

	s_replayObserverNameKey = TheNameKeyGenerator->nameToKey("ReplayObserver");
}

//-------------------------------------------------------------------------------------------------
void ControlBar::setObserverLookAtPlayer(Player *player)
{
	if (player != nullptr && player == ThePlayerList->findPlayerWithNameKey(s_replayObserverNameKey))
	{
		// Looking at the observer. Treat as not looking at player.
		m_observerLookAtPlayer = nullptr;
	}
	else
	{
		m_observerLookAtPlayer = player;
	}
}

//-------------------------------------------------------------------------------------------------
void ControlBar::setObservedPlayer(Player *player)
{
	if (player != nullptr && player == ThePlayerList->findPlayerWithNameKey(s_replayObserverNameKey))
	{
		// Looking at the observer. Treat as not observing player.
		m_observedPlayer = nullptr;
	}
	else
	{
		m_observedPlayer = player;
	}
}

//-------------------------------------------------------------------------------------------------
/** System callback for the ControlBarObserverSystem */
//-------------------------------------------------------------------------------------------------
WindowMsgHandledType ControlBarObserverSystem( GameWindow *window, UnsignedInt msg,
																			 WindowMsgData mData1, WindowMsgData mData2 )
{
	static NameKeyType buttonCommunicator = NAMEKEY_INVALID;

	switch( msg )
	{
		// --------------------------------------------------------------------------------------------
		case GWM_CREATE:
		{
				break;

		}

		//---------------------------------------------------------------------------------------------
		case GBM_MOUSE_ENTERING:
		case GBM_MOUSE_LEAVING:
		{
			break;
		}

		//---------------------------------------------------------------------------------------------
		case GBM_SELECTED:
		case GBM_SELECTED_RIGHT:
		{
			GameWindow *control = (GameWindow *)mData1;
			// Splitscreen: the panel that was clicked, not always the classic bar's.
			ControlBar *bar = ControlBarInstances::fromWindow( control );

			Int controlID = control->winGetWindowId();
			if( controlID == buttonCancelID)
			{
				rts::changeObservedPlayer(nullptr);

				bar->showObserverPlayerList();
			}

			for(Int i = 0; i <MAX_BUTTONS; ++i)
			{
				if( controlID == buttonPlayerID[i])
				{
					Player* player = static_cast<Player*>(GadgetButtonGetData(control));
					rts::changeObservedPlayer(player);

					bar->showObserverPlayerInfo();

					return MSG_HANDLED;
				}
			}

		//	if( controlID == buttonCommunicator && TheGameLogic->getGameMode() == GAME_INTERNET )
	/*
		{
				popupCommunicatorLayout = TheWindowManager->winCreateLayout( "Menus/PopupCommunicator.wnd" );
				popupCommunicatorLayout->runInit();
				popupCommunicatorLayout->hide( FALSE );
				popupCommunicatorLayout->bringForward();
			}
*/

			break;

		}

		//---------------------------------------------------------------------------------------------
		default:
			return MSG_IGNORED;

	}

	return MSG_HANDLED;

}

//-----------------------------------------------------------------------------
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

void ControlBar::populateObserverList()
{
	Int currentButton = 0, i;
	if(TheRecorder->isMultiplayer())
	{

		for (i = 0; i < MAX_SLOTS; ++i)
		{
			Player *p = ThePlayerList->getPlayerFromSlotIndex(i);
			if(p)
			{
				if(p->isPlayerObserver())
					continue;
				DEBUG_ASSERTCRASH(currentButton < MAX_BUTTONS, ("ControlBar::populateObserverList trying to populate more buttons then we have"));
				GadgetButtonSetData(m_observerButtonPlayer[currentButton], (void *)p);
				GadgetButtonSetEnabledImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getEnabledImage() );
				//GadgetButtonSetHiliteImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getHiliteImage() );
				//GadgetButtonSetHiliteSelectedImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getPushedImage() );
				//GadgetButtonSetDisabledImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getDisabledImage() );
				m_observerButtonPlayer[currentButton]->winSetTooltip(p->getPlayerDisplayName());
				m_observerButtonPlayer[currentButton]->winHide(FALSE);
				m_observerButtonPlayer[currentButton]->winSetStatus( WIN_STATUS_USE_OVERLAY_STATES );

				const GameSlot *slot = TheGameInfo->getConstSlot(i);
				Color playerColor = p->getPlayerColor();
				Color backColor = GameMakeColor(0, 0, 0, 255);
				m_observerStaticTextPlayer[currentButton]->winSetEnabledTextColors( playerColor, backColor );
				m_observerStaticTextPlayer[currentButton]->winHide(FALSE);
				AsciiString teamStr;
				teamStr.format("Team:%d", slot->getTeamNumber() + 1);
				if (slot->isAI() && slot->getTeamNumber() == -1)
					teamStr = "Team:AI";

				UnicodeString text;
				text.format(TheGameText->fetch("CONTROLBAR:ObsPlayerLabel"), p->getPlayerDisplayName().str(),
					TheGameText->fetch(teamStr).str());

				GadgetStaticTextSetText(m_observerStaticTextPlayer[currentButton], text );

				++currentButton;
			}
		}
		for(currentButton; currentButton<MAX_BUTTONS; ++currentButton)
		{
			m_observerButtonPlayer[currentButton]->winHide(TRUE);
			m_observerStaticTextPlayer[currentButton]->winHide(TRUE);
		}
	}
	else
	{
		for(i =0; i < MAX_PLAYER_COUNT; ++i)
		{
			Player *p = ThePlayerList->getNthPlayer(i);
			if(p && !p->isPlayerObserver() && p->getPlayerType() == PLAYER_HUMAN)
			{
				DEBUG_ASSERTCRASH(currentButton < MAX_BUTTONS, ("ControlBar::populateObserverList trying to populate more buttons then we have"));
				GadgetButtonSetData(m_observerButtonPlayer[currentButton], (void *)p);
				GadgetButtonSetEnabledImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getEnabledImage() );
				//GadgetButtonSetHiliteImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getHiliteImage() );
				//GadgetButtonSetHiliteSelectedImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getPushedImage() );
				//GadgetButtonSetDisabledImage( m_observerButtonPlayer[currentButton], p->getPlayerTemplate()->getDisabledImage() );
				m_observerButtonPlayer[currentButton]->winSetTooltip(p->getPlayerDisplayName());
				m_observerButtonPlayer[currentButton]->winHide(FALSE);
				m_observerButtonPlayer[currentButton]->winSetStatus( WIN_STATUS_USE_OVERLAY_STATES );

				Color playerColor = p->getPlayerColor();
				Color backColor = GameMakeColor(0, 0, 0, 255);
				m_observerStaticTextPlayer[currentButton]->winSetEnabledTextColors( playerColor, backColor );
				m_observerStaticTextPlayer[currentButton]->winHide(FALSE);
				GadgetStaticTextSetText(m_observerStaticTextPlayer[currentButton], p->getPlayerDisplayName());

				++currentButton;
				break;
			}
		}
		for(currentButton; currentButton<MAX_BUTTONS; ++currentButton)
		{
			m_observerButtonPlayer[currentButton]->winHide(TRUE);
			m_observerStaticTextPlayer[currentButton]->winHide(TRUE);
		}
	}
}

void ControlBar::populateObserverInfoWindow ()
{
	if(m_observerPlayerInfoWindow->winIsHidden())
		return;

	if( !m_observerLookAtPlayer )
	{
		m_observerPlayerInfoWindow->winHide(TRUE);
		m_observerPlayerListWindow->winHide(FALSE);
		m_observerButtonIdleWorker->winHide(TRUE);
		populateObserverList();
		return;
	}

	UnicodeString uString;
	KindOfMaskType mask,clearmask;
	mask.set(KINDOF_SCORE);
	clearmask.set(KINDOF_STRUCTURE);

	uString.format(L"%d",m_observerLookAtPlayer->countObjects(mask,clearmask));
	GadgetStaticTextSetText(m_observerStaticTextNumberOfUnits, uString);

	Int numBuildings = 0;
	mask.clear();
	mask.set(KINDOF_SCORE);
	mask.set(KINDOF_STRUCTURE);
	clearmask.clear();
	numBuildings = m_observerLookAtPlayer->countObjects(mask,clearmask);
	mask.clear();
	mask.set(KINDOF_SCORE_CREATE);
	mask.set(KINDOF_STRUCTURE);
	numBuildings += m_observerLookAtPlayer->countObjects(mask,clearmask);
	mask.clear();
	mask.set(KINDOF_SCORE_DESTROY);
	mask.set(KINDOF_STRUCTURE);
	numBuildings += m_observerLookAtPlayer->countObjects(mask,clearmask);
	uString.format(L"%d",numBuildings);
	GadgetStaticTextSetText(m_observerStaticTextNumberOfBuildings, uString);
	uString.format(L"%d",m_observerLookAtPlayer->getScoreKeeper()->getTotalUnitsDestroyed());
	GadgetStaticTextSetText(m_observerStaticTextNumberOfUnitsKilled, uString);
	uString.format(L"%d",m_observerLookAtPlayer->getScoreKeeper()->getTotalUnitsLost());
	GadgetStaticTextSetText(m_observerStaticTextNumberOfUnitsLost, uString);
	GadgetStaticTextSetText(m_observerStaticTextPlayerName, m_observerLookAtPlayer->getPlayerDisplayName());
	Color color = m_observerLookAtPlayer->getPlayerColor();
	m_observerStaticTextPlayerName->winSetEnabledTextColors(color, GameMakeColor(0,0,0,255));
	m_observerWinFlag->winSetEnabledImage(0, m_observerLookAtPlayer->getPlayerTemplate()->getFlagWaterMarkImage());
	m_observerWinGeneralPortrait->winHide(FALSE);
	m_observerButtonIdleWorker->winHide(FALSE);
}

void ControlBar::showObserverPlayerList()
{
	m_observerPlayerInfoWindow->winHide(TRUE);
	m_observerPlayerListWindow->winHide(FALSE);
	m_observerButtonIdleWorker->winHide(TRUE);
	populateObserverList();
}

void ControlBar::showObserverPlayerInfo()
{
	m_observerPlayerInfoWindow->winHide(FALSE);
	m_observerPlayerListWindow->winHide(TRUE);

	if(getObserverLookAtPlayer())
		populateObserverInfoWindow();
}
