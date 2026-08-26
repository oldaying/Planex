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

## References

- Code: `include/planex/planex.h` — Intent as value, no AI hooks
- Code: `src/closure.c` — comments mention "AI agent driving" as a possibility, not a feature
- Related docs: `docs/concepts/non-goals.md` — AI is also listed there
- External: Bret Victor, "Inventing on Principle" (2012) — direct-manipulation tradition
- External: Casey Muratori, "Immediate Mode GUI" (2005) — deterministic UI tradition
- External: Conal Elliott, "Denotational Design" — abstraction independent of use case
