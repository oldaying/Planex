# Continuous Intent — Speculation

> **Status: Speculation, not commitment.**
> This document records an insight about a potential UI essence gap shared by all mainstream UI libraries. It does not commit Planex to implementing anything described here. It exists to make the question explicit so future maintainers don't have to rediscover it.

---

## The observation

All mainstream UI libraries (React, Vue, SwiftUI, Flutter, Qt, GTK, Dear ImGui — and Planex, currently) model **intent as discrete events**:

- Click → `onClick`
- Key press → `onKeyDown`
- Submit → `onSubmit`
- Hover → `onMouseEnter` / `onMouseLeave` (still discrete transitions)

There is no abstraction for the **continuous gradient of intent** that real users experience:

```
approach → potential → decided → executing → retracting
```

The discrete-event model collapses this gradient into a single boolean: "the user clicked (true) or didn't (false)".

---

## Why this might matter (essence-level argument)

Planex's statement of UI essence says:

> UI is a semantic interface between human intent space and machine state space.

If "intent space" is a continuous gradient (which is what human cognition suggests — decision is rarely instantaneous), then modeling intent as discrete events **simplifies intent space in a way that loses information**.

This violates two essence principles:

1. **Essence statement 1** (intent ↔ state semantic interface) — the interface is lossy because intent's continuous shape is quantized into discrete events.
2. **Essence statement 3** (constraint is human cognitive bandwidth) — when intent is quantized, the system can't anticipate user actions, so the user has to explicitly confirm intent at every step (popups, undo buttons, debounces). This is cognitive bandwidth being spent on compensating for a UI abstraction deficiency.

---

## The four-stage model of continuous intent

If intent is continuous, every meaningful interaction has four phases, not one:

| Phase | Intent strength | What UI does | What system does |
|---|---|---|---|
| **Preparation** | 0% → 30% | Visual prewarming (button starts to glow, target zone edge-highlight) | Local queries, cached results, no server load |
| **Decision** | 30% → 80% | Medium-strength feedback (button red, count badge appears) | Async prep work, optimistic state |
| **Execution** | 80% → 100% | Peak feedback (full visual commitment) | Real operation committed |
| **Retraction** | 100% → 0% (decays over N ms) | Retract window indicator (toast with "undo") | Operation can be rolled back without explicit "undo" action |

Mainstream UI libraries only model **Execution**. The other three phases are faked with:
- Modals and confirm dialogs (a hack for missing Decision phase)
- "Undo" buttons (a hack for missing Retraction phase)
- Debouncing (a hack for missing Preparation phase)
- Hover effects (a hack for missing Preparation phase)

These are not features — they are **symptoms of an abstraction gap**.

---

## User-visible implications (if Planex were to implement continuous intent)

Concrete scenarios where continuous intent would change user experience:

### Scenario 1: Destructive operations without confirm dialogs

Current:
- User clicks "Delete" → Modal "Are you sure?" → User clicks "OK" → Delete

Continuous intent:
- User presses "Delete" → Button animates 0→30% (red glow) → Holds 500ms → 30→100% (count appears "deleting 3 items") → Release at 100% → Delete committed → 800ms retract window → User clicks "Undo" if they regret it

The confirm dialog disappears. Mistakes are handled by retraction, not pre-confirmation.

### Scenario 2: Form submit with readiness gradient

Current:
- User fills fields → "Submit" button disabled/enabled boolean → Click → Submit

Continuous intent:
- Each filled field raises intent strength → Button visually intensifies (saturation, shadow, animation rate) → At 100% (all valid), pressing and holding 300ms commits → Retract window follows

The user doesn't have to count "how many fields left" — the button's intensity tells them.

### Scenario 3: Drag-drop with fuzzy intent

Current:
- Drag into target → `onDragEnter` (boolean) → Drop → `onDrop` (boolean)

Continuous intent:
- Drag approaches target (100px = 20%, 50px = 50%, inside = 80%, center = 100%) → Drop position determines strength → Outer ring drop = "uncertain" → System asks "place at edge or center?" → Center drop = confident → Direct placement

Fuzzy intent is expressible without separate "I'm not sure" UI.

### Scenario 4: Search with progressive cost

Current:
- Type → Debounce 300ms → Server request → Display

Continuous intent:
- Type → Intent strength climbs with input length and pause duration → Low strength queries local cache → High strength queries server → Results appear progressively as strength grows

Server load drops because low-strength intent doesn't hit the network.

### Scenario 5: Video scrubbing with intent-based speed

Current:
- Hold →"→"→ Fixed 2x/4x/8x → Release → Stop

Continuous intent:
- Hold →"→"→ 0-500ms = 2x, 500-1500ms = 4x, 1500-3000ms = 8x, 3000ms+ = 16x → Release → 200ms retract window (re-press resumes at 80% of previous strength)

User doesn't need to manually switch speed modes — they just hold longer.

---

## The shared pattern

All five scenarios share four properties that mainstream UI libraries cannot express:

1. **Every frame is feedback** — visual state is a continuous function of intent strength, not a discrete state machine
2. **Operations have retract windows** — not "submitted" but "submitted within N ms retract window"
3. **Low-intent paths are lightweight** — local/cache/preview, only high-intent hits expensive paths
4. **Intent is probeable** — users can approach and withdraw without committing

These four properties are the signature of a continuous-intent UI. Any UI that has all four is doing continuous intent, even if it doesn't call it that.

---

## Existing real-world instances (continuous intent as local hacks)

Continuous intent is not hypothetical. It exists in real products as engineering hacks:

| Product | Hack | What's missing |
|---|---|---|
| iOS 3D Touch / Force Touch | Pressure sensitivity → preview vs open | General abstraction |
| Android long-press | Hold duration → tap vs menu | Two discrete states, not continuous |
| macOS Trash no-confirm trend | Drop + Undo button | Retract window only, no preparation phase |
| Slack "typing" indicator | Displayed on first keystroke | Strength not expressed |
| Linear keyboard hold navigation | Hold duration → distance | No retract window |

Each of these implements one fragment of continuous intent. None implements all four phases. This is evidence that:
- Users can perceive and respond to continuous intent feedback
- The concept is not alien or unusable
- But no UI library gives developers a general abstraction for it

---

## The philosophical prerequisite

Continuous intent requires accepting a philosophical claim:

> **Intent is fundamentally continuous, not discrete.**

This is contested:

- **Pro-continuous:** Cognitive science research on motor planning (200-400ms pre-movement intention accumulation), eye-tracking studies showing intention forms before action begins, anticipation in human-computer interaction research (Card/Moran/Newell 1983 GOMS model treats intention as a process).
- **Pro-discrete:** Speech-act theory (Searle, Winograd/Flores) models intent as discrete acts (assert/request/promise/declare). Decision theory often models choice as binary. Action logicians (von Wright) treat action as a transition between states, not a gradient.

If the philosophical claim is wrong — if intent is fundamentally discrete — then continuous intent is overengineering. Implementing it would add abstraction complexity without essence payoff.

If the philosophical claim is right — then current UI libraries (including Planex) are all missing a fundamental piece, and continuous intent is the next UI essence.

**Planex cannot resolve this debate.** It can only choose whether to bet on it.

---

## What this means for Planex

Planex does not currently implement continuous intent. Its Closure abstraction uses discrete `PX_INTENT_ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS` — five discrete speech-act types. This is the same simplification shared by all mainstream UI libraries.

Three possible responses:

### Option A: Speculate only (current Planex position)
- Acknowledge the gap in `limitations.md` (see L11)
- Do not implement
- Keep this speculation document as a future-research marker
- Revisit if user feedback indicates hover/drag/process bugs are a high-frequency pain point

### Option B: Implement partial (event stream as Relation extension)
- Add `px_event_stream` type to record multi-frame interaction trajectories
- Let hover/drag states be derived from event streams via Relation graph
- Do not change Closure's discrete Intent semantics
- Cost: medium (1-2 weeks)
- Essence impact: covers "process" but not "intent gradient"

### Option C: Implement full (continuous intent as new abstraction)
- Extend Closure's Intent to carry a strength field (0.0-1.0)
- Add Preparation / Decision / Execution / Retraction phase hooks to Closure
- Visual rendering must be a continuous function of intent strength
- Cost: large (1-2 months), breaks existing API
- Essence impact: directly addresses the essence-1 violation
- Risk: requires betting on the "intent is continuous" philosophical claim

---

## Decision criteria

Planex should move from Option A → Option B → Option C only when:

1. **A → B**: Multiple users report hover/drag bugs that are hard to test or debug. Indicates "process" gap is real and pain-causing.
2. **B → C**: After Option B is implemented, evidence shows that "process" alone isn't enough — users need to express intent strength, not just record interactions. Indicates "intent gradient" gap is real.

Without evidence from (1), Planex should not move to B. Without evidence from (2), Planex should not move to C.

---

## What this document is NOT

- Not a commitment to implement continuous intent
- Not a claim that current UI libraries are wrong (they work, they're useful)
- Not a manifesto for "the next UI paradigm"
- Not an excuse to defer fixing Perception (ADR-0001) or Relation necessity (ADR-0002) — those are higher priority

This document is a **marker**. It records that someone thought about this, that the question is real, and that future maintainers should not need to rediscover the question from scratch.

---

## See also

- [Limitations L11](limitations.md) — the limitation entry derived from this speculation
- [Why Four Abstractions](why-four-abstractions.md) — current manifesto, doesn't yet address this
- [ADR-0001](../decisions/ADR-0001-perception-currently-noop.md) — Perception gap, separate from continuous intent
- External: Don Norman, *The Design of Everyday Things* — 7-stage model that motivates Closure, but doesn't address intent continuity
- External: Conal Elliott, FRP — `Behavior = Time → a` is continuous, but doesn't extend to intent gradient
- External: Winograd/Flores, *Understanding Computers and Cognition* — speech-act theory underlying Closure's Intent kinds; takes discrete position
- External: Card/Moran/Newell, *The Psychology of Human-Computer Interaction* (1983) — GOMS model, treats intention as process
