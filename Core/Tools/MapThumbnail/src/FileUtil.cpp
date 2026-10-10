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

#include "FileUtil.h"

#include <cctype>
#include <fstream>

namespace MapThumbnail
{

std::string toLower(std::string text)
{
	for (char &c : text)
		c = char(tolower(static_cast<unsigned char>(c)));
	return text;
}

fs::path pathFromUtf8(const std::string &text)
{
#if defined(__cpp_char8_t)
	return fs::path(std::u8string(text.begin(), text.end()));
#else
	return fs::u8path(text);
#endif
}

std::string pathToUtf8(const fs::path &path)
{
#if defined(__cpp_char8_t)
	const std::u8string text = path.u8string();
	return std::string(text.begin(), text.end());
#else
	return path.u8string();
#endif
}

bool readWholeFile(const fs::path &path, std::vector<uint8_t> &out, uintmax_t maxSize)
{
	std::error_code ec;
	const uintmax_t size = fs::file_size(path, ec);
	if (ec || size > maxSize)
		return false;
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return false;
	out.resize(size_t(size));
	return size == 0 || bool(file.read(reinterpret_cast<char *>(out.data()), std::streamsize(size)));
}

bool findPathNoCase(const fs::path &dir, const std::string &relative, fs::path &out)
{
	fs::path current = dir;
	size_t start = 0;
	while (start <= relative.size())
	{
		size_t end = relative.find_first_of("\\/", start);
		if (end == std::string::npos)
			end = relative.size();
		const std::string part = relative.substr(start, end - start);
		start = end + 1;
		if (part.empty())
			continue;

		std::error_code ec;
		fs::path exact = current / pathFromUtf8(part);
		if (fs::exists(exact, ec))
		{
			current = exact;
			continue;
		}

		const std::string lowerPart = toLower(part);
		bool found = false;
		for (fs::directory_iterator it(current, ec), endIt; !ec && it != endIt; it.increment(ec))
		{
			if (toLower(pathToUtf8(it->path().filename())) == lowerPart)
			{
				current = it->path();
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}

	std::error_code ec;
	if (!fs::is_regular_file(current, ec))
		return false;
	out = current;
	return true;
}

} // namespace MapThumbnail
