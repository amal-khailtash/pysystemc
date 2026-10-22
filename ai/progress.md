# Implementation progress

Updated: 2026-10-22. Baseline: 0.14.0 (fe118f0).

Update after every feature batch: reconcile inventory statuses, recalculate counts and percentages, record validation, and revise remaining work.

## Public declaration coverage

Source: [public API inventory](../docs/_static/public-api-inventory.csv). Items are unique declarations including overloads. Untested does not imply unavailable.

| Namespace | Items | Supported | Complete % | Failing | Untested | Remaining | Remaining % |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| sc_core | 2,752 | 420 | 15.26% | 0 | 2,332 | 2,332 | 84.74% |
| sc_dt | 9,165 | 0 | 0.00% | 5 | 9,160 | 9,165 | 100.00% |
| tlm | 535 | 0 | 0.00% | 0 | 535 | 535 | 100.00% |
| tlm_utils | 251 | 0 | 0.00% | 0 | 251 | 251 | 100.00% |
| Total | 12,705 | 420 | 3.31% | 5 | 12,280 | 12,285 | 96.69% |

## Other progress measures

| Measure | Total | Complete / reviewed | Percentage | Remaining | Remaining % |
| --- | ---: | ---: | ---: | ---: | ---: |
| Public families | 794 | 299 | 37.66% | 495 | 62.34% |
| Declaration mappings | 12,705 | 1,320 reviewed | 10.39% | 11,385 partial | 89.61% |
| Upstream example baseline targets | 33 | 33 | 100.00% | 0 baseline targets | 0.00% |

Family counts use the fixed list in [features.rst](../docs/intro/features.rst): 793 regular rows plus the partial frozen sc_name_gen row. Mapping review is not runtime support. Example coverage applies only to selected baselines; alternate inputs, platforms and configurations remain untested.

## Groups and remaining work

| Group | Status | Evidence / remaining work |
| --- | --- | --- |
| Interface contracts B1–B4/C1 | Complete for recorded contracts | Native/Python identity, type, callback, reset/kill, error and teardown matrices. |
| sc_report | Complete: 25/25 | Native values, copy/assignment and legacy controls. |
| sc_report_handler | Complete: 32/32 | Configuration, borrowed handler tokens, cache/log/release and independent default actions including abort. |
| sc_attr_base | Complete: 5/5 | Native construction, copy and destruction. |
| sc_attr_cltn | Complete: 17/17 | Lookup, iterators, aliases, shallow copies and borrowed ownership. |
| sc_attribute | Complete: 7/7 | Generic template values, copy independence and lifecycle. |
| Other sc_core families | Partial / untested | Event-list constructors/expressions validated; busy mutation/swap errors remain. Finish vector, event, process and channel scopes. |
| sc_dt families | 5 failing; other declarations untested | Repair known failures; validate complete datatype overload/type/width scopes. |
| TLM sockets and utilities | Partial implementation; declaration inventory untested | Finish socket, timing, payload, extension and endian declaration/lifetime contracts. Representative interface tests do not close every overload. |
| Upstream example variants | Untested beyond baseline | Alternate stimuli, inputs, platforms and build configurations. |
| Complete public API gates | Open | Complete mapping/runtime validation and final installed distribution/native equivalence checks. |
| Aggregate documentation reconciliation | Open | CSV has 12,705 entries including four added sc_name_gen declarations; older prose still cites 12,701. |

## Feature batch history

| Date | Batch | Validation | Coverage change |
| --- | --- | --- | --- |
| 2026-10-22 | Initial 0.14.0 tracker | Release PR checks passed; trace/TLM corrections validated. Tutorial harness: 9 passed; direct workspace hello test: 1 passed. | Baseline: 404 supported declarations (3.18%); 299 complete families (37.66%). |

| 2026-10-22 | Event-list expression constructors | Final native/Python list and pthread timing integration: 2 passed (28.56s). Strict Sphinx, lint and formatting passed. | +16 supported declarations; total 420 (3.31%). Two list families remain partial pending busy mutation/swap error paths. |

Do not count editor configuration changes as public API implementations. Keep failing, untested and deferred scopes explicit.
