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

// FILE: ControlBarPopupDescription.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Electronic Arts Pacific.
//
//                       Confidential Information
//                Copyright (C) 2002 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
//	created:	Sep 2002
//
//	Filename: 	ControlBarPopupDescription.cpp
//
//	author:		Chris Huybregts
//
//	purpose:
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// DEFINES ////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------


// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/GlobalData.h"
#include "Common/BuildAssistant.h"
#include "Common/SeatManager.h"	// seatLog, for the temporary GX_TOOLTIPPROBE
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/ProductionPrerequisite.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "GameClient/AnimateWindowManager.h"
#include "GameClient/DisconnectMenu.h"
#include "GameClient/GameWindow.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetTextEntry.h"
#include "GameClient/GadgetPushButton.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/GameClient.h"
#include "GameClient/GameFont.h"
#include "GameClient/GameText.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/InGameUI.h"
#include "GameClient/ControlBar.h"
#include "GameClient/Display.h"
#include "GameClient/DisplayStringManager.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/OverchargeBehavior.h"
#include "GameLogic/Module/ProductionUpdate.h"
#include "GameLogic/ScriptEngine.h"

#include "GameNetwork/NetworkInterface.h"

static WindowLayout *theLayout = nullptr;
static GameWindow *theWindow = nullptr;
static AnimateWindowManager *theAnimateWindowManager = nullptr;
static Bool useAnimation = FALSE;
void ControlBarPopupDescriptionUpdateFunc( WindowLayout *layout, void *param )
{
	// Splitscreen: this update func is installed on EVERY instance's layout and run per
	// instance, but drove the global TheControlBar - dormant only while no seat>0 layout was
	// ever shown, and made live by the routing fix itself. Without this, seat N's popup is
	// evaluated against seat 0's m_showBuildToolTipLayout and never hides, while seat 0's
	// layout gets deleted instead. param is the owning bar (ControlBar::update passes `this`).
	ControlBar *bar = (ControlBar *)param;
	if( bar == nullptr )
		bar = TheControlBar;

	if(TheScriptEngine->isGameEnding())
		bar->hideBuildTooltipLayout();

	if(theAnimateWindowManager && !bar->getShowBuildTooltipLayout() && !theAnimateWindowManager->isReversed())
		theAnimateWindowManager->reverseAnimateWindow();
	else if(!bar->getShowBuildTooltipLayout() && (!TheGlobalData->m_animateWindows || !useAnimation))
		bar->deleteBuildTooltipLayout();


	if ( useAnimation && theAnimateWindowManager && TheGlobalData->m_animateWindows)
	{
		Bool wasFinished = theAnimateWindowManager->isFinished();
		theAnimateWindowManager->update();
		if (theAnimateWindowManager->isFinished() && !wasFinished && theAnimateWindowManager->isReversed())
		{
			delete theAnimateWindowManager;
			theAnimateWindowManager = nullptr;
			bar->deleteBuildTooltipLayout();
		}
	}

}

// ---------------------------------------------------------------------------------------
void ControlBar::showBuildTooltipLayout( GameWindow *cmdButton )
{
	if (TheInGameUI->areTooltipsDisabled() 	|| TheScriptEngine->isGameEnding())
	{
		return;
	}

	Bool passedWaitTime = FALSE;
	if(m_tooltipPrevWindow == cmdButton)
	{
		m_showBuildToolTipLayout = TRUE;
		if(!m_tooltipWaitInitialized &&  m_tooltipBeginWaitTime + cmdButton->getTooltipDelay() < timeGetTime())
		{
			//DEBUG_LOG(("%d beginwaittime, %d tooltipdelay, %dtimegettime", m_tooltipBeginWaitTime, cmdButton->getTooltipDelay(), timeGetTime()));
			passedWaitTime = TRUE;
		}

		if(!passedWaitTime)
			return;
	}
	else if( !m_buildToolTipLayout->isHidden() )
	{
		if(useAnimation && TheGlobalData->m_animateWindows && !theAnimateWindowManager->isReversed())
			theAnimateWindowManager->reverseAnimateWindow();
		else if( useAnimation && TheGlobalData->m_animateWindows && theAnimateWindowManager->isReversed())
		{
			return;
		}
		else
		{
//			m_buildToolTipLayout->destroyWindows();
//			deleteInstance(m_buildToolTipLayout);
//			m_buildToolTipLayout = nullptr;
			m_buildToolTipLayout->hide(TRUE);
			m_tooltipPrevWindow = nullptr;
		}
		return;
	}


	// will only get here the firsttime through the function through this window
	if(!passedWaitTime)
	{
		m_tooltipPrevWindow = cmdButton;
		m_tooltipBeginWaitTime = timeGetTime();
		m_tooltipWaitInitialized = FALSE;
		return;
	}
	m_tooltipWaitInitialized = TRUE;

	if(!cmdButton)
		return;
	if(BitIsSet(cmdButton->winGetStyle(), GWS_PUSH_BUTTON))
	{
		const CommandButton *commandButton = (const CommandButton *)GadgetButtonGetData(cmdButton);

		if(!commandButton)
			return;

		// note that, in this branch, ENABLE_SOLO_PLAY is ***NEVER*** defined...
		// this is so that we have a multiplayer build that cannot possibly be hacked
		// to work as a solo game!
		if (TheGameLogic->isInReplayGame())
			return;

		if (TheInGameUI->isQuitMenuVisible())
			return;

		if (TheDisconnectMenu && TheDisconnectMenu->isScreenVisible())
			return;

		//	if (m_buildToolTipLayout)
		//	{
		//		m_buildToolTipLayout->destroyWindows();
		//		deleteInstance(m_buildToolTipLayout);
		//
		//	}

		m_showBuildToolTipLayout = TRUE;
		//	m_buildToolTipLayout = TheWindowManager->winCreateLayout( "ControlBarPopupDescription.wnd" );
		//	m_buildToolTipLayout->setUpdate(ControlBarPopupDescriptionUpdateFunc);

		populateBuildTooltipLayout(commandButton);
	}
	else
	{
		// we're a generic window
		if(!BitIsSet(cmdButton->winGetStyle(), GWS_USER_WINDOW) && !BitIsSet(cmdButton->winGetStyle(), GWS_STATIC_TEXT))
			return;
		populateBuildTooltipLayout(nullptr, cmdButton);
	}
	m_buildToolTipLayout->hide(FALSE);

	if (useAnimation && TheGlobalData->m_animateWindows)
	{
		theAnimateWindowManager = NEW AnimateWindowManager;
		theAnimateWindowManager->reset();
		theAnimateWindowManager->registerGameWindow( m_buildToolTipLayout->getFirstWindow(), WIN_ANIMATION_SLIDE_RIGHT_FAST, TRUE, 200 );
	}


}


void ControlBar::repopulateBuildTooltipLayout()
{
	if(!m_tooltipPrevWindow || !m_buildToolTipLayout)
		return;
	if(!BitIsSet(m_tooltipPrevWindow->winGetStyle(), GWS_PUSH_BUTTON))
		return;
	const CommandButton *commandButton = (const CommandButton *)GadgetButtonGetData(m_tooltipPrevWindow);
	populateBuildTooltipLayout(commandButton);
}

//-------------------------------------------------------------------------------------------------
/** Splitscreen: resolve an id strictly inside THIS bar's own tooltip layout. The layout is a
	* separate top-level .wnd, so a global lookup finds an arbitrary bar's copy once more than one
	* bar exists. Loops every root because a layout may have more than one. */
//-------------------------------------------------------------------------------------------------
GameWindow *ControlBar::findTooltipWindowById( NameKeyType id ) const
{
	if( m_buildToolTipLayout == nullptr || TheWindowManager == nullptr )
		return nullptr;

	for( GameWindow *w = m_buildToolTipLayout->getFirstWindow(); w; w = w->winGetNextInLayout() )
		if( GameWindow *found = TheWindowManager->winFindChildById( w, id ) )
			return found;

	return nullptr;
}

//-------------------------------------------------------------------------------------------------
void ControlBar::populateBuildTooltipLayout( const CommandButton *commandButton, GameWindow *tooltipWin)
{
	if(!m_buildToolTipLayout)
		return;

	// Splitscreen: price against the player THIS bar shows, not the machine-wide local one -
	// line 570 of this same function already uses the per-instance accessor.
	Player *player = getCurrentlyViewedPlayer();
	UnicodeString name, cost, descrip;
	UnicodeString requiresFormat = UnicodeString::TheEmptyString, requiresList;
	Bool firstRequirement = true;
	const ProductionPrerequisite *prereq;
	Bool fireScienceButton = false;
	UnsignedInt costToBuild = 0;

	if(commandButton)
	{
		const ThingTemplate *thingTemplate = commandButton->getThingTemplate();
		const UpgradeTemplate *upgradeTemplate = commandButton->getUpgradeTemplate();

		ScienceType	st = SCIENCE_INVALID;
		if( commandButton->getCommandType() != GUI_COMMAND_PLAYER_UPGRADE &&
				commandButton->getCommandType() != GUI_COMMAND_OBJECT_UPGRADE )
		{
			if( commandButton->getScienceVec().size() > 1 )
			{
				for(size_t j = 0; j < commandButton->getScienceVec().size(); ++j)
				{
					st = commandButton->getScienceVec()[ j ];

					if( commandButton->getCommandType() != GUI_COMMAND_PURCHASE_SCIENCE )
					{
						if( !player->hasScience( st ) && j > 0 )
						{
							//If we're not looking at a command button that purchases a science, then
							//it means we are looking at a command button that can USE the science. This
							//means we want to get the description for the previous science -- the one
							//we can use, not purchase!
							st = commandButton->getScienceVec()[ j - 1 ];
						}

						//Now that we got the science for the button that executes the science, we need
						//to generate a simpler help text!
						fireScienceButton = TRUE;

						break;
					}
					else if( !player->hasScience( st ) )
					{
						//Purchase science case. The first science we run into that we don't have, that's the
						//one we'll want to show!
						break;
					}
				}
			}
			else if(commandButton->getScienceVec().size() == 1 )
			{
				st = commandButton->getScienceVec()[ 0 ];
				if( commandButton->getCommandType() != GUI_COMMAND_PURCHASE_SCIENCE )
				{
					//Now that we got the science for the button that executes the science, we need
					//to generate a simpler help text!
					fireScienceButton = TRUE;
				}
			}
		}

		if( commandButton->getDescriptionLabel().isNotEmpty() )
		{
			descrip = TheGameText->fetch(commandButton->getDescriptionLabel());

			Drawable *draw = TheInGameUI->getFirstSelectedDrawable();
			Object *selectedObject = draw ? draw->getObject() : nullptr;
			if( selectedObject )
			{
				//Special case: Append status of overcharge on China power plant.
				if( commandButton->getCommandType() == GUI_COMMAND_TOGGLE_OVERCHARGE )
				{
					{
						OverchargeBehaviorInterface *obi;
						for( BehaviorModule **bmi = selectedObject->getBehaviorModules(); *bmi; ++bmi )
						{
							obi = (*bmi)->getOverchargeBehaviorInterface();
							if( obi )
							{
								descrip.concat( L"\n" );
								if( obi->isOverchargeActive() )
									descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNukeReactorOverChargeIsOn" ) );
								else
									descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNukeReactorOverChargeIsOff" ) );
							}
						}
					}
				}

				//Special case: When building units & buildings, the CanMakeType determines reasons for not being able to buy stuff.
				else if( thingTemplate )
				{
					CanMakeType makeType = TheBuildAssistant->canMakeUnit( selectedObject, commandButton->getThingTemplate() );
					switch( makeType )
					{
						case CANMAKE_NO_MONEY:
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNotEnoughMoneyToBuild" ) );
							break;
						case CANMAKE_QUEUE_FULL:
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotPurchaseBecauseQueueFull" ) );
							break;
						case CANMAKE_PARKING_PLACES_FULL:
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotBuildUnitBecauseParkingFull" ) );
							break;
						case CANMAKE_MAXED_OUT_FOR_PLAYER:
							descrip.concat( L"\n\n" );
							if ( thingTemplate->isKindOf( KINDOF_STRUCTURE ) )
							{
								bool exists;
								UnicodeString text = TheGameText->fetch("TOOLTIP:TooltipCannotBuildBuildingBecauseMaximumNumber", &exists);
								if (!exists)
								{
									// TheSuperHackers @info The prior string does not exist in Generals.
									text = TheGameText->fetch("TOOLTIP:TooltipCannotBuildUnitBecauseMaximumNumber");
								}
								descrip.concat(text);
							}
							else
							{
								descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotBuildUnitBecauseMaximumNumber" ) );
							}
							break;
						//case CANMAKE_NO_PREREQ:
						//	descrip.concat( L"\n\n" );
						//	descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotBuildDueToPrerequisites" ) );
						//	break;
					}
				}

				//Special case: When building upgrades
				else if( upgradeTemplate && !player->hasUpgradeInProduction( upgradeTemplate ) )
				{
					if( commandButton->getCommandType() == GUI_COMMAND_PLAYER_UPGRADE ||
						  commandButton->getCommandType() == GUI_COMMAND_OBJECT_UPGRADE )
					{
						ProductionUpdateInterface *pui = selectedObject->getProductionUpdateInterface();
						if( pui && pui->getProductionCount() == MAX_BUILD_QUEUE_BUTTONS )
						{
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotPurchaseBecauseQueueFull" ) );
						}
						else if( !TheUpgradeCenter->canAffordUpgrade( getCurrentlyViewedPlayer(), upgradeTemplate, FALSE ) )
						{
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNotEnoughMoneyToBuild" ) );
						}
					}
				}

			}

		}

		name = TheGameText->fetch(commandButton->getTextLabel().str());

		if( thingTemplate && commandButton->getCommandType() != GUI_COMMAND_PURCHASE_SCIENCE )
		{
			//We are either looking at building a unit or a structure that may or may not have any
			//prerequisites.

			//Format the cost only when we have to pay for it.
			costToBuild = thingTemplate->calcCostToBuild( player );
			if( costToBuild > 0 )
			{
				cost.format( TheGameText->fetch("TOOLTIP:Cost"), costToBuild );
			}

			// ask each prerequisite to give us a list of the non satisfied prerequisites
			for( Int i=0; i<thingTemplate->getPrereqCount(); i++ )
			{
				prereq = thingTemplate->getNthPrereq(i);
				requiresList = prereq->getRequiresList(player);

				if( requiresList != UnicodeString::TheEmptyString )
				{
					// make sure to put in 'returns' to space things correctly
					if (firstRequirement)
						firstRequirement = false;
					else
						requiresFormat.concat(L", ");
				}
				requiresFormat.concat(requiresList);
			}
			if( !requiresFormat.isEmpty() )
			{
				UnicodeString requireFormat = TheGameText->fetch("CONTROLBAR:Requirements");
				requiresFormat.format(requireFormat.str(), requiresFormat.str());
				if(!descrip.isEmpty())
					descrip.concat(L"\n");
				descrip.concat(requiresFormat);

			}
		}
		else if( upgradeTemplate )
		{
			//We are looking at an upgrade purchase icon. Maybe we already purchased it?

			Bool hasUpgradeAlready = player->hasUpgradeComplete( upgradeTemplate );
			Bool hasConflictingUpgrade = FALSE;
			Bool missingScience = FALSE;
			Bool playerUpgradeButton = commandButton->getCommandType() == GUI_COMMAND_PLAYER_UPGRADE;
			Bool objectUpgradeButton = commandButton->getCommandType() == GUI_COMMAND_OBJECT_UPGRADE;

			if( !hasUpgradeAlready )
			{
				//Check if the first selected object has the specified upgrade.
				Drawable *draw = TheInGameUI->getFirstSelectedDrawable();
				if( draw )
				{
					Object *object = draw->getObject();
					if( object )
					{
						hasUpgradeAlready = object->hasUpgrade( upgradeTemplate );
						if( objectUpgradeButton )
						{
							hasConflictingUpgrade = !object->affectedByUpgrade( upgradeTemplate );
						}
					}
				}
			}
			if( hasConflictingUpgrade && !hasUpgradeAlready )
			{
				if( commandButton->getConflictingLabel().isNotEmpty() )
				{
					descrip = TheGameText->fetch( commandButton->getConflictingLabel() );
				}
				else
				{
					descrip = TheGameText->fetch( "TOOLTIP:HasConflictingUpgradeDefault" );
				}
			}
			else if( hasUpgradeAlready && ( playerUpgradeButton || objectUpgradeButton ) )
			{
				//See if we can fetch the "already upgraded" text for this upgrade. If not.... use the default "fill me in".
				if( commandButton->getPurchasedLabel().isNotEmpty() )
				{
					descrip = TheGameText->fetch( commandButton->getPurchasedLabel() );
				}
				else
				{
					descrip = TheGameText->fetch( "TOOLTIP:AlreadyUpgradedDefault" );
				}
			}
			else if( !hasUpgradeAlready )
			{

				//Do we have a prerequisite science?
				for( size_t i = 0; i < commandButton->getScienceVec().size(); i++ )
				{
					ScienceType st = commandButton->getScienceVec()[ i ];
					if( !player->hasScience( st ) )
					{
						missingScience = TRUE;
						break;
					}
				}

				//Determine the cost of the upgrade.
				costToBuild = upgradeTemplate->calcCostToBuild( player );
				if( costToBuild > 0 )
				{
					cost.format( TheGameText->fetch("TOOLTIP:Cost"), costToBuild );
				}

				if( missingScience )
				{
					if( !descrip.isEmpty() )
						descrip.concat(L"\n");
					requiresFormat.format( TheGameText->fetch( "CONTROLBAR:Requirements" ).str(), TheGameText->fetch( "CONTROLBAR:GeneralsPromotion" ).str() );
					descrip.concat( requiresFormat );
				}
			}
		}
		else if( st != SCIENCE_INVALID && !fireScienceButton )
		{
			TheScienceStore->getNameAndDescription(st, name, descrip);

			costToBuild = TheScienceStore->getSciencePurchaseCost( st );
			if( costToBuild > 0 )
			{
				cost.format( TheGameText->fetch("TOOLTIP:ScienceCost"), costToBuild );
			}

			// ask each prerequisite to give us a list of the non satisfied prerequisites
			if( thingTemplate )
			{
				for( Int i=0; i<thingTemplate->getPrereqCount(); i++ )
				{
					prereq = thingTemplate->getNthPrereq(i);
					requiresList = prereq->getRequiresList(player);

					if( requiresList != UnicodeString::TheEmptyString )
					{
						// make sure to put in 'returns' to space things correctly
						if (firstRequirement)
							firstRequirement = false;
						else
							requiresFormat.concat(L", ");
					}
					requiresFormat.concat(requiresList);
				}
				if( !requiresFormat.isEmpty() )
				{
					UnicodeString requireFormat = TheGameText->fetch("CONTROLBAR:Requirements");
					requiresFormat.format(requireFormat.str(), requiresFormat.str());
					if(!descrip.isEmpty())
						descrip.concat(L"\n");
					descrip.concat(requiresFormat);
				}
			}

		}
	}
	else if(tooltipWin)
	{

		if( tooltipWin == findBarWindowById( TheNameKeyGenerator->nameToKey("ControlBar.wnd:MoneyDisplay") ))
		{
			name = TheGameText->fetch("CONTROLBAR:Money");
			descrip = TheGameText->fetch("CONTROLBAR:MoneyDescription");
		}
		else if(tooltipWin == findBarWindowById( TheNameKeyGenerator->nameToKey("ControlBar.wnd:PowerWindow") ) )
		{
			name = TheGameText->fetch("CONTROLBAR:Power");
			descrip = TheGameText->fetch("CONTROLBAR:PowerDescription");

			Player* playerToDisplay = getCurrentlyViewedPlayer();

			if( playerToDisplay && playerToDisplay->getEnergy() )
			{
				Energy *energy = playerToDisplay->getEnergy();
				descrip.format(descrip, energy->getProduction(), energy->getConsumption());
			}
			else
			{
				descrip.format(descrip, 0, 0);
			}
		}
		else if(tooltipWin == findBarWindowById( TheNameKeyGenerator->nameToKey("ControlBar.wnd:GeneralsExp") ) )
		{
			name = TheGameText->fetch("CONTROLBAR:GeneralsExp");
			descrip = TheGameText->fetch("CONTROLBAR:GeneralsExpDescription");
		}
		else
		{
			DEBUG_CRASH(("ControlBar::populateBuildTooltipLayout We attempted to call the popup tooltip on a game window that has yet to be hand coded in as this fuction was/is designed for only buttons but has been hacked to work with GameWindows."));
			return;
		}

	}
	GameWindow *titleWin = findTooltipWindowById( TheNameKeyGenerator->nameToKey("ControlBarPopupDescription.wnd:StaticTextName") );
	GameWindow *win = titleWin;
	if(win)
	{
		GadgetStaticTextSetText(win, name);
	}

	win = findTooltipWindowById( TheNameKeyGenerator->nameToKey("ControlBarPopupDescription.wnd:StaticTextCost") );
	if(win)
	{
		if( costToBuild > 0 )
		{
			win->winHide( FALSE );
			GadgetStaticTextSetText(win, cost);
		}
		else
		{
			win->winHide( TRUE );
		}
	}

	win = findTooltipWindowById( TheNameKeyGenerator->nameToKey("ControlBarPopupDescription.wnd:StaticTextDescription") );
	if(win)
	{

		static NameKeyType winNamekey	= TheNameKeyGenerator->nameToKey( "ControlBar.wnd:BackgroundMarker" );

		ICoord2D size, newSize;
		Int diffSize;

		// Splitscreen: this tooltip's WindowLayout is never docked/rescaled by dockToRect (it is a
		// standalone popup, not one of this bar's registered layout roots), so win's raw current
		// size stays at its AUTHORED width forever. Scale it by this bar's own dock scale - the same
		// scale the X/Y position fix below uses - so the wrap width and the position it's drawn at
		// agree every frame instead of disagreeing in an 8-player quarter-width layout.
		const Real markerScale = getBarDockScale();

		// Splitscreen: capture this window's AUTHORED font once (same pattern ControlBar uses for
		// its own docked bar windows via captureAuthoredFont/applyScaledFont), then re-derive it at
		// the current dock scale every call. This tooltip is a standalone layout, not one of the
		// bar's registered dock windows, so nothing else was ever shrinking its font - the text
		// stayed full authored size even when the box/wrap were narrowed for 5-8 players.
		if( !m_tooltipAuthoredFontKnown )
		{
			GameFont *authoredFont = win->winGetFont();
			if( authoredFont != nullptr )
			{
				m_tooltipAuthoredFontName = authoredFont->nameString;
				m_tooltipAuthoredFontSize = authoredFont->pointSize;
				m_tooltipAuthoredFontBold = authoredFont->bold;
			}
			m_tooltipAuthoredFontKnown = TRUE;
		}
		GameFont *scaledFont = nullptr;
		if( m_tooltipAuthoredFontSize > 0 && TheFontLibrary != nullptr )
		{
			// Splitscreen: floor the FONT'S scale (only) well above the bar's own dock scale -
			// linear scaling made the description text unreadable at 5+ players. The box and
			// wrap width still shrink fully; the text just wraps onto more lines instead of
			// shrinking past legibility.
			const Real kMinFontScale = 0.65f;
			Real fontScale = markerScale;
			if( fontScale < kMinFontScale )
				fontScale = kMinFontScale;
			Int scaledPointSize = (Int)(m_tooltipAuthoredFontSize * fontScale + 0.5f);
			if( scaledPointSize < 1 )
				scaledPointSize = 1;
			scaledFont = TheFontLibrary->getFont( m_tooltipAuthoredFontName, scaledPointSize, m_tooltipAuthoredFontBold );
		}
		if( scaledFont == nullptr )
			scaledFont = win->winGetFont();
		win->winSetFont( scaledFont );

		// Splitscreen: anchor the wrap width to the AUTHORED win width, captured once, not to
		// win's CURRENT size.x - win's width is itself rescaled by markerScale below, and this
		// layout is reused across many calls, so reading "current" width back in as the wrap
		// basis fed the previous call's shrunk output in as if it were authored, narrowing the
		// wrap width a little further every call instead of ever settling.
		win->winGetSize(&size.x, &size.y);
		if( !m_tooltipAuthoredSizeKnown )
		{
			m_tooltipAuthoredWinWidth = size.x;
			m_tooltipAuthoredWinHeight = size.y;
		}

		DisplayString *tempDString = TheDisplayStringManager->newDisplayString();
		tempDString->setFont(scaledFont);
		tempDString->setWordWrap((Int)(m_tooltipAuthoredWinWidth * markerScale) - 10);
		tempDString->setText(descrip);
		tempDString->getSize(&newSize.x, &newSize.y);
		TheDisplayStringManager->freeDisplayString(tempDString);
		tempDString = nullptr;

		// Splitscreen: anchor the growth to the AUTHORED size, captured once, rather than to
		// whatever size a PREVIOUS call already grew the window to. m_buildToolTipLayout is
		// created once per bar and reused across many show/populate calls, so reading "current"
		// size here returns last call's answer - which produced a one-frame overshoot every time
		// the hovered button's description text changed height, before snapping back the next
		// frame once the stale baseline caught up. Same shape as the position fix above.
		diffSize = newSize.y - m_tooltipAuthoredWinHeight;
 		GameWindow *parent = m_buildToolTipLayout->getFirstWindow();
 		if(!parent)
 			return;

 		parent->winGetSize(&size.x, &size.y);
		if( !m_tooltipAuthoredSizeKnown )
		{
			m_tooltipAuthoredParentWidth = size.x;
			m_tooltipAuthoredParentHeight = size.y;
			if( titleWin != nullptr )
			{
				Int titleAuthoredW, titleAuthoredH;
				titleWin->winGetSize(&titleAuthoredW, &titleAuthoredH);
				m_tooltipAuthoredTitleWidth = titleAuthoredW;
			}
			m_tooltipAuthoredSizeKnown = TRUE;
		}
 		if(m_tooltipAuthoredParentHeight + diffSize < 102) {
			diffSize = 102 - m_tooltipAuthoredParentHeight;
		}

		const Int scaledParentWidth = (Int)(m_tooltipAuthoredParentWidth * markerScale + 0.5f);
		const Int finalParentHeight = m_tooltipAuthoredParentHeight + diffSize;
		parent->winSetSize(scaledParentWidth, finalParentHeight);

		// Splitscreen: this .wnd has exactly ONE top-level root (confirmed via a one-time
		// TOOLTIPGAP rootcount log - the earlier "second title root" theory was wrong).
		// StaticTextName is a CHILD of "parent", so winSetPosition(parent, ...) below already
		// moves it automatically - winGetPosition/winSetPosition are parent-relative for a child,
		// only winGetScreenPosition is absolute. The one real bug: titleWin's fixed AUTHORED
		// local position assumed parent's ORIGINAL (usually taller) authored height. Since parent
		// is resized every call (diffSize above), a short description can shrink the box below
		// where the title was authored to sit, leaving it poking out above the box's new top
		// edge ("outside the box"). Clamp its local Y to stay inside whatever height the box
		// actually has this frame.
		if( titleWin != nullptr )
		{
			Int titleLocalX, titleLocalY, titleW, titleH;
			titleWin->winGetPosition(&titleLocalX, &titleLocalY);
			titleWin->winGetSize(&titleW, &titleH);

			// Splitscreen: scale the title's WIDTH from its AUTHORED width, same reasoning as the
			// description box/text above - it's authored at the box's full, unscaled width, which
			// made it render far wider than the (dock-scale-shrunk) box and stick out past its
			// edges. Anchor to m_tooltipAuthoredTitleWidth, not the current size.x, for the same
			// compounding-shrink reason those fixes already document.
			const Int scaledTitleWidth = (Int)(m_tooltipAuthoredTitleWidth * markerScale + 0.5f);
			titleWin->winSetSize(scaledTitleWidth, titleH);

			const Int topPad = 4;
			if( titleLocalY < topPad )
				titleWin->winSetPosition(titleLocalX, topPad);
			else if( titleLocalY + titleH > finalParentHeight - topPad )
				titleWin->winSetPosition(titleLocalX, finalParentHeight - topPad - titleH);
		}

		// Splitscreen: anchor to the marker's LIVE on-screen position directly, instead of
		// reconstructing a position from two separately-cached AUTHORED positions (the marker's
		// and the box's own) plus a scale correction. That chain needed the box's authored
		// position captured before any dock transform ever touched it - fragile, and the source
		// of the scaling/positioning bugs already fixed in this function. winGetScreenPosition
		// already reflects this bar's real dock offset and scale, so no correction is needed.
		GameWindow *marker = findBarWindowById(winNamekey);
		if(!marker)
		{
			return;
		}
		ICoord2D markerPos;
		marker->winGetScreenPosition(&markerPos.x, &markerPos.y);

		Int absoluteX = markerPos.x - scaledParentWidth / 2;

		// Splitscreen: anchor the box's BOTTOM edge to the bar's real visible top edge, growing
		// upward as content grows, instead of down from the marker: the marker's own live
		// position turned out to sit at the BOTTOM of this bar (not the top, as originally
		// assumed - see the TOOLTIPGAP log), so anchoring off it directly pushed the box down
		// into the bar. "ControlBar.wnd:ControlBarParent" is the bar's actual top-level chrome
		// window (already used by showControlBarInstance's animation fix elsewhere in this file);
		// its live screen Y is the real top-of-bar edge, independent of the marker entirely.
		GameWindow *barChrome = findBarWindowById( TheNameKeyGenerator->nameToKey( "ControlBar.wnd:ControlBarParent" ) );
		Int barTopY = markerPos.y;	// fallback if the bar chrome window can't be resolved
		if( barChrome != nullptr )
		{
			ICoord2D barChromePos;
			barChrome->winGetScreenPosition(&barChromePos.x, &barChromePos.y);
			barTopY = barChromePos.y;
		}
		const Int gapAboveBar = 4;
		Int absoluteY = barTopY - gapAboveBar - finalParentHeight;

		// Keep the whole popup on the actual rendered display - not clamped to this seat's
		// viewport (the box is allowed to sit over a neighboring quadrant), just kept from
		// rendering off the edge of the window entirely, which an unclamped anchor can do for
		// buttons near a screen edge.
		if( TheDisplay != nullptr )
		{
			const Int dispW = (Int)TheDisplay->getWidth();
			const Int dispH = (Int)TheDisplay->getHeight();
			if( absoluteX < 0 )
				absoluteX = 0;
			else if( absoluteX + scaledParentWidth > dispW )
				absoluteX = dispW - scaledParentWidth;
			if( absoluteY < 0 )
				absoluteY = 0;
			else if( absoluteY + finalParentHeight > dispH )
				absoluteY = dispH - finalParentHeight;
		}

		parent->winSetPosition(absoluteX, absoluteY);

		// Scale win's width from its AUTHORED width too, same reasoning as parent above - reading
		// win's current (already-scaled) size.x back in here would compound the shrink every call.
		const Int scaledWinWidth = (Int)(m_tooltipAuthoredWinWidth * markerScale + 0.5f);
 		win->winSetSize(scaledWinWidth, m_tooltipAuthoredWinHeight + diffSize);

		GadgetStaticTextSetText(win, descrip);
	}
	m_buildToolTipLayout->hide(FALSE);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ControlBar::hideBuildTooltipLayout()
{
	if(theAnimateWindowManager && theAnimateWindowManager->isReversed())
		return;
	if(useAnimation && theAnimateWindowManager && TheGlobalData->m_animateWindows)
		theAnimateWindowManager->reverseAnimateWindow();
	else
		deleteBuildTooltipLayout();

}

void ControlBar::deleteBuildTooltipLayout()
{
	m_showBuildToolTipLayout = FALSE;
	m_tooltipPrevWindow= nullptr;
	m_buildToolTipLayout->hide(TRUE);
//	if(!m_buildToolTipLayout)
//		return;
//
//	m_buildToolTipLayout->destroyWindows();
//	deleteInstance(m_buildToolTipLayout);
//	m_buildToolTipLayout = nullptr;

	delete theAnimateWindowManager;
	theAnimateWindowManager = nullptr;

}
