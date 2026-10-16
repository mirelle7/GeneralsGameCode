"""Resolve the module+RVA frames in an Xbox ggc_log.txt crash trace to symbols.

TheSuperHackers @build githubawn 16/10/2026 Uses the PC's dbghelp.dll with the PDBs
from the UWP build tree.

usage: python symbolize-log.py <ggc_log.txt> <dir-with-binaries-and-pdbs>...
"""
import ctypes
import ctypes.wintypes as wt
import os
import re
import sys

dbghelp = ctypes.WinDLL('dbghelp.dll')
proc = wt.HANDLE(0x1234)  # any unique value works for an offline session
dbghelp.SymSetOptions(0x2 | 0x10 | 0x200)  # UNDNAME | LOAD_LINES | FAIL_CRITICAL_ERRORS
dbghelp.SymInitialize(proc, ';'.join(sys.argv[2:]).encode(), False)
dbghelp.SymLoadModuleEx.restype = ctypes.c_uint64
dbghelp.SymLoadModuleEx.argtypes = [wt.HANDLE, wt.HANDLE, ctypes.c_char_p, ctypes.c_char_p,
                                    ctypes.c_uint64, wt.DWORD, ctypes.c_void_p, wt.DWORD]


class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [('SizeOfStruct', wt.ULONG), ('TypeIndex', wt.ULONG), ('Reserved', ctypes.c_uint64 * 2),
                ('Index', wt.ULONG), ('Size', wt.ULONG), ('ModBase', ctypes.c_uint64), ('Flags', wt.ULONG),
                ('Value', ctypes.c_uint64), ('Address', ctypes.c_uint64), ('Register', wt.ULONG),
                ('Scope', wt.ULONG), ('Tag', wt.ULONG), ('NameLen', wt.ULONG), ('MaxNameLen', wt.ULONG),
                ('Name', ctypes.c_char * 512)]


class IMAGEHLP_LINE64(ctypes.Structure):
    _fields_ = [('SizeOfStruct', wt.DWORD), ('Key', ctypes.c_void_p), ('LineNumber', wt.DWORD),
                ('FileName', ctypes.c_char_p), ('Address', ctypes.c_uint64)]


bases = {}
next_base = 0x10000000


def base_for(module):
    global next_base
    if module in bases:
        return bases[module]
    path = None
    for d in sys.argv[2:]:
        for root, _, files in os.walk(d):
            if module.lower() in (f.lower() for f in files):
                path = os.path.join(root, next(f for f in files if f.lower() == module.lower()))
                break
        if path:
            break
    b = 0
    if path:
        b = dbghelp.SymLoadModuleEx(proc, None, path.encode(), None, next_base, 0, None, 0)
        next_base += 0x10000000
    bases[module] = b
    return b


for line in open(sys.argv[1], encoding='utf-8', errors='replace'):
    m = re.search(r'(\S+\.(?:exe|dll))\+0x([0-9a-fA-F]+)', line)
    if not m:
        print(line.rstrip())
        continue
    b = base_for(m.group(1))
    sym = ''
    if b:
        info = SYMBOL_INFO()
        info.SizeOfStruct = ctypes.sizeof(SYMBOL_INFO) - 512
        info.MaxNameLen = 511
        disp = ctypes.c_uint64()
        addr = b + int(m.group(2), 16)
        if dbghelp.SymFromAddr(proc, ctypes.c_uint64(addr), ctypes.byref(disp), ctypes.byref(info)):
            sym = f'{info.Name.decode()}+0x{disp.value:x}'
            ln = IMAGEHLP_LINE64()
            ln.SizeOfStruct = ctypes.sizeof(IMAGEHLP_LINE64)
            d32 = wt.DWORD()
            if dbghelp.SymGetLineFromAddr64(proc, ctypes.c_uint64(addr), ctypes.byref(d32), ctypes.byref(ln)):
                sym += f'  ({os.path.basename(ln.FileName.decode())}:{ln.LineNumber})'
    print(line.rstrip() + ('   ' + sym if sym else ''))
