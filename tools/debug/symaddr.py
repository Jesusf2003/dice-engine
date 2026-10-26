"""Resolves crash offsets inside an exe to function + source line, using its .pdb (Windows only, needs no debugger).

usage: symaddr.py <exe> <hex offset> [<hex offset> ...]
Offsets are relative to the module base, as Windows' "Application Error" event shows them
("Desplazamiento con errores" / "Fault offset"), e.g.: symaddr.py bin\\quaked364_d.exe 3c5885
"""
import ctypes, sys
from ctypes import wintypes as W

import os

if len(sys.argv) < 3:
    sys.exit(__doc__)
exe, offs = sys.argv[1], [int(x, 16) for x in sys.argv[2:]]
if not os.path.isfile(exe):
    sys.exit(f"{exe}: no such file")
dbg = ctypes.WinDLL("dbghelp.dll")
dbg.SymLoadModuleEx.restype = ctypes.c_uint64
dbg.SymLoadModuleEx.argtypes = [W.HANDLE, W.HANDLE, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint64, W.DWORD, ctypes.c_void_p, W.DWORD]
h = W.HANDLE(0x1234)
dbg.SymSetOptions(0x2 | 0x10)  # UNDNAME | LOAD_LINES
assert dbg.SymInitialize(h, None, False)
base = dbg.SymLoadModuleEx(h, None, exe.encode(), None, 0x140000000, 0, None, 0)
assert base, ctypes.GetLastError()

class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [("SizeOfStruct", W.ULONG), ("TypeIndex", W.ULONG), ("Reserved", ctypes.c_uint64 * 2),
                ("Index", W.ULONG), ("Size", W.ULONG), ("ModBase", ctypes.c_uint64), ("Flags", W.ULONG),
                ("Value", ctypes.c_uint64), ("Address", ctypes.c_uint64), ("Register", W.ULONG),
                ("Scope", W.ULONG), ("Tag", W.ULONG), ("NameLen", W.ULONG), ("MaxNameLen", W.ULONG),
                ("Name", ctypes.c_char * 512)]

class LINE(ctypes.Structure):
    _fields_ = [("SizeOfStruct", W.DWORD), ("Key", ctypes.c_void_p), ("LineNumber", W.DWORD),
                ("FileName", ctypes.c_char_p), ("Address", ctypes.c_uint64)]

for off in offs:
    addr = base + off
    s = SYMBOL_INFO(); s.SizeOfStruct = 88; s.MaxNameLen = 511
    disp = ctypes.c_uint64()
    name = s.Name.decode() if dbg.SymFromAddr(h, ctypes.c_uint64(addr), ctypes.byref(disp), ctypes.byref(s)) else "?"
    ln = LINE(); ln.SizeOfStruct = ctypes.sizeof(LINE); d2 = W.DWORD()
    loc = f"{ln.FileName.decode()}:{ln.LineNumber}" if dbg.SymGetLineFromAddr64(h, ctypes.c_uint64(addr), ctypes.byref(d2), ctypes.byref(ln)) else ""
    print(f"{off:#x} {name}+{disp.value:#x} {loc}")
