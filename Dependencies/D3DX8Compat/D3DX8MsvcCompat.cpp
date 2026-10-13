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

// Minimal MSVC runtime support for linking the MSVC compiled d3dx8.lib (min-dx8-sdk) into MinGW-w64 Clang/LLD builds.
// MinGW-w64 only ships the debug D3DX8 import library (d3dx8d.dll), which is not available on end user systems.
// The C++ exception handlers (__CxxFrameHandler, _except_handler3) are provided by msvcrt.dll.
// Everything else that d3dx8.lib needs from the MSVC static runtime is provided here.

#if !defined(__i386__)
#error D3DX8MsvcCompat is only meant for 32 bit x86 builds
#endif

#include <stdlib.h>

typedef void (__attribute__((thiscall)) *ElementFunc)(void*);

// void __stdcall `eh vector constructor iterator'(void*, unsigned int, int, void (__thiscall*)(void*), void (__thiscall*)(void*))
extern "C" void __stdcall EhVectorConstructorIterator(void* ptr, unsigned int size, int count, ElementFunc ctor, ElementFunc dtor)
	__asm__("??_L@YGXPAXIHP6EX0@Z1@Z");

// void __stdcall `eh vector destructor iterator'(void*, unsigned int, int, void (__thiscall*)(void*))
extern "C" void __stdcall EhVectorDestructorIterator(void* ptr, unsigned int size, int count, ElementFunc dtor)
	__asm__("??_M@YGXPAXIHP6EX0@Z@Z");

extern "C" void __stdcall EhVectorConstructorIterator(void* ptr, unsigned int size, int count, ElementFunc ctor, ElementFunc dtor)
{
	(void)dtor;
	char* p = static_cast<char*>(ptr);
	for (int i = 0; i < count; ++i, p += size)
		ctor(p);
}

extern "C" void __stdcall EhVectorDestructorIterator(void* ptr, unsigned int size, int count, ElementFunc dtor)
{
	// Elements are destroyed in reverse order.
	char* p = static_cast<char*>(ptr) + size * count;
	while (count-- > 0)
	{
		p -= size;
		dtor(p);
	}
}

__asm__(
	".intel_syntax noprefix\n"

	// Marker symbol the MSVC compiler references from objects using floating point.
	".data\n"
	".globl __fltused\n"
	"__fltused:\n"
	"	.long 0x9875\n"

	// Offset of the SEH exception list head in the x86 TEB, accessed as fs:[__except_list].
	".globl __except_list\n"
	".set __except_list, 0\n"

	".text\n"

	// void* __cdecl operator new(unsigned int)
	".globl \"??2@YAPAXI@Z\"\n"
	"\"??2@YAPAXI@Z\":\n"
	"	jmp _malloc\n"

	// void __cdecl operator delete(void*)
	".globl \"??3@YAXPAX@Z\"\n"
	"\"??3@YAXPAX@Z\":\n"
	"	jmp _free\n"

	// Stack probe. Allocates eax bytes on the stack, touching every page on the way down.
	// Same algorithm as _chkstk/_alloca_probe of the MSVC runtime.
	".globl __alloca_probe\n"
	"__alloca_probe:\n"
	"	push ecx\n"
	"	lea ecx, [esp + 4]\n"
	"	sub ecx, eax\n"
	"	sbb eax, eax\n"
	"	not eax\n"
	"	and ecx, eax\n"
	"	mov eax, esp\n"
	"	and eax, 0xFFFFF000\n"
	"1:\n"
	"	cmp ecx, eax\n"
	"	jb 2f\n"
	"	mov eax, ecx\n"
	"	pop ecx\n"
	"	xchg esp, eax\n"
	"	mov eax, dword ptr [eax]\n"
	"	mov dword ptr [esp], eax\n"
	"	ret\n"
	"2:\n"
	"	sub eax, 0x1000\n"
	"	test dword ptr [eax], eax\n"
	"	jmp 1b\n"

	// Unsigned 64 bit shift right: edx:eax >>= cl
	".globl __aullshr\n"
	"__aullshr:\n"
	"	cmp cl, 64\n"
	"	jae 2f\n"
	"	cmp cl, 32\n"
	"	jae 1f\n"
	"	shrd eax, edx, cl\n"
	"	shr edx, cl\n"
	"	ret\n"
	"1:\n"
	"	mov eax, edx\n"
	"	xor edx, edx\n"
	"	and cl, 31\n"
	"	shr eax, cl\n"
	"	ret\n"
	"2:\n"
	"	xor eax, eax\n"
	"	xor edx, edx\n"
	"	ret\n"

	".att_syntax prefix\n"
);
