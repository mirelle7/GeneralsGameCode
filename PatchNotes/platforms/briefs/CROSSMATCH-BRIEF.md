# Brief: wasm <-> macOS LAN match ("crossmatch")

Goal (from the user, 2026-10-19): prove a real multiplayer game works between the **WebAssembly build
(in a browser on this Windows PC)** and the **native macOS arm64 build (on the user's MacBook)**: host on one,
join from the other, start a match, let it run, and check that neither side desyncs. This is the end-to-end
test of upstream PR #2670 (deterministic math) on our `platforms` branch. We already know the math CRC is
identical everywhere (`-mathCrcCheck` = 76B53840 / 4E2B6D2C on MSVC x86/x64, Linux, Android arm64, wasm);
this checks it in a real game. Modern mode only (no retail compatibility; builds use
RTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF).

## Machines
- **PC** (Windows, this machine, LAN IP: find it with `ipconfig`, expected 192.168.x.x).
  Runs the wasm build in a browser (puppeteer-core + Chrome as in <notes-dir>/build-wasm.md and
  scripts/web/lan-two-player-test.js) and the relay.
- **Mac**: `ssh -o BatchMode=yes <mac-user>@<mac-ip>` (Bash tool needs `dangerouslyDisableSandbox: true`;
  prefix remote commands with `export PATH=~/bin:$PATH`). Same LAN, arm64, macOS 27, Xcode 27. The macOS build is
  made by another agent (pf/apple / `<worktrees>/pf-apple`, binary under `build/macos-generalsmd-sdl3-bgfx/GeneralsMD/Release/`);
  coordinate through the file <notes-dir>/build-apple.md. If the binary isn't there yet, work on the
  PC side first, then check again.
- The user is at the PC and the Mac is logged in on its screen, so GUI apps can be launched on it (`open`, or run
  the binary with the GUI session; `screencapture` may need a one-time permission prompt the user has to approve).

## How wasm networking works today (read the code, don't trust this summary blindly)
Browsers cannot do UDP. In the wasm build, `Core/GameEngine/Source/GameNetwork/udp.cpp` (`__EMSCRIPTEN__` path)
tunnels every datagram over one WebSocket to `scripts/build/web/relay.py` (port 8090) using a 12-byte header
(srcIP, srcPort, dstIP, dstPort, big-endian) + payload; the relay routes unicast by destination IP and fans
broadcast (255.255.255.255, used by LAN discovery on the lobby port) out to the other WebSocket clients. Each tab
declares an identity IP through `?ip=N` (127.0.0.N); see `ggc_url_ip_index()` in IPEnumeration.cpp and the
relay.py `declared_ip` logic. Game traffic uses hard-coded UDP ports (lobby 8086 and the game ports; find them in
LANAPI.h / Transport / NetworkDefs), so peers must differ by IP, not port.

## What has to be built: a bridge between the relay and the real LAN
A native Mac game sends real UDP on the LAN (broadcast + unicast to peer IPs). Proposed design (change it if the
code shows a better way; explain why):
1. Give `relay.py` a **real-UDP side**: bind UDP sockets (SO_REUSEADDR, broadcast enabled) on the game ports on
   the PC; datagrams arriving from the LAN are wrapped and sent to the WebSocket client(s); datagrams from a
   WebSocket client whose destination IP is a real LAN host (or broadcast) are sent out as real UDP (broadcast:
   to the LAN broadcast address, e.g. <lan-broadcast> and/or 255.255.255.255).
2. The browser tab's identity IP must be a **real, routable LAN IP**, so the Mac can unicast back: use the PC's LAN
   IP. That needs `?ip=` (or a new param, e.g. `?ipaddr=192.168.x.x`) to accept a full dotted IP in
   `ggc_url_ip_index()`/`getAddresses()`; keep the existing `?ip=N` behaviour for loopback tabs unchanged.
   Make sure the relay does NOT loop its own outgoing UDP back to the WebSocket, and that the Mac's packets that carry
   the PC's IP as the destination go to the browser.
3. Windows firewall: the relay needs inbound UDP on those ports. If a firewall rule is needed, do NOT silently
   change system firewall settings: tell the user the exact command and ask (report it, and stop on that step).
4. Both sides need **identical game data**: the user's Steam install on this PC has zeroed regions in MapsZH/TerrainZH/
   MusicZH/TexturesZH.big. Use the clean copy `<test-exe-dir>` for the browser (serve.ini: `STEAM_ZH_DIR=<test-exe-dir>`,
   see build-wasm.md) and for the Mac: compare checksums of `~/zh-game` on the Mac against zhcooptest (sha256 of the .big files)
   and only if they differ copy the clean ones over with rsync/scp (about 3.4 GB; the Mac has ~31 GB free).
   The user's ~/zh-game folder is theirs, so put the clean copy in a NEW folder (`~/zh-game-clean`) instead of overwriting it.
5. The LAN lobby may compare exe/INI CRCs or versions between peers, and the exe differs between wasm and macOS.
   Find what it checks (LANAPIhandlers.cpp: game CRC / version / "GameInfo" compatibility); if it rejects the mix, find
   out whether that's expected in modern mode and what the real upstream mechanism is (don't just disable checks; report).

## Test procedure
1. Start `serve.py` (data) and the bridged `relay.py` on the PC. Open the wasm game in headless-or-visible Chrome
   (visible is better here so the user can watch; use the throttling-disable flags from the earlier notes).
2. Run the macOS game on the Mac with the clean data. Both sides in the LAN lobby: confirm each sees the other
   (host on wasm, join from Mac; then if time allows, the reverse).
3. Start a skirmish/LAN game with both humans (and optionally AI). Let it run several minutes of game time with
   real orders from both sides if you can script that, or at minimum idle/move units, so the simulation CRC check
   (the game exchanges per-frame CRCs and shows "Game desynced"/"CRC Mismatch" and writes logs) gets exercised.
4. Report: did discovery work, did the join work, did the match start on both, did it run N frames/minutes, did any
   desync/mismatch message appear on either side (check game logs on both: Mac `~/Library/Application Support/...` or
   the game's log dir; wasm console), with screenshots if possible. If it desyncs, find the first desynced frame and
   which object/value (the game's CRC debug logging can help) and report the evidence; don't paper over it.

## Rules
- Work in a worktree + branch (create it: `git -C <repo> worktree add -b pf/crossmatch <worktrees>/pf-crossmatch platforms`).
  If the wasm module must be rebuilt for the `?ip=` change, use the existing wasm build tree in <worktrees>/pf-wasm
  (fast-forward it to your branch tip or cherry-pick) so you don't rebuild FFmpeg etc. (build recipe in build-wasm.md).
  Commit changes in small commits, conventional messages, **no Co-Authored-By trailer**, never push to GitHub.
- Windows/VC6 must not regress: relay.py and the `?ip=` parsing are wasm-only, keep any C++ change under `__EMSCRIPTEN__`.
- Don't modify the user's registry, firewall or other apps. Don't delete anything you did not create.
- Report in <notes-dir>/crossmatch.md and in your final reply: exact commands, what worked, what didn't,
  evidence for/against desync, and a list of commits.
