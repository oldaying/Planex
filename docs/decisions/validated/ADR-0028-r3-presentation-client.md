# ADR-0028: Stage R3 — the presentation client: the tree rendered from outside, addresses named and persisted, the E-series commissioning kit registered ready-not-run

## Status of This Memo

Proposed. This is the stage-R3 decision ADR-0027 §7 registered and
deliberately did not execute: "R3, when it is decided, is the
presentation-and-persistence stage." Nothing in ADR-0026 or ADR-0027
starts R3; this memo is the start, and it follows the same
registration discipline both predecessors set: every prediction in
§8 is fixed here, before any R3 source file exists.

## When to use this decision

- When the question is how the v5 semantic tree (S15) meets a screen
  WITHOUT the core gaining a visual field — the S15 contract "vision
  is one projection, executed as a CLIENT" (vol. 4, carried by
  ADR-0026 Decision 3c's vocabulary ban).
- When the cut registry's registered deferral comes due: ADR-0026
  CAVEATS — "The cut registry is capacity-bounded (registered: 64
  named cuts) and does not persist across world free; named-address
  persistence is an R2 question" — which ADR-0027 §7 in turn
  registered for R3 ("the named-address persistence the cut registry
  defers").
- When the E-series human legs need their commissioning kit — the
  preparation work "that only the maintainer can commission"
  (ADR-0027 §7) — without a single participant being run or the
  frozen slice instrument being touched.

## When NOT to use this decision

- This is not a promotion of v5 to a shipping direction. The
  promotion gate is ADR-0026 §5, executed by the maintainer on the
  E-series evidence; nothing here opens it, and the E-kit in §7 is
  explicitly ready-not-run.
- This is not an instrument change. The slice stays frozen
  (ADR-0026 Decision 2: "the slice stays frozen as the registered
  instrument"); no participant-facing interactive layer is added to
  it here, and none may be without a new instrument ADR.
- This is not a license to put pixels in the core. The visual-field
  vocabulary ban over `v5/src/` and `v5/include/` stays exactly as
  ADR-0026 registered it; R3 adds a client zone OUTSIDE those paths,
  and adds two new bans (§6) that keep the dependency direction
  one-way.
- This is not the black-box format's revision. The black-box export
  (S3) is a registered surface; the snapshot of §4 is a NEW artifact
  and the black-box bytes are asserted unchanged (F7).

## Status

**Validated.**

- **Date:** 2026-11-01
- **Deciders:** the derivation series (vol. 5 → vol. 6 → vol. 7 →
  ADR-0025 → ADR-0026 → ADR-0027 → this memo)
- **Four-column discipline:** ADR-0024 applies (this memo is
  ≥ 0024): Refutation Criteria and Regression Clause sections are
  mandatory and load-bearing.
- **Stage lineage:** R1 = the promoted core (ADR-0026); R2 = the
  mechanism layer (ADR-0027); R3 = this memo.

The enforcement this ADR ships is implemented and proven to bite:
the rebound gate's two new bans — dependency direction (`if5c_`
symbols may not enter the core) and client purity (client sources
may not submit/append/debit/undo/adopt) — each catch their own
planted synthetic violation (F11), and the E-series runbook's
ready-not-run account is enforced against the repository state at
every commit by `scripts/check_e_series_runbook.sh` (F12). F1–F12
are all PASS with the measured outcome transcribed in
`## Validation`. Promotion to **Accepted** remains a maintainer
ruling; the E-series commissioning this kit registers stays with
the maintainer.

## Context

R1 and R2 built and measured the durable core and the mechanism
layer. What they deliberately left behind — each item registered in
a NOT-CARRIED column or a CAVEATS entry, none silently dropped —
is this stage's input list:

1. **Pixel denotations.** ADR-0027 mapping (d): "pixel denotations
   (S15's contract keeps vision out of the core; a renderer is a
   client of the tree, R3's question)." The shipping tree answers
   this with `src/fb.c` (a 305-line software framebuffer: RGBA
   surface, rect fills, a 5×7 font, ASCII dump, BMP save) and the
   denotative examples — `examples/calculator_denotative.c` (759
   lines) renders a multi-region calculator from Estimates to
   pixels and compares against a committed golden `.expected` file.
   That is the precedent R3 re-derives on the intent-flow ontology:
   the render is a PURE function of state, unit-testable, with
   committed goldens — now the state is the semantic tree at a view
   instead of a cell graph.
2. **Geometric hit-testing.** ADR-0027 mapping (f): "hit-testing on
   visual geometry cannot exist in a core with no visual fields ...
   NOT CARRIED: geometric hit-testing (client-side, forever, by
   S15's contract)."
3. **Animation easing curves.** ADR-0027 mapping (a): "animation
   easing curves (presentation, R3)" — the estimate-trajectory
   mapping's two-sample special case (`from → to over duration`)
   has its core-side form (the trace along a view sequence) and
   lacks only the client-side interpolation.
4. **The cancel reason string.** ADR-0027 mapping (g): "the v5
   cancel is a cut; attaching a reason word would be a new record
   field — a successor-ADR question, registered." This memo is that
   successor.
5. **The named-address registry.** ADR-0026 CAVEATS: the registry
   is "64 named cuts" in capacity language, but the R1 registry has
   ids and no names, and nothing persists across world free. The
   deferral chain is explicit: ADR-0026 → "an R2 question" →
   ADR-0027 §7 → R3. It also warned that persistence "touches the
   black-box format, which is itself a registered surface —
   changing it is a successor-ADR matter"; §4 answers by NOT
   touching it.
6. **Sample-trajectory persistence.** ADR-0027 mapping (g): NOT
   CARRIED, "samples are working-set, by the same accountization
   that keeps region names working-set." ADR-0027 §7 conditions
   its carry on "a measured need" — which has not appeared (the
   E-series legs that could produce one are registered-not-run).
7. **The E-series human legs.** ADR-0026 §5 pre-committed the
   promotion gate to their demotion clauses; ADR-0027 §7 registered
   their preparation as R3 work. The legs themselves require
   participants and are the maintainer's to commission.

The engineering case for proceeding now is the same shape as R2's:
the mechanism layer landed with M1–M9 all PASS and V1's frozen
surface byte-identical (ADR-0027 Validation); the honest account
(the human legs are registered-not-run, the promotion gate is
vacant) is unchanged by this stage and is restated in CAVEATS. What
R3 adds is the LAST structural layer the derivation can build
without human evidence: the surface a participant would eventually
look at, the persistence an address book eventually needs, and the
kit a maintainer would eventually commission from — each built so
that NOT carrying the human claim is visible in the artifact, not
just in the memo.

## Decision

### 1. The shape: a client zone, a name column, a snapshot — additive, one-way

R3 adds three things, each additive to the v5 tree:

- **`v5/client/`** — the presentation zone. Its own sources, its
  own header (`v5/client/include/planex/if5c.h`), its own Makefile,
  its own tests and demo app. It links the core; the core never
  links it (§6's new dependency-direction ban enforces this
  statically). The ADR-0026 vocabulary ban's scan paths are
  `v5/src/` and `v5/include/`; the client zone sits outside them BY
  CONSTRUCTION, not by exemption — the ban exists to keep vision
  out of the ONTOLOGY, and the client is where the ontology's
  projections meet a surface. This is the same one-way shape the
  shipping tree already has (`src/fb.c` and `examples/` are clients
  of the core abstractions, never their storage).
- **The registry's name column** (`v5/src/if5_cut.c`, world-side) —
  `if5_cut_name` / `if5_cut_by_name`: the working-set column the
  "64 named cuts" language always implied, symmetric with region
  names (seeds are not content; names are how addresses are
  ADDRESSED, not what they contain).
- **The snapshot** (`v5/src/if5_snapshot.c`) — `if5_snapshot_save`
  / `if5_snapshot_load`: a new, versioned, canonical byte format
  carrying the durable truth (the log and arena, verbatim), the
  seeds (sources, region specs, type table, sys_sid), the receipt
  stream, the cut registry with its names, and the cursor/tick.
  The load path installs the records verbatim and rebuilds every
  cache through one pass (§4) — it never re-adjudicates, because
  the decisions are already in the records' flags, PROV, and
  clocks.

No existing v5 source changes semantically. The ONE world-struct
change is the name column (`char cut_names[IF5_MAX_CUTS]
[IF5_NAME_MAX]`), a working-set addition whose exact byte delta is
registered in §5 with its measured value (F10) — the same
discipline R2 used when its registries moved the delta from 11,776
to 12,568 with a registered reason. The black-box export format is
NOT touched: a snapshot is a new artifact, and F7 asserts the
black-box bytes are identical across a round trip.

### 2. The client, module by module — with what is honestly not carried

Each module below states the construction, what is GAINED over the
shipping precedent, and what is HONESTLY NOT CARRIED. As in
ADR-0027 Decision 2, the not-carried column is load-bearing: a
client that pretended to be the whole presentation story would be
the silent-alteration failure mode run in reverse.

**(a) The framebuffer (`if5c_fb.c`).** An RGBA surface with
deterministic primitives (fill rect, frame rect, h/v lines, a
re-derived 5×7 font, `draw_text` with foreground and background),
an ASCII dump for text-assertable tests, and a BMP writer with a
canonical header (no timestamps — BMP has none; goldens are
byte-stable). GAINED over `src/fb.c`: nothing claimed — it is the
same idea re-derived so v5 stays zero-dependency and never links
shipping code. NOT CARRIED: every font beyond the derived 5×7,
anti-aliasing, sub-pixel anything, output formats beyond BMP/ASCII.
Client policy, upgradeable without an ADR.

**(b) The layout (`if5c_layout.c`).** `if5c_layout(w, view, policy,
out)` walks the semantic tree (`if5_tree_walk`) at the view and
assigns geometry DETERMINISTICALLY: tree depth becomes layout row,
declaration order within a parent becomes column, each node's cell
carries its name, its value formatted by payload family (the three
built-in route formats — NUM decimal, SET hex, STR line — mirrored
client-side), its tier as emphasis, and its contribution count. The
S17 schedule renders as the attention margin: each entry a marker
row whose glyph encodes the entry kind (SURPRISE / BREAKER / TIER /
ARBIT / MERGE) — the surprise timetable, drawn. GAINED: the layout
is a pure function of (world, view, policy constants) — the same
view lays out the same everywhere, forever, which is what makes
golden bytes and hit-testing meaningful. NOT CARRIED — deliberately,
and this is the sharpest boundary in this memo: the five
presentation relation kinds (BESIDE, AFFORDS, WITHDRAWS_FOR,
PRESENTS_FOR, INTERPRETS_AS) that ADR-0027 mapping (b) retired from
the core are NOT resurrected as client metadata. The client derives
geometry STRUCTURALLY from the tree (depth, order); carrying
asserted layout edges client-side would resurrect, one zone over,
the exact asserted-edge pattern the cause-graph replaced. They stay
gone. If a measured need for explicit layout assertions ever
appears, that is a successor-ADR question (CAVEATS).

**(c) The hit test (`if5c_hit.c`).** `if5c_hit(layout, x, y)` →
the rid whose cell contains the point, 0 if none. This is the
discharge of ADR-0027 mapping (f)'s deferral — geometric
hit-testing, "client-side, forever" — and it is only possible
BECAUSE the layout is deterministic: the inverse of a pure function
is well-defined. NOT CARRIED: nothing. This one is the point.

**(d) The easing (`if5c_ease.c`).** The estimate-trajectory
mapping's two-sample special case, executed: `if5c_ease_frames`
interpolates between the layout at view A and the layout at view B
under a registered two-family easing set (linear and smoothstep),
as a PURE t-mapping — sample index in, frame out. GAINED: animation
with zero core vocabulary and zero nondeterminism; the frame
sequence is a function, so it is golden-testable like everything
else. NOT CARRIED: wall-clock timing (ADR-0027 mapping (e)'s
registered boundary — binding frames to milliseconds is a
deployment calibration and an E-series protocol question); frame
dropping; anything asynchronous.

**(e) The annotation table (`if5c_annotate.c`).** Cancel reasons,
the ADR-0027 mapping (g) successor question, discharged WITHOUT a
record-field change: `if5c_annotate_cut(client, cut_id, reason)`
keeps a bounded client-side table keyed by the cut registry's ids.
The cut is the semantic content (the history change); the reason is
presentation (why a human asked for it). GAINED: the question
"why was this cut made" is answerable without touching the record
word — which is the only alternative that would have preserved the
differential. NOT CARRIED: annotation durability (annotations are
client session state, not world state — a reloaded world has its
cuts and names but fresh annotations; registered in CAVEATS).

### 3. The named-address registry

`if5_cut_register` gains an optional name at registration;
`if5_cut_name(w, id)` reads it; `if5_cut_by_name(w, name)` resolves
it. Names are NUL-terminated, unique on register (a duplicate name
is refused, honestly), bounded by `IF5_NAME_MAX`, and stored in the
world's working set beside the addresses they label. Consequences,
each registered: the world struct grows by exactly
`IF5_MAX_CUTS * IF5_NAME_MAX` bytes (F10 measures and registers the
new exact working-set delta against the slice with this as the
stated reason — history bytes do not move); the R1 differential's
frozen surfaces (log bytes, three projections, black box) are
untouched; the registry remains capacity-bounded at 64 (ADR-0026's
registered bound — raising it is a successor-ADR question, not a
tuning knob).

### 4. The snapshot: verbatim install, one rebuild pass, a canonical format

**The format** (version 1, little-endian, fixed): a magic
`IF5SNAP1`; a header with version, counts, and the tick; the seed
section — sources with their budget configuration, the payload-type
table as NAMES (never function pointers: the loader demands the
application re-register the same types in the same slots and
refuses on mismatch, honestly, with the offending slot in the
error), the region specs (name, ptype, tier, seed, creation
parent), `sys_sid`, the black-box configuration; the durable
section — the log records verbatim (the same 48-byte words the
differential freezes) and the arena bytes verbatim; the audit
section — the receipt stream; the address section — the cut
registry with names; the cursor (`now`) and stats. Canonical: no
timestamps, no padding drift, no allocator-dependent bytes — the
same world saves to the same file every time (F8).

**The load path.** Create a fresh world; the application registers
its payload types; `if5_snapshot_load` verifies the type table
against the file (refusing on drift), seeds sources and regions
through the ordinary seeding channel, then installs the log and
arena VERBATIM — records already adjudicated are not re-adjudicated
(the flags, PROV edges, and vector clocks in the records ARE the
decisions; re-running the adjudicator over installed records would
be a second decision) — then runs ONE rebuild pass,
`if5__rebuild_caches`, which replays the BOOKKEEPING over the
installed log in order: knowledge clocks (`svc`, `vc_now`), ledger
balances from the packed debit words, region value caches, stats,
and the receipt stream from the audit section. F9 asserts the
rebuild reproduces the incrementally-built caches exactly.

**What the snapshot is not.** It is not the black-box format (F7
asserts those bytes unchanged). It is not a security boundary — a
well-formed but hostile file can install a lying log; the loader
validates structure (bounds, arena offsets, cut prunes in range)
and refuses the malformed, and no more is claimed. It is not a
merge tool — it replaces a world wholesale. And process SLOTS are
not persisted (ADR-0027's registered accountization: the OPENING
records are durable, slots are not; a reloaded world shows the
process history in the log and has all slots free).

### 5. The differential discipline: V1 stays the master gate

The R1 differential (both cores, TRIP-17 + seeded streams + the E2
shape, byte-compared on the frozen surface) still runs untouched in
CI. The working-set delta assertion — R1: 11,776 bytes exact;
R2: 12,568 bytes exact, reason registered — moves once more, to
the value F10 measures, reason registered here in advance: the
registry's name column (`IF5_MAX_CUTS * IF5_NAME_MAX` bytes) plus
the snapshot's bookkeeping fields if any land in the world (none
are planned — the snapshot is a module, not a world field). The
48-byte-per-change slope and the history-byte invariance are
asserted unchanged. "May extend, never alter" now covers a stage
whose entire subject is the space OUTSIDE the durable word — and
the assertion that it stayed outside is F1.

### 6. Wiring and the two new bans

`v5/client/Makefile` (self-contained, `CFLAGS_EXTRA` passthrough,
builds the client library, its test suites, and the demo app);
root `CMakeLists.txt` gains the client suites and the demo as
ctest targets (`planex_v5c_lib` never links into `planex_lib`;
neither does `planex_v5_lib` — unchanged); `ci.yml`'s v5 job family
runs the client suites and the golden check on all three OS
families; TESTING.md and the changelog record the layer; the ADR
index is regenerated with `gen_adr_index.sh --update` (the R2
drift lesson, now procedural).

The rebound gate (`check_slice_rebound.sh`) keeps every existing
ban and adds two, each with its own synthetic bite:

- **The dependency-direction ban.** `v5/src/` and `v5/include/`
  must not reference any `if5c_` symbol — the core must not depend
  on its client. The existing visual-vocabulary scan's paths stay
  exactly `v5/src` + `v5/include` (the client zone is outside by
  construction; this is registered here so a future "just extend
  the scan to v5/client" edit is a visible ADR-level change, not a
  gate tweak).
- **The client purity ban.** `v5/client/src/if5c_*.c` must not call
  `if5_submit`, `if5__append`, or `if5__post_debit` — the client
  renders, it does not speak. The demo app (`v5/client/apps/`)
  submits through the ordinary channel like any application; the
  ban scopes to the client library modules, mirroring how the
  process-sample ban scopes to `if5_process_sample`'s body.

### 7. The E-series commissioning kit — ready, not run

`measurements/e-series/RUNBOOK.md` carries one section per leg
family — E1a, E1b, E2-experimental, E3a, E3b, E3c, E4, E5a, E5b —
each with the five fields a commissioner needs, all derived from
the registered sources (vol. 6 ch. 3, quoted in ADR-0026 §5) and
NONE invented here: the registered question; the instrument (which
frozen arm/binary); the primary metric and its registered boundary;
the demotion clause verbatim; the data file schema the leg's
results land in (CSV, header-first, the `e2_residency.csv`
convention). The runbook's header states the account honestly:
zero legs claimed run, zero participants, zero data files — the
kit's completeness is F12's subject, and its emptiness is a
feature. The per-leg analyzers are NOT pre-written: their shape
depends on data the legs do not yet have, and pre-writing them
would be predicting results — the `analyze_e2_residency.sh`
convention (recompute script lands WITH the data) is cited as the
registered pattern instead.

The demo app (`v5/client/apps/if5c_calc.c`) is the
calculator-complexity validation the shipping tree did with
`examples/calculator_denotative.c`, re-derived on the ontology:
regions for the calculator's state, digits and operators as
compiled submissions through the route channel, the display as a
fold rendered through the client, hit-tested buttons, a scripted
session, and a committed golden BMP. It exists so that WHEN the
maintainer commissions a leg that needs a participant-facing
surface on v5, the surface exists and is golden-tested — it is
readiness, not an instrument (the registered instrument stays the
slice), and CAVEATS says so.

### 8. Predictions — the F-series, registered before any R3 source exists

All are machine-verifiable engineering assertions. None is a human
claim; the human legs remain the E-series' to make or break.

- **F1 (client purity).** Rendering the tree through every client
  module leaves the durable byte image unchanged (memcmp, the S24
  instrument extended) and performs zero submissions (runtime count
  + the §6 static ban).
- **F2 (renderer determinism).** The same (world, view, policy)
  renders to byte-identical framebuffers across runs, across
  processes, and across a save→load round trip.
- **F3 (layout totality).** Every view-live region receives
  exactly one cell; the layout tree equals the semantic tree's
  shape (same nodes, same parent/depth, same order) over the
  randomized differential shapes.
- **F4 (hit-test inversion).** For every region's cell, a point
  inside hit-tests to that region; every point outside all cells
  returns 0; verified over randomized layouts.
- **F5 (easing endpoints and monotonicity).** At t=0 the frame is
  byte-equal to the view-A render; at t=1, to the view-B render;
  between, every pixel channel moves monotonically toward B, under
  both registered easing families.
- **F6 (named-address round trip).** A cut registered with a name
  survives save→load with the same id, the same address bytes
  (prefix + prunes), and the same name; lookup-by-name resolves to
  the same id; duplicate names are refused at register time.
- **F7 (snapshot fidelity).** Across a round trip: log bytes, the
  three frozen projections, the black-box export, the receipt
  stream, the tree walk (node for node), the schedule (entry for
  entry), and every registered cut's fold are all identical to the
  pre-save world.
- **F8 (snapshot canonicity).** Saving a world twice yields
  byte-identical files; a reloaded world re-saves to the identical
  file (fixpoint).
- **F9 (rebuild correctness).** The one-pass cache rebuild over an
  installed log reproduces the incrementally-built world's
  knowledge clocks, ledger balances, region value caches, and
  stats, exactly.
- **F10 (the V1 surface re-verified).** The R1 differential passes
  untouched; the 48-byte-per-change slope and history bytes are
  unchanged; the working-set delta assertion updates to the exact
  measured number with §3's name column as the registered reason.
- **F11 (the gates bite).** The core visual-vocabulary scan stays
  clean with the client present; the dependency-direction ban and
  the client purity ban each catch their own planted synthetic
  violation.
- **F12 (the kit's honest completeness).** All nine leg entries
  exist with all five fields; the runbook claims zero runs, zero
  participants, zero data files — verified by the file's own
  account section matching the repository state.

## Essence Check

### Q1. Which essence axis does this decision affect?

The intent axis' last structural claim: that presentation is a
PROJECTION's client, never a stored field — S15's contract,
executed. The tradition lines are the ones the repo already sits
in. The denotational tradition names the whole approach: the
shipping examples call themselves `calculator_denotative.c`
because they render the DENOTATION of the semantic state, and R3
is that idea executed against the tree instead of the cell graph.
Sutherland's Sketchpad (1963) is the interactive-graphics
tradition's origin point for drawing-as-a-function-of-the-model.
Harel's statecharts are the classic statement of the demo's
complexity class — a calculator is a visual state machine, exactly
as the shipping example's header says. The model/view separation
is Krasner & Pope's 1988 Smalltalk-80 cookbook (the view and the
controller are clients of the model; the model carries no
knowledge of them), the immediate-mode formulation (renderer as a
pure function of application state, Muratori 2005 — the same
tradition the vol. 7 A7 trial cited for IMGUI's date) is what the
pure-layout claim instantiates, and the persistence side follows
Fowler's event-sourcing account (the log is the record; every
other structure is a rebuild) already in the shipping lineage via
ADR-0027's mapping citations.

### Q2. Does it compress or increase human cognitive bandwidth?

Unmeasured, and registered as such — the same boundary ADR-0025
CAVEATS, ADR-0026 Essence Q2, and ADR-0027 Essence Q2 drew, and
the same honesty: concept-count is not bandwidth. What can be said
on engineering evidence: the shipping presentation stack asks the
implementer to hold cells, observers, trajectories, asserted
relations, and a framebuffer protocol; the v5 client asks for a
tree walk, a policy struct, and a t value. Whether that difference
is worth anything to a human is exactly what E1b/E2-experimental/
E4 are registered to measure, on the instrument, with participants
— not here.

### Q3. Is there a gap between the claim and the implementation?

Yes, and it is itemized: §2's not-carried columns (fonts,
anti-aliasing, the five presentation relation kinds, wall-clock
timing, annotation durability), §4's non-claims (not the black-box
format, not a security boundary, not a merge tool), and §7's
ready-not-run boundary. The demo app is a validation of the client
at calculator complexity, exactly as its shipping predecessor was
— it is not a claim that the ontology renders any arbitrary
interface.

### Q4. What is the cost, and who can verify it?

~1.5k lines of client and snapshot sources, ~1.5k lines of tests,
CI minutes (client suites on three OS families). Verification is
public and mechanical: `make -C v5 test`, `make -C v5/client test`,
the golden BMP diff, the rebound gate with its two new bites,
ASAN/UBSan, and the -Wconversion family on the new sources.

### Q5. What are the counterexamples?

A layout that needs BESIDE (two regions that must sit side by side
regardless of tree shape) is not expressible without smuggling
asserted edges back — the honest answer is "not carried," and a
measured need for it is this memo's sharpest refutation signal. A
snapshot that must MERGE two worlds is not provided. An annotation
that must survive a reload is not provided. Each is registered
rather than papered over.

## Consequences

### Positive

- S15's "vision is one projection" is no longer a claim with a
  ban attached; it is a rendering path with committed goldens. The
  sharpest remaining argument against the ontology — "it has no
  surface" — is answered within the ontology's own rules.
- The cut registry becomes an address book in fact: names,
  persistence, round-trip fidelity. Undo addresses become
  referenceable the way region names already are.
- The E-series kit converts the promotion gate from "the maintainer
  would have to figure out how" to "the maintainer commissions, the
  runbook states what happens" — with zero instrument drift.
- The two new bans give the client/core boundary teeth in both
  directions: the core cannot reach the client's vocabulary, and
  the client cannot reach the core's write channel.

### Negative

- The world struct grows (the name column) — a registered,
  measured, bounded cost (F10), and the last planned working-set
  addition to the R1 core.
- The snapshot is a versioned binary format the project now owes
  compatibility discipline to (Regression Clause inherits this).
- ~3k lines of new review surface before any human evidence
  arrives; if the E-series demote the ontology, this layer lands
  in the same stall as everything above R1.

### Neutral

- The client zone's visual vocabulary is now LEGAL in one
  directory of the v5 tree; the ban's scope is unchanged, but a
  reviewer must know where the line sits (§6 registers the map).
- Goldens are committed binaries (BMP) — the same trade the
  shipping examples made with `.expected` files.

## Alternatives Considered

### Alternative 1: Render inside the core as a projection

Register an `if5_render` projection through the B2 channel and let
the "vision is one projection" clause be LITERAL — projections all
the way down. Rejected: a projection returning pixels would put
the framebuffer in the world (or its registry), i.e., visual state
one `if5_projection_invoke` away from the durable layer, and the
vocabulary ban would have to gain exemptions for the core for the
first time. The clause's own wording — "executed as a CLIENT" —
was registered by ADR-0027 §7 quoting vol. 4; taking it literally
inverts its point. The client zone keeps the ontology clean and
the render testable, which is what the clause is FOR.

### Alternative 2: Persistence by replay-through-submit

Load = re-submit the saved records through the ordinary channel on
a freshly seeded world, letting the adjudicator re-derive
everything. Rejected on honesty grounds, registered here: the
records in a saved log are ADJUDICATED (their flags encode verdicts
already rendered); re-adjudicating installed records is a second
decision over the same intent, and any future adjudicator change
would silently rewrite history on load — the exact
"extend becomes alter" failure the differential exists to prevent.
Verbatim install + one bookkeeping pass (§4) makes the load path
copy history, not re-decide it.

### Alternative 3: Build the participant-facing interactive layer now

A keyboard/mouse-driven TRIP-17 harness on the slice or on v5,
ready for participants. Rejected: the slice is the frozen
instrument (ADR-0026 Decision 2), and an interactive layer is an
instrument change requiring its own ADR; on the v5 side, building
it now would spend the promotion gate's question before the
maintainer can ask it (the gate's legs are registered on the
slice). The demo app's scripted session is the closest legal
approximation and is registered as readiness only.

### Alternative 4: Carry the five presentation relation kinds as client metadata

BESIDE et al. as a client-side table, "since the client is
presentation anyway." Rejected: it would resurrect the
asserted-edge pattern one zone over (Essence Q5's counterexample),
and it is unnecessary — the structural layout covers the demo's
needs. Registered as NOT CARRIED in §2(b) with its trigger (a
measured need) in CAVEATS.

## CAVEATS

- No human-subject claim is made or implied anywhere in this stage.
  The E-series legs remain registered-not-run; the promotion gate
  remains vacant; the demo app renders for a scripted session, not
  a person. This memo does not move the human gate one inch, and
  F12's "zero runs, zero participants, zero data files" is the
  enforced statement of that.
- The easing families are a t-mapping, not a timing system. Any
  claim of the form "60 fps" or "sub-16ms" would be a deployment
  calibration claim this memo forbids itself from making
  (ADR-0027 mapping (e)'s boundary, restated).
- Annotations do not survive reload (§2e). A measured need for
  durable annotations is a successor-ADR question — as is a
  measured need for explicit layout assertions (§2b), a registry
  capacity beyond 64 (§3), and a snapshot version 2 (§4).
- The golden BMPs pin the demo's exact geometry. A policy change
  moves them; that is what they are for, but reviewers must not
  treat a golden bump as a no-op.
- The snapshot's structural validation is not a security boundary
  (§4). Loading an untrusted file is out of contract.
- The working-set delta number moves once more (§5) with the name
  column as the registered reason. If F10 cannot attribute the
  delta to that reason exactly, the stage does not land until it
  can — the number is an assertion, not an estimate.

## Known issues

- **Issue:** the client purity ban scopes to
  `v5/client/src/if5c_*.c` by filename pattern; a file named
  otherwise inside the zone escapes the static check.
- **Why accepted:** the zone is small, reviewed, and gated at the
  Makefile level (the client library is exactly the if5c_ modules);
  the ban's bite is demonstrated synthetically (F11) and its
  escape hatch is visible in a diff.
- **Tracking:** any addition to `v5/client/src/` must either match
  the pattern or extend the gate's file list in the same commit.

- **Issue:** `if5__rebuild_caches` trusts the installed log's
  internal consistency (PROV edges in range, debit words well
  formed) up to the structural validation §4 lists; a
  well-formed-but-lying file installs a lying world.
- **Why accepted:** the snapshot is a persistence format, not a
  trust boundary — the same contract every event store has; the
  differential and the F-series keep the WRITER honest, which is
  the side this project controls.
- **Tracking:** CAVEATS registers the boundary; a future
  cryptographic-integrity extension is a successor-ADR question.

- **Issue (found at landing):** the rebuild's alphabet has an
  honest edge. A successful STANDALONE `if5_debit` (the public
  primitive) posts no record — the spent totals it moves are
  log-invisible, and no snapshot can reconstruct their
  interleaving (the receipts are the only witnesses, and only
  for the overdrafts). The same class: structural records
  submitted through `if5_submit` instead of the core's own
  creators (the C7 manual path is byte-identical only in its
  full-knowledge configuration), and a mid-history
  `if5_set_system_source` reconfiguration (which source paid
  which merge becomes unattributable).
- **Why accepted:** each case is either refused or
  authoritatively persisted, never silently wrong: the loader
  installs the persisted spent totals and REFUSES when the
  log-visible portion exceeds them; the cross-checks (stats,
  cursor, receipts count, ledger carved/sub/breaker) catch the
  shapes that misreplay. The F9 test drives the verified
  alphabet — the differential's own op set — and the boundary
  is stated where it lives, here.
- **Tracking:** a record-format change that logged standalone
  debits (a successor-ADR question — it moves the 48-byte word)
  would close the gap; the persisted-totals design makes the
  gap harmless until then.

## Refutation Criteria

This decision is falsified, in whole or in part, by any of:

1. **The differential moves.** Any byte change on the V1 surface
   (log, three projections, black box) traced to R3 — the stage
   exists to prove it can build OUTSIDE the frozen word without
   touching it.
2. **The client cannot stay pure.** A measured need to submit from
   inside the client library (not the demo, the library) that
   cannot be honestly re-expressed as "the application submits,
   the client renders" — meaning the pure-function rendering claim
   was wrong at calculator complexity already.
3. **The structural layout fails the demo.** If the calculator
   cannot be laid out readably from tree shape + policy without
   asserted layout edges, §2(b)'s sharpest not-carried claim is
   refuted at the lowest possible complexity, and the five
   relation kinds' retirement (ADR-0027 mapping (b)) inherits the
   finding.
4. **The rebuild pass diverges.** Any cache the incremental path
   builds and the rebuild pass cannot reproduce exactly (F9) means
   the "bookkeeping, not decisions" decomposition of §4 is false —
   some decision lives outside the record word, which would be a
   finding about the CORE, not the snapshot.
5. **The E-series demotion clauses fire** (ADR-0026 §5's list,
   verbatim): the promotion gate closes, R3+ stalls, and this
   layer's costs land without its benefits — the same consequence
   ADR-0026 registered for R2+, unchanged by this memo.

## Regression Clause

A successor that supersedes or deprecates this memo inherits:

1. The client/core boundary's two bans (dependency direction,
   client purity) and the registered scan paths — moving the line
   is an ADR-level change, not a gate tweak.
2. The snapshot format's compatibility discipline: version 1
   readers refuse version ≥ 2 loudly (never partially interpret);
   a format change is a successor ADR with its own migration
   story.
3. The black-box format's inviolability across save/load (F7) and
   the V1 surface's freeze (F10) — unchanged from ADR-0026 §5's
   inheritance, restated because this stage adds the most
   write-adjacent code since R1.
4. The E-series kit's ready-not-run account: a successor may add
   legs' DATA (when commissioned), but may not backfill claims of
   runs that did not happen — the account section's
   zero-runs/zero-participants/zero-files statement must match the
   repository state at every commit (F12's check, made permanent).
5. The F-series' status: superseding this memo does not retire an
   F-prediction that has already FAILED — it must be carried as a
   finding into the successor's Context, the way this memo carries
   ADR-0027's not-carried columns.

## Validation

Registered before implementation, per the series discipline
(ADR-0025 beat 1 → beat 2; ADR-0026; ADR-0027); this section is
the LANDING RUNS' transcript, filled after the fact, from the
actual gate and suite outputs. **F1–F12: all PASS.**

- **F1 (client purity): PASS.** Rendering through every client
  module (layout, render, hit, ease, dump, hash) leaves the
  durable image byte-identical (log, receipts, stats — memcmp
  around the full call sequence) and moves the submit counter
  by exactly zero. The static half: the rebound gate's client
  purity ban scans clean.
- **F2 (renderer determinism): PASS.** Two independent
  layout+render passes produce byte-identical framebuffers
  (FNV-1a over the pixel plane, plus full RGBA byte compare);
  re-rendering over the same surface is idempotent; and the
  round-trip half — the same view rendered from a RELOADED
  world — is byte-identical (test_v5c_persist).
- **F3 (layout totality): PASS.** Every view-live region receives
  exactly one cell; per-cell (name, rid, parent, depth, value,
  contrib, ptype, tier) equals the tree walk's node in the same
  order; the depth-row and pre-order-column rules hold; columns
  strictly increase; every cell sits inside the composed
  surface.
- **F4 (hit-test inversion): PASS.** 200 randomized tree shapes
  (3–9 regions, up to 3 levels, random types and tiers), 1,216
  cells, 15,280 probe points: every cell's center and four
  inward corners answer that cell's rid; six outside probes per
  layout answer 0; and a 40-point randomized sweep per layout
  agrees with a brute-force containment scan on every point.
- **F5 (easing): PASS.** Both families (linear, smoothstep):
  t=0 renders view A byte-exactly; t=1 renders view B
  byte-exactly; and across a 16-step sequence the per-channel
  distance to B never increases (monotone under exact integer
  rational arithmetic — no floating point anywhere).
- **F6 (named-address round trip): PASS.** A cut registered with
  a name survives save→load with the same id (1-based), the same
  address bytes (frontier, prefix, prunes — memcmp), and the
  same name; lookup-by-name resolves to the same id; duplicate
  names are refused at register time on BOTH the original and
  the reloaded world; unnamed entries answer "".
- **F7 (snapshot fidelity): PASS.** Across a round trip over the
  FULL op alphabet (single and batch region creation, causal and
  debited submissions, a ladder-to-breaker, concurrent merges,
  forks, arbitrations with witnesses, delegation, an undo, an
  explicit stitch, a black-box export, a reparent, a process
  commit and a process cancel, three named cuts): log bytes,
  arena bytes, the three frozen projections (at now, at a mid
  cut, at a rewound view), the black-box export, the receipt
  stream, the tree walk (node for node), the schedule (entry for
  entry), and every registered cut's fold on every region are
  identical. The strongest form also holds: the reloaded world
  CONTINUES identically — one more submission on each core
  produces byte-identical records and projections.
- **F8 (snapshot canonicity): PASS.** Saving the same world twice
  yields byte-identical files; a reloaded world re-saves to the
  identical file (the fixpoint holds).
- **F9 (rebuild correctness): PASS.** The one-pass rebuild
  reproduces, exactly: every source's knowledge clock (svc),
  vc_now, every ledger (spent_initiative, spent_surprise,
  sub_ceiling, carved, breaker_tripped), every region cache
  (cache, last_iid, head_a, head_b — white-box, the
  differential's precedent), the full stats block, and the tick
  — compared field-by-field against the incrementally-built
  pre-save world. The cursor cross-check (replayed final view ==
  persisted view) and the stats cross-check held on every load.
- **F10 (the V1 surface re-verified): PASS.** The R1 differential
  re-ran untouched: TRIP-17 + 100 seeded streams × 200 ops +
  the E2 shape, byte-identical on every compared surface; the
  48-byte-per-change slope intact; v5 residency at the E2 shape
  1.661× against the registered 3.0 boundary; and the
  working-set delta assertion moved from 12,568 to **13,336
  bytes exactly** — 64 × IF5_NAME_MAX = 768 bytes of name
  column and nothing else, the reason registered in §3 before
  the measurement (the assertion is exact, and it attributed).
- **F11 (the gates bite): PASS.** With the client present, the
  core visual-vocabulary scan stays clean over v5/src +
  v5/include; the dependency-direction ban caught its planted
  `if5c_` reference inside a copy of the core; the client purity
  ban caught its planted `if5_submit` inside a copy of a client
  module — both synthetic violations, both caught.
- **F12 (the kit's honest completeness): PASS.** All nine leg
  entries (E1a, E1b, E2-experimental, E3a, E3b, E3c, E4, E5a,
  E5b) exist with all five fields; the account section says
  READY — NOT RUN; and `scripts/check_e_series_runbook.sh`
  verifies the account against the repository state (no data
  directory, no CSVs) — wired into CI on both Linux job
  families.

### Implementation-era findings (caught before landing, recorded)

1. **The ARBIT witness carries the newcomer's path counters.**
   The first rebuild draft counted every applied record by
   re-deriving its concurrency; the tier-3 NEWCOMER's
   adjudication happened at its witness (one append later), so
   the draft double-counted it as causal — and the stats
   cross-check REFUSED the load with both totals printed
   (causal 12/11, arrivals 3/3). The old head keeps the counts
   its own submission earned; only the newcomer's counting
   moves to the witness. The cross-check design paid for itself
   on its first day.
2. **A tier-1 fork does not touch the working-set value cache.**
   The second refusal (region cache 1/2): `if5__tier1_fork`
   updates heads and last_iid but NOT `r->cache` — a forked
   region's cache stays at the causal fold until the fork
   resolves, while the FOLD is chain-selected. The divergence
   between cache and fold on forked regions is FROZEN slice
   semantics; the rebuild replicates it, it does not repair it.
3. **The tier-2 system debit is flag-witnessed.** The system's
   background debit for a merge appears in no record's debit
   word — but the CONCUR_MERGE flag PROVES it succeeded (a
   failed debit leaves the newcomer unapplied). The rebuild
   derives the system's spent from the flags: the one
   log-invisible ledger effect, and it is not invisible at all.
4. **Standalone `if5_debit` successes are genuinely invisible.**
   A successful standalone debit posts no record, no receipt —
   only the ledger total moves (Known issue 3). The snapshot
   persists the totals authoritatively; the loader refuses when
   the visible portion exceeds them. The honest boundary,
   registered rather than papered over.
5. **The attention margin's first geometry was a staircase.**
   The marks' x was read from a running max inside the placement
   loop, so each mark stepped 112px further right — a 4,944px
   wide, diagonal margin. The dimension check in the demo caught
   it; the fixed-column margin is now the registered geometry
   and the golden pins it.

### The regression battery at landing

`make BACKEND=headless CFLAGS_EXTRA=-Werror` clean; root suites
39/39; slice suites + rebound gate green; v5 suites 8/8 families
under -Werror plus the differential PASS; the client suites 3/3
plus the two-frame golden check byte-identical; ctest 27/27;
ASAN+UBSan clean on every v5 and client suite AND the demo
(golden byte-identical under sanitizers too); the six client
sources, the snapshot module, and the two touched cores compile
clean under -Wconversion -Wsign-conversion -Wshadow; every shell
gate green (build-sync, doc sections, essence admission, ADR
lifecycle, links, terms, limitations freshness, stale
abstraction counts, rebound with both R3 bites, e-series
account); the ADR index matches its generator.

## HISTORY

- **2026-11-01 — Registered (this memo).** Stage R3 decision
  registered before any R3 source exists: the client zone, the
  name column, the snapshot, the E-series kit, the F-series.
  Predecessors: ADR-0026 (R1, V1–V9 all PASS), ADR-0027 (R2,
  M1–M9 all PASS, both tracked Known issues discharged).
- **2026-11-01 — Implemented and measured.** The client zone
  (`v5/client/`: fb, font, layout, hit, ease, annotate), the
  registry's name column, the snapshot module, the calculator
  demo with two committed goldens, and the E-series runbook with
  its account gate. F1–F12 all PASS from the landing runs
  (transcribed above, including five implementation-era findings
  — two of them caught by the snapshot's own cross-checks, which
  is what the cross-checks are for). Testing 121 → 136 sections;
  ctest 21 → 27. The human gate is unmoved: zero human-subject
  claims anywhere in the stage.
- **2026-11-01 — Validated.** The enforcement is implemented and
  proven to bite: both new rebound-gate bans (dependency direction,
  client purity) catch their own planted synthetics (F11), and
  `check_e_series_runbook.sh` enforces the kit's ready-not-run
  account in CI (F12). F1–F12 PASS. Acceptance is a maintainer
  ruling; the E-series commissioning stays with the maintainer.

## References

- ADR-0024 — the four-column discipline this memo follows.
- ADR-0025 — the vertical slice: the frozen instrument, the
  measurement face, C1–C9.
- ADR-0026 — R1: the promoted core, the cut registry (§3b), the
  capacity bound and persistence deferral (CAVEATS), the promotion
  gate (§5), the visual-field ban (Decision 3c).
- ADR-0027 — R2: the seven mappings whose NOT-CARRIED columns are
  this stage's input list (Decision 2), the R3 registration (§7),
  the cancel-reason successor question (mapping (g)).
- `src/fb.c`, `examples/calculator_denotative.c`,
  `examples/calculator_denotative.expected` — the shipping
  presentation precedent this stage re-derives.
- `slice/` — the frozen instrument (ADR-0026 Decision 2); the
  E-series kit points at it and changes nothing.
- vol. 4 (矿脉图) S15/S17 — the seats the client renders; vol. 6
  (评测协议) ch. 3 — the E-series registrations the kit transcribes;
  vol. 7 (翻案书) — the A7 immediate-mode tradition line and the
  constructive duties this stage's seats discharge into a surface.
- Muratori, "Immediate-Mode GUI" (2005) — the pure-function
  rendering tradition (via vol. 7's A7 record); Krasner & Pope,
  "A Cookbook for Using the Model-View-Controller User Interface
  Paradigm" (1988) — the view-as-client separation; Fowler,
  "Patterns of Enterprise Application Architecture" (2002) —
  event sourcing: the log is the record, the rest is rebuild.
