# Callback boundary candidate progress

Status: **complete**.

| Gate | Status | Detail |
|---|---|---|
| contract | passed | One candidate, fixed original TLM loop; no retries or additional workload. |
| cost_classification | passed | Reuse three published diagnostic profiles; callback body self time includes cppyy native calls; result wrapper is distinct ~8ms. |
| candidate_choice | passed | Freeze void-return validation branch at registration, retaining *args, None enforcement, callable ownership, and native bridge. |
| correctness | passed | 2 void-wrapper/native result checks and 4 Python/native TLM models agree. Six lifecycle/error/duplicate models passed. |
| measurement | passed | screen 3/3, 1.59% reduction, 2/3 wins; fixed measurement finished |
| decision | passed | Rejected: 1.59% median reduction, 2/3 wins; no confirmation or runtime change. |
| docs_commit | passed | Root/Luna evidence reconciliation, strict Sphinx and lint passed; report and seven-gate closure grouped in result commit. |

Fixed scope: callback cost review of the existing 20,000-transfer workload.
At most one candidate; three alternating feasibility pairs. Confirmation runs only
if reduction is at least 5% with 3/3 wins, then ten pairs require at least 5%
and 10/10 wins. No resampling or second candidate. Failed gates stop measurement.
Every child has a 60-second deadline. Each execution gate has a fixed outer timeout.
Profile timings are separate from uninstrumented candidate measurements.
Python 3.14; VCD and coverage off. No broader audit or new workload.

Machine tracker: `../_static/performance-callback-progress.json`.
