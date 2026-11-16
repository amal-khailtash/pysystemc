# Tracking, correctness and example batch

Status: **running**.

Portability is dropped from this batch. Python 3.14; 3.15 deferred.

| Item | Status | Acceptance |
|---|---|---|
| contract | passed | Three named jobs, fixed acceptance and stopping point. |
| tracking | pending | 10 alternating actual recorder on/off pairs; whole-process and original loop statistics, hashes/behavior reconciled. |
| correctness | pending | Native/Python wide-signal contracts at 65/137/257 bits; values/events/times/lifetime. |
| examples | pending | Existing packet-switch port expanded to seeds1/7/23; complete native send/receive/drop comparison. |
| final_closeout | pending | Root acceptance, docs/lint/diff and grouped commits; no pending batch item. |

Fixed scope: ten tracking on/off pairs with no resampling; three wide-signal
widths (65, 137, 257); three packet-switch seeds (1, 7, 23). Each simulation
uses a fresh process with a 60-second deadline. Root accepts worker results
through recorded validation. Commit each completed group and stop after all
three results and final documentation checks. No new optimization candidate.
