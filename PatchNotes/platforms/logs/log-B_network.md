# Log: group B_network

All 61 conflict hunks resolved. Every file in this group now matches HEAD (trunk + wasm-clean) byte for byte
(`git diff HEAD -- <file>` is empty for all 10 files), with CRLF line endings kept. Each change theirs made in these files was
a duplicate of a fix trunk already had, usually a later and more complete version of it. No functionality was lost (see below).

Note: the auto-merge also brought in theirs' versions without conflict markers in a few places, which had to be removed by hand:
- LANAPI.h: the `LANWideChar`/`lanWriteWide`/`lanReadWide` block and the relaxed `static_assert`
- udp.cpp: the `ggc_socklen_t` typedef
- WebBrowser.cpp / StagingRoomGameInfo.cpp: a second, nested `#if defined(_WIN32)` guard and a second non-Win `TheWebBrowser` definition

| file | hunk | resolution | rule | NEEDS-REVIEW? |
|---|---|---|---|---|
| LANAPI.h | 10 hunks: `LANMessage` wide-string fields + helpers | ours (trunk `UnsignedShort` fields + `lanWideStrCopy`/`lanWideStrToUnicode`). Theirs' `LANWideChar` (= UnsignedShort) + `lanWriteWide`/`lanReadWide` is the same fix. The wire layout is identical either way: 2-byte UTF-16, same as Windows retail. Trunk's strict `static_assert(sizeof(LANMessage) <= MAX)` is kept; it now holds on every platform, so theirs' `sizeof(WideChar)!=2 ||` relaxation is not needed | 2 | no |
| LANAPI.cpp | 14 hunks: name/gameName/chat copies | ours (trunk `lanWideStrCopy`), the same as theirs' `lanWriteWide` | 2 | no |
| LANAPIhandlers.cpp | 22 hunks: incoming name/gameName/chat reads + reply copies | ours (trunk `lanWideStrToUnicode`/`lanWideStrCopy`). Theirs' diff in this file contained only the wide-char change | 2 | no |
| udp.cpp | 1: Emscripten relay `connect()` URL | ours (wasm-clean's newer version: `?relay=` override, https -> no relay, early return on an empty URL) replaces theirs' plain `ws://host:8090` | 2 | no |
| udp.cpp | 4: `namelen`/`alen`/`len` socket-length vars | ours (`socklen_t`; trunk's udp.h does `typedef int socklen_t` on Win32). Theirs' `ggc_socklen_t` typedef was removed too, since nothing else uses it | 2 | no |
| IPEnumeration.cpp | 2: Emscripten `ggc_url_ip_index` + loopback list | ours (wasm-clean 30/07 version of the same the author code, plus trunk's `<ifaddrs.h>`/`<ws2ipdef.h>` includes) | 2 | no |
| WebBrowser.h | 4: ATL include guards + non-Win stub class | ours (trunk's inline stub). Theirs' stub declared out-of-line methods. Trunk's WebBrowser.cpp guards the whole implementation, so trunk's inline stub is the one that matches | 2 | no |
| WebBrowser.cpp | 1: closing `#endif` | ours. Theirs' second, nested `_WIN32` guards and `TheWebBrowser` definition were also removed | 2 | no |
| StagingRoomGameInfo.cpp | 1: SNMP pointer guard | ours. Theirs' second, nested `#if !defined(_WIN32)` early-return block was also removed (trunk has the same thing) | 2 | no |
| PingThread.cpp | 1: `IcmpSendEcho` function-pointer cast | ours (`DWORD(HANDLE, DWORD, LPVOID, WORD, LPVOID, LPVOID, DWORD, DWORD)`). Theirs' `DWORD`/`void*` variant was the same Win64 pointer-size fix | 2 | no |
| Transport.cpp | 1: `doRecv` slot fill | ours (trunk's reworked loop, already uses `sin_addr.s_addr` instead of `S_un.S_addr`) | 2 | no |

## Builder notes
- The `LANWideChar`, `lanWriteWide`, `lanReadWide` and `ggc_socklen_t` symbols are gone. A repo-wide grep found no users outside these files.
- `socklen_t` on Windows comes from trunk's `typedef int socklen_t` in `GameNetwork/udp.h` (VC6 + MSVC). If a UWP/Win64/MinGW TU also pulls in `ws2tcpip.h`, that is a harmless redeclaration of the same typedef, but check it if a "redefinition" error ever shows up.
- No new files and no CMake changes from this group.
- LAN wire protocol: Windows is byte-identical to retail. Non-Windows builds also send 2-byte UTF-16, so cross-platform LAN works.
