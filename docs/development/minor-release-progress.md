# Minor release progress

Status: **running**. Target: **1.1.0**; account: **amal-khailtash**.

Progress: **5/7 gates passed**. Five local validation gates, then PR/release metadata and publication.

| Gate | Status | Detail |
|---|---|---|
| dependencies | passed | uv lock --upgrade and uv sync --locked: 13 packages updated; Python 3.14 retained. Commit 1caaca5. |
| seven_misses | passed | All seven former misses executed and reconciled; explicit helper/reflection/backend robustness scope retained. |
| full_suite_coverage | passed | 1,003/1,003 passed; no failures/errors/skips. 4594/4594 Python statements covered; zero exclusions or misses; 113 frozen runtime hashes unchanged. |
| fresh_distributions | passed | Fresh uv_build candidate wheel/sdist built with current 1.0.0 metadata; 113/113 tracked runtime files match both archives. Final 1.1.0 artifacts will be built by Publish. |
| installed_examples | passed | 43/43 selected comparators; no failures/errors/skips; exact node set, Python/native equivalence and 113/113 installed runtime hashes accepted. Wheel pthread/resource check passed. |
| feature_and_release_prs | pending | Feature feat: PR, required CI and Release Please metadata PR for minor 1.1.0; matching versions and changelog. |
| publication | pending | Trusted PyPI publication, hash verification, final GitHub release and documentation. |

Python 3.14 baseline; 3.15 deferred. Upgrade dependencies with existing uv settings.
Coverage robustness fixtures are classified separately from native support.
Run the entire current suite and coverage, build fresh distributions, verify all
43 installed-example cases, then merge validated feature and release metadata
PRs through the existing Release Please/Publish workflow. Verify PyPI artifacts
and final GitHub release. Commit groups and preserve failed-run evidence.
