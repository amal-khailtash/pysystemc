# Tracking, correctness and example batch

Status: **complete**.

Portability is dropped from this batch. Python 3.14; 3.15 deferred.

| Item | Status | Acceptance |
|---|---|---|
| contract | passed | Three named jobs, fixed acceptance and stopping point. |
| tracking | passed | 10/10 pairs reconciled; on overhead 5.36%; no resampling. |
| correctness | passed | 3/3 widths match native values/events/times/deltas; retained callback ownership. Commit 648cabe. |
| examples | passed | 3/3 packet seeds agree; 16 installed-harness checks passed; 43 selectable installed cases. Commit 04c5d55. |
| final_closeout | passed | Root acceptance and independent measurement reconciliation; strict docs/lint/native-format passed; grouped commits complete this batch. |

Fixed scope: ten tracking on/off pairs with no resampling; three wide-signal
widths (65, 137, 257); three packet-switch seeds (1, 7, 23). Each simulation
uses a fresh process with a 60-second deadline. Root accepts worker results
through recorded validation. Commit each completed group and stop after all
three results and final documentation checks. No new optimization candidate.
