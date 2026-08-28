# ADR-0003: No AI integration in Planex core

## Status

Accepted

Date: 2026-08-24

## Context

In the 2020s, many UI libraries and frameworks have been adding AI features: AI-generated UI, AI-driven interactions, "AI agents" that drive the UI by emitting intents. Planex's design — with Intent-as-value rather than callback — has been noted as particularly amenable to AI agent driving.

There is real pressure (industry trend, user expectations, Venture capital narratives) to make Planex "AI-native" or to add an "AI agent runtime" on top of Planex.

**Planex will not do this.**

This is not a technical limitation — Planex's Intent-as-value design genuinely enables AI agent driving. The decision to abstain is philosophical.

## Decision

**Planex will not integrate AI — not as a runtime component, not as a built-in feature, not as a recommended pattern in core documentation.**

Specifically:

- No AI model in the dependency tree
- No `px_agent_*` API
- No documentation that frames AI as a primary use case
- No examples that position Planex as "the AI agent UI runtime"
- Intent-as-value remains valuable for **serialization, replay, undo, audit** — none of which require AI

## Consequences

### Positive
- Planex remains useful in contexts where AI is unwelcome: embedded systems, air-gapped environments, regulated industries, privacy-sensitive applications.
- The abstraction's value is decoupled from the AI hype cycle. If AI-native UI goes the way of "conversational UI" (a 2016 trend that largely failed), Planex is unaffected.
- Intent-as-value is justified by its **non-AI** benefits: serialization, replay, audit. These are independently valuable and don't depend on AI being real.
- Users who want to drive Planex with an AI agent can still do so — the Intent stream is serializable, an external process can emit Intents. Planex just doesn't facilitate it as a first-class feature.

### Negative
- We forgo a market segment that's currently hot. Some users will choose AI-native alternatives over Planex.
- Reviewers may ask "but what about AI?" and find the answer unsatisfying.
- The Intent-as-value design will be misread by some as "AI-ready but unused" rather than as a justified-independent design choice.

### Neutral
- This decision is reversible at the architecture level. If a future Planex 2.0 wants to add AI, the Intent-as-value foundation supports it without rewrite. We're just not exercising that option now.

## Alternatives Considered

### Alternative 1: Position Planex as "the AI agent UI runtime"
- **What:** Lean into the AI narrative. Market Planex as the only UI library that's structurally compatible with AI agents.
- **Why rejected:** This was seriously considered in the project's history and explicitly rejected. Three reasons:
  1. It ties Planex's success to the AI industry's continued hype. If AI-native UI turns out to be a fad, Planex dies with it.
  2. It corrupts the abstraction's justification. Intent-as-value is justified by serialization / replay / audit / undo — all non-AI benefits. Framing it as "AI-ready" makes the abstraction's value dependent on AI being relevant.
  3. It draws the wrong users. Users who come for "AI agent UI" will not value the deterministic / authorable tradition Planex belongs to. They will request features (auto-generated UI, probabilistic layout, model-driven interaction) that contradict Planex's stance.

### Alternative 2: Add an optional `px_agent.h` header that's off by default
- **What:** Compromise — ship the capability but don't enable it by default.
- **Why rejected:** Optional APIs are not neutral. Once `px_agent_*` exists, demos use it, docs reference it, users depend on it. The "optional" framing degrades over time. Either AI is part of Planex or it isn't; "optional" is a way of saying "yes but slowly".

### Alternative 3: Stay silent on AI
- **What:** Don't mention AI at all. Let users decide.
- **Why rejected:** Silence creates ambiguity. Given the current industry pressure, "no position" gets read as "we'll add it eventually". An explicit non-goal is more honest.

## CAVEATS

This ADR records a *philosophical commitment* (no AI in Planex core). It does NOT:

- Forbid users from driving Planex with an external AI agent. The Intent stream is serializable; an external process (LLM, agent framework, remote service) can emit Intents and Planex will execute them. The decision is that Planex does not *facilitate* this as a first-class feature, not that it forbids it.
- Forbid future Planex 2.0 from adding AI. The decision is reversible at the architecture level — if a future major version finds a principled reason to add AI, the Intent-as-value foundation supports it without rewrite. This ADR records a v1.x stance, not a permanent veto.
- Address AI in *tooling* (developer productivity). If a future Planex contributor uses Copilot / Claude / GPT to write Planex code, that's orthogonal — the ADR concerns the runtime, not the development workflow.
- Address the philosophical question of whether Planex's Intent-as-value design is *better because of AI compatibility*. It is not. Intent-as-value is justified by serialization, replay, audit, undo — all non-AI benefits. AI compatibility is a side-effect, not a justification.
- Address adjacent non-AI-but-probabilistic features (fuzzy matching, predictive text, ML-based input classification). Those are evaluated individually on their own merits, not blanket-prohibited by this ADR.

The decision here is narrowly scoped: no AI runtime, no AI API, no AI marketing. Adjacent questions are deferred to individual evaluation.

## Known issues

- **Issue**: External reviewers familiar with the 2020s AI-native UI trend may dismiss Planex as "behind the times" without reading the ADR. The decision is philosophical, but the surface signal ("no AI") reads as a technical gap to AI-fluent audiences.
- **Why accepted**: Planex's stance is research-grade essence-driven design, not market-share maximization. The audience that dismisses Planex for lacking AI is not Planex's target audience; the audience that values determinism, audit, and embedded-friendly design is. The cost of being misread by the wrong audience is lower than the cost of corrupting the abstraction's justification.
- **Tracking**: accepted as permanent cost for v1.x. Re-evaluation at Planex 2.0 if a principled AI integration is proposed.
- **Mitigation**: the FAQ entry "Why no AI?" (in `docs/faq.md`) and this ADR provide the counter-signal. Reviewers who read either will understand the decision; those who don't would not have been satisfied by a partial AI integration either.

- **Issue**: The Intent-as-value design will be misread by some as "AI-ready but unused" rather than as a justified-independent design choice. This misreading creates a false expectation that "turning on AI" is a configuration flip.
- **Why accepted**: the alternative — weakening Intent-as-value to make AI-incompatibility obvious — would corrupt the abstraction. The misreading is a documentation problem, not a design problem.
- **Tracking**: deferred to documentation improvement; FAQ entry can be sharpened in a future docs commit.
- **Mitigation**: this ADR's Alternatives Considered section explicitly rejects the "AI-ready" framing (Alternative 1). Reviewers who cite this ADR can correct the misreading.

## HISTORY

- 2026-08-24: Proposed
- 2026-08-24: Accepted
- 2026-08-28: Confirmed still-Accepted at v0.5 cycle close; no supersession, no deprecation, no reconsideration triggered

## References

- Code: `include/planex/planex.h` — Intent as value, no AI hooks
- Code: `src/closure.c` — comments mention "AI agent driving" as a possibility, not a feature
- Related docs: `docs/concepts/canonical/non-goals.md` — AI is also listed there
- External: Bret Victor, "Inventing on Principle" (2012) — direct-manipulation tradition
- External: Casey Muratori, "Immediate Mode GUI" (2005) — deterministic UI tradition
- External: Conal Elliott, "Denotational Design" — abstraction independent of use case
