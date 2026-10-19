# Conflict resolution brief: `platforms` merge of feature/xbox-uwp

## Situation
- Worktree: `<worktrees>/platforms`, branch `platforms`. A merge is in progress, set up by hand
  with `git merge-tree --merge-base=218fd403c8e HEAD origin/feature/xbox-uwp` and checked out with
  `read-tree -u --reset`. The index holds the merged tree, so there are NO unmerged index stages.
  Conflicts exist only as zdiff3 markers in the files (`<<<<<<< HEAD`, `||||||| 218fd403c8e`,
  `=======`, `>>>>>>> <sha>`).
- **HEAD side ("ours")** = bobtista's current `bobtista/topic/trunk` (ab40ada509a, bgfx/SDL3/OpenAL,
  maintained, heading upstream) plus our already-merged `ci/wasm-clean` (Emscripten) and three
  `fix/*` bgfx fixes.
- **Theirs** = `origin/feature/xbox-uwp`: the user's own port work for Android, iOS, macOS, Nintendo Switch,
  Win64/OpenAL, an early wasm, and Xbox One UWP (last 3 commits). It was built on an OLD April
  bobtista trunk (218fd403c8e = merge base). The user's commits are 5ad07d05a40..06cf302294f.
  Their commit messages ("slop", "do not use") are JOKES. This is real, working port code; never
  judge it by the message.
- Goal: ONE branch where every target builds: Win32 VC6, Win32 MSVC (DX8 and bgfx), Win64,
  MinGW, Linux, macOS, iOS, Android, Switch, wasm, and Xbox UWP. Nothing from either side may be lost.

## Useful commands (read-only git only)
- Theirs' own change to a file: `git diff 218fd403c8e origin/feature/xbox-uwp -- <path>`
- Trunk's change since that base: `git diff 218fd403c8e ab40ada509a -- <path>`
- Find where trunk moved a file: `git log --diff-filter=R --name-status --format=%h 218fd403c8e..ab40ada509a -- '*<name>'`
  or `git ls-files '*<name>'`
- Who or what introduced a line: `git log -S'<text>' --oneline 218fd403c8e..origin/feature/xbox-uwp`

## Decision rules (in order)
1. **Keep both sides' functionality.** Usually the right answer is trunk's code plus our platform
   additions (extra `#if defined(__ANDROID__)`, `__SWITCH__`, `TARGET_OS_IPHONE`, `GGC_PLATFORM_UWP`,
   `__EMSCRIPTEN__`, `_WIN64` branches, CMake platform blocks, presets).
2. **Duplicate fixes:** if both sides fixed the same problem differently, prefer trunk's version (ties
   go to trunk, since it's maintained and shrinks future conflicts). Keep ours only if it covers a
   platform trunk's doesn't (then merge the platform bit into trunk's version), or if trunk's is
   clearly broken for one of our targets.
3. **All platforms:** a fix must not break another target. In particular, **Windows behaviour must
   stay byte-identical** where possible: put platform changes behind platform macros.
4. **VC6:** anything compiled in the Win32 DX8 build must stay VC6-compatible: no `auto`, `nullptr`
   is OK (macro), `override` is OK (macro), no `= default/= delete`, no `using X = Y`, no `<cstdint>`
   or `<stdint.h>` (use `Utility/stdint_adapter.h` / existing types), no lambdas, no `enum class`,
   no range-for, no `std::unique_ptr`. Code inside non-Windows or bgfx-only guards may be modern C++.
5. **No invented null-guards** or speculative defensive code. Port what the sides actually did.
6. Keep the surrounding code's style. `TheSuperHackers @tag author dd/mm/yyyy` comments from either
   side are kept as they are.
7. If a hunk is genuinely ambiguous (you can't tell which behaviour is right without running it),
   pick the trunk behaviour plus the platform guard, and **flag it** in your log as NEEDS-REVIEW.

## Modify/delete conflicts (trunk deleted or moved the file, theirs edited it)
The old-path file currently sits in the tree with THEIR full version. Do this:
- Find where trunk moved the code (e.g. `WW3D2/BgfxBackend*.cpp` and `DX8Backend.cpp` are now in
  `WW3D2/Backend/`; `WWLib/DbgHelpLoader.*` moved to `Dependencies/DbgHelp/`; check the others).
- Port THEIR change (the diff from 218fd403c8e to xbox for that path) onto the new location by hand.
- Then delete the old-path file from the working tree (plain file delete, not `git rm`).
- If trunk deleted it outright (no new home, e.g. the feature was removed), check why. If theirs'
  change is still needed somewhere, port it to the new place. Otherwise drop it and log it.

## Hard rules
- Only edit files in YOUR group list, plus the new-location files for your group's moves.
  Other agents are editing other files in this same worktree at the same time.
- **Do not run git commands that write**: no `add`, `rm`, `checkout`, `restore`, `stash`, `reset`,
  `commit`, `merge`. Read-only git (diff/log/show/grep/ls-files) is fine.
- Do not build. Builds happen after all groups are done.
- When done, no file in your group may contain `<<<<<<<`, `|||||||`, `=======` (as a conflict line), or `>>>>>>>`.
- Preserve each file's existing line endings.

## Log (required)
Write `<notes-dir>/log-<GROUP>.md`: a table with one row per non-trivial hunk:
`file | hunk (line or function) | resolution (ours/theirs/combined/ported) | rule # | NEEDS-REVIEW?`.
Trivial "identical apart from whitespace or comments" hunks can be summarised in one line. End with
a short list of anything a builder should watch out for (new files that need adding to CMake,
renamed symbols, and so on).
