# End-to-end performance progress

Status: **complete**.

Measured pairs: **30/30**.

| Gate | Status | Detail |
|---|---|---|
| contract | passed |  |
| equivalence | passed | Baseline/current and native reference behavior match; source imports guarded; native library identical. |
| import | passed | 10/10 pairs |
| event_signal | passed | 10/10 pairs |
| tlm_transport | passed | 10/10 pairs |
| report | passed | docs/_static/performance-end-to-end.json |
| validation_commit | passed | Independent source/log/behavior/statistics acceptance, strict Sphinx, lint and diff checks passed; final results and tracker committed together. |

Baseline: `291d569cc6bf396bed2f89c019adb9452d2b9580`. Current: `fc53a971a6d6db0c01db5986affa4ae1d8953212`.

Fixed scope: import, event/signal model, annotated TLM transport; ten alternating pairs each.
20,000 cycles/transfers. VCD, coverage and profiling off. Same Python/native dependencies.
60 seconds per subprocess; 30-minute round deadline. A failed gate stops the round.
No extra workloads, resampling, optimization search or percentage summation.

Machine-readable progress: `../_static/performance-end-to-end-progress.json`.
Results: `../_static/performance-end-to-end.json` (published after measurement).
