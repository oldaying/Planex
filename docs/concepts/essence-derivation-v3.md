# Planex Essence Derivation v3 — A First-Principles Audit

> **Status**: Design analysis, not yet an ADR. Author: Super Z. Date: 2026-08-27.
>
> Triggered by the user's directive (session continuation): "Planex's current design is not necessarily correct — we need to improve the design from the standpoint of 'derivation from UI essence'."
>
> This document **does not patch docs**. It questions whether the existing 5 abstractions (Estimate / Closure / Perception / Relation / px_loop) are the right set after re-deriving UI essence from first principles — and proposes concrete API-level refactoring paths.
>
> Reads: [essence-derivation-v2.md](essence-derivation-v2.md), [why-four-abstractions.md](why-four-abstractions.md), [ui-essence-layers.md](ui-essence-layers.md), [ADR-0007](../decisions/ADR-0007-essence-derivation-v2-revision.md), [ADR-0008](../decisions/ADR-0008-feedback-as-fifth-essence-category.md), [planex.h](../../include/planex/planex.h), and the four research reports (`/home/z/my-project/research/{history,academic,cross,systems,critical}/`).

---

## Part I — Why v2's derivation was still under-evidenced

`essence-derivation-v2.md` claims to ground Planex's 5 essence categories in "6 independent literature surveys". The 6 traditions were:

1. UI early history (Sketchpad → CUA)
2. HCI theory (GOMS → Dourish)
3. Functional/reactive (Conal Elliott → re-frame)
4. Modern architecture (React → Dear ImGui)
5. Phenomenology (Heidegger → Turkle)
6. Mathematical formalization (denotational design → statecharts)

This is **better than v1's single-author derivation** — but it is still under-evidenced in three specific ways, each of which produces a measurable blind spot in the resulting essence set.

### Blind spot 1: Semiotics is absent as an independent tradition

The cross-disciplinary survey (`research/cross/`, the **1-c** report in the worklog) identified seven disciplines. v2 folded **phenomenology** in as Tradition 5 but did not give **semiotics** (Peirce, Saussure, Eco) its own tradition. Peirce's triadic sign relation — *representamen ↔ object ↔ interpretant* — is the single most important theoretical lens for "what is a UI made of" because UI is precisely *a sign vehicle that an interpreter reads as denoting some state of the world*. Folding semiotics into "phenomenology" loses the triadic structure that would have forced the derivation to ask "where is the interpretant in Planex's model?" — which is exactly the question that exposes Perception's binary design (§ III-3 below).

Concretely: Peirce's *interpretant* (the sign *generated in the interpreter's mind* by encountering the representamen) has no home in v2's 5 essence categories. It is not State (it is generated, not stored), not Communication (it is reception, not emission), not Presentation (Presentation covers the representamen side only), not Relational ontology (it is a 3-place relation, not 2-place), not Feedback (it is generated even in a one-shot read with no loop). v2 simply never asked the question.

### Blind spot 2: Cybernetics / second-order cybernetics is absent

Bateson's definition of information as **"a difference that makes a difference"** (1972, *Steps to an Ecology of Mind*) and Maturana/Varela's autopoiesis (1980, *Autopoiesis and Cognition*) are both in the 1-c cross-disciplinary survey but appear nowhere in the 6 traditions. Their absence matters because they would have forced two questions:

- *Difference that makes a difference* ⇒ information only exists when it changes a downstream state. Planex's `px_estimate_observer` fires on every `px_estimate_set`, but the loop's `px_loop_audit_entry` does not record whether the perception **actually changed the user's next intent**. The cybernetic closure (Bateson's "second difference") is missing from the audit.
- *Autopoiesis* ⇒ the system's boundary with its environment is self-constructed. UI is not a static boundary but a *self-maintaining* one. Planex's Relation graph is structural (declared once, queried forever); it has no notion of "the boundary reconfigures itself when the situation changes". This is the same gap Suchman's situatedness exposes, but from a different tradition — and **two independent traditions converging on the same gap is exactly the kind of evidence v2 claims to weight**.

### Blind spot 3: Speech-act pragmatics is collapsed to a single layer

Searle's full taxonomy (1969, *Speech Acts*) distinguishes three levels of any utterance:

- **Locutionary** — what is said (the propositional content)
- **Illocutionary** — what is done in saying it (the act: requesting, promising, asserting…)
- **Perlocutionary** — what is brought about by saying it (the effect on the hearer: persuading, frightening, reassuring…)

Winograd/Flores (1986) imported the **illocutionary** level into HCI as the 5 speech-act types that Planex's `px_intent_kind` enum directly mirrors (ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS). This is correct for the illocutionary axis. But the perlocutionary level — "this utterance *caused* the user to form intent Y" — is the level at which UI is *actually* bidirectional. A button does not merely receive a REQUEST; it **causes** the user to update their belief about what is possible next. v2 treats Searle as a source for one essence category (Communication = illocutionary) and never asks whether the perlocutionary dimension is its own essence category.

It is. The perlocution is what makes UI a *conversation* rather than a *form submission*. Without it, Closure can model "user said REQUEST(increment)" but cannot model "system's RESPONSE caused the user's next REQUEST to differ from what it would otherwise have been" — which is precisely the closed-loop essence that v2 elevates Feedback to capture. The Feedback essence and the Perlocution essence are **related but not identical**: Feedback is the structural loop (A→B→A); Perlocution is the *semantic* dimension of the loop's return path (what the system's utterance *does to* the user). v2 conflates them.

### Why these three blind spots matter together

v2's "≥3 traditions converge" threshold was supposed to filter out single-tradition claims. But the threshold only filters *out*; it cannot manufacture coverage that the sample lacks. If the sample lacks semiotics, cybernetics, and perlocutionary pragmatics, then any essence category that *only* those traditions would surface is invisible to v2 — no matter how strong the convergence.

This is a sampling problem, not a methodology problem. v2's methodology is sound. The sample is not.

---

## Part II — First-principles derivation from "UI as semantic boundary"

Forget the 6 traditions for a moment. Start from the *minimum* definition the four research reports converged on (worklog §1-e Stage Summary):

> **UI is the semantically bidirectionally-readable boundary between human / machine / world.**

That definition has three terms — **Actor** (A), **System** (S), **World** (W) — and one relation — **boundary** (B). Anything that exists as UI must be constructible from these four primitives. Let us derive, step by step, what essence categories are forced.

### Derivation 1 — There must be a "thing" that is on the boundary

A boundary that has nothing on it is not a boundary. Something must be exchanged or pointed-to across A↔S↔W. Call this the **Object** category (the thing referred-to, pointed-at, exchanged).

Planex abstraction: **Estimate**. ✓.

### Derivation 2 — The "thing" must be carried by a sign vehicle

For Actor to read state of System, the state must be *carried* across the boundary by something perceptible — pixels, sounds, haptic resistance, glyphs. Peirce calls this the *representamen*. Planex calls it the perception's output. The representamen is **not** the Object; a number rendered as "7" and the same number rendered as a bar of length 7 are different representamina denoting the same Object.

Planex abstraction: **Perception** (its `px_perceive_fn` returns the representamen). ✓.

### Derivation 3 — Reading the sign vehicle must generate an interpretant in the actor

Here is where v2 stopped too soon. When Actor encounters the representamen, an **interpretant** is generated in Actor's mind — the meaning Actor constructs. This is not stored in the system (it lives in Actor), but it is *part of the UI loop*: without it, there is no semantics, only mechanics. The interpretant is what makes the sign vehicle *read as* signifying the Object. Without an interpretant, "7" is just pixels.

**Planex abstraction: NONE.** Planex's Perception produces a representamen and stops. There is no first-class concept for "what the actor took this to mean" or even "what the system *intended the actor to take this to mean*" (system-side interpretant). This is the first missing essence.

### Derivation 4 — The actor's utterance to the system is also a sign vehicle with a perlocution

When Actor emits intent to System ("increment counter"), that utterance is itself a sign vehicle — it carries an illocutionary force (REQUEST) and a perlocutionary target (System *causes* state to change). The Closure abstraction captures the illocutionary force (5 intent kinds). It does **not** capture the perlocutionary dimension: "this closure, when triggered, causes the actor's belief about what's possible next to change in way Z". A Closure that succeeds, a Closure that fails, and a Closure that succeeds-but-the-result-was-surprising are **three different perlocutionary outcomes** that Planex currently distinguishes only via `px_closure_status` (IDLE/RUNNING/DONE/FAILED) — which is operational, not semantic.

**Planex abstraction: NONE at the perlocutionary layer.** Closure covers illocution only. This is the second missing essence.

### Derivation 5 — Boundary requires a relation, not just endpoints

The boundary is **not** a property of A or S alone; it is a relation between them, and it is **situated** (a different actor, or the same actor in a different situation, gives the boundary different meaning). v2 elevated this to "Relational ontology" — correct, but with one critical caveat v2 under-specifies: the relation is **3-place** (Actor ↔ System ↔ World), not 2-place (thing ↔ thing). Planex's `px_declare(g, a, kind, b)` is a 2-place API: it relates two `void*` pointers. There is no slot for the Actor whose situation gives the relation meaning.

**Planex abstraction: Relation, but the API is 2-place and cannot express the 3-place essence.** This is the third missing essence — or rather, the third *partially-covered* essence.

### Derivation 6 — Reading must close back to writing

Without closure, the boundary is unidirectional I/O, not a UI. When Actor reads a representamen, generates an interpretant, decides to act, and emits a new sign vehicle to System, the loop closes. v2 calls this Feedback and elevates it to essence — correct. But the closure has a *semantic* property v2 did not separate: the loop's return path is not "state changed → state perceived" but "system's utterance *did something to* the actor, which *caused* the next utterance". That *did-something-to* is the perlocution (Derivation 4) — and it lives **in** the loop, not outside it. So Feedback-as-essence and Perlocution-as-essence are structurally distinct: Feedback is the topology (loop exists), Perlocution is the semantics of the loop's return edge.

**Planex abstraction: px_loop, but only as a topology.** The audit log records `(closure_triggered?, perception_invoked?, timestamp)` — it does not record "what perlocutionary effect did this iteration have on the actor's next intent". This is the fourth essence gap, layered on top of the second.

### Derivation 7 — A boundary can break, and the breaking is essence

Heidegger's *Zuhandenheit* (1927, *Being and Time*) is the central phenomenological claim about tools: a tool in skilled use **withdraws** — it disappears from awareness, and the actor attends to the task. The tool becomes *present-at-hand* (Vorhandenheit) when it breaks — when the pen runs out of ink, the actor suddenly notices the pen instead of the writing. Winograd/Flores (1986) made this the foundation of *Understanding Computers and Cognition*: a UI that never breaks down is not a UI, because breakdown is how the user comes to *see* the UI as a UI at all. Breakdown is not failure of essence; breakdown is the **moment essence becomes visible**.

Planex's `px_loop` audit can detect "perception_invoked=false" — but that is **operational breakdown** (the loop stalled). What Heidegger means is **semantic breakdown**: the actor's interpretant no longer matches the system's representamen, the user no longer understands what the system is telling them. The representamen is still arriving; the interpretant has broken. Planex has no abstraction for this.

Worse: Planex's `explicit-abstraction` design stance (see `non-goals.md`) is in **direct tension** with Zuhandenheit. Explicit-abstraction says "the user must always be able to see and manipulate the abstractions". Zuhandenheit says "the abstractions should disappear during skilled use". These are not reconcilable as stated. This is the deepest tension in the project — and v2 did not flag it.

**Planex abstraction: NONE.** Breakdown is the fourth missing essence.

### Derivation 8 — The boundary evolves; what is useful is not what is true

Hoffman (2015, *Interface Theory of Perception*) and Friston (2010, *Free Energy Principle*) both argue — from evolutionary biology and theoretical neuroscience respectively — that the perceptual interface does not track truth, it tracks *useful fiction*. The same state can be presented many ways; the right presentation is the one that helps the actor act well, not the one that accurately mirrors state. UI that does not adapt to history, context, or skill level is an evolutionary dead-end.

`Estimate.confidence` is documented in `ui-essence-layers.md` as a placeholder for this Layer 5 functionality. Planex does not implement it. This is the fifth missing essence — but unlike the others, v2 *did* acknowledge it (as a deferred candidate under "Evolutionary"). v2 was honest here; the gap is just deferred.

### Derivation 9 — The boundary is a medium, not a window

Kay (1977, *Personal Dynamic Media*) and Engelbart (1962, *Augmenting Human Intellect*) — and Bret Victor (2011-2024, *Dynamicland*) — converge on the claim that UI's deepest essence is **medium**: a substrate for human expression and thought, not a window onto machine state. This is Layer 6 in `ui-essence-layers.md`. Planex is a widget toolkit, not a medium. v2 acknowledged this honestly as deferred.

### Summary: the essence set v3

From 9 derivations, the first-principles essence set is **9 categories**, not 5:

| # | Essence category | Status in Planex v0.4 | Source |
|---|---|---|---|
| 1 | Object / state-being-pointed-to | ✓ Estimate | Peirce object, Norman state, FRP behavior |
| 2 | Sign vehicle / representamen | ✓ Perception (fn output) | Peirce representamen, Norman feedback |
| 3 | **Interpretant** (generated meaning) | ✗ **MISSING** | Peirce interpretant, Eco reader-response |
| 4 | Illocutionary act | ✓ Closure intent_kind | Searle 1969, Winograd/Flores 1986 |
| 5 | **Perlocutionary effect** | ✗ **MISSING** (px_closure_status is operational, not semantic) | Searle 1969, Grice 1957 |
| 6 | Relational ontology (3-place) | ◐ Relation (2-place API, missing actor) | Heidegger, Gibson, Suchman, Maturana |
| 7 | Loop topology | ✓ px_loop | Bateson, CSP trace, statechart transition |
| 8 | **Semantic breakdown / Zuhandenheit-recovery** | ✗ **MISSING** | Heidegger 1927, Winograd/Flores 1986, Dourish 2001 |
| 9 | Adaptation / useful-fiction | ✗ deferred | Hoffman 2015, Friston 2010 |
| (10) | Medium-ness | ✗ deferred | Kay 1977, Engelbart 1962, Victor 2014 |

**Planex v0.4 covers 3 of 9 essence categories cleanly (1, 2, 4, 7), partially covers 1 (6), and is missing 3 entirely (3, 5, 8)** — with 2 honestly deferred (9, 10).

This is a **stricter** picture than v2's "5 of 5 implemented, 4 deferred". The difference is not a matter of opinion; it is a matter of which traditions were sampled.

---

## Part III — The five suspicious design points, examined one by one

The user enumerated six suspicious design points. The re-derivation above confirms each — but adds precision: each suspicious point maps to a specific essence gap identified in Part II.

### Suspicious point 1 — Perception is binary (state → representation), lacks Peirce's interpretant

**Essence gap**: § II Derivation 3 (Interpretant) + § II Derivation 5 (perlocution in the perception's return).

**What's wrong concretely**: `px_perception_new` takes a `px_perceive_fn` that returns `void*` (the representamen). There is no slot for:

- *System-side intended interpretant*: "we rendered '7' because we wanted the user to read it as 'seven items pending'"
- *Actual interpretant* (observed or inferred): "the user clicked the next-pending button, so their interpretant of '7' was *queue length*, not *7 of 10 progress*"

Without either, Planex cannot:

- detect a *misreading* (user's interpretant ≠ system's intended interpretant — a semantic bug, not a runtime bug)
- adapt the representamen to the actor's inferred interpretant (Layer 5)
- log the *interpretation* history, only the *perception* history (`px_loop_audit` records perception_invoked=true/false, not interpretant-constructed=true/false)

**API symptom**:
```c
typedef void* (*px_perceive_fn)(px_estimate* const* inputs, int n, void* user);
//                                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//   produces representamen. No interpretant channel.
```

### Suspicious point 2 — Closure covers only Searle illocutionary, missing perlocutionary

**Essence gap**: § II Derivation 4 (Perlocution).

**What's wrong concretely**: `px_intent_kind` has 5 values (ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS) — all illocutionary forces. `px_closure_status` (IDLE/RUNNING/DONE/FAILED) is **operational**, not perlocutionary. A closure that *succeeded technically* but produced *user surprise* is indistinguishable from one that *succeeded and produced user confidence*. The perlocutionary effect ("the user, upon seeing DONE, formed a different next-intent than they would have formed upon seeing DONE with a different feedback text") is not modeled.

Concretely: `px_closure_set_feedback(c, "Saved successfully")` and `px_closure_set_feedback(c, "Saved. 3 fields were auto-corrected.")` produce the **same** Closure struct, the **same** audit log entry, the **same** px_loop iteration — but very different perlocutionary effects on the actor. Planex cannot express this difference.

**API symptom**:
```c
void px_closure_set_feedback(px_closure* c, const char* text);
//   ^^^^^^^^^^^^^^^^^^^^^^^ — text is the representamen of feedback.
//   No perlocutionary force slot, no perlocutionary outcome type.
```

### Suspicious point 3 — Relation lacks actor dimension (inter-thing, not inter-actor)

**Essence gap**: § II Derivation 5 (3-place relation).

**What's wrong concretely**: `px_declare(g, a, kind, b)` takes two `void*`s. There is no `actor` slot. This means Planex cannot express:

- "Closure X is triggered-by actor A but observed-by actor B" (multi-user)
- "The TRIGGERS edge between Closure C and Estimate E holds *for actor A in situation S*, but not for actor B in situation S'" (situated)
- "The affordance (PX_REL_AFFORDS) of button B for actor A is 'clickable', for actor B (a screen-reader user) is 'tabbable'" (modal)

The Limitations document (`limitations.md` L2 "Situatedness") already admits this. But the API reflects the under-claim: relations are 2-place, period.

**API symptom**:
```c
px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind, void* b);
//                                  ^^^^^                 ^^^^^
//   Two void* — no actor slot. Relational-ontology essence is 3-place.
```

### Suspicious point 4 — Zuhandenheit / breakdown has no primitive (conflicts with explicit-abstraction)

**Essence gap**: § II Derivation 7 (Breakdown).

**What's wrong concretely**: Planex's `non-goals.md` declares `explicit-abstraction` as a stance — "users must always be able to see and manipulate the abstractions". Heidegger's Zuhandenheit says the opposite: a skilled user does not see the abstractions, they see the task. The two are in **direct tension**, and Planex has no primitive to mediate it.

What's needed is a way to express:

- "These abstractions are *currently withdrawn* from the actor's awareness" (in flow)
- "These abstractions are *currently present* to the actor" (in breakdown)
- The transition between the two (a breakdown event)

Without this, Planex cannot model the most fundamental UI phenomenon: the button "disappears" when the user is in flow, and "reappears" when the user can't find it. This is not a feature; it is the essence of *tool-being*.

**API symptom**: no `px_breakdown_*` API exists. No relation kind for "WITHDRAWN_FOR(actor, situation)" or "PRESENT_TO(actor, situation)". `px_loop_audit` has no entry for "interpretant construction failed (semantic breakdown) even though perception_invoked=true".

### Suspicious point 5 — Intent symbol system is closed (5-element enum, not extensible)

**Essence gap**: § II Derivation 4 (Perlocution) + the general principle that *symbolic systems for UI must be learnable and extensible* (Halliday 1978 metafunctions; 1-c cross-disciplinary survey § linguistics).

**What's wrong concretely**: `px_intent_kind` is a closed 5-element enum mirroring Winograd/Flores' 5 illocutionary types. This means:

- New illocutionary types discovered by future Searle-style taxonomy work cannot be added without breaking the ABI.
- Domain-specific illocutionary forces ("AUTHORIZE", "WITNESS", "RATIFY" in legal UI; "PRESCRIBE", "DIAGNOSE" in medical UI; "PROPOSE", "OBJECT" in deliberative UI) cannot be expressed — they all collapse to REQUEST or DECLARE.
- AI-agent intents (which per ADR-0003 Planex does not integrate, but which *as a category of essence* still need to be expressible for cross-system replay) cannot be typed.

**API symptom**:
```c
typedef enum {
    PX_INTENT_ASSERT, PX_INTENT_REQUEST, PX_INTENT_PROMISE,
    PX_INTENT_DECLARE, PX_INTENT_EXPRESS,
    PX_INTENT_COUNT
} px_intent_kind;
//   Closed enum. Cannot add "PX_INTENT_AUTHORIZE" without recompiling.
```

### Suspicious point 6 (user's list) — 6-tradition derivation sample may be insufficient

**Essence gap**: § I (Blind spots 1, 2, 3).

Already addressed in Part I. The 6 traditions missed semiotics (Peirce), cybernetics (Bateson, Maturana), and perlocutionary pragmatics (Searle level 3). Each missing tradition independently surfaces an essence category v2 did not have. **Three missing traditions → three missing essence categories (Interpretant, Perlocution, and partly Breakdown).**

---

## Part IV — Three refactoring paths

Given the 9-essence re-derivation, there are three plausible paths to bring Planex's design closer to first-principles essence coverage.

### Path A — Conservative: internal dimension augmentation (keep 5 abstractions)

Keep Estimate / Closure / Perception / Relation / px_loop. Add the missing essence categories as **new dimensions inside existing abstractions**.

| Essence gap | Path A solution |
|---|---|
| Interpretant | Add `px_perception_set_intended_interpretant(p, const char* semantics)` and a second fn type `px_interpret_fn` that the loop calls after the perceive fn, to compute "what the actor is likely to construct as interpretant given this representamen + context". |
| Perlocution | Add `px_closure_set_perlocution(c, px_perlocution_kind kind, const char* outcome_text)`. Add `px_perlocution_kind` enum: PERSUADE/INFORM/ALERT/REASSURE/FRUSTRATE/SURPRISE. `px_loop_audit_entry` gains `perlocution_kind` field. |
| Relation actor | Add `px_declare_for(g, void* a, px_rel_kind kind, void* b, px_actor* actor)`. Old `px_declare` becomes a wrapper that passes `NULL` actor (universal). |
| Breakdown | Add `px_loop_breakdown_kind` enum (FLOW/BREAKDOWN/RECOVERY) and `px_loop_mark_breakdown(loop, kind, const char* reason)`. `px_loop_audit` records breakdown transitions. No new abstraction. |
| Closed Intent | Replace `px_intent_kind` enum with `px_intent_kind` as a `const char*` string (like `px_rel_kind` could become). Provide 5 built-in constants `PX_INTENT_ASSERT_STR` etc. for the common case. |

**Pros**: minimal API surface change; backward-compatible (with `NULL` actor wrappers); preserves the "5 abstractions" tagline.

**Cons**: each abstraction now carries multiple essence dimensions — violating the orthogonality that Planex's essence-driven design is supposed to guarantee. "Closure now does illocution AND perlocution" is exactly the kind of conflation v2 criticized in v1 (where Relation was demoted to structural because v1 conflated topology with ontology).

### Path B — Moderate: 3 abstractions augmented + 1 new abstraction (Breakdown)

Keep Estimate / Closure / Perception / Relation / px_loop, but **split** Closure and Perception along their essence dimensions, and **add** a 6th abstraction for breakdown.

| Abstraction | Path B change |
|---|---|
| Estimate | Unchanged. Already covers Object essence. |
| Closure | Split into Illocution (current `px_closure_*`) and a new `px_perlocution` API attached to a Closure, representing "what this closure does to the actor's mental state when it completes". Perlocution has its own enum (`PX_PERLOC_PERSUADE` etc.) and its own audit field. |
| Perception | Add a parallel `px_interpretant` API. A Perception produces a representamen; an Interpretant (attached to a Perception or standalone) computes the actor's likely interpretation. `px_loop` calls both: perceive → interpret → audit records both. |
| Relation | Add `px_actor` struct + `px_declare_for(g, a, kind, b, actor)` API. Old `px_declare` becomes `px_declare_for(g, a, kind, b, NULL)`. Add relation kind `PX_REL_WITHDRAWS_FOR` (Zuhandenheit) and `PX_REL_PRESENTS_FOR` (breakdown). |
| px_loop | Audit entry gains `perlocution_kind`, `interpretant_constructed` (bool), `breakdown_transition` (enum). `px_loop_mark_breakdown` API. |
| **NEW: Breakdown** | 6th abstraction. A `px_breakdown` represents a transition: (from-state, to-state, reason, actor, situation, recovery_path). `px_breakdown_record(b, kind, actor, reason)` pushes onto a per-actor breakdown log. `px_breakdown_recover(b)` records recovery. Distinguished from `px_loop_audit` because breakdown is semantic (the actor stopped understanding), audit is operational (the loop ran / didn't run). |
| Intent | `px_intent_kind` becomes `const char*` (open symbol). Built-ins: `PX_INTENT_ASSERT` etc. as `extern const char* const`. |

**Pros**: each abstraction now maps cleanly to one essence category; orthogonality preserved; Breakdown gets first-class treatment matching its essence weight (Heidegger-Winograd/Flores-Dourish all treat it as essence, not derived).

**Cons**: 6 abstractions instead of 5; "5 essence" tagline breaks (becomes "6 essence + 3 partial + 2 deferred" or similar); Breakdown is a significant new API surface (~15 functions); the `px_actor` struct must be designed carefully to avoid becoming a dumping ground.

### Path C — Radical: 8 abstractions matching the 8 essence categories one-to-one

Each essence category gets its own abstraction. Estimate stays (Object). Perception splits into SignVehicle + Interpretant. Closure splits into Illocution + Perlocution. Relation gets the actor parameter. px_loop stays (Loop topology). Breakdown is added. Adaptation is added (deferred in v2, promoted here). Medium stays deferred.

**Pros**: maximum orthogonality; each essence maps to one abstraction; cleanest claim ("we implement 8 of 10 essence categories, 2 deferred").

**Cons**: 8 abstractions in C17 for embedded/desktop is heavy. Planex's `non-goals.md` explicitly rejects "kitchen-sink API" (NG-3). This path may violate Planex's own scoping.

### Recommendation: Path B

Path A under-claims (conflates essence dimensions inside one abstraction — the same flaw v1 had). Path C over-claims (8 abstractions is heavy for Planex's scope and contradicts non-goals NG-3). **Path B is the right tradeoff**: each essence category that Planex chooses to implement gets a first-class abstraction; the conflation problems of Path A are avoided; the scope of Path C is rejected.

Concretely, Path B:

- Keeps the **5 existing abstractions** with their essence claims intact.
- Splits **Closure** and **Perception** along essence dimensions they currently conflate.
- Adds **Breakdown** as a 6th abstraction — the only new abstraction, justified by 4 traditions converging on it (Heidegger, Winograd/Flores, Dourish, Suchman) — exactly v2's "≥3 traditions" threshold.
- Promotes `px_actor` to a first-class struct (used by Relation, Breakdown, and the perlocution/interpretant APIs) — without making "Actor" itself an abstraction (Actor is a parameter, not an abstraction).
- Opens `px_intent_kind` from enum to `const char*` — fixing the closed-symbol problem without changing the data model.
- Leaves Adaptation and Medium as deferred essence candidates (matching v2's honesty).

---

## Part V — Concrete API diff for Path B

This section shows the *exact* API changes Path B would introduce, against the current `include/planex/planex.h`. Not yet a patch — a design sketch for review.

### V.1 — `px_actor` (new first-class struct, not an abstraction)

```c
/* ============================================================
 * Actor — first-class struct (not a 6th abstraction)
 *
 * The Actor is the human (or AI agent) whose situational relation
 * to the system gives the boundary meaning. Per Suchman, Heidegger,
 * Maturana: UI cannot be defined without the actor. But the actor
 * is not itself an abstraction — it is a parameter that Relation,
 * Breakdown, Perlocution, and Interpretant take.
 * ============================================================ */

typedef struct px_actor px_actor;

px_actor* px_actor_new(const char* id, void* user_data);
void      px_actor_free(px_actor* a);
const char* px_actor_id(const px_actor* a);
void*     px_actor_user_data(const px_actor* a);
```

### V.2 — Relation (augmented; 2-place API preserved as wrapper)

```c
/* Existing 2-place API preserved as wrapper (actor=NULL = universal). */
px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind, void* b);
/* Becomes: */
px_relation* px_declare_for(px_graph* g, void* a, px_rel_kind kind,
                            void* b, px_actor* actor);
/* Backward-compat macro: */
#define px_declare(g, a, kind, b) px_declare_for(g, a, kind, b, NULL)

/* New relation kinds for Zuhandenheit/breakdown (replaces the
 * implicit "PX_REL_AFFORDS handles everything" approach): */
typedef enum {
    PX_REL_BESIDE,
    PX_REL_DEPENDS_ON,
    PX_REL_TRIGGERS,
    PX_REL_VARIES_WITH,
    PX_REL_AFFORDS,
    PX_REL_CONTAINS,
    PX_REL_WITHDRAWS_FOR,   /* a is withdrawn from actor (in flow)   */
    PX_REL_PRESENTS_FOR,   /* a is present to actor (in breakdown)   */
    PX_REL_INTERPRETS_AS,  /* a is interpreted by actor as meaning b */
    PX_REL_COUNT
} px_rel_kind;

/* Query: all relations of `kind` that hold for `actor` in current
 * situation. NULL actor = universal. */
px_node_list px_query_for(px_graph* g, void* node, px_rel_kind kind,
                           px_actor* actor);
```

### V.3 — Closure (augmented with perlocution, not split)

To keep API count manageable, Path B keeps Closure as one abstraction but adds a **perlocution sub-API**. This is a small conflation (Path A-style) accepted as a tradeoff for not exploding to 8 abstractions.

```c
/* New: perlocutionary force — what the closure does to the
 * actor's mental state upon completion. Distinct from
 * px_closure_status (operational) and from px_intent_kind
 * (illocutionary force of the actor's input). */
typedef enum {
    PX_PERLOC_UNSPECIFIED = 0,
    PX_PERLOC_INFORM,      /* "now you know X"           */
    PX_PERLOC_PERSUADE,    /* "now you should believe X"   */
    PX_PERLOC_REASSURE,    /* "now you need not worry"    */
    PX_PERLOC_ALERT,       /* "now you should attend"      */
    PX_PERLOC_FRUSTRATE,   /* "now you may give up"        */
    PX_PERLOC_SURPRISE,    /* "now you should re-evaluate" */
    PX_PERLOC_COUNT
} px_perlocution_kind;

/* Set the perlocutionary force + free-text outcome. The system's
 * "Saved successfully" and "Saved. 3 fields were auto-corrected."
 * would both be PX_PERLOC_INFORM but with different outcome_text.
 * A "Validation failed" would be PX_PERLOC_ALERT with the reason. */
void  px_closure_set_perlocution(px_closure* c,
                                  px_perlocution_kind kind,
                                  const char* outcome_text);

px_perlocution_kind px_closure_perlocution_kind(const px_closure* c);
const char* px_closure_perlocution_text(const px_closure* c);
```

### V.4 — Perception (augmented with interpretant, not split)

Same tradeoff: keep Perception as one abstraction, add an interpretant sub-API.

```c
/* New: system's intended interpretant — what the system *wanted*
 * the actor to take this representamen to mean. Optionally, an
 * interpret_fn can compute a predicted actual interpretant given
 * the actor's history (Layer 5 hook). */

typedef void* (*px_interpret_fn)(void* representamen,
                                  px_actor* actor,
                                  void* user);

void  px_perception_set_intended_interpretant(px_perception* p,
                                               const char* semantics);

/* Optional: register a function that predicts the actor's actual
 * interpretant from the representamen + actor. NULL = no prediction
 * (Layer 5 not implemented). */
void  px_perception_set_interpret_fn(px_perception* p,
                                      px_interpret_fn fn,
                                      void* user);

const char* px_perception_intended_interpretant(const px_perception* p);
```

### V.5 — Intent (open symbol system)

```c
/* Old: closed enum.
 * New: open symbol. Built-ins provided as extern const. */
extern const char* const PX_INTENT_ASSERT;
extern const char* const PX_INTENT_REQUEST;
extern const char* const PX_INTENT_PROMISE;
extern const char* const PX_INTENT_DECLARE;
extern const char* const PX_INTENT_EXPRESS;

typedef const char* px_intent_kind;   /* was: enum */

/* For domains needing custom intents (legal UI: AUTHORIZE/WITNESS;
 * medical UI: PRESCRIBE/DIAGNOSE; deliberative UI: PROPOSE/OBJECT):
 * the caller passes any string. */
typedef struct {
    px_intent_kind kind;   /* now a const char* */
    void*          payload;
    size_t         payload_size;
} px_intent;

/* Built-in intent kind strings (defined in closure.c). */
/* Old enum values become: */
/*   PX_INTENT_ASSERT   == "ASSERT"   */
/*   PX_INTENT_REQUEST  == "REQUEST"  */
/*   etc. */

/* Comparison helper (since strcmp is needed, not ==): */
bool px_intent_kind_eq(px_intent_kind a, px_intent_kind b);
```

### V.6 — px_loop (augmented audit; breakdown integration)

```c
/* Extended audit entry — captures perlocution + interpretant +
 * breakdown transition, not just trigger+perceive. */
typedef struct {
    bool   closure_triggered;
    bool   perception_invoked;
    bool   interpretant_constructed;   /* NEW */
    px_perlocution_kind perlocution_kind;  /* NEW */
    int    breakdown_transition;        /* NEW: 0=none, +1=entered, -1=recovered */
    double timestamp_ms;
} px_loop_audit_entry;

/* Remaining API unchanged: px_loop_new, px_loop_step, etc. */
```

### V.7 — Breakdown (new 6th abstraction)

```c
/* ============================================================
 * Breakdown — 6th abstraction (Path B)
 *
 * Per Heidegger Zuhandenheit/Vorhandenheit, Winograd/Flores
 * breakdown-recovery, Dourish embodiment, Suchman situatedness:
 * a UI that cannot break down is not a UI. Breakdown is the
 * moment the boundary becomes visible to the actor.
 *
 * This abstraction records semantic breakdown — the actor's
 * interpretant no longer matches the system's representamen.
 * Distinguished from operational loop stall (which px_loop
 * audit captures via perception_invoked=false).
 *
 * A Breakdown is *per actor*: A's breakdown is not B's. A
 * Breakdown has a *recovery path*: how the actor (or the
 * system on the actor's behalf) restores the interpretant.
 * ============================================================ */

typedef struct px_breakdown px_breakdown;

typedef enum {
    PX_BD_NONE = 0,
    PX_BD_INTERPRETANT_MISMATCH,  /* actor misread representamen */
    PX_BD_AFFORDANCE_LOST,          /* tool stopped withdrawing    */
    PX_BD_LOOP_STALL,               /* semantic loop broke         */
    PX_BD_SITUATION_SHIFT,          /* situation changed, old
                                       relations no longer hold    */
    PX_BD_COUNT
} px_breakdown_kind;

/* Record a breakdown for `actor`. `reason` is free text.
 * `related` is an optional pointer to the closure/estimate/relation
 * that the breakdown concerns. */
px_breakdown* px_breakdown_record(px_actor* actor,
                                    px_breakdown_kind kind,
                                    const char* reason,
                                    void* related);

/* Mark recovery: the actor's interpretant has been restored
 * (by explanation, by undo, by system adaptation). */
void  px_breakdown_recover(px_breakdown* b, const char* how);

/* Query the per-actor breakdown history. */
int   px_breakdown_count(px_actor* actor);
px_breakdown* px_breakdown_get(px_actor* actor, int idx);

/* Convert a Breakdown into a Relation-graph declaration:
 *   px_breakdown_to_relation(b, g, node)
 *     declares: PX_REL_PRESENTS_FOR(node, actor) — the node
 *     is now present-to-hand for this actor (it has broken down).
 * This is the bridge between the Breakdown abstraction and
 * the existing Relation abstraction. */
void  px_breakdown_to_relation(px_breakdown* b, px_graph* g, void* node);
```

### V.8 — Summary of API change sizes

| Component | New functions | Changed signatures | New types | LOC estimate |
|---|---|---|---|---|
| px_actor | 4 | 0 | 1 struct | ~80 |
| Relation | 2 | 1 (px_declare→px_declare_for + macro) | 3 enum values | ~120 |
| Closure (perlocution) | 3 | 0 | 1 enum | ~100 |
| Perception (interpretant) | 4 | 0 | 1 fn type | ~120 |
| Intent | 1 helper | 1 (kind: enum→str) | 0 | ~40 |
| px_loop audit | 0 (struct extension only) | 1 struct field extension | 0 | ~30 |
| Breakdown | 5 | 0 | 1 struct + 1 enum | ~250 |
| **Total** | **19** | **3** | **4 types** | **~740 LOC** |

This is a *moderate* API delta — comparable to ADR-0008's `px_loop` introduction (~200 LOC). The biggest single piece is Breakdown.

---

## Part VI — ADR-0009 draft (sketch)

```markdown
# ADR-0009: Re-derivation from UI essence — Path B refactoring

## Status
Proposed (not yet Accepted). Date: 2026-08-27.

Supersedes: the "5 of 5 essence categories implemented" claim in
ADR-0008. ADR-0008's `px_loop` decision stands; what changes is the
essence *coverage* claim.

## Context

v2's 6-tradition derivation (essence-derivation-v2.md) sampled UI
history, HCI theory, FRP, modern architecture, phenomenology, and
mathematical formalization. v2's "≥3 traditions converge" threshold
filtered out single-tradition claims.

A first-principles re-derivation from "UI is the semantically
bidirectionally-readable boundary between human/machine/world"
(see essence-derivation-v3.md) surfaces 9 essence categories,
not 5. The 4 categories v2 missed correspond to 3 traditions v2
did not sample: semiotics (Peirce interpretant), perlocutionary
pragmatics (Searle level 3), and second-order cybernetics (Bateson
"difference that makes a difference", Maturana autopoiesis).

The 4 missed categories:
1. Interpretant (Peirce) — generated meaning, not stored state
2. Perlocution (Searle) — effect-of-utterance, distinct from
   illocutionary force
3. Breakdown (Heidegger-Winograd/Flores-Dourish-Suchman) — semantic
   loop stall, distinct from operational stall
4. Relational-ontology-with-actor (Situatedness) — 3-place relation,
   not 2-place

## Decision

### D1. Path B (moderate) refactor — see essence-derivation-v3.md § V

- Keep 5 existing abstractions.
- Augment Closure with perlocution sub-API.
- Augment Perception with interpretant sub-API.
- Augment Relation with px_actor parameter (3-place).
- Add px_breakdown as 6th abstraction.
- Open px_intent_kind from enum to const char*.
- Extend px_loop_audit_entry with perlocution + interpretant +
  breakdown fields.

### D2. Essence claim revised

OLD (ADR-0008): "Planex implements 5 of 5 essence categories."

NEW (this ADR): "Planex implements 6 of 9 essence categories:
Object (Estimate), Sign-vehicle (Perception), Illocution (Closure),
Relational-ontology (Relation, 3-place augmented), Loop-topology
(px_loop), Breakdown (px_breakdown, new). 3 essence categories
are partially covered: Interpretant (Perception sub-API, requires
interpret_fn implementation), Perlocution (Closure sub-API,
requires perlocution tracking in audit). 2 essence categories
are deferred: Adaptation (Hoffman/Friston), Medium
(Kay/Engelbart/Victor)."

### D3. Relation-with-actor API

px_declare → px_declare_for (with macro back-compat). Old
2-place semantics preserved as "universal" (actor=NULL).

### D4. Breakdown as 6th abstraction

Justified by 4-tradition convergence (Heidegger, Winograd/Flores,
Dourish, Suchman) — exceeds v2's ≥3 threshold.

### D5. Intent symbol system opened

px_intent_kind: enum → const char*. ABI break, but Planex is
pre-v1.0 per ADR-0008 constraints.

## Scope

This ADR sketches the design. Implementation requires:
- Update planex.h per § V diff
- Implement px_actor.c, breakdown.c (~330 LOC)
- Augment closure.c, perception.c, relation.c, feedback.c
- Add tests/test_breakdown.c, test_perlocution.c, test_interpretant.c
- Update why-four-abstractions.md → why-six-abstractions.md (rename)
- Update essence-derivation-v2.md → essence-derivation-v3.md
- Update limitations.md, ui-essence-layers.md, non-goals.md
- Update path-C-lineage.md if Breakdown's lineage needs to be
  documented (it does — Heidegger 1927 → Winograd/Flores 1986 →
  Dourish 2001 → Suchman 1987 → Planex v0.5)

## Consequences

### Positive
- Essence claim matches first-principles derivation.
- Zuhandenheit-breakdown tension with explicit-abstraction is
  mediated: abstractions are explicit *and* can be marked withdrawn
  (in flow) or present (in breakdown). The two stances reconcile.
- Intent symbol system becomes extensible — domain UIs (legal,
  medical, deliberative) can express their own illocutionary forces.
- Relation becomes truly 3-place, matching the relational-ontology
  essence v2 itself elevated.
- Planex can express perlocution — the difference between
  "Saved successfully" and "Saved. 3 fields were auto-corrected."
  becomes semantically typed, not just textually different.

### Negative
- 6 abstractions breaks the "5 abstractions" tagline. (But the
  tagline was wrong; better to break it than to keep the over-claim.)
- ~740 LOC of new code; ~3-4 weeks of implementation.
- ABI break on px_intent_kind.
- px_actor struct must be designed carefully or it becomes a
  dumping ground (cf. React Context).

### Neutral
- ADR-0008's px_loop is unchanged in shape; only its audit
  entry struct gains fields.
- Existing demos work via the back-compat macro for px_declare.

## Alternatives Considered

### A1. Path A (conservative — no new abstraction)
Rejected: conflates essence dimensions inside existing abstractions.
This is the exact flaw v1 had (demoting Relation to structural
because v1 conflated topology with ontology). v2 corrected it
for Relation; doing the same conflation for Closure/Perception
repeats v1's mistake.

### A2. Path C (radical — 8 abstractions)
Rejected: 8 abstractions in C17 for embedded/desktop is heavy.
Violates non-goals NG-3 (no kitchen-sink API). The marginal
essence gain over Path B is small (only Adaptation and Medium
become first-class, both of which v2 honestly deferred and which
have no implementation pressure).

### A3. Don't refactor; keep v2's claim
Rejected: v2's "≥3 traditions" threshold was applied to an
under-sampled tradition set. The first-principles re-derivation
exposes 4 missed essence categories — keeping v2's claim is
the same over-claim pattern v2 itself criticized in v1.

## References
- essence-derivation-v3.md (this design analysis)
- essence-derivation-v2.md (the v2 derivation this supersedes)
- ADR-0007 (v2 essence framing — superseded)
- ADR-0008 (px_loop — stands, audit struct extended)
- ui-essence-layers.md (Layers 4 and 6 — Breakdown addresses
  Layer 4 first-class)
- non-goals.md NG-3 (kitchen-sink API rejection — Path B respects)
- limitations.md L2 (Situatedness gap — px_actor + 3-place
  Relation closes this gap)
- limitations.md L3 (anti-pattern tests — Breakdown provides
  a new anti-pattern: "Planex can detect semantic breakdown,
  React cannot model it at all")

## See also
- ADR-0003 (no AI integration) — unchanged. Path B's perlocution
  and interpretant are non-AI; they structure the channel that
  a future AI agent could use, without requiring AI.
- path-C-lineage.md — must be updated to include the
  Heidegger → Winograd/Flores → Dourish → Suchman → Planex
  breakdown lineage.
```

---

## Part VII — Open questions this re-derivation surfaces

1. **Is "5 essence + 1 partial + 4 deferred" (v2) or "6 implemented + 3 partial + 2 deferred" (Path B / v3) the right count?**
   The honest answer is: **both are projections of the same underlying reality through different tradition-samples**. v2 sampled 6 traditions and got 5 essence. v3 starts from first principles + adds 3 traditions v2 missed and gets 9 essence. The right move is to **acknowledge the sample-dependence**: essence counts are not metaphysical facts, they are convergences over sampled traditions. Future v4 could sample more traditions and find more essence categories.

2. **Does Breakdown as 6th abstraction violate explicit-abstraction?**
   It actually *resolves* the tension. With Breakdown first-class, an abstraction can be `PX_REL_WITHDRAWS_FOR(actor)` (in flow, hidden) or `PX_REL_PRESENTS_FOR(actor)` (in breakdown, visible). Explicit-abstraction says "users must be able to see and manipulate the abstractions" — yes, *when they need to*. Breakdown says *when* they need to. The two stances are complementary, not contradictory.

3. **Should Perlocution be its own abstraction (Path C) or a sub-API of Closure (Path B)?**
   Path B conflates them slightly (Closure does illocution AND perlocution). Path C splits them. The tradeoff: Path B keeps API surface smaller; Path C is more orthogonal. **Recommendation**: start Path B; if perlocution grows complex (e.g., perlocution chains, perlocution timeouts), promote to abstraction in v0.6.

4. **Should Interpretant be its own abstraction or a sub-API of Perception?**
   Same tradeoff as Q3. Path B sub-API; promote in v0.6 if needed.

5. **Is `px_actor` a struct or an abstraction?**
   A struct, parameterized into Relation/Breakdown/Perlocution/Interpretant. Treating Actor as an abstraction would make it a 7th essence category — tempting (Suchman, Maturana both treat actor as essence) but probably over-claiming for Planex's scope. **Recommendation**: keep as struct; revisit if multi-user UI demand surfaces.

6. **What about Adaptation (Hoffman/Friston) — is it really deferred?**
   v2 deferred it. v3 also defers it. The honest answer: Planex's `Estimate.confidence` is a stub for this essence category. A future v0.6 could promote it. **No design pressure to do so now.**

7. **Does the re-derivation affect ADR-0003 (no AI integration)?**
   No. The new essence categories (Interpretant, Perlocution, Breakdown) are all *non-AI* — they are about structuring the semantic channel. A future AI agent could *populate* the interpretant or *predict* the perlocution, but the essence structure exists without AI. ADR-0003 stands.

8. **Should the 6-tradition sample be re-run as an 8-tradition sample (adding semiotics + cybernetics)?**
   This is the cleanest validation. **Recommendation**: commission a 2-agent research sprint (semiotics tradition: Peirce/Saussure/Eco; cybernetics tradition: Bateson/Maturana/Varela/Beer) and re-run the convergence analysis. If they surface the same 3 missed categories (Interpretant, Perlocution partly, Breakdown), v3 is validated. If they surface *additional* categories, v4 is needed.

---

## Part VIII — What this document deliberately does NOT do

- Does not change code. Path B is a *proposal*, not a patch.
- Does not claim the 9-essence set is final. It is a re-projection through a wider tradition sample.
- Does not hide the tradeoff: Path B conflates illocution+perlocution and representamen+interpretant inside single abstractions. This is a deliberate compromise to avoid Path C's API bloat.
- Does not claim to resolve the explicit-abstraction vs Zuhandenheit tension *philosophically*. It resolves it *engineerically* (via the WITHDRAWS_FOR/PRESENTS_FOR relation kinds and the Breakdown abstraction). The philosophical tension may still be a real disagreement.
- Does not force the user to accept Path B. The user should read this document, decide whether the 9-essence re-derivation is convincing, and if so, decide whether Path B's tradeoffs are acceptable. If not, Path A or Path C remain on the table.

---

## Next actions for the user

1. **Read this document end-to-end** (~25 min).
2. **Decide**:
   - Is the 9-essence re-derivation convincing? (If not, where does it over-reach?)
   - Is Path B the right tradeoff? (If not, which path?)
3. **If yes to both**: the next step is to draft the actual code patch (`planex.h` diff + new files `actor.c` / `breakdown.c` + augmented `closure.c`/`perception.c`/`relation.c`/`feedback.c` + new tests). Estimated 1-2 sessions of code work.
4. **If unsure**: run a 2-tradition validation sprint (semiotics + cybernetics, ~1 agent session) to validate the 3 missed categories are real and not artifacts of single-author re-derivation.
5. **If the user wants to push deeper**: the next layer of questioning is whether even the 9-essence set is sample-dependent — i.e., whether essence *counts* are metaphysical facts or projections through sampled traditions. This is a methodology question, not a design question, and may warrant its own write-up.

---

## Appendix A — Quick-reference: essence category mapping across v1, v2, v3

| Essence category | v1 status | v2 status | v3 status (this doc) |
|---|---|---|---|
| 1. Object / state | essence (Estimate) | essence (Estimate) | essence (Estimate) |
| 2. Sign vehicle / representamen | essence (Perception) | essence (Perception) | essence (Perception fn output) |
| 3. Interpretant | — | — | **essence, MISSING** |
| 4. Illocution | essence (Closure) | essence (Closure) | essence (Closure intent_kind) |
| 5. Perlocution | — | — | **essence, MISSING (status≠perlocution)** |
| 6. Relational ontology | structural (v1: topology) | essence (Relation, 2-place) | essence (Relation, needs 3-place) |
| 7. Loop / Feedback | — | essence (px_loop) | essence (px_loop) |
| 8. Breakdown / Zuhandenheit | — | deferred | **essence, MISSING (Path B: new px_breakdown)** |
| 9. Adaptation | — | deferred | deferred (Estimate.confidence stub) |
| 10. Medium-ness | — | deferred | deferred |

**Counts**: v1 = 3 essence + 1 structural. v2 = 5 essence + 4 deferred. v3 = 9 essence + 2 deferred, of which 3 are missing and 1 is partial.

---

## Appendix B — The 5 suspicious points, mapped to essence gaps

| User's suspicious point | Essence gap (v3) | Path B fix |
|---|---|---|
| 1. Perception binary, lacks interpretant | § II-3 Interpretant | `px_perception_set_intended_interpretant` + `px_interpret_fn` |
| 2. Closure covers illocution only, lacks perlocution | § II-4 Perlocution | `px_closure_set_perlocution` + `px_perlocution_kind` enum |
| 3. Relation inter-thing, lacks actor | § II-5 3-place relation | `px_actor` struct + `px_declare_for` |
| 4. Zuhandenheit/breakdown has no primitive; conflicts with explicit-abstraction | § II-7 Breakdown | New `px_breakdown` abstraction + `PX_REL_WITHDRAWS_FOR`/`PRESENTS_FOR` |
| 5. Intent symbol system closed | (Cross-cutting: Halliday metafunctions) | `px_intent_kind`: enum → `const char*` |
| 6. 6-tradition derivation sample insufficient | § I (Blind spots 1-3) | Re-derive with 9 traditions (this document is the result) |

All six suspicious points are now accounted for — five as essence gaps with API-level fixes, and one (the sampling problem) as the meta-issue this document itself addresses.
