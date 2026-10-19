# Build brief: get one target of the `platforms` branch building

## Context
`platforms` (<worktrees>/platforms) = bobtista's bgfx/SDL3/OpenAL trunk (ab40ada509a) merged with
the user's port branches: wasm-clean, three bgfx fixes, and feature/xbox-uwp (Android, iOS, macOS,
Switch, Win64, Xbox UWP). Merge commits: 9463fd1de33 (wasm), 7c93d8d491a (xbox/mobile, 295
hand-resolved hunks), then cleanup 4eb02f31b80. It has NEVER been built. The conflict-resolution
logs, including NEEDS-REVIEW items and "builders must know" notes, are in
<notes-dir>/log-*.md. Read the ones relevant to your target first.

You have your own worktree `<worktrees>/pf-<target>` on branch `pf/<target>` (starts at
`platforms`). Six other agents are building other targets in their own worktrees at the same
time, so this machine is shared: **use at most -j8** for compiles.

## Goal
1. Configure and build your target(s) until they compile and link (both games where the target
   supports Generals; otherwise GeneralsMD/Zero Hour `z_generals`).
2. Where it is feasible on this machine, run it far enough to show it works (instructions per
   target in your prompt). Report honestly what you ran and what you did not.

## How to fix things
- Commit each fix as its own small commit on `pf/<target>`, with a conventional message:
  `bugfix(area): ...` or `build(area): ...` (imperative, sentence case, short body saying why).
  **Never add a Co-Authored-By trailer.** Never push to `origin` or any GitHub remote.
- Fix the real cause; prefer restoring what one of the merged sides had over inventing new code.
  `git diff 218fd403c8e origin/feature/xbox-uwp -- <path>` shows the user's original port change
  to a file; `ab40ada509a` is pure trunk. If the merge dropped something your platform needs,
  restore it behind a platform guard.
- **Windows must stay byte-identical in behaviour** and the Win32 DX8 build VC6-compatible: no
  `auto`, `= default/delete`, `using X = Y`, `<cstdint>`/`<stdint.h>`, lambdas, range-for,
  `enum class` in code that VC6 compiles. Platform fixes go behind platform macros.
- No invented null-guards or speculative defensive code.
- If a fix needs a change to shared code that another target probably also needs, still make it,
  but list it under "shared-code commits" in your report so the merge-back is easy.
- Commit messages and code comments must not mention AI or Claude.
- Delete only build directories you create yourself. Ask (in your report) before removing anything else.

## Report (your final reply)
- Per target: configure OK?, build OK?, ran? (what exactly), with the exact commands that worked.
- List of your commits (`git log --oneline platforms..pf/<target>`), marking the shared-code ones.
- Anything still broken, with the first real error, and NEEDS-REVIEW items you hit.
- Write the same report to <notes-dir>/build-<target>.md.
