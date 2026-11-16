# End-to-end performance progress

Status: **running**.

Measured pairs: **0/30**.

| Gate | Status | Detail |
|---|---|---|
| contract | pending |  |
| equivalence | pending |  |
| import | pending |  |
| event_signal | pending |  |
| tlm_transport | pending |  |
| report | pending |  |
| validation_commit | pending |  |

Baseline: `291d569cc6bf396bed2f89c019adb9452d2b9580`. Current: `fc53a971a6d6db0c01db5986affa4ae1d8953212`.

Fixed scope: import, event/signal model, annotated TLM transport; ten alternating pairs each.
20,000 cycles/transfers. VCD, coverage and profiling off. Same Python/native dependencies.
60 seconds per subprocess; 30-minute round deadline. A failed gate stops the round.
No extra workloads, resampling, optimization search or percentage summation.

Machine-readable progress: `../_static/performance-end-to-end-progress.json`.
Results: `../_static/performance-end-to-end.json` (published after measurement).
