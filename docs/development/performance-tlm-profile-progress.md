# TLM profiling and candidate progress

Status: **running**.

| Gate | Status | Detail |
|---|---|---|
| contract | passed |  |
| profile | passed | 3/3 diagnostic profiles |
| candidate_choice | pending |  |
| correctness | pending |  |
| measurement | pending |  |
| decision | pending |  |
| docs_commit | pending |  |

Fixed scope: three diagnostic profiles of the existing 20,000-transfer workload.
At most one candidate; three alternating feasibility pairs. Confirmation runs only
if reduction is at least 5% with 3/3 wins, then ten pairs require at least 5%
and 10/10 wins. No resampling or second candidate. Failed gates stop measurement.
Every child has a 60-second deadline. Each execution gate has a fixed outer timeout.
Profile timings are separate from uninstrumented candidate measurements.
Python 3.14; VCD and coverage off. No broader audit or new workload.

Machine tracker: `../_static/performance-tlm-profile-progress.json`.
