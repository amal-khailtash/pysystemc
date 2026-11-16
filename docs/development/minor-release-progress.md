# Minor release progress

Status: **running**. Target: **1.1.0**; account: **amal-khailtash**.

| Gate | Status | Detail |
|---|---|---|
| dependencies | passed | uv lock --upgrade and uv sync --locked: 13 packages updated; Python 3.14 retained. Commit 1caaca5. |
| seven_misses | targeted_pass | Seven explicit defensive/helper/reflection/backend statements; targeted fresh-process test passed. Commit 26d7137; full coverage confirmation pending. |
| full_suite_coverage | running | Full current suite with four workers and subprocess coverage; frozen 113 runtime source hashes. |
| fresh_distributions | passed | Fresh uv_build wheel/sdist built; 113/113 tracked runtime files match checkout and both archives. Installed parity remains in installed_examples. |
| installed_examples | running | All 43 selected installed-wheel comparators in isolation; Python/native behavior and installed source parity required. |
| feature_and_release_prs | pending | Existing workflow; minor 1.1.0, matching metadata and changelog. |
| publication | pending | Trusted PyPI publication, hash verification, final GitHub release and documentation. |

Python 3.14 baseline; 3.15 deferred. Upgrade dependencies with existing uv settings.
Coverage robustness fixtures are classified separately from native support.
Run the entire current suite and coverage, build fresh distributions, verify all
43 installed-example cases, then merge validated feature and release metadata
PRs through the existing Release Please/Publish workflow. Verify PyPI artifacts
and final GitHub release. Commit groups and preserve failed-run evidence.
