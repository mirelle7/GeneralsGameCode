# Build portal

A static website that lists every build of Generals / Zero Hour, in both supported
compilers (VC6 SP6 and modern MSVC) and with the major CMake switches, plus a
configurator that tells you whether a given combination is valid and how to build it.

```
matrix.yaml            single source of truth: games, compilers, configs, switches, rules, tiers
scripts/portal.py      generate | resolve | publish
scripts/package_builds.py   zips CI artifacts and writes per-build manifests
scripts/test_portal.py      rule tests, CMake-name checks, JS/Python parity
site/                  index.html, app.js, style.css (no build step, no dependencies)
```

## Local preview

```sh
pip install pyyaml
python3 build-portal/scripts/test_portal.py
python3 build-portal/scripts/portal.py generate
python3 -m http.server -d build-portal/site 8000     # http://localhost:8000
```

## How it works

* A build is identified by `<game>-<compiler>-<config>[-<hash>]`. The hash covers only
  switches that differ from their default, so ids stay stable when switches are added.
* **Tier 1** (CI presets, both games) and **tier 2** (curated variants) are expanded by
  `portal.py generate` into a CI matrix. Anything else is "on request": the site links to a
  prefilled GitHub issue with the exact `cmake` command.
* `.github/workflows/portal.yml` builds the matrix through `reusable-build-toolchain.yml`
  (new optional inputs `cmake_args`, `artifact_suffix`), packages the results and deploys
  the site to GitHub Pages.
* `app.js` re-implements `resolve()` from `portal.py`; the test suite checks they agree.

## Editing the matrix

Add a switch to `switches:` (its `cmake` name must exist under `cmake/config-*.cmake`; a test
checks that), express dependencies with `when:`, and cross-switch constraints with `rules:`.
Add a curated variant under `tier2:`. `generate` fails if any planned build is invalid.

## Limits and caveats

* Retail CRC compatibility is only claimed for VC6 with `retail_compat` not `OFF`.
* Only game binaries are published, never game data. GitHub Pages has a ~1 GB size limit;
  move `site/builds/` to object storage when the tier 2 set outgrows it.
* The Pages actions in `portal.yml` are referenced by version tag, not commit SHA.
* The workflow has not been run on GitHub yet. Windows build steps reuse the existing, proven
  reusable workflow, but the packaging and Pages publish steps are untested.
