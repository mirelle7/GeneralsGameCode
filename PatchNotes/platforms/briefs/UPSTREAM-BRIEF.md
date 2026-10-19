# Conflict resolution brief: merging upstream/main into `platforms`

Read RESOLVER-BRIEF.md in this folder first: its rules (keep both sides, VC6 safety, no invented
null-guards, hard rules about not running write-git commands, log format) all still apply. Only the
situation differs, as described here.

## Situation
- Worktree `<worktrees>/platforms`, branch `platforms`. A merge of `upstream/main` (2886271c5cb,
  2026-10-18) is in progress, set up by hand with
  `git merge-tree --merge-base=b687c073b62 HEAD upstream/main` and `read-tree -u --reset`. Conflicts are
  zdiff3 markers in the files; the base section is `b687c073b62` (upstream 2026-09-27, the last
  point trunk had synced with upstream).
- **HEAD side ("ours")** = `platforms`: bobtista's bgfx/SDL3 trunk + the user's Android/iOS/macOS/Switch/
  Win64/wasm/Xbox UWP ports (already merged and hand-resolved; logs in log-*.md in this folder).
- **Theirs ("upstream/main")** = TheSuperHackers/GeneralsGameCode main, 127 commits since 2026-09-27.
  The headline one is **PR #2670** (merge fb253791832, by Okladnoj): "Route game logic math through
  WWMath with 3-mode deterministic support". It adds the `gamemath` library, deterministic
  WWMath entry points, `RETAIL_COMPATIBLE_CRC` / `USE_DETERMINISTIC_MATH` switches, deterministic D3DX vector math
  for BezierSegment, removes `visualc.h`, and moves Trig and other math through WWMath. **The goal is
  that CPUs of different platforms (x86/x64/ARM, Windows/Linux/macOS/Android/Switch/wasm) do not desync in
  multiplayer.** Other notable upstream changes: BIGFile/ArchiveFile merge (#3505), MiniDumper moved to a
  static library (#3506), unichar type (#3321), std::bit_cast pointer casts (#3532), unify of common
  system files (#3463), IABS/F2DW consolidations, hash-map port, logging fixes.
- Bobtista's trunk (our HEAD) had its OWN earlier, partial attempt at deterministic math (e.g.
  `Core/GameEngine/Source/Common/Diagnostic/SimulationMathCrc.cpp`, Bezier float math, a gamemath.cmake,
  `Trig.cpp`, `BaseDefines.h` additions). Where the two overlap, **upstream's merged #2670 version is
  the canonical, maintained solution: take upstream's version** and delete or fold trunk's duplicate.
  Never keep two competing deterministic-math implementations. Keep any of trunk's/our additions that
  #2670 does not cover (e.g. platform guards for Android/Switch/wasm/UWP, non-Windows support), expressed
  on top of upstream's structure.
- Determinism is the whole point: the final result must make game logic math go through the #2670
  wrappers on every platform, with the same results as upstream's modes. Double-check any hunk in
  game-logic code (AI, PartitionManager, PhysicsUpdate, projectile, pathfinding, Bezier, Trig) against
  upstream's intent. Do not leave a raw libm/`<cmath>`/`D3DX` call where upstream routed it through WWMath.

## Rules of thumb for this merge
1. Upstream's refactors (file merges, moved headers, renamed types, deleted files) win structurally.
   Where upstream deleted or moved a file that HEAD modified (modify/delete), port HEAD's change onto the
   new location by hand, then delete the old-path file with a plain file delete.
2. Keep our platform additions (guards for `__ANDROID__`, `__SWITCH__`, `__EMSCRIPTEN__`, `GGC_PLATFORM_UWP`,
   `TARGET_OS_IPHONE`, non-Windows paths, bgfx backend code in WW3D2/Backend/).
3. Windows behaviour and VC6 compatibility must not regress (upstream CI builds VC6 + MSVC).
4. Upstream added `<cstdint>`-free idioms; follow upstream's new project-wide `unichar` type and
   `std::bit_cast`-style fixes (#3321, #3532) where HEAD touched the same lines.
