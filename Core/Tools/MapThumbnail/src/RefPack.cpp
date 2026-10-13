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

#include "RefPack.h"

#include <cstring>

namespace MapThumbnail
{

namespace
{

const size_t MaxMapSize = 64 * 1024 * 1024;

} // namespace

bool refPackDecode(const uint8_t *src, size_t srcLen, std::vector<uint8_t> &out, size_t maxSize)
{
	size_t s = 0;
	if (srcLen < 2)
		return false;

	const unsigned type = (src[0] << 8) | src[1];
	s = 2;
	if ((type & 0xff) != 0xfb)
		return false;

	const size_t sizeBytes = (type & 0x8000) ? 4 : 3;
	if (type & 0x100)
		s += sizeBytes; // compressed size, unused
	if (s + sizeBytes > srcLen)
		return false;

	size_t ulen = 0;
	for (size_t i = 0; i < sizeBytes; ++i)
		ulen = (ulen << 8) | src[s++];
	if (ulen > maxSize)
		return false;

	out.clear();
	out.reserve(ulen);

	auto literal = [&](size_t run) -> bool {
		if (run > srcLen - s || out.size() + run > ulen)
			return false;
		out.insert(out.end(), src + s, src + s + run);
		s += run;
		return true;
	};
	auto copy = [&](size_t offset, size_t run) -> bool {
		if (offset > out.size() || out.size() + run > ulen)
			return false;
		// Overlapping copies repeat the bytes just written, so go one byte at a time.
		size_t from = out.size() - offset;
		for (size_t i = 0; i < run; ++i)
			out.push_back(out[from + i]);
		return true;
	};

	for (;;)
	{
		if (s >= srcLen)
			return false;
		const unsigned first = src[s++];

		if (!(first & 0x80)) // short form
		{
			if (s + 1 > srcLen)
				return false;
			const unsigned second = src[s++];
			if (!literal(first & 3))
				return false;
			if (!copy(((first & 0x60) << 3) + second + 1, ((first & 0x1c) >> 2) + 3))
				return false;
		}
		else if (!(first & 0x40)) // int form
		{
			if (s + 2 > srcLen)
				return false;
			const unsigned second = src[s++];
			const unsigned third = src[s++];
			if (!literal(second >> 6))
				return false;
			if (!copy(((second & 0x3f) << 8) + third + 1, (first & 0x3f) + 4))
				return false;
		}
		else if (!(first & 0x20)) // very int form
		{
			if (s + 3 > srcLen)
				return false;
			const unsigned second = src[s++];
			const unsigned third = src[s++];
			const unsigned fourth = src[s++];
			if (!literal(first & 3))
				return false;
			if (!copy(((first & 0x10) << 12) + (second << 8) + third + 1, ((first & 0x0c) << 6) + fourth + 5))
				return false;
		}
		else
		{
			const size_t run = ((first & 0x1f) << 2) + 4;
			if (run <= 112)
			{
				if (!literal(run))
					return false;
			}
			else // end of stream with 0..3 trailing literals
			{
				if (!literal(first & 3))
					return false;
				break;
			}
		}
	}

	return out.size() == ulen;
}

bool unwrapCompressed(const uint8_t *&data, size_t &size, std::vector<uint8_t> &storage)
{
	if (size < 8 || memcmp(data, "EAR\0", 4) != 0)
		return true;

	const size_t expected = data[4] | (data[5] << 8) | (data[6] << 16) | (size_t(data[7]) << 24);
	if (expected > MaxMapSize)
		return false;
	if (!refPackDecode(data + 8, size - 8, storage, MaxMapSize) || storage.size() != expected)
		return false;

	data = storage.data();
	size = storage.size();
	return true;
}

} // namespace MapThumbnail
