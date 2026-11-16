# TLM profiling and candidate progress

Status: **complete**.

| Gate | Status | Detail |
|---|---|---|
| contract | passed |  |
| profile | passed | 3/3 diagnostic profiles |
| candidate_choice | passed | 40,000 ordinary sc_core module reads dominate ~24 ms self time per diagnostic; guarded base-slot lookup selected. Time constructor retry excluded. |
| correctness | passed | 2 module/native flag checks and 4 Python/native TLM models agree; actual MRO status retained. |
| measurement | passed | screen 3/3, 1.37% reduction, 3/3 wins; fixed measurement finished |
| decision | passed | Rejected: 1.37% median reduction below 5%; no confirmation or runtime change. |
| docs_commit | passed | Root/Luna reconciliation, strict Sphinx and lint passed; report and closure tracker grouped in result commit. |

Fixed scope: three diagnostic profiles of the existing 20,000-transfer workload.
At most one candidate; three alternating feasibility pairs. Confirmation runs only
if reduction is at least 5% with 3/3 wins, then ten pairs require at least 5%
and 10/10 wins. No resampling or second candidate. Failed gates stop measurement.
Every child has a 60-second deadline. Each execution gate has a fixed outer timeout.
Profile timings are separate from uninstrumented candidate measurements.
Python 3.14; VCD and coverage off. No broader audit or new workload.

Machine tracker: `../_static/performance-tlm-profile-progress.json`.
