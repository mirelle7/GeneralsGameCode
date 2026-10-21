/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
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

// TheSuperHackers @feature githubawn 21/10/2026 Game engine for platforms without a Win32
// device layer. It only supports -headless runs: there is no window, input, audio or
// rendering device, and the subsystems that headless replaces with dummies are created as
// dummies.

#pragma once

#include "Common/GameEngine.h"

class HeadlessGameEngine : public GameEngine
{
public:
	HeadlessGameEngine();
	virtual ~HeadlessGameEngine() override;

	virtual void serviceWindowsOS() override;

protected:
	virtual GameLogic *createGameLogic() override;
	virtual GameClient *createGameClient() override;
	virtual ModuleFactory *createModuleFactory() override;
	virtual ThingFactory *createThingFactory() override;
	virtual FunctionLexicon *createFunctionLexicon() override;
	virtual LocalFileSystem *createLocalFileSystem() override;
	virtual ArchiveFileSystem *createArchiveFileSystem() override;
	virtual NetworkInterface *createNetwork();
	virtual Radar *createRadar(Bool dummy) override;
	virtual WebBrowser *createWebBrowser() override;
	virtual AudioManager *createAudioManager(Bool dummy) override;
	virtual ParticleSystemManager *createParticleSystemManager(Bool dummy) override;
};
