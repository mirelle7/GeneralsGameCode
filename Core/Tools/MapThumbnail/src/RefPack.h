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

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace MapThumbnail
{

/// Decodes a RefPack stream (the format behind CompressionManager's "EAR" type).
/// Unlike EAC/refdecode.cpp, every read and write is bounds checked, because file browsers
/// hand thumbnailers whatever file happens to be named *.map.
bool refPackDecode(const uint8_t *src, size_t srcLen, std::vector<uint8_t> &out, size_t maxSize);

/// If data is an "EAR\0" container, decompresses it into storage and points data/size at it.
/// Uncompressed data is left as is. Returns false only for a damaged container.
bool unwrapCompressed(const uint8_t *&data, size_t &size, std::vector<uint8_t> &storage);

} // namespace MapThumbnail
