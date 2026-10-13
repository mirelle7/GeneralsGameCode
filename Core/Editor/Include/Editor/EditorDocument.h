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

// EditorDocument.h
// TheSuperHackers @feature The map document of the in-game WorldBuilder. Ported from the MFC
// CWorldBuilderDoc: same load order, same chunk layout on save, no MFC.

#pragma once

#include "Lib/BaseType.h"
#include "Common/AsciiString.h"

class DataChunkInput;
struct DataChunkInfo;
class MapObject;
class WorldHeightMapEdit;

class EditorDocument
{
public:
	enum { MAX_WAYPOINTS = 16000 };

	EditorDocument();
	~EditorDocument();

	/// Loads a .map file from a path on disk. Replaces the global map object, polygon trigger
	/// and sides lists, exactly like the classic WorldBuilder.
	Bool load(const AsciiString &path);

	/// Saves to a path on disk with the compression stored in the map's world dict.
	Bool save(const AsciiString &path);

	/// Frees the heightmap and the global map lists.
	void close();

	Bool isOpen() const { return m_heightMap != nullptr; }
	WorldHeightMapEdit *getHeightMap() const { return m_heightMap; }
	const AsciiString &getPath() const { return m_path; }

	Int getNumWaypointLinks() const { return m_numWaypointLinks; }
	void getWaypointLink(Int ndx, Int *waypointID1, Int *waypointID2) const;
	MapObject *getWaypointByID(Int waypointID);
	Int getNextWaypointID() { return ++m_curWaypointID; }

private:
	static Bool parseWaypointDataChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);

	void validate();
	void compressWaypointIds();
	void updateWaypointTable();

	WorldHeightMapEdit *m_heightMap;
	AsciiString m_path;

	Int m_curWaypointID;
	Bool m_waypointTableNeedsUpdate;
	MapObject *m_waypointTable[MAX_WAYPOINTS];

	struct WaypointLink
	{
		Int waypoint1;
		Int waypoint2;
	};
	Int m_numWaypointLinks;
	WaypointLink m_waypointLinks[MAX_WAYPOINTS];
};
