# Changelog

All notable changes to Planex are documented here.

Format based on [Keep a Changelog](https://keepachangelog.com/).

---

## [Unreleased]

### Added — ADR-0028 stage R3: the presentation client — the tree rendered from outside, addresses named and persisted, and the E-series commissioning kit registered ready-not-run

- **ADR-0028 registered before any R3 source existed** (718 lines, four-column compliant, F1–F12 fixed as pending): the client-zone decision (v5/client/ sits OUTSIDE the visual-vocabulary ban's scan paths BY CONSTRUCTION, not by exemption), the name column, the snapshot format and its "verbatim install + one rebuild pass, never re-adjudicate" load discipline, the E-kit's ready-not-run boundary, and the promotion gate untouched (ADR-0026 §5's clauses carry unchanged).
- **The presentation client** (`v5/client/`): the S15 "vision is one projection" clause executed as a CLIENT — `if5c_fb` (an RGBA surface with deterministic primitives, the derived 5x7 font, and a canonical timestamp-free 24-bit BMP writer), `if5c_layout` (the tree at a view assigned geometry: depth is the row, the pre-order walk is the column, children keep declaration order; the S17 surprise timetable renders as the attention margin, one kind-glyphed severity-graded marker per entry), `if5c_hit` (the inverse of the layout — ADR-0027 2f's "client-side, forever" deferral, discharged), `if5c_ease` (the two-sample trajectory as a PURE t-mapping over exact integer rationals — no floating point anywhere in the client, no wall clock, ever), and `if5c_annotate` (ADR-0027 2g's cancel-reason successor question discharged WITHOUT a record field: the reason is presentation, keyed by the cut registry's id). The five presentation relation kinds (BESIDE et al.) are NOT carried — not even as client metadata: the layout derives structurally from the tree, and asserted layout edges stay gone (the stage's sharpest not-carried claim, with its trigger registered).
- **The named-address registry** (`if5_cut_register_named` / `if5_cut_name` / `if5_cut_by_name`): the working-set column the "64 named cuts" language always implied, symmetric with region names — unique on register (a duplicate name is refused honestly), bounded by IF5_NAME_MAX, answered "" when unnamed. The world grows by exactly IF5_MAX_CUTS x IF5_NAME_MAX = 768 bytes: the differential's working-set delta assertion moves from 12,568 to 13,336 with the name column as the registered reason (F10; history bytes and the 48 B/change slope unchanged).
- **The snapshot** (`v5/src/if5_snapshot.c`): a NEW canonical, versioned, little-endian byte format — seeds (source identities + budget configuration, the payload-type table as NAMES with slot-by-slot verification on load, region names/config, sys_sid), the durable truth VERBATIM (the 48-byte log words and the arena), the audit stream (receipts), the address book (cuts with names, their cached frontiers re-derived and cross-checked on load), and the cursor. The black-box export format — a registered surface — is NOT touched (F7 asserts its bytes across a round trip). Load = verbatim install + `if5__rebuild_caches`: ONE ordered pass that replays the BOOKKEEPING (knowledge clocks, ledger arithmetic, region caches, fork heads, path counters) reading each record's own flags/PROV/vc — the decisions — and writing no decision byte. Every rebuildable cache is cross-checked against the persisted values; a divergence is a REFUSED load, never a wrong world. Five refusal classes tested: non-fresh world, bad magic, truncation, trailing bytes, type-table drift.
- **Implementation-era findings, all caught by the F-series before landing and registered**: (1) the tier-3 ARBIT witness carries the NEWCOMER's path counters (the old head keeps the counts its own submission earned — the first rebuild draft double-counted the newcomer as causal, and the stats cross-check refused the load); (2) a tier-1 fork does NOT touch the working-set value cache (a forked region's cache stays at the causal fold until the fork resolves — frozen slice semantics, replicated, not repaired); (3) the tier-2 system debit is log-invisible as a debit word but the CONCUR_MERGE flag is its witness (a merge only happens if the debit succeeded); (4) standalone `if5_debit` successes post NO record at all — the spent totals persist authoritatively and the loader refuses only when the log-visible portion exceeds them (the honest boundary, registered); (5) the schedule marks' first-draft geometry was a diagonal staircase (the margin's x was read inside the placement loop) — the golden caught it.
- **The E-series commissioning kit** (`measurements/e-series/RUNBOOK.md` + `scripts/check_e_series_runbook.sh`): nine leg entries (E1a, E1b, E2-experimental, E3a, E3b, E3c, E4, E5a, E5b), each with the five fields a commissioner needs, all derived from the registered sources and nothing invented; the account — zero runs, zero participants, zero data — is ENFORCED against the repository state at every commit (F12's gate: no data directory, no CSVs, the status line must say READY — NOT RUN).
- **The calculator demo** (`v5/client/apps/if5c_calc.c`): the shipping tree's `calculator_denotative.c` validation re-derived on the ontology — a 22-region state machine (registers + a keypad subtree), buttons driven through the client's own hit test (the demo's ENTIRE input path is `if5c_hit`), the display as a fold, and one undo re-rendering the world as it stood before '=' (N2's addressable time, drawn). Two committed goldens (session + undo frames, 24-bit BMPs by content, .golden by extension so the transient-screenshot ignore rule does not eat the fixtures).
- **Gates extended with their own bites** (`scripts/check_slice_rebound.sh`): the DEPENDENCY-DIRECTION ban (v5/src + v5/include may not reference any if5c_ symbol — the core never links its client) and the CLIENT PURITY ban (v5/client/src/if5c_*.c may not submit/append/debit/undo/adopt — the client renders, it does not speak), each with a planted synthetic violation the gate must catch.
- **Test suites** (`v5/tests/test_v5_snapshot.c`, `v5/client/tests/test_v5c_{render,hit,persist}.c`): F1–F12 across the board — client purity (durable-image memcmp + zero submits), renderer determinism (byte-identical renders, across processes and across save/load), layout totality (one cell per region, the layout tree IS the semantic tree), the hit inversion (200 randomized trees, 15,280 points, against a brute-force containment scan), easing endpoints and per-channel monotonicity under both families, the named-address round trip, snapshot fidelity over the FULL op alphabet (merges, forks, arbitrations, delegation, a breaker, undo, an explicit stitch, an export, a reparent, process commits and a cancel — the reloaded world also CONTINUES identically: one more submission on each core, byte-compared), format canonicity (save-twice and re-save fixpoints), and the rebuild's exactness (svc, ledgers, region caches, heads, stats, tick — white-box, the differential's precedent). Repo total 121 -> 136 sections; ctest 21 -> 27.
- **F1–F12 all PASS; the R1 differential re-run PASS** (TRIP-17 + 100 seeded streams + the E2 shape, byte-identical; residency 1.661x at the E2 shape against the registered 3.0 boundary; working-set delta 13,336 exactly). ASAN+UBSan clean on all suites; the new sources compile clean under -Wconversion -Wsign-conversion -Wshadow.

### Added — ADR-0027 stage R2: the mechanism layer re-derived — seven abstractions as projections, queries, and judgments, and the two tracked Known issues discharged

- **ADR-0027 registered before any R2 source existed** (671 lines, four-column compliant, M1–M9 fixed in the Validation section as pending): the seven mapping arguments (each with its NOT-CARRIED column as contract), the differential obligations, and the refutation criteria. Inputs cited from the code survey: estimate.c 737 lines, relation.c 218, closure.c 440, perception.c 460, feedback.c 512, hit.c's compile line, interaction.c 401.
- **Estimate as a fold** (`v5/src/if5_estimate.c`): the estimate is a QUERY at an address — value/contributions/anchor read off the fold, an EVIDENCE ordinal (none/forked/merged/chain — an ordinal, never a probability; the header and the ADR's CAVEATS both say so), the trajectory as the fold at every integer cut (the shipping animation is the two-sample special case), and the dependency set made explicit (the Stage-3 derived-estimate machinery — subscriptions, cycle detection, propagation-depth accounting — is GONE: dependency is structural here).
- **Relation as the cause-graph** (`v5/src/if5_cause.c`): DEPENDS_ON is the PROV edge, TRIGGERS the causal-successor query, VARIES_WITH the concurrency enumeration, CONTAINS the folded containment — and five kinds (BESIDE/AFFORDS/WITHDRAWS_FOR/PRESENTS_FOR/INTERPRETS_AS) are NOT CARRIED, on the record: layout and presentation assertions with no causal semantics.
- **Closure as a judgment** (`v5/src/if5_judge.c`): the five execution-side stages as a READ of the adjudicated record — goal = the semantic address, intent = the value word, adjudication = the verdict (six values, registered precedence), observation = the record's own causal coordinate (the scalar observed prefix is NOT recoverable — the record word stores no declared prefix; the struct carries the coordinate and does not fake the prefix), evaluation = the fold at the view. The Action callback is replaced by the type-registered semilattice join — the axiom table's own content decision, stated as such.
- **Perception as the projection family** (`v5/src/if5_projection.c`): a WORLD-scoped registry (one of the shipping tree's three process-global registries, world-scoped and gone), with the three frozen R1 projections registered through the SAME public channel at world creation and WRAPPED, not modified — M4 pins the registry path byte-identical to the direct call. The tree and schedule gain text renderings through the same channel (B2's equality of standing, applied a second and third time: payloads, projections, routes).
- **px_loop's budget as N4's ledger** (no new module — an argument + M7): the budget moves from the loop object to the ledger and from after-the-fact counting to REFUSAL; the breaker receipt IS the overrun record, itemized and PROV-anchored. The loop itself needs no object: it is the application's own cycle over submit + projection, with the world as the single coordination point.
- **Intent compilation as routing** (`v5/src/if5_route.c`): a compile function per registered payload type, routed through the open registry; built-in NUM/SET/STR routes register through the same public channel. The sugar property is contract (M6): the compiled record is byte-equal to the direct submission of the same value — the NULL default is omniscient, the same convention if5_user_act carries, so the routed record never silently diverges.
- **px_interaction as the adjudication surface** (`v5/src/if5_process.c`): begin opens the process IN THE LOG (an IF5_T_PROCESS record through the ordinary channel — identity is an iid, the opening is adjudicated, undoable, provable); samples are pure working-set (zero submissions — now STATICALLY enforced: the rebound gate bans submit/append/debit calls from if5_process_sample's body, with its own synthetic bite); commit submits through the ordinary channel as a causal successor of the opening (default omniscient; a blind outcome is the caller's explicit choice); cancel IS a cut — the opening is undone and the process leaves causal history the way an undone change does.
- **Known issue 1 discharged — containment becomes content**: IF5_T_REPARENT (region = child, value = destination, excluded from the value fold), the tree folds containment at every view (last live reparent, else the creation seed — the single-create REGION record's aux word, which was zero on every R1 stream so the historical bytes are unchanged), and the working-set parent becomes a cache of the live fold. An undone reparent restores the prior tree (M8). The ptype enum's user-extension start moves 12 -> 14, once, in a tree with no external users.
- **Known issue 2 discharged — the per-source knowledge cut** (`if5_kcut` in `v5/src/if5_cut.c`): record identity, never clock comparison (the REBORN collision cannot bite). Membership is a REACHABILITY test over the PROV graph — O(1) storage per cut, no capacity cliff. Scope honesty registered twice over: it is STRUCTURAL dependency knowledge (PROV-declared: own records + merge parents + cursor targets); the implicit knowledge dependency of observed-cut declarations is not recoverable from the record word (the judgment mapping's discovery), and observational knowledge is out of scope by decision. The single-source degeneration and the frontier recomputation are pinned by M9.
- **Two test suites** (`v5/tests/test_v5_mech.c`, `v5/tests/test_v5_structure.c`): M1–M7 and M8–M9, every assertion checked against an INDEPENDENT recomputation (a second way to compute the same thing — the closed form over the record words, the pairwise re-derivation, the fixpoint membership walk, the in-test R1-order walker). Repo total 108 -> 121 sections; ctest 19 -> 21.
- **Build wiring**: the v5 Makefile grows the six modules and both suites and gains the HEADER-DEPENDENCY fix (a stale if5_merge.o kept writing stats at the pre-ADR-0027 struct offset and the calibration suite caught it — headers are prerequisites, not suggestions; the lesson is registered in the Makefile's own comment). CMakeLists, ci.yml's three job families, and TESTING.md updated.
- **M1–M9 all PASS; the R1 differential re-run PASS** (TRIP-17 + the 100 seeded streams + the E2 shape, byte-identical; v5 residency at the E2 shape 1.681x against the registered 3.0 boundary — the ratio moved from R1's 1.702x because the mechanism layer's world-scoped registries grew the working set by ~1KB while history stayed byte-identical, which is the registered direction of honesty: the delta is asserted exact, never assumed). ASAN+UBSan clean on all seven v5 suites; the six new sources + the two touched cores compile clean under -Wconversion -Wsign-conversion -Wshadow.

### Added — ADR-0026 stage R1: the v5 derivation lands — the promoted core, the cut algebra, three seats, and the differential

- **The promotion** (`v5/src/if5_{core,merge,budget}.c`, `v5/include/planex/if5.h`, `v5/src/if5_internal.h`): the slice's ifl core promoted to library grade as if5_ — the seven registered design fixes carried over with their comments, provenance in every file header, `if5_intent` still exactly 48 bytes (V8). The one record-format delta over the slice, registered openly: the debit word packs the debit CLASS (bits 12–13) alongside the severity rung — no registered observation surface reads the word (verified by the differential), and it is what makes the surprise timetable itemizable from the black box alone.
- **The global cut algebra** (`v5/src/if5_cut.c`) — ADR-0025's registered Phase-4-scale known issue, discharged at prefix+prune granularity: cuts as first-class values (a log prefix bound + pruned subtree roots + the cached living-knowledge frontier), the principal lattice (join refuses pruned inputs — the boundary is registered; meet is total), the prune constructor (the undo address), the subset order, the PUBLIC down-closure verifier (promoted from the slice's test-private checker; its first run caught the REBORN clock collision and now carries the same exemption the fold does), and a 64-entry world-level address registry — the "global-cut addresses beyond region granularity" ADR-0025's tracking clause anticipated for E1b's audit family.
- **S15 seated** (`v5/src/if5_tree.c`): the semantic tree, first class — a total, deterministic, depth-first walk over region containment (creation gains an optional parent; working-set metadata in R1, registered as a known issue), each node carrying name / value / role / tier / contributions / PROV anchor and NO visual field. The no-borrowing clause is statically enforced: the rebound gate's new v5 section bans visual-field vocabulary (rect/glyph/font/color/colour/pixel/rgba) from v5's core, with its own synthetic bite.
- **S17 seated** (`v5/src/if5_schedule.c`): the surprise timetable — a PURE QUERY over records the core already logs (N5's no-new-slots discipline: no new payload family; verified from outside by the seats suite). Surprise debits, breaker receipts, tier moves, arbitration exclusions and merges, each entry PROV-backed, per-source filterable, view-addressable: onboarding, diffs, changelogs and AI explanations are projections of this one object.
- **S24 seated** (the two-layer boundary, in `v5/tests/test_v5_seats.c`): every projection — the three text projections, the tree, the schedule, the cut queries — must leave the durable layer's byte image unchanged; the seats suite snapshots log count / history bytes / working set / stats / receipts / ledgers around repeated projection fire at now, historical and rewound views, and memcmps.
- **The differential** (`v5/tests/test_v5_differential.c`) — the rewrite's master gate: both cores in one binary, driven in lockstep through TRIP-17, 100 seeded random streams x 200 ops, and the E2 shape (1000 regions, 4000 changes, the registered hotspot mix), byte-compared on the registered observation surface. **V1 PASS: zero mismatches anywhere. The registered working-set delta asserted exact: 11,776 bytes.** The differential caught its own driver twice before landing (a double PRNG draw; merge heads re-read after the first core's call mutated the table) — recorded in the ADR: a weaker comparator would have shipped those as "equivalence".
- **V9, residency measured on v5 directly: 1.702x at the n=2000 load floor** (boundary 3.0; the slice measured 2.102x — the ratio is lower for the registered reason: the working set grew by the asserted delta while history stayed byte-identical). Incremental cost 48.0 B/change, inside the registered [40.8, 55.2] band.
- **The suites** (`v5/tests/`): calibration 8/8 (the slice's battery re-derived on if5, down-closure now via the promoted public verifier), property 6/6 (1M groups/round; the planted non-associative join rejected at the door; the counterexample shrinks to the same minimal triple 0/0/3 the slice found), cut 6/6 (4,000 randomized law rounds + 4,000 pruned-cut rounds against an independent membership fold), seats 6/6 (S15 totality/determinism/liveness, S17 PROV-backing/itemization/filtering/view-addressing, S24 purity), differential PASS. **Total suite count 82 -> 108** (TESTING.md updated).
- **Wiring**: `v5/Makefile` (self-contained, CFLAGS_EXTRA passthrough), root CMakeLists (planex_v5_lib + five ctest targets — never linked into planex_lib, the v4/slice firewall), ci.yml (cmake jobs run the v5 suites; make jobs run `make -C v5 test` + the extended rebound gate; windows arrays extended), and `scripts/check_slice_rebound.sh` grew the v5 section + bite.
- **The shipping API, the slice, and the measurements are untouched** — v5 is additive by construction; rollback is tree removal plus wiring reversion (the ADR's Regression Clause). Promotion to a shipping direction stays gated on the E-series human legs' registered demotion clauses.

- **Added**: ADR-0026 (proposed) — the v5 derivation: the full rewrite
  begins as a parallel tree, gated. The engineering gate is adjudicated
  PASSED on ADR-0025's landed numbers (C1-C9; E2 engineering leg
  2.102x, R^2 = 1.000000, 48.0 B/change); the human gate is carried as
  registered vacancy, and promotion to a shipping direction is
  pre-committed to the E-series demotion clauses. Stage R1 registers:
  the ifl-to-if5 library-grade promotion, the global cut algebra
  (discharging ADR-0025's registered Phase-4-scale known issue at
  prefix+prune granularity), the S15/S17/S24 seats, and the
  differential gate against the frozen slice. Registered before any
  v5/ source existed.

### Added — ADR-0025 implementation + measurement: the slice lands, calibrated, with the series' first machine-measured number

- **The core** (`slice/src/ifl_{core,merge,budget}.c`, `slice/include/planex/ifl.h`): sources with vector clocks (Fidge/Mattern), the append-only intent log as the only history structure, per-region runtime tiers (fork / tolerant merge / strict LWW with ARBIT-witnessed fold exclusion), the open payload-type registry (the built-ins register through the same channel as user types; a non-associative join offered to the registry is REJECTED at the door), the attention ledger with the monotone 1/2/4/8 ladder and receipted overdraft, causal slices / state@t / PROV walks, undo as sugar over an explicit three-step projection path (`ifl_view_rewind` + a CURSOR witness + `ifl_view_adopt` — the new public third step, so the manual path is writable and C7's differential demand is honest), black-box windowing and export, and the three projections (semantic / set / history).
- **The scenario and the two arms**: TRIP-17 (`slice/app/scenario.h`, 45 ops — delegation chain depth 2, the four registered beats: fork+stitch, concurrent tier-2 triples, tier-3 LWW, sub-budget overdraft with a hop-2 receipt, undo-branch-rewind, black-box export) shared by the treatment arm (`slice_app`, the ifl_ encoding) and the control arm (`classic_arm`, the frozen shipping body, honestly-optimally encoded: mode machine per handler entry, attribution side-list, undo snapshots, manual version records — every classical cost counted and printed; C9's comparison surface).
- **The calibration layer** (`slice/tests/test_slice_calibration.c`, 8 sections): C3 single-source short-circuit (all four adjudication counters zero — emergent, no consulted special case), C4 zero-budget silence (every agent initiative a receipt; the fold holds only the user's own writes), C5 all-tier-3 degeneration (zero fork residue, LWW-in-window), C7 undo-sugar differential (sugar vs the written-out manual three steps: byte-identical states, projections, and the entire log), I6 provenance completeness (every exclusion ARBIT-witnessed, every merge dual-derived), I7 ladder monotonicity and the one-currency breaker, C9's treatment side, and the joint down-closure checker (the Known-issues mitigation).
- **The property suite** (`slice/tests/test_slice_property.c`, 6 sections, ~4M random groups): the semilattice laws over NUM(max) and SET(union); the turnstile bite (a planted commutative-idempotent-but-non-associative join is rejected at registration); counterexample shrinking (a planted violation shrinks to a minimal triple).
- **The E2 engineering leg** (`slice/app/e2_engineering_leg.c`, self-judging): 1000 objects, 4000 changes (checkpoints 500/1000/2000/4000), 85/15 causal/concurrent mix realized as 604 stale-knowledge submissions -> 527 tier-2 merges. **The series' first registered, machine-measured number: residency = 2.102x working set at n=2000 (boundary <= 3.0), linear R^2 = 1.000000, slope 48.0 bytes/change, incremental within +/-15%.** Two measurement-adjudication notes are registered in the code where they were decided: history bytes count records (n_log x 48) + live arena + live receipts, not allocator capacity (capacity prices the growth policy, not the ontology); the working set is the world struct itself (the region table lives inside it — no double count); and the incremental baseline starts at load-complete (the document-load blob is the registered load, not the mutation stream's marginal cost). Raw data + recomputation: `measurements/e2-engineering-leg/` + `scripts/analyze_e2_residency.sh`.
- **The rebound gate** (`scripts/check_slice_rebound.sh`): forbids kind dispatch, a quiet-system mode flag, and single-source special-casing anywhere in `slice/src` + `slice/include` — then plants all three in a scratch copy and requires every bite to land (a gate that cannot bite is decoration). The gate itself needed its own fix during development: a pattern beginning with `-` parsed as an rg flag and failed silently behind `|| true` — the exact toothless-gate shape the bite demo exists to catch, now guarded with `--`.
- **Wiring**: `slice/Makefile` (self-contained, `CFLAGS_EXTRA` passthrough, targets `all`/`apps`/`test`/`rebound-check`/`clean`); root `CMakeLists.txt` gains `planex_slice_lib` + both arms + both suites + the E2 leg as ctest entries; `ci.yml` runs the suites + arms + E2 leg in all four build jobs and the rebound gate in the make-based jobs; `TESTING.md` documents the layer (total 82).
- Design notes landed during implementation, each registered where it lives: the 48-byte intent record matches ADR-0025 4f's registered accounting unit exactly (the `aux` field is load-bearing: ARBIT's loser, CURSOR's previous cut, MERGE's policy tag); the living-knowledge rule (a submitter's declared knowledge excludes what the current view has revoked — without it an undo erases the future, not the past); the REBORN exemption (a same-source post-undo submission re-bases its clock below the revoked record's slot; the numeric collision alone cannot tell "after e" from "instead of e"); the fork-head time-travel rule (a historical view walks the chain spine back to its own cut — a later merge in the working set must not leak into a historical fold); the concurrency beat's hot-object aiming (607 stale submissions landing on cold objects realized only 2 pairs — the shape decision is on the record, not buried in the generator).

### Added — ADR-0025: the intent-flow vertical slice registered (Program I's falsification instrument, predictions before implementation)

- **The registration**: ADR-0025 (Proposed) commits the slice's design before any `slice/` code exists on the branch — the research series' instrument spec (one interface, one person, two agents, three intent flows, time-addressable undo) operationalized as a repo decision. Third columns of all five axioms under test (N1–N5) are quoted verbatim in the Decision; the E-series experiment registry from the evaluation-protocol volume is bound in as the slice's acceptance list, so the instrument cannot land "without reading rules" (the series' own falsification condition for this step).
- **Predictions C1–C9 registered pre-implementation**: four-column closure; calibration green across I1–I7; I1 single-source short-circuit (all four adjudication counters zero); I2 zero-budget silence (no unaccounted changes, no passive-mode branch, receipts everywhere); I3 all-tier-3 degeneration (zero fork residue); I4 semilattice laws over 1M random groups with shrinking plus a synthetic non-associative join that the registry must reject; I5 undo-as-query byte-identity; C8 the E2 engineering leg (1000 objects, ≥2000 changes, residency ≤3× working set at n=2000, R²≥0.99 linearity — with the at-all-n vs at-registered-load reading adjudicated openly in the ADR, raw data to be published for re-adjudication under any stricter reading); C9 the classical control arm's measured machinery costs. Falsified-if clauses attached to each.
- **Operationalizations fixed**: TRIP-17 scenario shape; the open payload-type registry (B2's no-closed-enum rule enforced by making the built-ins register through the same channel as user types); tier semantics including the tier-3 fold-exclusion interpretation of "only one version exists in the log" (retention is the stricter reading for I6); undo as slice redefinition with CURSOR intents for attribution; the monotone debit ladder (1/2/4/8, values E4's to measure, monotonicity the slice's to keep); semantic-tree-first projection per the rehabilitation volume; S5 (example-as-spec) explicitly deferred with its enabling condition recorded, not silently dropped.
- **Placement**: a `slice/` parallel tree following the `v4/` precedent (separate library, separate namespace `ifl_`, not linked into `planex_lib` — except the control arm, which deliberately links the frozen shipping body at the branch point).
- Verification: `check_doc_sections --check --four-column` reports all 24 ADR files with both ADR-0024 sections present for ≥0024; `check_essence_admission --check` 6/6 PASS (ADR-0025's tradition citation is the D5-verified Heidegger/Dourish readiness-rupture lineage behind the debit ladder, cited in Context where it belongs); `gen_adr_index --check` clean after `--update`; `check_adr_lifecycle`, `check_links` (79 files), `find_orphans`, `check_terms`, stale-abstraction and limitations-freshness gates all PASS.

### Added — ADR-0024: the four-column discipline — every ADR states its refutation criteria and regression clause

- **The gap**: ADR-0014 made *enforcement-mechanism* ADRs falsifiable (the Validated state: mechanism implemented, proven on a synthetic case, output preserved), but the ordinary decision stream — retirements, promotions, probes — carried no written statement of what would count against it. The leak-budget line was retired twice (ADR-0013, then ADR-0019), and both retirements assembled their evidence *at retirement time*, because nobody had written down at acceptance time which observation should trigger the retirement. Falsifiable in principle, unfalsifiable in practice.
- **The decision**: `TEMPLATE.md` and `TEMPLATE-GUIDE.md` gain two mandatory sections for ADR-0024 and later — `## Refutation Criteria` (the decision's expiry condition: what observable evidence would count against it, where it shows up, who notices) and `## Regression Clause` (the contract a successor inherits: what must not silently return). ADR-0001..0023 are exempt, by number and by honesty — backfilling criteria for decisions whose outcomes are already known is the theater version of falsifiability. ADR-0024 itself carries both sections filled (the discipline's first instance must pass its own gate).
- **The enforcement**: `scripts/check_doc_sections.sh` gains an opt-in `--four-column` mode — ADRs numbered ≥ 0024 must additionally carry both sections or the script exits 1. The default mode (what CI runs) is behavior-identical; wiring `--four-column` into `docs.yml` is the promotion step toward Validated, per ADR-0014's own rule that a mechanism must be demonstrated before it is wired. The synthetic-case demonstration is preserved verbatim in ADR-0024's `## Validation`.
- **The demonstration caught its own defect**: the prototype's first run exposed that bare `[[ "$num" -ge 24 ]]` reads zero-padded ADR numbers as octal — `0009`/`0008`/`0018`/`0019` errored and silently escaped, meaning a real ADR-0028 would never have been checked. Fixed with `(( 10#$num >= threshold ))`; the trap, the fix, and the post-fix enforcement output (exit 1 naming both missing sections for synthetic ADR-0025 *and* the octal-trap file ADR-0028; plain `--check` exit 0 on the same dirty tree; restored tree exit 0) are all preserved in the ADR.
- Research basis: Popper's demarcation (*Conjectures and Refutations*, 1963, ch. 1) for refutation criteria; the project's 2026-10 research notes ("four-column discipline" — statement, defense, refutation condition, regression clause) for the section contract.
- Verification: all 8 shell doc gates + 4 make check targets PASS (default `check_doc_sections --check` rules unchanged: `main`@`35189a9` reports "all 22 ADR files / all 9 mandatory sections", this branch reports "all 23" — the +1 is ADR-0024 itself, so the message delta is the new file, not a rule change); `--four-column` demo replayed end-to-end as recorded above, and replayed again at push time, byte-identical to the preserved transcript; `gen_adr_index.sh --check` clean after `--update` added the ADR-0024 row; `check_links.sh` clean on all four touched files.

### Fixed — test_h1's counting destructor dropped the pointer it was counting (the suite's one ASAN leak, 320B/5 allocs)

- **The finding**: the probe's verification run flagged test_v06_interaction as leaking 320B in 5 allocations under ASAN (`detect_leaks=1`) — byte-identical signature on main, so noted there as pre-existing and out of scope. Root cause, found this round: the h1 harness's `free_counting` destructor incremented its counter and **discarded the representamen pointer** — `(void)rep;` — without freeing it. The framework side is innocent and vindicated: `px_perception_set_free_fn` invoked the destructor exactly 5 times (4 on cache overwrite + 1 at perception free, which is what the `g_freed_representamens == 5` assert proves), and the test's own `perceiving_malloc` handed it 5 x 64B that its destructor then threw away. The test measured *invocation* while claiming to test *reclamation*; the assert cannot see the difference, ASAN can.
- **The fix**: one line — `free(rep)` before the counter increment. The test now proves both halves: the destructor fires (assert) and actually reclaims (ASAN clean).
- Verification: `make test` 39/39 with the fix; ASAN (address+undefined, `-fno-sanitize-recover=undefined`, `detect_leaks=1`) on test_v06_interaction exits 0 with no SUMMARY — the leak that existed on main since the h1 section landed is gone; `free_counting` has a single call site (h1 only), so no other test's semantics change.

### Fixed — round-2 audit doc corrections (D5, D6, D7, D9, D10): five claims that contradicted the tree they live in

- **D5 — `px_loop`'s home file, three ways.** `abstraction-form.md` enforced the abstraction↔file binding through `src/loop.c`; `leak-budgets.md` section 5 headed the same abstraction `src/app.c`; `architecture.md` (correct) says neither — `px_loop_step`/`px_loop_replay` live in `src/feedback.c`, and `loop.c` exists only in the v4 proposal tree. The two stale pointers now name `src/feedback.c`, with the proposal-tree trap called out inline. A reader following the file-structure claim to find the loop's implementation now lands on the file that contains it.
- **D6 — Tier 2 declared unbuildable while being built.** `examples/README.md`'s Tier 2 header said the four prototype demos (counter/calculator denotative, counter_interactive, editor_meaning) "are **not in the CMake build** because they would fail to compile" on the pre-ADR-0005 6-arg `px_closure_new`, "will be migrated... in subsequent commits." All four are in both build systems (CMake `STDOUT_DEMOS`/`WINDOWED_DEMOS`, Makefile `EXAMPLES_NO_X11`/`EXAMPLES_WINDOWED`), compile against the current 5-arg constructor (`counter_denotative.c:335`), and the three stdout ones are snapshot-gated byte-identical by `check-examples` — the migration the README scheduled as future happened long ago. The section now states what is true: in build, migrated, kept as research reference. This also removes the README↔roadmap-matrix contradiction (the matrix's own Tier 2 line said "in build, migrated to new API" all along).
- **D7 — a removal that never happened.** The deprecation registry marked `px_closure_get_status(c)` as **removed** (v4 design), replacement `px_perlocution_status(per)`. The function ships and is callable (`planex.h:569`, `closure.c:383`), and the replacement is a v4-proposal API that does not ship — the registry recorded a design intent as a landed fact. Per the registry's own three-state vocabulary this is **deprecated** (still callable, removal at a future boundary), and the row now says so, with the still-shipping status and the not-yet-shipping replacement called out. The intro's "the ADR that decided the removal" is generalized to deprecation-or-removal.
- **D9 — `planex.h`'s top comment frozen at v0.1.** The aggregate header — the first thing a reader opens — said "Three core abstractions", "Closure: 7-stage interaction unit (Goal→…→Evaluation)", and "Status: Stage 0 — validating abstractions via stdout. Pixel rendering comes in Stage 1." Reality: 7 abstractions since v0.7 (ADR-0008/0017/0018), Closure is the 5-stage execution unit (ADR-0005 moved the evaluation side to Perception — stated correctly 400 lines deeper in the same file), version 0.7.0 with windowed X11/Win32/Cocoa/backends and a UBSan-strict CI battery. The comment now matches the file it heads; the version-macro block below it was already correct and is untouched.
- **D10 — a roadmap phase the repo rejected, still marked pending.** `roadmap-matrix.md` declares "All 35 cells green (7 abstractions × 5 dimensions)" at the top while its embedded v0.4 roadmap still lists "Phase 3: More widget demos — Recreate slider, form, tabs, modal, todo_app... Estimated effort: 1 week" as live work. That plan was abandoned: the demos were deleted in the v0.3 redesign and NG-5 settles the question in the opposite direction (no component library; widgets emerge from the abstractions; `examples/` holds reference implementations). The v0.4 Roadmap section is now banner-marked as historical with the current plan of record ([v0.8-roadmap.md](concepts/state/v0.8-roadmap.md)) linked, and Phase 3 carries an explicit NOT TAKEN annotation citing NG-5, with the era of its "4-abstraction" phrasing noted.
- Verification: all 8 shell doc gates + 4 make check targets PASS; `check-examples` 15/15 byte-identical (the D9 comment edit touches no code path); CMake build + ctest 9/9; every corrected claim re-verified against the tree at review time (file paths, line numbers, build-list memberships, symbol existence) rather than against the audit report alone.

### Added — the semantic-drift probe (ADR-0023): mid-flight reinterpretation measured, the B→C criterion does not trigger on this axis

- **The question**: the continuous-intent speculation gates Option C (intent gradient) on evidence that the landed process abstraction alone is insufficient. Its five scenarios all stress *strength*; none stresses *reinterpretation* — the gesture's meaning changing while the trajectory continues, the exact point where the classical event model pays mode-machine teardown. Nobody had measured it.
- **The protocol**: predictions P1–P5 were registered in ADR-0023 *before* the probe was written (API closure; trajectory continuity; two-step undo from one gesture; THE INVARIANT surviving the drift; the classical control's cost). `examples/semantic_drift_probe.c` implements one deterministic gesture — press, drag in a list, cross into a trash margin, release, plus an ESC-after-drift segment — three ways: bridge repointing (`px_interaction_on_commit` rebinds the live process at the boundary), hook compilation (the phase hook compiles the outcome from the trajectory + a mode field), and the classical control (mode flags per handler, anchor reset, hand-rolled undo). The crossing-commit is an app policy, printed as such.
- **The measurements**: P1–P4 PASS — zero framework edits (the probe compiles against the shipping header); 37 samples as ONE trajectory with 21 pre-drift samples still queryable and the path measure reading across the seam (180px); `px_undo_count() == 2` after one gesture with both steps independently restorable, and cancel-after-drift leaving exactly the crossing-committed step (scoped to the unresolved remainder); 0 estimate writes during samples, drift included, 3 at transitions. P5 measured: the classical control pays 37 mode consults, an anchor reset amputating all 21 pre-drift samples, and a manual undo stack. **Verdict**: Option C's open territory narrows to strength/gradient — untested here by construction.
- **The lock**: `tests/test_v06_interaction.c` section `[K]` (5 tests, suite 27→32) — rebind fires the rebound closure only; the ring spans both segments with measures reading across the seam; one gesture, two registry undo steps; the invariant holds through a mid-stream rebind; cancel after rebind kills only the remainder.
- Verification: full battery green — 7 suites + `test_completeness` under UBSan strict (`-fno-sanitize-recover=undefined`), all examples UBSan-clean, `check-examples` 15/15 byte-identical (the probe's `.expected` snapshot), `check-build-sync` 27 examples in both build systems, CMake build + ctest 9/9, all shell doc gates, `gcc -Wconversion` silent on the new files, and ASAN (address+undefined, `detect_leaks=1`) clean on both new files — the suite's one pre-existing leak (`test_h1`, 320B/5 allocs) is unchanged from main and out of scope here.

### Fixed — gate 13's landing shipped three defects of the class it exists to catch (caught by its own first two CI runs, 41178587420 and 41179280419)

- **The catch**: the push that landed gate 13 (`make check-build-sync` wired into ci.yml's linux-make and strict-warnings jobs) failed both of those jobs at the gate step itself, and the windows job at `hover_drag_interaction.c(246)` — the gate's first CI run ever, since the branch that carried it never triggered CI (the push filters are main-only). Fixing those two exposed a third on the next run: same two jobs, same step, different cause.
- **Defect one — the mode bit**: `scripts/check_build_sync.sh` was committed `100644`. Every other script in `scripts/` ships `100755`; the Makefile invokes `./scripts/check_build_sync.sh --check`, so a fresh checkout got exit 126 (permission denied) while the dev tree that wrote it stayed green — the file had been chmod-ed locally after `git add`. A gate built to kill drift invisible outside a fresh checkout shipped with a defect invisible outside a fresh checkout. Fixed: the executable bit recorded in the index.
- **Defect two — the first compile**: adding `hover_drag_interaction.c` to CMakeLists (the Makefile-only half of the build drift) gave the windows `/WX` job its first-ever compile of that file, which met a stock C4244: `px_rect_make(ITEM_X, ITEM_Y_START + i * ITEM_H, ...)` passes a runtime `int` where the parameter is `float`. MSVC treats the conversion as possible precision loss and `/WX` makes it fatal; gcc's `-Wall -Wextra -Wpedantic` (the strict-warnings tier) does not flag int→float at all, and the linux-cmake job builds without `-Werror` — every Linux tier was green on the same source. Fixed at source per the f702d0a precedent: an explicit `(float)` cast, value-preserving (`check-examples` stays byte-identical); a `-Wconversion` sweep of the file reports it as the only conversion site.
- **Defect three — the non-portable dependency**: with the bit fixed, the gate ran on CI and immediately died again: `./scripts/check_build_sync.sh: line 76: rg: command not found`. The script called ripgrep four times — present on the authoring machine (so every local replay, including one from a fresh clone, stayed green), absent from the GitHub runner image. The missing command emptied the Makefile list, so check 1a flagged all 26 examples as "not in the Makefile". The other twelve scripts in `scripts/` are grep/sed/awk and have always run clean on CI; the new one regressed the portability contract its own header states ("pure shell, no toolchain"). Ported to `grep -v` / `grep -oE` / `grep -q` — the patterns are plain ERE, so behavior is identical where ripgrep exists and defined where it does not. A fresh checkout was not enough to catch this; it took a fresh environment.
- **Verification, replayed after each defect**: clean-clone `stat` confirms the executable bit, `make check-build-sync` PASSes (26 examples on disk == Makefile == CMakeLists, 21 src files in both build systems), the `-Werror` build plus all seven suites green, `check-examples` byte-identical, `gcc -Wconversion` on the file silent, and `grep -rn 'rg ' scripts/` finds no script left depending on it. Falsifiability replay of the ported gate on a mutated clone — one demo deleted from CMakeLists and one phantom `run-*` target added to the Makefile — fires both detection classes by name and exits 1.

### Fixed — ADR-0004's Known-issues record claimed "ASAN is enabled in CI builds" (audit D8): no workflow has ever run ASAN

- **The finding**: ADR-0004's first Known issue carried two claims that matched no workflow — the Why-accepted line cited "`valgrind`/ASAN in CI", and the Mitigation line asserted "the test suite (78 tests) exercises free-path coverage; ASAN is enabled in CI builds". Neither sanitizer exists in any workflow; the sanitizer tier that does exist is UBSan strict (static-analysis Gate 3: `-fno-sanitize-recover`, full suite plus every example). The "78 tests" count was stale by a factor of two.
- **The correction**: the record now states what runs — 163 test functions across 7 binaries plus the 75-check pattern-completeness gate, UBSan strict in CI, ASAN locally during audits. Why ASAN is not a CI tier yet is stated rather than papered over: a leak-detecting tier would currently fail on the interpretant prototype's documented by-design leak (the canonical demos' perception-referent leaks are retired in this same push). The decision itself — C17, zero deps — is untouched; a HISTORY entry records the correction with its date and cause.
- Verification: ADR section structure intact (Gate 1's 9 mandatory sections), lifecycle state unchanged (Accepted, still matches its directory), doc gates replayed locally, and the 163/7-binary/75-check figures re-counted from this push's green runs.

### Fixed — the canonical demos never registered their perceptions' free_fn (audit C5): the no-leak pattern existed since v0.6, but no demo showed it

- **The finding**: the v0.6 ownership mechanism (`px_perception_set_free_fn`) frees a perception's cached representamen on overwrite and on perception free — the framework side was complete. But every canonical demo registered perceptions whose functions return `malloc`'d strings and passed no destructor, so each auto-invoke after a state change overwrote a leaked string: the demos taught the exact pattern the mechanism was built to retire. The framework was right; the examples were the bug.
- **The fix**: seventeen perceptions across seven demos now register their destructor — `counter_4abs` (2), `multi_perception` (4), `integration_4abs` (3), `confidence_demo` (2), `async_demo` (2), `animation_demo` (2), `perception_smoke` (2). The demos whose denotations are not heap objects (`editor_meaning` and the rest) deliberately register nothing — freeing non-heap storage would be the bug those demos avoid by construction.
- Verification: stdout is byte-identical — `check-examples` 14 PASS / 0 FAIL against every `.expected`. The before/after leak proof: `counter_4abs` at main@4e7ab2a under ASAN with `detect_leaks=1` reports `768 byte(s) leaked in 8 allocation(s)` (four count changes × two perceptions — one dropped string per set per channel) and exits 1; the fixed tree reports no leaks and exits 0. All seven modified demos re-run the same way: zero leaks, exit 0.

### Fixed — px_loop_replay's snapshot buffer was a 64 KB stack frame (audit C3) — now a heap allocation of exactly n entries

- **The finding**: replay collected its entries into `px_loop_audit_entry entries[PX_LOOP_AUDIT_CAPACITY]` — 1024 entries at 64 bytes each, a 64 KB frame carved onto the caller's stack on every call, whatever the replay size. Harmless under a desktop default stack; fatal on the 4–8 KB stacks of the MCU/RTOS class the library's own positioning names as a target. The embeddability claim and the stack frame contradicted each other in the same function.
- **The fix**: `malloc` exactly n entries — n is already clamped to what the ring actually holds (and to capacity), so the allocation is bounded by real history, never by capacity — and `free` before return. Allocation failure degrades to "nothing replayed" (returns 0) rather than crashing the caller's thread. The collect/replay semantics are untouched: same clamps, same chronological order, same audit suppression during replay.
- **The regression lock**: `test_e3_replay_full_ring` fills the 1024-entry ring past full (1100 steps) and replays 5000 — locking the maximum-n path: clamp to 1024, every entry replaying once. The existing replay tests (e1, e2, f3) lock the semantics; they run unchanged.
- Verification, both directions: the seven CI binaries green (`test_core` 39/39, `test_feedback` 14/14, `test_v05` 12/12, `test_v06` 27/27, `test_v07` 21/21, `test_v08` 31/31, `test_orthogonality` 19/20 + 1 documented skip). The before/after proof under a constrained stack: a probe that steps 1100 times and replays 5000, linked once against the pre-fix `feedback.c` (main@4e7ab2a) and once against this fix, both run with `ulimit -s 64` (a 64 KB stack): pre-fix dies with SIGSEGV — the array alone exceeds the limit; fixed exits 0 with `replayed=1024`. Unconstrained, both versions agree — the observable contract never changed, only the frame did.

### Fixed — px_estimate_unobserve (audit C2): the observer subscription that outlived both death orders

- **The finding**: `px_derived_remove_source` removed a source from the derived's source array but left the derived subscribed to it — the source's observer list kept a node whose user pointer aimed at the derived, so a later `px_estimate_set` on the source called `px_derived_recompute` on whatever now lived at that address. The source comment called it "a known limitation — Stage 20 can add px_estimate_unobserve()" and advised freeing the source after removal — advice that covered only the source-dies-first order. The derived-dies-first order (free the derived, then set a source) had no safe sequence at all: the subscription outlived both parties.
- **The fix**: `px_estimate_unobserve(e, fn, user)` — the exact inverse of one `px_estimate_observe` call: removes and frees the first node matching the pair, returns whether a node was removed. Wired into both death orders: `px_derived_remove_source` unsubscribes before recomputing (one call reverses one subscription — a source added twice is removed and unsubscribed twice), and `px_estimate_free` now unsubscribes a dying derived from every still-live source. The free path is guarded by `px__estimate_is_live` — pointer comparison only, never a dereference — so the teardown itself cannot trip over a source that died first. A source freed without `px_derived_remove_source` remains the documented app contract; the guard makes the free safe, not the recompute.
- **The regression lock**: four tests in `test_core` (`unobserve_removes_subscription`, `unobserve_first_match_only`, `remove_source_unsubscribes`, `derived_death_unsubscribes`). The death-order tests witness recomputes through an observer on the derived — value equality alone cannot distinguish the dangling path, because recomputing a derived whose source list no longer contains the mover produces the same number on both paths.
- Verification: full suite green (`test_core` 39/39, all 7 binaries pass), the new tests re-run under `-fsanitize=address` with `detect_leaks=1` — zero leak reports, exit 0. The before/after proof, run both ways: a probe linking the pre-fix `estimate.c` (main@4e7ab2a) aborts under ASAN with `heap-use-after-free ... in px_derived_recompute`, called from `derived_on_source_changed` — the exact call chain the finding names; the same probe against this fix exits 0. `check-examples` 26/26, doc gates replayed locally.

### Fixed — test_orthogonality's d1 step 6 read freed memory: the second replay of a captured intent (caught on windows-latest, run 33402526410)

- **The catch**: the windows job of the janitor push (content: two BMP deletions plus this changelog — zero C changes) failed in the per-suite exit-code loop: `test_orthogonality` aborted at line 586, `px_estimate_value(count) == 42.0`, exit 0xC0000409. The same source had been green on the previous run — with no code delta, an assertion on heap-adjacent memory pointed at a lifetime defect, not logic. Second real catch for the hardened loop, and the mirror case of the QPC flake: that one was a clock the test couldn't witness; this one was bytes the test shouldn't have been reading at all.
- **The mechanism**: d1 captures one intent and replays it twice. `px_closure_last_intent()` returns a borrow — planex.h documents the payload as "owned by closure until the next trigger" — and replay #1's internal trigger retires that borrow. Replay #2 then `memcpy`s from the freed block: a use-after-free read that passes only while the allocator hasn't reused the bytes. Linux glibc tcache left the four freed bytes intact for 200/200 local runs (and 500/500 after the fix — the old form never failed locally); this Windows CRT heap reused them on the first try. The test's comment claimed more than the header promises ("proves intent is a value, not a one-shot") — the flake was that over-claim made physically visible.
- **The fix**: copy the payload bytes into a caller-owned buffer while the borrow is live (the same idiom test_core's serializable-intent test already uses — d1 was the outlier), and replay #2 runs from that copy. The claim is now the precise one: an intent's kind, size, and payload bytes are values — copyable, replayable, non-one-shot — while a captured struct's payload pointer is a documented borrow with an expiry. Deterministic on every platform by construction.
- **The audit**: every other `px_closure_last_intent` call site checked — test_core (field reads + the serialize-then-replay copy), test_v06 (field reads), test_v4 (kind query), test_v07 (single immediate replay), antipattern_closure (immediate reads), integration_4abs (kind for audit log). All consume within the borrow window; d1 was the sole double-replay. The undo stack itself stores only `(estimate*, value, confidence)` triples — no intent payloads — so the library's own redo path has no exposure of this class.
- Verification: clean-tree full suite 7/7 (`make test`), `check-examples` 14/14, hermetic UBSan replay clean (10 examples, exact CI commands), 8/8 doc gates replayed locally, `check-completeness` green, 500/500 consecutive runs of the fixed suite.

### Removed — two orphaned root-level BMPs (counter.bmp, slider.bmp): re-seed snapshot residue, invisible to .gitignore

- **The finding**: `counter.bmp` (256×128, 131 KB) and `slider.bmp` (320×96, 123 KB) sat at the repository root with zero references — no source, test, example, doc, or script mentions either filename; every demo writes its screenshot under its full name (`counter_denotative.bmp`, `calculator_denotative.bmp`, …), and no slider demo has ever existed in this repository. Their dimensions match no current demo output either (the counter demos render 256×96, not 256×128). They entered in the 4db6e6c re-seed snapshot — a bulk import predating the `*.bmp` ignore rule, and tracked files survive `.gitignore`, which is why the rule that should have kept them out also kept them invisible to every later cleanup pass.
- **The removal**: 254 KB of unreferenced binary, ~18% of the 1438 KB repository — gone. Nothing reads the files, so the working tree is unaffected; the demos' transient `.bmp` outputs keep landing wherever the binaries run, still covered by the existing ignore rule. Local clones recover the space automatically on the next fetch (the two blobs leave the tree; repo history before 4db6e6c is unreachable by default).

### Fixed — test_v07's b3 was a flaky gate on windows-latest (QPC granularity vs the 1ns budget; caught by the hardened windows job, which the old step format would have swallowed)

- **The catch**: run 33393381147's windows job (the identity-rewrite push — content byte-identical to the previous green run) failed in the new per-suite exit-code loop: `test_v07` aborted with `entries[i].budget_exceeded == true` (assert, exit 0xC0000409). Two prior windows runs of the same content had passed — a genuine flake, and exactly the defect class the hardened step exists for: under the old one-command-per-line format, the abort would have been overwritten by the last suite's exit 0 and the step would have gone green.
- **The mechanism**: `test_b3_overrun_is_loud_and_counted` sets a 1ns budget and asserts every step's overrun is recorded. But `budget_exceeded` is `iteration_ms > budget`, and `iteration_ms` is two `px_now_ms()` reads — QPC-backed on Windows, 100ns-class granularity. A null-perception loop step can complete inside a single QPC tick, so the audit measures `iteration_ms == 0.0` exactly and the 1ns budget is NOT flagged — a clock-granularity false negative (not evidence of sub-nanosecond execution). Whether a step spans a tick is a race with the counter: same binary, different runs, different verdicts. Linux never saw it because CLOCK_MONOTONIC resolves nanoseconds.
- **The fix**: `perceive_one_tick` — the b3 perception now spins (bounded, so a broken clock fails the test instead of hanging it) until `px_now_ms()` advances at least one tick. Every step's duration is then resolvable: `iteration_ms >= 1 tick > 1ns`, so the overrun path — the machinery the test exists to exercise (recorded in the entry, counted, warned exactly once) — fires deterministically on every platform. The test's premise shifts from "any real iteration exceeds 1ns" (a physics claim the clock cannot always witness) to "any MEASURABLE iteration exceeds 1ns" (a claim the test now guarantees by construction).
- Verification: 20 consecutive local runs green, full suite 7/7, check-examples 14/14, 12/12 doc gates, hermetic UBSan replay clean.

### Fixed — the committer identity drift (five commits authored as the sandbox default, rewritten to the maintainer)

- 1047000 through 8761801 (the duplicate-run-key fix, the NDEBUG guards, the analyzer tier, the Gate 3 false-verification fix, the CodeQL BMP-permissions fix) were committed from a sandbox whose git config carried the container default identity — `Z User <z@container>` — because the repository had no repo-local `user.name`/`user.email` and the earlier sessions' identity had not persisted into `.git/config`. All five were rewritten (`git filter-branch` env-filter, author and committer, dates preserved) to `Olday <baima126@outlook.com>`, the identity of the prior 48 commits; the rewrite changed their hashes, and every in-repo reference to `c098b63` was re-pointed to its rewritten hash `f702d0a` in the same pass (workflow comments included). The repo-local identity is now set in `.git/config` so the class of drift cannot silently recur here. Note for local clones: the rewritten range starts at the parent of the old 1047000 — a `git fetch && git reset --hard origin/main` (or re-clone) resyncs, since the divergence is metadata-only and the trees are byte-identical.

### Fixed — CodeQL's first run earned its keep: the umask-dependent BMP permissions (src/fb.c, one alert, one real defect)

- **The finding**: the CodeQL tier's very first analysis produced exactly one alert — `cpp/world-writable-file-creation` (high), `px_fb_save_bmp`'s `fopen(path, "wb")`. Not a false positive, and reproduced locally before touching anything: under `umask 0`, `counter_denotative` wrote its BMP with mode 666 — `fopen("wb")` creates `0666 & ~umask`, so the screenshot surface (every `PLANEX_HEADLESS_BMP` frame and both denotative examples) inherits whatever permissive umask the process happens to run under. A taint/permission analysis catching in minutes what a year of passing builds never saw: the tier's opening argument.
- **The fix**: POSIX builds create the file with an explicit `0644` via `open(O_WRONLY|O_CREAT|O_TRUNC, 0644)` + `fdopen` — umask-independent, with the failure paths handled (missing fd → error, fdopen failure → close + error). Windows keeps `fopen` un-changed: the CRT creation mode has no umask dependency worth modeling (access is ACL-governed), and the branch compiles nothing new on MSVC — the /WX build is untouched by this change.
- **Verification, the full discipline**: regression-proof first (umask 0 before: 0666; after: 0644), then a clean-tree rebuild — 7/7 make test targets, `check-examples` 14/14, the hermetic UBSan replay (all 10 examples, windowed demos included) clean, Gates 1-2 replayed with the locally provisioned analyzers (cppcheck blocking layer exit 0, style count unchanged at 18, clang-tidy clean), and a clean `BACKEND=x11` examples build. The alert auto-resolves when this commit lands in the next CodeQL scan.

### Fixed — the analyzer tier's first run found a false verification: Gate 3's windowed demos (the dirty build/ directory lesson, run 33388320275)

- **The failure**: the UBSan gate's first CI run failed at `a11y_orca_demo` — `No such file or directory`. Root cause one: under `BACKEND=headless`, `make examples` builds only `EXAMPLES_NO_X11` (the Makefile keeps windowed demos out of the aggregate); the gate's run list named four binaries that were never built. Root cause two, the deeper one: the gate could not have worked as written even with the binaries — `px_app_run` presents a frame only when something changed (event, tick, or animation; `app.c`), while the headless auto-quit counter counts PRESENTS (`headless.c`), so an event-driven demo fed `</dev/null` never presents, never advances the counter, and hangs forever (empirically: 3 of the 4 windowed demos; only `counter_interactive`, which animates, terminates).
- **The false verification**: the local "pre-verified clean" run had executed in a dirty `build/` directory — the windowed binaries it "ran" were stale artifacts from earlier x11/atspi builds, masking both the missing-build and the hang. The pre-run proof was not hermetic, so it proved nothing; that is now part of the record (and the hermetic replay — `rm -rf build`, then the exact workflow commands — is the standard going forward, saved as a re-runnable script).
- **The fix, two drive modes, documented in the workflow**: the windowed demos are built explicitly via their per-target rules (`make build/counter_interactive …`), then driven with stdin `'q'` — headless.c's own documented close idiom — instead of EOF; stdout/BMP examples keep the EOF auto-quit (their own present-per-frame loops terminate). Every example run gets `timeout 30` so a future hang fails in 30 seconds instead of blocking the job for six hours. All four windowed demos are now in the run list (`counter_perception_window` joins the original three), each exiting 0 under strict UBSan in the hermetic replay.
- Also fixed in passing: the workflow header's blanket "all locally pre-verified clean before this workflow ever ran" claim — true for Gates 1, 2 and 4, false for Gate 3; the header now claims only what each gate's comment can defend.

### Changed — CI: the windows job stops being able to lie (three false-PASS shapes closed; the f702d0a hold-open items land)

- **`/WX` lands** (the MSVC `-Werror` equivalent): the build has been warning-free since f702d0a (the C4244 float literals and the C4996 POSIX-name nags fixed at the source level; the latest windows-latest build log — run 33388320286, VS 2026 / MSVC 19.51 — contains zero `warning C` lines), so a NEW warning from a future toolchain or SDK rev now fails the build instead of scrolling past. This is the MSVC mirror of the `strict-warnings` Linux job, applied to the job that already builds everything.
- **The missing-exe precheck**: a `Test-Path` pass over all ten test executables before anything runs. PowerShell's call operator on a missing path does not fail a step — `$LASTEXITCODE` keeps its stale value — the exact false-PASS shape the local pwsh loop hit during the f702d0a diagnosis (a `CommandNotFoundException` followed by a green PASS printed from the previous command's exit code). A silently-unbuilt binary now fails loudly before the first test.
- **The exit-code loop**: the old one-command-per-line form had the mirror-image flaw — a failing suite mid-list left the step's exit code to whatever the LAST command returned. The loop records every nonzero exit per suite and fails the step on any failure; assertions are live in Release per f702d0a's `#undef NDEBUG` guards (Gate 4 of static-analysis.yml keeps them that way).
- **The copy residue**: `-DCMAKE_EXE_LINKER_FLAGS="-lX11 -lXext -lfreetype -lfontconfig"` — GCC-style flags, meaningless to MSVC's `link.exe`, carried over from the Linux job since the job was written — removed. It was harmless but it lied about what the build depended on.

### Added — CodeQL: the semantic tier (taint-tracking data-flow over the full C surface, reporting posture)

- `.github/workflows/codeql.yml`: the third leg of the analyzer tier — the path-sensitive source-to-sink flow class that neither cppcheck nor clang-tidy models. Advanced setup (the workflow file IS the setup — runs on the next push, no Settings toggle; the default-setup toggle stays off since the two modes are mutually exclusive and the file path is version-controlled). Weekly scheduled scan (03:17 UTC, Monday) plus push/PR/manual. The traced build is the exact `linux-cmake` configuration (`cmake -B build && cmake --build build -j` after the same dep set), so the analyzed translation units are the ones the green CI compiles — no second build worldview.
- **Posture, stated honestly in the workflow header**: reporting, not blocking — alerts land in Security → Code scanning for triage, the same tier as cppcheck's style layer; the blocking tiers remain ci.yml and static-analysis.yml. Query suite `security-extended` (the C/C++ default — security-focused, no quality-alert noise). Promoting an alert class to blocking is a deliberate in-review edit, not a default. README gains Static Analysis and CodeQL badges beside the CI one.

### Fixed — the backend verification issue template named nine demos that no longer exist

- `.github/ISSUE_TEMPLATE/backend_verify.yml`'s demo checklist still listed the `counter_x11`/`slider_x11`/`perf_x11` era — binaries that predate the repo restructure and have not existed for a year. The checklist now names the four current windowed demos with what each actually exercises, and says outright what "testing a11y on Windows" means: the screen-reader bridge is a stub there — verify the Tab focus ring and Enter activation, not speech. A verification report filed from the old template could not have described a real test run.

### Added — the analyzer tier (static-analysis.yml: four self-enforcing gates)

- **Gate 1 — cppcheck** (bug-pattern level, blocking + style report): the static sibling of the C4700 class the first real Windows build surfaced — uninitialized reads, leaks, out-of-bounds shapes the compiler does not flag. Runs `--enable=warning --error-exitcode=1` over the portable library surface (win32/cocoa/x11 excluded: their platform headers do not exist on Linux, and the a11y-atspi-bridge job in ci.yml owns the AT-SPI probe; font_ttf.c excluded: freetype-optional file). Style-level findings (18: `constParameterPointer` const-correctness and kin) are counted and reported non-blocking — the compression-metric precedent (WARN tracked, not blocked), not a silent green.
- **Gate 2 — clang-tidy** (curated `bugprone-*`/`clang-analyzer-*`/`performance-*`, `--warnings-as-errors`, zero tolerated noise): six noise classes are disabled WITH documented reasons in the workflow header — `_POSIX_C_SOURCE` feature-test macros (`bugprone-reserved-identifier`), the MSVC `_s`-function worldview on glibc's standard fprintf/strncpy (`security.insecureAPI.*`), adjacent-same-type-parameter API-design critique firing on the whole math-shaped surface (`easily-swappable-parameters` — LLVM disables it by default too), the fb pixel-extraction `uint32_t`→`int` idiom, the registry `void*` pattern, and window-bounded `y*width` indexing. Muting a check now requires editing that rationale in review.
- **Gate 3 — UBSan strict** (`-fsanitize=undefined -fno-sanitize-recover`): runtime UB detection over the full suite — 7 make test targets + all 10 examples, windowed demos included (two drive modes; see the first-run fix entry above for why). Any UB aborts the run.
- **Gate 4 — assertion-liveness probe**: mutation testing applied to the f702d0a guards themselves — a one-file probe compiled with `-DNDEBUG` must STILL abort (exit 134). If it exits 0, the guards have regressed and every assert-based suite is vacuous again; caught before it can return, not after.
- **Method note**: Gates 1, 2 and 4 were verified clean at their exact workflow commands locally before the workflow ever ran (cppcheck 2.17.1 built from source, clang-tidy 19.1.7 from deb, the NDEBUG probe) — their first CI runs replayed pre-proven commands. Gate 3's local verification turned out to be non-hermetic (see the first-run entry above); it was re-proven hermetic — `rm -rf build`, exact workflow commands, windowed demos included — before the fix landed.

### Fixed — the vacuous Windows Release tests (assert() was compiled out on MSVC; found by the first real Windows build)

- **The defect**: the Windows CI job (and any user on MSVC Release) ran every assert-based suite with its assertions compiled to no-ops. CMake's default MSVC Release flags add `/DNDEBUG` (multi-config generators honor `--config Release`; the Linux single-config path ignores it — `CMAKE_BUILD_TYPE` empty, no `-DNDEBUG`), and 10 of the 11 test binaries verify their invariants exclusively through `assert()` — the `TEST()` macros only count and print. A vacuous pass still proves "no crash", nothing more; the Linux paths were the only ones with live assertions. The tell that surfaced it: MSVC's C4700 on `test_v08.c`'s out-param copy — with `NDEBUG`, the `assert()` that calls `px_afford_compile_process(&di)` IS the initialization, so the copy read genuinely uninitialized memory; MSVC's flow analysis was right and the suite was silent.
- **The fix (self-enforcing at the source level)**: every assert-based test source (8 `tests/*.c` suites + the 3 examples-as-tests) now opens with the classic guard — `#ifdef NDEBUG` / `#undef NDEBUG` — before its first `#include`, so `assert()` stays live in every configuration, every toolchain, Makefile or CMake, command-line `-DNDEBUG` included. Guard inserted by `scripts/add_ndebug_guard.py` (idempotent, kept for future suites). `tests/test_v3_prototype.c` is exempt — it verifies through its own check macros, zero `assert()` uses.
- **Verification, three layers**: (1) the guard probe — a one-file program compiled with `-DNDEBUG` still aborts on a false assert (exit 134); (2) the full suite rebuilt headless with `CFLAGS_EXTRA=-DNDEBUG` (the Windows-Release equivalent): all seven make test targets pass with live assertions; (3) the mutation test — one deliberately inverted `assert()` in `test_v08` under `-DNDEBUG` fails the suite (exit 134), then reverts. The normal path re-verified: all targets + `check-examples` 14/14 unchanged.
- **Warning hygiene from the same build** (MSVC-only noise, none visible to the Linux strict `-Werror` job): the three `C4244` int→float conversions in `px_rect_make` call sites (`a11y_orca_demo.c`, `palette_afford.c`, `designer_tools.c`) become explicit float literals — exactly representable, zero output drift (`check-examples` re-run proves it); `C4996` for `strdup` in `test_v3_prototype.c` is silenced by `_CRT_NONSTDC_NO_WARNINGS` (added next to the existing `_CRT_SECURE_NO_WARNINGS` in CMakeLists' MSVC branch). The remaining `C4101`/`C4700` family disappears with `NDEBUG` — those variables were "unused" only because their assertions were.
- Follow-up held open: the Windows job could add `/WX` (MSVC `-Werror` equivalent) and a missing-exe precheck so a silently-unbuilt binary cannot pass green (the pwsh loop that surfaced this bug had exactly that blind spot — it printed PASS from `$LASTEXITCODE`'s stale zero after a CommandNotFoundException).

### Added — v0.8: the observed orca pass (Cross-cutting A — L9's orca gap resolved, the ledger row flips to ready)

- `scripts/verify_orca_e2e.sh`: the end-to-end verification harness — XTEST keys → Xvfb → the x11 backend → `px_app_run`'s graph-derived focus ring → `on_focus` → the a11y query side → the AT-SPI2 bridge mirror → atk-bridge → D-Bus → orca → the `SPEECH OUTPUT` lines in its debug log, asserted against the app's own stdout trace (the semantic ground truth). XTEST synthesizes events server-side (XSendEvent carries a synthetic flag the chain must not see); the harness owns the full bridge compile line (every src translation unit + atk/atk-bridge via pkg-config), sets up the D-Bus session with the a11y bus service, and cleans up leaked daemons between runs (the found-by-inspection lesson: dbus-run-session does not kill D-Bus-activated services). Rootless stacks supported via `ORCA_ROOT`. The observed pass (orca 48.1, at-spi2-core 2.56, Xvfb, Debian 13): the window, every later focus move (three, with values), and both activation announcements are spoken; the first focus move is documented as racing client attachment.
- `scripts/x11_key_inject.c`: the XTEST key injector (finds the window by title under a WM-less Xvfb, replays the token stream); links with `-l:libXtst.so.6` when the dev package is absent.
- `examples/a11y_orca_demo.c`: the evidence app — three color swatches + reset, all four regions on the AFFORDS graph, so the focus ring Tab walks AND the a11y mirror are both DERIVED from the same edges (zero second sources of truth: the keyboard channel and the screen reader read the same graph — the A6 claim, projected). Pointer and key activate through one closure; selection lives in one estimate the mirror derives from on every flush.
- `src/a11y_bridge_atspi.c`: the observed run drove TWELVE rounds of real fixes into the bridge (each annotated in the source) — the previously-landed eleven (AtkUtil root provider, registration completion, the D-Bus pump, the frame + active-window projection, the state projections, the mirror subclass, the eager listener activation, per-region objects, the announcement signal, the focus-gated materialization) plus the round-twelve clean teardown below. A compile probe cannot see any of them — exactly why the observed pass was the roadmap's success criterion.

### Fixed — v0.8: the bridge teardown crash + the frame's NULL child (found by the orca run, round twelve)

- **The detach-time crash**: freeing the mirror after `atk_bridge_adaptor_cleanup()` fired the bridge's stale weak notifies into the cache's freed hash tables (a GLib fatal abort or a plain SEGV, heap-layout dependent). Root cause, traced by disassembly into atk-bridge 2.56: (1) the cache teardown `foreach` weak-unrefs the refmap VALUES — always NULL, never the objects — so every entry's weak ref survives as stale; (2) the teardown itself re-registers objects (tidy_windows marshals `Window:destroy` references through the registration path), seeding fresh weak refs milliseconds before the tables are freed. **The fix is the GTK exit semantic: detach marks every mirror object DEFUNCT (the widget-destruction signal — the bridge deregisters each one: cache weak ref, register weak ref, refmap entry, D-Bus path) and deliberately does NOT call `atk_bridge_adaptor_cleanup()`** — no cleanup, no freed tables, every remaining weak notify removes itself from a live table and cannot crash. The honest cost, recorded in `a11y.h`: the app stays registered on the accessibility bus until process exit, exactly like any GTK application; re-attaching after detach is not supported (single bridge per process).
- **The attach-time NULL child**: the frame's children array was seeded with a NULL slot (the pre-fix code added `b->element` before it became the frame) — every client enumeration tripped over it (`g_object_ref(NULL)` criticals in the orca log) and the announced children-changed indices drifted from the array positions. The frame's initial child is the alert alone; regions join when they materialize on focus; `px_mirror_add_child` now computes the signal index from the array slot (one source of truth).

### Changed — v0.8: docs — the verification recorded where it belongs

- `limitations.md` L9: the orca gap RESOLVED (the observed pass, the harness, the twelve rounds); Windows/macOS stubs remain the open half. `v0.8-roadmap.md`: Cross-cutting A → Done, the conditions ledger a11y-bridges row partial → ready (rows flip only in the PR that moves the capability). `PLATFORMS.md`: the Accessibility section records the observed pass and the harness-owned compile line; `a11y.h` documents the flush-regularly and detach contracts honestly.

### Decided — v0.8: the dual-path adjudication (Line 3, ADR-0022 — the raw surface is a declared transition state)

- No default-flip, no deprecation — **explicit keep, declared**: the afford graph is already canonical for every event class it serves (pointer discrete + continuous, keyboard focus + activation); the raw callbacks keep two declared roles — the **fallback** for graph-served classes (unresolved presses, keys that compile to nothing) and the **only surface** for the classes with no compile form (wheel — L12; non-activation keys — ADR-0020 CAVEATS; IME). `intent_graph` opt-in mechanics, defaults, and behavior are unchanged; what changed is doctrine.
- The evidence census is recorded in ADR-0022 (dated, recomputable): of 25 examples, 2 route through the graph (the evidence apps, `on_click` NULL by design), 2 windowed apps stay raw with recorded reasons (the boundary-exposing demo per ADR-0006; the perception demo), 3 event classes have no compile form. Corpus Category D stays 7/15 clean — bounded by channel coverage (L11/L12/NG-6), not by the dual-path question.
- Per-callback retirement conditions named (a callback retires to the deprecation registry only when a compile form exists AND a real example routes that class through the graph AND the corpus re-score supports it — the ADR-0019 process), plus the drift guard: **no new event class may ship raw-only**.
- Docs: `limitations.md` L16 opened (the declared state); `intent.md` gains the routing-surface doctrine table; `app.h` labels each callback's routing role at the point of use; corpus P35 note upgraded in place (the drag-begin seam ADR-0021 closed; what keeps it forced is the missing resizable-panel demo — no verdict change, distribution stays **39/23/6**); v0.8 roadmap Line 3 → Decided and the intent-compilation ledger row closes.
- Fixed in passing: `intent.md`'s one-page non-goals table still listed "continuous/transient interaction as first-class … deferred to v1.0+" — a stance that died with ADR-0018's v0.7 promotion of `px_interaction` (the partial-sync drift the v0.7 doc pass missed). The row now states the canonical NG-6 (mobile/touch) boundary instead.

### Added — v0.8: the drag-begin afford, the process form (Line 2, ADR-0021 — L15b retired)

- `px_afford_compile_process(g, x, y, button, out)`: the process form of intent compilation — an AFFORDS edge targeting a `px_interaction` resolves a pointer-down to the PROCESS (the inert-trajectory machine), not to a closure. Same relation (`PX_REL_AFFORDS`), second resolution form; same last-declared-first rule; window-free, backend-free; miss zeroes the payload. **Drag-ability becomes graph data — the begin seam is gone.**
- `px_drag_intent`: the drag-begin value — region label embedded (replay-safe after the region is freed, the same value contract as `px_pointer_intent`/`px_key_intent`); press x/y/button ride along as context (they seed the first trajectory sample), never as routing keys.
- `px_region_affords_process(g, r)`: drag-ability as a pure graph query — the reader the a11y projection and the corpus evidence use.
- `px_is_interaction(node)` / `px_is_closure(node)`: registry-backed kind predicates over void* AFFORDS targets — pointer identity, no type punning. Interactions and closures are process-global registered objects (the regions/perceptions precedent).
- `px_interaction_reset(it)`: the rearm — an AFFORDS edge points at a stable target that must survive its second drag. Terminal outcomes stay final; reset returns the machine to IDLE (trajectory + cancel reason cleared) and KEEPS everything bound (hook, commit/cancel bridges, phase estimate). No transition fires.
- `px_app_run` process routing (opt-in `intent_graph`): the down tries the process form first — a region affording both a closure and a process resolves the down to the process (the press is genuinely ambiguous; the trajectory arbitrates — a tap is a small-displacement commit whose bridge re-enters through the closure form). While active, moves SAMPLE the process (preview derived per frame from the trajectory; `on_mouse_move`/`on_mouse_up` do not fire — the process owns the gesture) and the release COMMITS it. A new press supersedes (cancels) the active process; an app-side cancel is honored — the next move/up releases the stream. Closure-only regions keep v0.7 semantics byte-for-byte.
- `PX_A11Y_STATE_DRAGGABLE`: the query-side state bit, derived from `px_region_affords_process` (never hand-set). The AT-SPI2 bridge surfaces it in the element description — atk's `AtkStateType` enum has no draggable state, so the bit rides as text beside values (the bridge's documented posture).
- `examples/designer_tools.c`: the Line 2 real-application evidence — a designer-tool palette whose drags are data-driven (three chips + slider afford processes; ONE routing rule, zero region branches, zero hand-wired begins). Ten-step script: three graph-routed drags, the dual-form chip's tap AND drag on the same region (arbitration by measure), the closure-only control, two slider drags on one process object (the reset pin), empty-space no-ops, a mid-drag cancel.
- `tests/test_v08.c` sections E–G (suite 18 → 31): the process compile + value contract (E), form orthogonality — the closure form skips process targets and vice versa, the kind predicates, the Line 1 focus-ring pins holding (F), and process reuse + the app-level pointer routing decision verbatim (G).
- `docs/decisions/accepted/ADR-0021-v08-drag-begin-afford.md`: the decision record — one relation/two forms vs a second affordance vocabulary, process-owns-the-down vs dual-fire, registry predicates vs struct tags, reset vs per-gesture allocation.

### Changed — v0.8: L15b retired; P28/P36 evidence upgraded (no verdict changes)

- `limitations.md` L15b RESOLVED (ADR-0021) — **L15 closed entire**: every channel and every action form Planex serves now routes through the AFFORDS graph.
- `ui-pattern-corpus.md`: no re-scores — P28 (drag-drop) and P36 (drag slider) stay ✅; their begin evidence upgrades from hand-wired to graph-routed (`designer_tools.c`), recorded in the Category D note (ADR-0021). Distribution stays **39/23/6**.
- `tests/test_v07.c` f3 (the dangling-edge regression): its verification vehicle is now the process-form reader — the pre-v0.8 `px_afford_at` blind cast was load-bearing in the test; the spirit (the edge names the LIVE process across a rebuild) is unchanged and now also asserts the kind-honest miss.

### Fixed — v0.8: the latent px_afford_at type confusion

- `px_afford_at` / `px_afford_compile` / `px_afford_compile_focus` / the focus-ring scan now FILTER AFFORDS targets by kind (`px_is_closure`): the pre-v0.8 code cast the first non-NULL target to `px_closure*` — its comment claimed "first closure wins" but nothing checked. An AFFORDS edge to an interaction or an estimate resolves nothing in the closure form (and vice versa) — safer for every declaration shape, and the prerequisite that made the process form possible without type confusion.

### Added — v0.8: the keyboard channel (Line 1, ADR-0020 — L15a retired)

- `px_afford_focus_first / next / prev(g, from)`: the DERIVED focus ring — a region is focusable iff it affords at least one closure; ring order is region creation order (the registry read forward; the z-order scan is the same registry read backward). Wraparound both directions; NULL/freed/unfocusable `from` is "nowhere" and resolves to the ring head; empty ring returns NULL.
- `px_key_intent`: the keyboard intent value — region label embedded (replay-safe after the region is freed, the same value contract as `px_pointer_intent`); the activating key rides along as context, not as a routing key.
- `px_afford_compile_focus(g, focused, key, out)`: the window-free activation compile. Same last-declared-first multi-edge rule as the pointer channel (one ring, one rule, two channels); miss zeroes the payload.
- `px_app_run` keyboard routing (opt-in beside `intent_graph`, compile-before-dispatch, same order as pointer-downs): Tab/Shift-Tab walk the ring and report each move via the new optional `on_focus(region_label, user)` callback (a value — the region may be freed later); Enter/Space compile the focused region's afforded closure; everything else falls back to `on_key` unchanged. NULL `intent_graph` keeps legacy key dispatch identical.
- `px_event.modifiers` + `PX_MOD_SHIFT/CTRL/ALT`: the event model gains a modifier bitmask (the missing piece that made Shift-Tab inexpressible). X11 fills it from key state (and now reports Tab under Shift as `'\t'`); headless stdin gains `t` (Tab), `T` (Shift-Tab), `e` (Enter) named-key injections; win32/cocoa leave it 0.
- `examples/palette_afford.c` keyboard session [9]–[13]: the same app driven by keyboard alone — ring walk (slider visibly absent: no discrete affordance), Enter activation of swatch-blue (the SAME closure as the pointer click — one act, two channels, the actions sort by payload shape), Shift-Tab reverse, position-free canvas clear, Space reset.
- `tests/test_v08.c` (18 tests): focus-ring semantics (A), the key-compile value contract (B), the app-level routing decision verbatim (C), channel orthogonality — both compile entry points resolve the same region to the same closure (D). Wired into all CI jobs (5/5).
- `docs/decisions/accepted/ADR-0020-v08-keyboard-channel.md`: the decision record — derived ring vs declared focus API, focus-as-Estimate deferred, keyboard-as-second-class-pointer rejected; P61 re-score.

### Changed — v0.8: P61 re-score (ADR-0020 corpus amendment)

- `ui-pattern-corpus.md` P61 (Keyboard navigation): ⚠️ forced → ✅ clean (EXAMPLE `palette_afford.c`). "Focus is transient, not state" answered: the ring is derived graph data; the focus position is interaction state reported by value. Distribution **38/24/6 → 39/23/6**; `test_completeness.c` constants updated in the same commit.
- `limitations.md` L15a retired (RESOLVED in v0.8, ADR-0020); L15b (drag-begin seam) remains open — the v0.8 Line 2 obligation.

### Fixed — v0.8: a11y.c strict compile (local-toolchain found)

- `src/a11y.c` gained `#define _POSIX_C_SOURCE 200809L` (the same header pattern every other src/ file carries): without it, `px_sleep_ms`'s `nanosleep` is an implicit declaration under `-std=c17` on GCC 14 — invisible on CI's GCC 13, an error on newer toolchains. (Sibling of the Task-12 fix that added the same macro to `a11y_bridge_atspi.c`.)

## [0.7.0] — 2026-08-30

### Added — v0.7: intent compilation promoted to the 6th canonical abstraction (ADR-0017)

- `px_pointer_intent`: the compiled-pointer value — region label embedded by value (replay-safe after the region is freed), x/y/button ride along as payload context, not routing keys.
- `px_afford_compile(g, x, y, button, out)`: the window-free compile step (pure function of registry + graph + position); miss zeroes the payload so stale data cannot leak through the fallback path.
- `px_app_desc.intent_graph`: opt-in routing in `px_app_run` — pointer-downs compile before dispatch; unresolved clicks fall back to `on_click`; NULL keeps legacy dispatch identical (zero cost when unset).
- Multi-edge resolution specified and pinned: a region with several AFFORDS edges resolves **last-declared-first** (tests/test_v07.c a7).
- `examples/palette_afford.c`: the real-application evidence — five affordances, one routing rule, zero raw-coordinate callbacks; button-3 context action discriminated in the payload; slider drag as inert trajectory + one committed write; undo through the graph.

### Added — v0.7: px_interaction promoted to the 7th canonical abstraction (ADR-0018)

- The v0.6 prototype is canonical; THE INVARIANT (inert samples, transitions-only seams) is now normative, enforced by test_v06_interaction.c section D. `publish_phase` remains the only sanctioned seam to Estimate. No signature changes.
- Corpus Category D re-scored (same-commit amendment): P24–P28, P32, P36 flip to ✅ CLEAN (EXAMPLE-grounded); P29/P37 downgrade ❌→⚠️. Distribution 31/29/8 → **38/24/6**; test_completeness constants updated (75/75 checks).
- Abstraction count 5 → 7 across all non-exempt docs; `check_stale_abstraction_count.sh` now enforces the 7-count (stale 5/five pattern + stops-at-px_loop enumeration pattern).

### Added — v0.7: Path C adoption posture (Cross-cutting B)

- Host separability is now a **stated property**: `abstraction-form.md` and `intent.md` each gain a section — the 7 canonical abstractions are portable invariants; C17 is the first host, not the ontology; if the embeddability bet fails the ideas move hosts rather than die with one (the falsifiability machinery — leak budgets, corpus, admission bar — is documents and tests, not code). The inverse is recorded with it: no ecosystem-rewrite demands — every v0.7 feature is opt-in and drop-in (the wedge the 60-year Path C failure record demands; the Intentional-Programming demand is structurally refused).

### Changed — v0.7: Closure constructor split retires the last L2 (Line 5, ADR-0019)

- `px_closure_new_with_graph(goal, kind, action, evaluation, user, graph)`: the undo graph arrives with the closure — the bind-before-trigger ordering rule is deleted from the API's grammar (the mistake is unwritable). `px_closure_bind_graph` is **deprecated** (registry entry; removal candidate v1.0), still functional through the window; the v0.6 one-time warning now guards only deliberate unbound use.
- **Aggregate L2 = 0/99 = 0%** — first zero. Retire curve: 17% (v0.4) → 3.8% (v0.5) → 1.7% (v0.6) → 0% (v0.7). The leak-budget gate holds it there.
- Examples migrated to the safe form: `undo_via_graph.c`, `palette_afford.c`, `integration_4abs.c`, `counter_perception_window.c`. Tests: `test_v07.c` section E. `UPGRADING.md` gains the v0.7 migration entries (deprecation + default-budget behavior change + additive structs).

### Added — v0.7: a11y AT-SPI2 bridge behind the query-side contract (Line 4)

- `src/a11y_bridge_atspi.c`: the Linux adapter (ATK + atk-bridge provider path) — a replaceable adapter behind the v0.6 query-side contract, not an ontology commitment. The mirror is minimal (application root + current element + alert): roles/states/names/values sync on flush; the announcement ring drains into the alert object (orca reads alert name changes). Known limits recorded in-source: values ride in the description (no AtkValue on no-op objects), flat tree, identical consecutive announcements fire once.
- `px_a11y_bridge_atspi_attach/flush/detach` (a11y.h): without `-DPX_A11Y_ATSPI` the bridge is an honest stub — attach returns NULL, no-ops are NULL-safe (test_v07 d1); zero dependency, zero regression.
- CI gains the `a11y-atspi-bridge` compile-probe job: installs atk headers and compiles the adapter under the flag — absent headers report "condition unmet" (row stays partial); present headers make the compile blocking (API drift is caught).
- `PLATFORMS.md`: Linux accessibility flips from "API + logging" to "AT-SPI2 bridge behind PX_A11Y_ATSPI + query side". `limitations.md` L9 partial-resolution: orca end-to-end verification remains the open external condition.

### Fixed — v0.7: edge lifecycle — px_undeclare (the CI-found dangling edge)

- The v0.7 push's first CI run exposed two real defects; both are fixed here.
- **Dangling AFFORDS edges**: `examples/hover_drag_interaction.c` aborted on its final affordance assertion on the Ubuntu runner while passing on Debian — the example freed and recreated the drag process but left its five AFFORDS edges pointing at the dead object; `px_afford_at` returned a freed pointer. The local pass was allocator layout luck (Debian glibc reused the freed address; the runner's glibc did not). No Planex API could express the honest fix: the graph had create/read without delete.
- `px_undeclare(g, a, kind, b)`: retires the first edge matching (a, kind, b) — the edge-lifecycle counterpart of `px_has_relation`, symmetric predicate, propagation-accounted. **Edge-lifecycle contract** stated at the declaration site: edges name endpoints by pointer, nothing cascades, the declarer retires before freeing the endpoint.
- `hover_drag_interaction.c`: the re-arm sequence now retires → frees → recreates → re-declares; the assertion tightens to the live process pointer (no address-reuse luck involved) — its second arm (`commit_reorder`) was never an AFFORDS target.
- `tests/test_v07.c` section F (3 tests): retirement semantics, sibling-edge sparing, and the dangling-edge regression pinned as the discipline.
- **CI `a11y-atspi-bridge`**: `at-spi2-atk` no longer exists on Ubuntu 24.04 (folded into at-spi2-core); the probe installs the split dev packages `libatk1.0-dev` (atk headers) + `libatk-bridge2.0-dev` (the bridge header) and gates on the `atk` / `atk-bridge-2.0` pkg-config entries — the header's on-disk layout is distro-dependent (jammy: `atk-bridge-2.0/`, noble: `at-spi2-atk/2.0/`), so pkg-config is the only reliable resolver.
- **The probe's first real compile found two portability defects in the adapter** (exactly the API-drift class it exists to catch): the source hard-coded `#include <atk-bridge-2.0/atk-bridge.h>` (jammy layout; unwritable on noble) and omitted `_POSIX_C_SOURCE` (strict c17 hides `nanosleep` from planex.h's `px_sleep_ms`). Fixed: the source includes `<atk-bridge.h>` bare — the build's pkg-config cflags resolve the layout — and defines `_POSIX_C_SOURCE 200809L` like every other source. Verified against real atk 2.52 + glib 2.80 headers: clean `-Wall -Wextra` compile; the ATK API calls themselves had zero drift.

### Added — v0.7: Estimate schema — the describable value contract (Line 3)

- `px_estimate_schema` (kind + name + optional print/equal) beside the value: opt-in via `px_estimate_set_schema` (borrowed pointer, app-owned static const; zero cost when unset). Not a type system — a describable contract: tests assert "this estimate is INT and equals 3" (`px_estimate_schema_of` + `px_value_kind_name`), values denotate kind-aware (`px_estimate_describe` — INT/DOUBLE/PERCENT/BOOL defaults or custom print), equality is kind-aware (`px_estimate_value_equal` — exact for discrete kinds, 1e-9 for DOUBLE).
- `px_a11y_set_value_estimate`: the a11y query side reads the schema for value naming — the seam the Line 4 platform bridges adapt.
- Leak-budget: the void* L1 entry gains its retirement path — the **contract half** closes (the schema declares what the value means); the **pointer half** (void* user in callbacks) stays as documented permanent C17 host cost. Estimate section re-enumerated: 27 ops, L1 4, L2 0.

### Added — v0.7: budget as contract (Line 2)

- `PX_LOOP_DEFAULT_BUDGET_MS` (16ms): every `px_loop` ships with a deadline — the feedback axiom's "instantly visible" given a number. `px_loop_set_budget(loop, 0)` is the explicit opt-out. Overruns are loud: warn-once on stderr in all builds, abort under `-DPX_DEBUG_BUDGET` strict mode; `px_loop_budget_overruns()` counts.
- Propagation accounting in every audit entry: `propagation_edges` (per-step delta of the new `px_relation_edges_walked()` monotonic counter) + `propagation_depth` (`px_derive_depth_peak()` / `px_derive_depth_reset()` — read and reset are separate ops so no query carries a side effect).
- `tests/test_v07.c` section B (4 tests); `test_v06_interaction.c` j1/j2 updated for the default (j2 renamed `budget_explicit_opt_out`).

### Changed — v0.7.0 release cut

- `PLANEX_VERSION` bumped `0.6.0` → `0.7.0`. **No git tag** — per the release convention, a release is the version constant + this changelog cut (the v0.6.0 tag was deleted at maintainer direction; tags are not part of the release process).
- `docs/concepts/state/v0.7-roadmap.md` → `docs/concepts/history/v0.7-roadmap.md` (superseded banner added; all 8 referencing docs re-pointed). New `docs/concepts/state/v0.8-roadmap.md` takes its place: keyboard affordances (L15a) → drag-begin afford (L15b) → dual-path retirement decision, with the conditions ledger carried forward at its v0.7.0 state.

## [0.6.0] — 2026-08-30

### Added — v0.6: interaction prototype (ADR-0016, proposed)

- New `px_interaction` process abstraction (prototype, not canonical): begin → sample* → commit|cancel phase machine with a trajectory ring and pure-metric accessors (position / velocity / distance / duration), built on the design invariant **sample laziness** — zero observer notifications and zero perception invocations while no one samples. Three bridges: `on_phase` hook (BEGAN / MOVED / ENDED / COMMITTED / CANCELLED transitions), Closure `on_commit` / `on_cancel` triggers, `publish_phase` converting transitions to Estimate writes. Implemented in `src/interaction.c` (~370 lines); 27 tests in `tests/test_v06_interaction.c` (sections A–F prototype, G–J v0.6 retire verification), wired into all 4 CI jobs (linux-cmake, linux-make, windows, strict-warnings). Auto-begin semantics: the first sample enters the trajectory *before* the BEGAN transition fires (the starting event belongs to the trajectory).
- New `px_region` + `px_afford_at` (prototype): global region registry with z-order scan and AFFORDS-graph affordance query — intent compilation as a graph query instead of raw-coordinate hit-testing. Implemented in `src/hit.c` (~160 lines).
- New example `examples/hover_drag_interaction.c` + `.expected` snapshot: the boundary-closure demo against `hover_drag_4abs.c` (130 mouse events → 2 estimate writes; 5 of 7 HACKs retired; cancel as a first-class outcome; swipe derivable from metrics).
- ADR-0016 (`docs/decisions/proposed/ADR-0016-interaction-prototype-option-b.md`, proposed): follows the ADR-0009 v3-prototype precedent — evidence-gathering code without touching the 5 canonical abstractions. Promotion to a 6th canonical abstraction requires its own ADR against the ADR-0011 admission bar, real-application evidence, and a Category D re-scoring.

### Added — v0.6: six audit-fix retires (aggregate L2 3.8% → 1.7%)

- `PX_EV_WHEEL` added to `window.h` + x11 (Button4/5) / win32 (WM_MOUSEWHEEL) / cocoa (scrollWheel) backends + `on_wheel` app callback.
- Closure `bind_graph` omission is now loud: one-time stderr warning when undo is enabled with no graph bound (the ordering dependency itself remains; the constructor split is the v0.7 retire path).
- Estimate: `px_estimate_predict` / `px_estimate_surprise` — Friston predictive-loop seeds; confidence gains a framework-side consumer.
- Perception: `px_perception_set_free_fn` — opt-in representamen destructor; the cache leak is retired across clear_cache / fire / free paths.
- px_loop: view-only and replay branches now invoke only the loop's bound perception (scope leak retired — feedback section L2 → 0%, was 9%); audit entries gain `iteration_ms` / `budget_ms` / `budget_exceeded`; `px_loop_set_budget` / `px_loop_budget`.
- a11y: query-side API (getters, 16-entry announcement ring, `set_verbose`) — the stable contract that platform bridges will adapt to; render fast path (row-level memcpy) in `app.c`.

### Changed — v0.6 documentation sync

- `limitations.md` L12: "not abstracted" → "prototype landed in v0.6 (canonical promotion pending)", with the open items (multi-touch / NG-6, intent gradient, Category D re-scoring, promotion ADR).
- `leak-budgets.md`: v0.6 retire summary — aggregate L2 3.8% → 1.7% (remaining: Closure `bind_graph` ordering dependency, loud since v0.6).

### Added — Abstraction-form comparative study (research report)

- New `docs/research/2025-08-28-abstraction-as-form-comparative-study.md` (289 lines) — survey of 8 alternative organizational forms (DSL, component library, pattern language, ECS, FRP, data-driven config, Kay-OOP, tagless final), 6 critique traditions (Worse-Is-Better, Simple Made Easy, Leaky Abstractions, Abstraction Inversion, Rule of Three, Over-Abstraction/End-of-Civ), 5 philosophy-driven design precedents (Winograd/Flores Coordinator, Conal Elliott Fran/Pan, Dourish/Ishii embodied UI, Friston free-energy, Kay Smalltalk), and 9 production C/UI libraries for calibration (SDL, GLib/GObject, Cairo, Wayland, libuv, GTK, Qt, Dear ImGui, Redis). **Verdict**: abstraction is the correct primary form for Planex's stated goals (intent-as-value, multi-channel denotation, cognitive-bandwidth constraint, C17 feasibility, no-AI-as-driver per ADR-0003), with three caveats — Planex must (1) explicitly distinguish "abstraction-as-typed-value" (Planex's form) from "abstraction-as-encapsulation" (rejected), (2) explicitly rebut the Rule of Three via an essence-justified vs duplication-justified ADR, (3) quantify per-abstraction leak budgets. Seven gaps identified with three tiers of recommendations (T1.1–T1.3, T2.1–T2.4, T3.1–T3.3). The 32 web searches and 7 page_reader fetches are not committed to the repo; their artifacts are kept under `/home/z/my-project/research-task14/` and `/home/z/my-project/scripts/extract_page_text.py`. Status: reference research output, not a decision.

### Added — v4 essence derivation clean-room (not yet an API; design artifact)

- New `docs/concepts/history/essence-derivation-v4-clean.md` (767 lines) — clean-room re-derivation of Planex's abstraction surface, with methodological self-audit (Part VII) against 10 constitutive demands of first-principles derivation across 6 methodological traditions (Aristotelian, Cartesian, Husserlian, Popperian, Quinean, Wittgensteinian + Brooks/Lakatos for software-specific demands). **Result: v4 meets 0 of 10 demands**; the framing is downgraded from "essence-derived" to "tradition-grounded design rationale".
- New `v4/` prototype: 8 abstractions (Intent / Relation / Closure / Interpretant / Perlocution / Estimate / Actor / Perception + Loop binding) + 9 test binaries (~133 assertions, all green, zero warnings). v4 is a verification artifact, **not** a shipping API (v0.4 unchanged).
- Audit sources (60 Wikipedia articles): `/home/z/my-project/research/firstprinciples/body/*.txt` (not committed to repo; single-source Wikipedia for this audit, deeper work would consult primary texts).

### Changed — Framing downgrade (ADR-0010)

- **ADR-0010** (`docs/decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md`) accepted: v1–v4 essence-derivation lineage is now framed as design rationale, not essence discovery. Code and API unchanged. Future ADRs/docs use "8-abstraction design proposal" not "8 essence categories"; the word "deferred" is replaced by three-state labeling (essence-but-unimplemented / not-essence / undecided). ADR-0009 remains Proposed; its implementation decisions stand, its essence-claim language is downgraded. The 10 constitutive demands of first-principles derivation (see ADR-0010 Context) become the project's institutional bar for any future re-claim of "first-principles derivation". v5 path (genuine re-derivation per Part VII.11 of v4 doc) is left open but not committed.

### Added — Documentation versioning industry survey (research report)

- New `docs/research/2025-08-28-doc-versioning-top-solutions.md` — survey of 8 foundational documentation traditions (Diátaxis, Docs as Code, SemVerDoc, ADRs, C4 + arc42, Literate Programming, RFC/PEP/TC39, SSoT/DRY), 6 platforms (MkDocs Material, Docusaurus, GitBook, Sphinx, Backstage TechDocs, Docsy), and 5 large-project case studies (Linux Kernel, Kubernetes, Python, Rust, arXiv). Confirms Planex's three-system numbering (doc version / section number / software version) is industry-standard, not a defect — arXiv, PEPs, and Rust RFCs all use the same pattern. Identifies gaps: (1) no `Stability` field on concept docs (rust-lang-style experimental/unstable/stable/legacy), (2) no machine-readable `Superseded by` cross-link on ADRs, (3) no CI link-checker, (4) no RFC track separate from ADRs, (5) no generated API reference. Three-tier recommendations provided. Status: reference research output, not a decision.

### Changed — Document versioning consistency

- Renamed `docs/concepts/essence-derivation.md` → `docs/concepts/history/essence-derivation-v1.md` (filename now matches the v1/v2/v3/v4 convention used by its successors; the doc was already referred to as "v1" everywhere but its filename and title didn't show it).
- Updated H1 title to "Essence Derivation v1" (was just "Essence Derivation").
- Updated H1 title of `docs/concepts/history/essence-derivation-v3.md` to "Planex Essence Derivation v3 — A First-Principles Audit" (was "Planex Essence Re-derivation — A First-Principles Audit"; filename had v3 but title didn't).
- Updated all cross-references to the renamed v1 file across 5 docs (ADR-0007, ADR-0010, changelog, why-four-abstractions — 2 occurrences).
- Added `docs/concepts/state/versioning.md` — authoritative reference for the three independent number systems used in Planex docs: (1) document version (`v1`, `v2`, `v3`, `v4` — Arabic, no dot, for derivation-lineage docs); (2) section number within a long-form doc (`Part I`, `Part VII` — Roman, for sections, not versions); (3) software release (`v0.4`, `v0.4.0`, `v1.0+` — Arabic, with dot, for code releases per SemVer).
- Added `> **Applies to**: v0.4` status lines to 10 concept docs that previously had no version marker (architecture, glossary, limitations, non-goals, ui-essence-layers, why-four-abstractions, alternative-perspectives, path-C-lineage, roadmap-matrix, ui-pattern-coverage). Docs without a marker are now either tied to v0.4 explicitly or noted as release-independent.

---

## [0.4.0] — 2026-08-27

### Added — Feedback as 5th essence category (ADR-0008)

- **`px_loop` abstraction** — closed-loop coupling as first-class (per essence-derivation-v2.md)
- New `src/feedback.c` (~200 lines) — binds Closure (intent side) to Perception (view side)
- New `px_loop` API: `px_loop_new`, `px_loop_free`, `px_loop_step`, `px_loop_step_view_only`
- Audit log: `px_loop_audit_count`, `px_loop_audit_get`, `px_loop_audit_clear` — each iteration records (triggered? perceived? timestamp)
- Pause/resume: `px_loop_pause`, `px_loop_resume`, `px_loop_is_paused` — interrupt the loop for batch updates / modal blocking
- Replay: `px_loop_replay(loop, n)` — re-run last n iterations for testing/debugging
- 13 new tests in `tests/test_feedback.c` — all pass
- **Closes the essence gap acknowledged in ADR-0007**. Planex now implements 5 of 5 essence categories (was 4 of 5 in v0.3).

### Added — Essence derivation v2 (ADR-0007)

- New `docs/concepts/history/essence-derivation-v2.md` — based on 6-tradition literature survey (2835 lines of research reports in `research/reports/`)
- v1 (`essence-derivation-v1.md`) marked SUPERSEDED with summary of v2 corrections
- `why-four-abstractions.md` rewritten: framing changed from "4 = 4 essence axes" to "5 essence categories: 4 implemented + Feedback (now first-class in v0.4) + 4 deferred (Embodiment, Situatedness, Affordance-as-relation, Breakdown)"
- `limitations.md` updated: L13 (Feedback gap) added then resolved by ADR-0008; L14 (deferred essence candidates) added
- ADR-0007 records the revision with full Essence Check (Q1-Q5)
- ADR-0008 records the Feedback design with full Essence Check

### Changed — Version bump

- `PLANEX_VERSION` updated from `0.1.0` to `0.4.0` (the constant was stale; actual functionality has been at 0.3 since 2026-08-26)

### v0.4 essence claim (per ADR-0007 + ADR-0008)

```
Planex implements 5 of 5 essence categories:
  ✅ State                          → Estimate
  ✅ Communication (human→machine)  → Closure
  ✅ Presentation (machine→human)    → Perception
  ✅ Relational ontology             → Relation
  ✅ Feedback / closed-loop coupling → px_loop  (NEW in v0.4)

Deferred essence candidates (acknowledged, not implemented):
  - Embodiment (Dourish)
  - Situatedness (Suchman)
  - Affordance-as-relation (Gibson)
  - Breakdown (Heidegger-Winograd/Flores)
```

---

## [0.3.0] — 2026-08-26

### Added — Perception abstraction (ADR-0005)
- **Perception promoted to 4th first-class abstraction** (ADR-0005)
- New `px_perception` API: `px_perception_new`, `px_perception_free`, `px_perception_count`
- Phase 2 runtime: `px_perception_invoke_all`, `px_perception_invoke_for_estimate`
- `px_perceptions_for_estimate` — query which perceptions depend on an Estimate
- `px_app_desc.perception` field — replaces `on_render` callback (backward compatible)
- Perception-driven real window: `counter_perception_window` demo
- `multi_perception` demo — 4 denotations of same Estimate (visual + a11y + json + log)

### Added — Undo-via-graph (ADR-0002 closed)
- New `src/undo.c` — undo stack with Relation graph-driven scoping
- `px_undo_record`, `px_undo`, `px_undo_count`, `px_undo_clear`
- `px_undo_set_enabled` / `px_undo_is_enabled` — global on/off
- `px_closure_bind_graph` — bind graph to closure for auto-snapshot
- `undo_via_graph` demo — 7 tests, proves Relation is necessary
- Undo in real window: `counter_perception_window` supports Z/Ctrl+Z

### Added — Anti-pattern tests (matrix all green)
- `antipattern_estimate` — 3 anti-patterns (time + confidence + derived)
- `antipattern_closure` — 5 anti-patterns (intent + goal + eval + lifecycle + speech acts)
- `antipattern_perception` — 4 anti-patterns (multi-denotation + pure fn + lifecycle + selective)
- **Total: 12 anti-pattern arguments proving each abstraction is necessary**

### Changed — API migration (breaking, pre-v1.0)
- `px_closure_new` signature: 6-arg to 5-arg (removed `perception` parameter)
- Closure restructured: 7 stages to 5 stages (execution side only)
- `px_perception_fn` typedef removed from Closure (moved to Perception API)
- 25 legacy 3-abstraction-era demos deleted (commit c24bbcab)
- 4 prototype demos migrated to new API (counter_denotative, calculator_denotative, counter_interactive, editor_meaning)

### Changed — Documentation
- README rewritten: "3 abstractions" to "4 abstractions mapping UI essence four axes"
- New manifesto: `why-four-abstractions.md` (replaces why-three-abstractions.md)
- `ui-essence-layers.md` — six-layer nested structure of UI essence
- `path-C-lineage.md` — Planex place in 60-year Path C history
- `alternative-perspectives.md` — four academic schools (Cognitive + Mathematical/Linguistic adopted)
- `continuous-intent-speculation.md` — future-research marker
- ADR-0001 superseded by ADR-0005
- Limitations L1 updated: "Phase 1 done, Phase 2 pending"
- Roadmap matrix: all 20 cells green
- v0.4 roadmap added

### Fixed
- `layout.c` — 13 C4244 warnings eliminated (int to float conversion)
- `closure.c` — C4244 sign-compare warning eliminated
- `counter_perception_window` — text truncation on HiDPI (window widened 256 to 320)

### Stats
- 78 tests, all passing (was 42 in v0.2.0, was 25 in v0.1.0)
- 13 build targets, all passing (was 3 in v0.2.0, was 24 with failures in v0.1.0)
- Zero warnings on MSVC /W3 + gcc -Wall -Wextra
- 5 ADRs (was 5 in v0.2.0, was 0 in v0.1.0)
- 27+ documentation files (was 26+ in v0.2.0, was 11 in v0.1.0)

---

## [0.2.0] — 2026-08-25

### Added — ADR-0005: Perception promoted to 4th abstraction
- **ADR-0005 accepted**: Perception promoted from no-op placeholder to 4th first-class abstraction
- ADR-0001 superseded by ADR-0005
- New `px_perception` struct + API (`px_perception_new`, `px_perception_free`, `px_perception_count`)
- `px_closure_new` signature changed: 6-arg to 5-arg (removed `perception` parameter)
- Closure restructured: 7 stages to 5 stages (execution side only, Norman stages 1-4 + 7)
- `px_perception_fn` typedef removed from Closure, moved to Perception API
- Phase 1: API surface complete, `px_perceptions_for_estimate` is stub

### Added — Documentation overhaul
- README rewritten: "3 abstractions" to "4 abstractions mapping UI essence four axes"
- New manifesto: `why-four-abstractions.md`
- `ui-essence-layers.md` — six-layer nested structure of UI essence
- `path-C-lineage.md` — Planex place in 60-year Path C history
- `alternative-perspectives.md` — four academic schools (Cognitive + Mathematical/Linguistic adopted)
- `continuous-intent-speculation.md` — future-research marker
- ADR template updated with mandatory Essence Check (5 questions)
- Limitations L1 updated: "Phase 1 done, Phase 2 pending"
- Non-goals updated: "three abstractions" to "four abstractions"

### Changed — Demo redesign
- 25 legacy 3-abstraction-era demos deleted (commit c24bbcab)
- New canonical demos: `counter_4abs` (4-abstraction hello world), `multi_perception` (why Perception is 4th)
- `perception_smoke` — 9 Phase 1 API tests
- `examples/README.md` catalog rewritten for post-ADR-0005 era

### Changed — Test migration
- `tests/test_core.c` migrated: 5 `px_closure_new` calls updated to 5-arg signature
- 33/33 tests still passing after migration

### Fixed
- `layout.c` — 13 C4244 warnings eliminated (int to float conversion)
- `closure.c` — C4244 sign-compare warning eliminated

### Stats
- 4 abstractions (was 3 with no-op)
- 42 tests (9 Phase 1 + 33 migrated), all passing
- 3 build targets in STDOUT_DEMOS (counter_4abs, multi_perception, perception_smoke)
- Zero warnings on MSVC /W3 + gcc -Wall -Wextra
- 5 ADRs (was 0)

---

## [0.1.0] — 2026-08-22

### Added
- Three core abstractions: Relation, Estimate, Closure
- 10 widget demos proving emergence (counter, slider, radio, dropdown, checkbox, form, wizard, modal, tabs, todo)
- Framebuffer renderer with BMP output
- 8x16 bitmap font (ASCII)
- X11 window backend with XShm optimization
- Win32 (GDI) window backend — tested on Windows 10
- Cocoa (NSWindow) window backend — code complete, untested
- Headless backend (BMP output, stdin input)
- IME support: X11 (XIM), Win32 (IMM32), Cocoa (NSTextInputClient)
- TTF font rendering via FreeType (optional)
- Color emoji support (FT_LOAD_COLOR + BGRA)
- Fallback font chain (up to 4 fonts)
- Fontconfig integration (find fonts by name)
- HiDPI / Retina support (DPI scale detection)
- Accessibility API (roles, states, announce)
- Window resize handling
- 60fps animation with auto-sampling (Behavior = Time to a)
- Derived estimates with automatic dependency tracking
- Dynamic derived sources (add/remove at runtime)
- Closure auto-evaluation (failure to FAILED + feedback)
- Closure Promise/Declare/Fail (machine-initiated status)
- Relation-driven layout helpers (beside, below, center)
- CMake build system (MSVC, GCC, Clang)
- GitHub Actions CI (Linux, macOS, Windows)
- Todo app (first real application)
- Professional documentation (Diataxis structure)

### Known Limitations
- No GPU rendering (software rasterization only)
- No clipboard / drag-drop
- No preedit rendering (IME intermediate text)
- Accessibility is API + logging (no real screen reader integration)
- Cocoa backend untested on real macOS
- No mobile (iOS/Android) support
- px_estimate_observe does not support unobserve (dangling pointers on remove)
