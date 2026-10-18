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

// This file contains fundamental types that newer cpp standards provide as built-in types.

#pragma once

// VC6 does not have wchar_t as a built-in type.
#if defined(_MSC_VER) && _MSC_VER < 1300
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif

// VC6 with STLport does not provide the C library functions in namespace std through <cstdio>,
// <cstdlib> and <cstring>, so alias the global ones for code that uses the std:: spelling.
#if defined(_MSC_VER) && _MSC_VER < 1300
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
namespace std
{
	using ::sin;
	using ::cos;
	using ::tan;
	using ::FILE;
	using ::size_t;
	using ::printf;
	using ::fprintf;
	using ::sprintf;
	using ::sscanf;
	using ::fopen;
	using ::fclose;
	using ::fflush;
	using ::fread;
	using ::fwrite;
	using ::fseek;
	using ::ftell;
	using ::fgets;
	using ::fputs;
	using ::puts;
	using ::atoi;
	using ::atol;
	using ::atof;
	using ::strtol;
	using ::strtoul;
	using ::strtod;
	using ::malloc;
	using ::calloc;
	using ::realloc;
	using ::free;
	using ::getenv;
	using ::exit;
	using ::qsort;
	using ::strlen;
	using ::strcmp;
	using ::strncmp;
	using ::strcpy;
	using ::strncpy;
	using ::strcat;
	using ::strchr;
	using ::strrchr;
	using ::strstr;
	using ::memcpy;
	using ::memmove;
	using ::memset;
	using ::memcmp;
	using ::memchr;
}
#endif
