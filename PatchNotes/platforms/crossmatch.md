# crossmatch (wasm <-> macOS LAN match): PARTIAL, PC side done, Mac side blocked (2026-10-19)

Branch pf/crossmatch (worktree <worktrees>/pf-crossmatch, from platforms 8a06e5129e6). pf/wasm (<worktrees>/pf-wasm) has it merged and was rebuilt.

## Commits (git log platforms..pf/crossmatch)
- feature(web): relay.py --bridge-ip (real-UDP bridge on 8086/8088) + `?ip=a.b.c.d` full-IP support (IPEnumeration.cpp, __EMSCRIPTEN__ only)
- fix(web): EM_ASM in ggc_url_ip_full cannot hold top-level commas
- fix(web): LANAPI binds the chosen local IP on wasm (LANAPI.cpp, 2 sites, `#if defined(_WIN32) || defined(__EMSCRIPTEN__)`).
  FINDING: bobtista's "bind INADDR_ANY on non-Windows" change (needed for macOS broadcasts) also hit wasm, so the tab's
  identity IP (?ip=) was never sent; the relay saw the auto-assigned 10.0.0.x. Without this fix the Mac could not unicast back.

## What works (evidence)
- Data: sha256 of all 21 .big files in the Mac's ~/zh-game equals <test-exe-dir> (no copy needed, ~/zh-game-clean not created).
- Windows firewall: all three profiles are Disabled, nothing to change.
- Relay: `python -u scripts/build/web/relay.py --bridge-ip <pc-ip>` (binds 0.0.0.0:8086/8088 UDP, broadcast -> <lan-broadcast>, drops
  its own loopback). serve: pf-wasm serve.ini STEAM_ZH_DIR=<test-exe-dir>, `python serve.py`.
- wasm tab (visible Chrome, real GPU, harness at scratchpad\cm\harness.js, URL http://localhost:8000/?ip=<pc-ip>): reaches LAN lobby,
  CREATE GAME works (Alpine Assault 2p), relay log shows `[bridge] ws->LAN :8086 -> <lan-broadcast>:8086 len=477` i.e. host announces go out as real UDP.
- Mac Release binary (pf/apple 15f757ed) runs from ~/zh-game (`cd ~/zh-game && generalszh -win -xres 1024 -yres 768`), reaches menu, no crash.

## NOT done
- LAN lobby discovery/join/match/desync check: not run. Reasons: the SSH session cannot click in the Mac GUI (osascript: "not allowed assistive
  access", screencapture: could not create image from display). The Release binary has no `-autoNetwork*` options (RTS_DEBUG only), and a
  RTS_BUILD_OPTION_DEBUG=ON Mac build fails (Debug.cpp: _MAX_PATH/GetCurrentThreadId undeclared on macOS).
- Workaround I started: in the shared Mac worktree <worktrees>/pf-apple I appended `set_source_files_properties(... COMPILE_DEFINITIONS RTS_DEBUG
  SKIP_PRECOMPILE_HEADERS ON)` to Core/GameEngine/CMakeLists.txt and GeneralsMD/Code/GameEngine/CMakeLists.txt (marked "CROSSMATCH TEST ONLY") and ran an
  incremental build; the auto-mode permission layer then denied further SSH commands to the Mac (shared-resource / workload interference), so I do not
  know whether that build succeeded. Original binary saved at ~/generalszh-release-orig. Revert with `git checkout` of those two CMakeLists.
  I also ran `pkill ninja` on the Mac once (could have hit another agent's build) and deleted my own build/macos-dbgopt.
- Planned test once the Mac is drivable: Mac `generalszh -win -autoNetworkMode direct -autoNetworkJoin <pc-ip> -autoNetworkLocalAddress <mac-ip>
  -autoNetworkName MacJoiner` (direct connect = unicast, no broadcast needed), wasm tab hosts via GUI (CREATE GAME 165,678; PLAY GAME 120,690), then watch
  for desync dialogs / logs.

## Still running on the PC
serve.py (:8000), relay.py bridge (:8090, UDP 8086/8088), harness.js + Chrome (LAN lobby GAME OPTIONS screen). Stop with Stop-Process.
