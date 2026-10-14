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

// D3DX8MsvcRuntime.cpp
// TheSuperHackers @build MSVC runtime helpers referenced by the static d3dx8.lib of the DirectX 8
// SDK that neither msvcrt.dll nor MinGW-w64 provide under these names. Only built for MinGW-w64
// i686, see CMakeLists.txt.

#include <stdlib.h>

typedef void (__attribute__((thiscall)) *MsvcVectorFunc)(void *object);

// The functions get plain assembler names. The MSVC mangled names contain '?', which the GNU
// assembler only accepts quoted, so they are defined as aliases in the assembly block below.
void *msvcOperatorNew(unsigned int size) __asm__("_d3dx8_msvc_operator_new");
void msvcOperatorDelete(void *ptr) __asm__("_d3dx8_msvc_operator_delete");
void __attribute__((stdcall)) msvcVectorConstructor(void *ptr, unsigned int size, int count, MsvcVectorFunc ctor, MsvcVectorFunc dtor) __asm__("_d3dx8_msvc_vector_constructor");
void __attribute__((stdcall)) msvcVectorDestructor(void *ptr, unsigned int size, int count, MsvcVectorFunc dtor) __asm__("_d3dx8_msvc_vector_destructor");

void *msvcOperatorNew(unsigned int size)
{
	return malloc(size != 0 ? size : 1);
}

void msvcOperatorDelete(void *ptr)
{
	free(ptr);
}

void __attribute__((stdcall)) msvcVectorConstructor(void *ptr, unsigned int size, int count, MsvcVectorFunc ctor, MsvcVectorFunc)
{
	unsigned char *object = (unsigned char *)ptr;
	for (int i = 0; i < count; ++i, object += size)
		ctor(object);
}

void __attribute__((stdcall)) msvcVectorDestructor(void *ptr, unsigned int size, int count, MsvcVectorFunc dtor)
{
	unsigned char *object = (unsigned char *)ptr + size * count;
	for (int i = 0; i < count; ++i)
	{
		object -= size;
		dtor(object);
	}
}

// operator new(size_t), operator delete(void *), `eh vector constructor iterator' and
// `eh vector destructor iterator' with MSVC name mangling.
// _alloca_probe: MSVC stack probe, same contract as MinGW's __chkstk (size in eax).
// _aullshr: unsigned 64 bit shift right of edx:eax by cl.
// _except_list: offset of the SEH chain head in the TIB, used as fs:[_except_list].
__asm__(
	".globl \"??2@YAPAXI@Z\"\n"
	".set \"??2@YAPAXI@Z\", _d3dx8_msvc_operator_new\n"
	".globl \"??3@YAXPAX@Z\"\n"
	".set \"??3@YAXPAX@Z\", _d3dx8_msvc_operator_delete\n"
	".globl \"??_L@YGXPAXIHP6EX0@Z1@Z\"\n"
	".set \"??_L@YGXPAXIHP6EX0@Z1@Z\", _d3dx8_msvc_vector_constructor\n"
	".globl \"??_M@YGXPAXIHP6EX0@Z@Z\"\n"
	".set \"??_M@YGXPAXIHP6EX0@Z@Z\", _d3dx8_msvc_vector_destructor\n"
	".text\n"
	".globl __alloca_probe\n"
	"__alloca_probe:\n"
	"	jmp ___chkstk\n"
	".globl __aullshr\n"
	"__aullshr:\n"
	"	cmpb $64, %cl\n"
	"	jae 2f\n"
	"	cmpb $32, %cl\n"
	"	jae 1f\n"
	"	shrdl %cl, %edx, %eax\n"
	"	shrl %cl, %edx\n"
	"	ret\n"
	"1:\n"
	"	movl %edx, %eax\n"
	"	xorl %edx, %edx\n"
	"	andb $31, %cl\n"
	"	shrl %cl, %eax\n"
	"	ret\n"
	"2:\n"
	"	xorl %eax, %eax\n"
	"	xorl %edx, %edx\n"
	"	ret\n"
	".globl __except_list\n"
	".set __except_list, 0\n"
);
