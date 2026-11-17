# Minor release progress

Status: **complete**. Released: **1.1.0**; account: **amal-khailtash**.

Progress: **7/7 gates passed**. [GitHub release](https://github.com/amal-khailtash/pysystemc/releases/tag/v1.1.0) · [PyPI](https://pypi.org/project/pysystemc/1.1.0/) · [Released documentation](https://pysystemc.readthedocs.io/en/v1.1.0/)

| Gate | Status | Detail |
|---|---|---|
| dependencies | passed | uv lock --upgrade and uv sync --locked: 13 packages updated; Python 3.14 retained. Commit 1caaca5. |
| seven_misses | passed | All seven former misses executed and reconciled; explicit helper/reflection/backend robustness scope retained. |
| full_suite_coverage | passed | Local 1,003/1,003 passed; 4594/4594 statements covered with zero exclusions. Feature and released CI each passed all 1,003; 4593/4594 covered, with bundled fmt selection independently exercised in installed-wheel mode. All seven original robustness misses covered. |
| fresh_distributions | passed | Final 1.1.0 wheel/sdist built by trusted Publish; both exact PyPI hashes verified; 113 runtime sources match checkout/wheel/sdist/install. Historical 1.0.0 candidate evidence retained. |
| installed_examples | passed | Final 1.1.0 wheel: 43/43 native comparators passed; exact selected nodes, source isolation and 113/113 source parity verified; Python 3.14.0. |
| feature_and_release_prs | passed | PR #24 merged ff2ec227 after all feature checks; generated 1.1.0 metadata PR #25 merged 005afb6 after exact four-file and uv lock checks. |
| publication | passed | Publish 44002446538 succeeded: exact PyPI wheel/sdist hashes, non-draft GitHub release, released 1,003-test suite/Codecov upload and completed RTD tag build 35945538 verified. |

Python 3.14 remains the baseline; Python 3.15 is deferred. The seven explicit
robustness fixtures retain their distinction from broader supported native
configurations. Local statement coverage is 4,594/4,594; feature and released
CI coverage is 4,593/4,594. The sole CI miss selects bundled fmt headers, a path
exercised by the isolated installed-wheel checks with `FMT_HOME` unset.

The final 1.1.0 wheel passed all 43 selected native comparators. Read the Docs
build 35945538 finished successfully at tag commit `005afb6`; a queued build
alone was not accepted. Exact archive hashes and retained failed-run evidence
are recorded in `docs/_static/minor-release-publication.json`. Historical candidate
and finite audit/performance records remain preserved.

No further item is queued in this release checklist.
