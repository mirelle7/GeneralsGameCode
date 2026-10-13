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

// EditorDocument.cpp
// TheSuperHackers @feature Map document of the in-game WorldBuilder, ported from CWorldBuilderDoc.

#include "Editor/EditorDocument.h"

#include "Editor/EditorUIHooks.h"
#include "Editor/WHeightMapEdit.h"

#include "Common/DataChunk.h"
#include "Common/Debug.h"
#include "Common/Dict.h"
#include "Common/Errors.h"
#include "Common/file.h"
#include "Common/FileSystem.h"
#include "Common/MapObject.h"
#include "Common/MapReaderWriterInfo.h"
#include "Common/NameKeyGenerator.h"
#include "Common/PlayerTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/WellKnownKeys.h"
#include "GameLogic/PolygonTrigger.h"
#include "GameLogic/SidesList.h"
#include "W3DDevice/GameClient/WorldHeightMap.h"
#include "Compression.h"

#include <vector>

namespace
{
	/// Collects the whole map in memory and writes it compressed, like the MFC
	/// CompressedCachedMFCFileOutputStream did.
	class CompressedFileOutputStream : public OutputStream
	{
	public:
		virtual Int write(const void *pData, Int numBytes) override
		{
			const UnsignedByte *bytes = (const UnsignedByte *)pData;
			m_data.insert(m_data.end(), bytes, bytes + numBytes);
			return numBytes;
		}

		Bool writeToFile(const AsciiString &path)
		{
			if (m_data.empty())
				return FALSE;

			CompressionType compressionToUse = CompressionManager::getPreferredCompression();
			Dict *worldDict = MapObject::getWorldDict();
			if (worldDict)
			{
				Bool exists = FALSE;
				compressionToUse = (CompressionType)worldDict->getInt(TheKey_compression, &exists);
				if (!exists || compressionToUse > COMPRESSION_MAX || compressionToUse < COMPRESSION_MIN)
					compressionToUse = CompressionManager::getPreferredCompression();
			}

			const Int srcLen = (Int)m_data.size();
			Int compressedLen = CompressionManager::getMaxCompressedSize(srcLen, compressionToUse);
			std::vector<UnsignedByte> compressed(compressedLen > 0 ? compressedLen : 0);
			if (compressedLen > 0)
				compressedLen = CompressionManager::compressData(compressionToUse, &m_data[0], srcLen, &compressed[0], compressedLen);

			File *file = TheFileSystem->openFile(path.str(), File::WRITE | File::CREATE | File::TRUNCATE | File::BINARY);
			if (file == nullptr)
				return FALSE;

			Int written;
			if (compressedLen > 0)
				written = file->write(&compressed[0], compressedLen) == compressedLen ? compressedLen : -1;
			else
				written = file->write(&m_data[0], srcLen) == srcLen ? srcLen : -1;

			file->close();
			return written > 0;
		}

	private:
		std::vector<UnsignedByte> m_data;
	};
}

EditorDocument::EditorDocument() :
	m_heightMap(nullptr),
	m_curWaypointID(0),
	m_waypointTableNeedsUpdate(TRUE),
	m_numWaypointLinks(0)
{
	for (Int i = 0; i < MAX_WAYPOINTS; ++i)
		m_waypointTable[i] = nullptr;
}

EditorDocument::~EditorDocument()
{
	close();
}

void EditorDocument::close()
{
	REF_PTR_RELEASE(m_heightMap);
	m_path.clear();
	m_numWaypointLinks = 0;
	m_curWaypointID = 0;
	m_waypointTableNeedsUpdate = TRUE;

	WorldHeightMap::freeListOfMapObjects();
	PolygonTrigger::deleteTriggers();
}

Bool EditorDocument::load(const AsciiString &path)
{
	CachedFileInputStream theInputStream;
	if (!theInputStream.open(path))
	{
		EditorUI::messageFormat("Could not open map '%s'.", path.str());
		return FALSE;
	}

	close();

	try
	{
		PolygonTrigger::deleteTriggers();
		ChunkInputStream *pStrm = &theInputStream;

		// Read the logical data (map objects, sides, polygon triggers). Fills the global lists.
		WorldHeightMap *terrainHeightMap = NEW WorldHeightMap(pStrm, true);
		REF_PTR_RELEASE(terrainHeightMap);
		pStrm->absoluteSeek(0);

		// Read and keep the graphical data.
		m_heightMap = NEW_REF(WorldHeightMapEdit, (pStrm));
		pStrm->absoluteSeek(0);

		try
		{
			DataChunkInput file(pStrm);
			if (file.isValidFileType())
			{
				// Backwards compatible files aren't valid data chunk files.
				file.registerParser("WaypointsList", AsciiString::TheEmptyString, parseWaypointDataChunk);
				if (!file.parse(this))
					throw(ERROR_CORRUPT_FILE_FORMAT);
			}
		}
		catch (...)
		{
			// Legacy files aren't chunk format, same as the classic WorldBuilder.
		}
		theInputStream.close();
	}
	catch (...)
	{
		theInputStream.close();
		close();
		EditorUI::messageFormat("Map '%s' is corrupt or not a map file.", path.str());
		return FALSE;
	}

	validate();
	compressWaypointIds();

	// Always assign unique IDs, like the classic WorldBuilder.
	MapObject::fastAssignAllUniqueIDs();

	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		if (pMapObj->isWaypoint() && pMapObj->getWaypointID() >= m_curWaypointID)
			m_curWaypointID = pMapObj->getWaypointID();
	}

	m_heightMap->optimizeTiles();
	m_path = path;
	return TRUE;
}

Bool EditorDocument::save(const AsciiString &path)
{
	if (m_heightMap == nullptr)
		return FALSE;

	CompressedFileOutputStream theStream;
	try
	{
		DataChunkOutput *chunkWriter = new DataChunkOutput(&theStream);

		m_heightMap->saveToFile(*chunkWriter);

		chunkWriter->openDataChunk("WaypointsList", K_WAYPOINTS_VERSION_1);
		chunkWriter->writeInt(m_numWaypointLinks);
		for (Int i = 0; i < m_numWaypointLinks; ++i)
		{
			chunkWriter->writeInt(m_waypointLinks[i].waypoint1);
			chunkWriter->writeInt(m_waypointLinks[i].waypoint2);
		}
		chunkWriter->closeDataChunk();

		// The chunk writer emits its table of contents and data on destruction.
		delete chunkWriter;
	}
	catch (...)
	{
		EditorUI::messageFormat("Writing map '%s' failed.", path.str());
		return FALSE;
	}

	if (!theStream.writeToFile(path))
	{
		EditorUI::messageFormat("Could not write map '%s'.", path.str());
		return FALSE;
	}

	m_path = path;
	return TRUE;
}

Bool EditorDocument::parseWaypointDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData)
{
	EditorDocument *pThis = (EditorDocument *)userData;
	pThis->m_numWaypointLinks = file.readInt();
	if (pThis->m_numWaypointLinks > MAX_WAYPOINTS)
		pThis->m_numWaypointLinks = MAX_WAYPOINTS;
	for (Int i = 0; i < pThis->m_numWaypointLinks; ++i)
	{
		pThis->m_waypointLinks[i].waypoint1 = file.readInt();
		pThis->m_waypointLinks[i].waypoint2 = file.readInt();
	}
	DEBUG_ASSERTCRASH(file.atEndOfChunk(), ("Unexpected data left over."));
	return true;
}

void EditorDocument::getWaypointLink(Int ndx, Int *waypointID1, Int *waypointID2) const
{
	*waypointID1 = 0;
	*waypointID2 = 0;
	if (ndx >= 0 && ndx < m_numWaypointLinks)
	{
		*waypointID1 = m_waypointLinks[ndx].waypoint1;
		*waypointID2 = m_waypointLinks[ndx].waypoint2;
	}
}

// The classic WorldBuilder also swapped "Fundamentalist" beta names and asked for replacement
// models here. The in-game editor reports problems instead; both fixes come with the object tools.
void EditorDocument::validate()
{
	Int missingTemplates = 0;
	Bool needToFixTeams = FALSE;

	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		if (pMapObj->isScorch() || pMapObj->isWaypoint() || pMapObj->isLight() || pMapObj->getFlag(FLAG_ROAD_FLAGS) || pMapObj->getFlag(FLAG_BRIDGE_FLAGS))
			continue;

		if (pMapObj->getThingTemplate() == nullptr)
		{
			++missingTemplates;
			DEBUG_LOG(("Map object '%s' has no thing template.", pMapObj->getName().str()));
		}

		Bool exists;
		AsciiString teamName = pMapObj->getProperties()->getAsciiString(TheKey_originalOwner, &exists);
		if (!exists)
		{
			needToFixTeams = TRUE;
			continue;
		}

		TeamsInfo *teamInfo = TheSidesList->findTeamInfo(teamName);
		if (teamInfo == nullptr)
		{
			needToFixTeams = TRUE;
			continue;
		}

		AsciiString teamOwner = teamInfo->getDict()->getAsciiString(TheKey_teamOwner);
		if (TheSidesList->findSideInfo(teamOwner) == nullptr)
			needToFixTeams = TRUE;
	}

	if (missingTemplates > 0)
		EditorUI::messageFormat("%d map objects use templates that do not exist in this game.", missingTemplates);

	if (needToFixTeams)
		EditorUI::message("Some objects reference teams or players that do not exist. Fix the teams before playing this map.");
}

void EditorDocument::updateWaypointTable()
{
	if (!m_waypointTableNeedsUpdate)
		return;

	m_waypointTableNeedsUpdate = FALSE;
	for (Int i = 0; i < MAX_WAYPOINTS; ++i)
		m_waypointTable[i] = nullptr;

	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		if (!pMapObj->isWaypoint())
			continue;

		const Int id = pMapObj->getWaypointID();
		DEBUG_ASSERTCRASH(id > 0 && id < MAX_WAYPOINTS, ("Bad waypoint id."));
		if (id > 0 && id < MAX_WAYPOINTS)
		{
			if (m_waypointTable[id] != nullptr)
			{
				pMapObj->setWaypointID(getNextWaypointID());
				m_waypointTableNeedsUpdate = TRUE;
			}
			else
			{
				m_waypointTable[id] = pMapObj;
			}
		}
	}
}

MapObject *EditorDocument::getWaypointByID(Int waypointID)
{
	updateWaypointTable();
	if (waypointID > 0 && waypointID < MAX_WAYPOINTS)
	{
		MapObject *pObj = m_waypointTable[waypointID];
		if (pObj && pObj->isWaypoint())
			return pObj;
	}
	return nullptr;
}

void EditorDocument::compressWaypointIds()
{
	updateWaypointTable();

	// Renumber the waypoints 1..n and remap the links. Links to waypoints that no longer exist
	// are dropped.
	std::vector<Int> oldToNew(MAX_WAYPOINTS, 0);
	m_curWaypointID = 0;
	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		if (!pMapObj->isWaypoint())
			continue;

		const Int oldID = pMapObj->getWaypointID();
		const Int newID = getNextWaypointID();
		if (oldID > 0 && oldID < MAX_WAYPOINTS && m_waypointTable[oldID] == pMapObj)
			oldToNew[oldID] = newID;
		pMapObj->setWaypointID(newID);
	}

	Int kept = 0;
	for (Int i = 0; i < m_numWaypointLinks; ++i)
	{
		const Int id1 = m_waypointLinks[i].waypoint1;
		const Int id2 = m_waypointLinks[i].waypoint2;
		const Bool valid = id1 > 0 && id1 < MAX_WAYPOINTS && id2 > 0 && id2 < MAX_WAYPOINTS && oldToNew[id1] != 0 && oldToNew[id2] != 0;
		if (valid)
		{
			m_waypointLinks[kept].waypoint1 = oldToNew[id1];
			m_waypointLinks[kept].waypoint2 = oldToNew[id2];
			++kept;
		}
	}
	m_numWaypointLinks = kept;

	m_waypointTableNeedsUpdate = TRUE;
	updateWaypointTable();
}
