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
// device layer; see HeadlessGameEngine.h.

#include "StdDevice/Common/HeadlessGameEngine.h"

#include "Common/BIGFileSystem.h"
#include "Common/GameEngine.h"
#include "GameClient/ParticleSys.h"
#include "GameLogic/GameLogic.h"
#include "GameNetwork/NetworkInterface.h"
#include "StdDevice/Common/DummyAudioManager.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "W3DDevice/Common/W3DModuleFactory.h"
#include "W3DDevice/GameLogic/W3DGameLogic.h"
#include "W3DDevice/GameClient/W3DGameClient.h"
#include "W3DDevice/Common/W3DFunctionLexicon.h"
#include "W3DDevice/Common/W3DRadar.h"
#include "W3DDevice/Common/W3DThingFactory.h"

HeadlessGameEngine::HeadlessGameEngine()
{
}

HeadlessGameEngine::~HeadlessGameEngine()
{
}

void HeadlessGameEngine::serviceWindowsOS()
{
}

GameLogic *HeadlessGameEngine::createGameLogic() { return NEW W3DGameLogic; }
GameClient *HeadlessGameEngine::createGameClient() { return NEW W3DGameClient; }
ModuleFactory *HeadlessGameEngine::createModuleFactory() { return NEW W3DModuleFactory; }
ThingFactory *HeadlessGameEngine::createThingFactory() { return NEW W3DThingFactory; }
FunctionLexicon *HeadlessGameEngine::createFunctionLexicon() { return NEW W3DFunctionLexicon; }
LocalFileSystem *HeadlessGameEngine::createLocalFileSystem() { return NEW StdLocalFileSystem; }
ArchiveFileSystem *HeadlessGameEngine::createArchiveFileSystem() { return NEW BIGFileSystem; }
NetworkInterface *HeadlessGameEngine::createNetwork() { return NetworkInterface::createNetwork(); }

// -headless replaces these with dummies anyway, and there is no device for the real ones.
Radar *HeadlessGameEngine::createRadar(Bool /*dummy*/) { return NEW RadarDummy; }
WebBrowser *HeadlessGameEngine::createWebBrowser() { return nullptr; }
AudioManager *HeadlessGameEngine::createAudioManager(Bool /*dummy*/) { return NEW DummyAudioManager; }
ParticleSystemManager *HeadlessGameEngine::createParticleSystemManager(Bool /*dummy*/) { return NEW ParticleSystemManagerDummy; }
