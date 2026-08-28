# Concepts — Theory and Position Papers

> See [`doc-organization.md`](../doc-organization.md) Principle 4 (Reader intent drives top-level sectioning) for the rationale behind the split.

This directory is split along **reader intent**, not topic. Pick the subdirectory that matches your question:

| You're asking… | Subdirectory |
|-----------------|--------------|
| "What does Planex canonically claim?" | [`canonical/`](canonical/) — normative position papers |
| "What does Planex currently implement?" | [`state/`](state/) — descriptive state docs |
| "What might Planex do in future?" | [`speculation/`](speculation/) — proposals not yet accepted |
| "What did Planex used to claim?" | [`history/`](history/) — superseded derivation docs |
| "What literature is Planex drawing from?" | [`background/`](background/) — academic surveys, lineage docs |

## Why split by reader intent

A flat `concepts/` directory conflates five different reader questions into one pile. A reader looking for "what Planex canonically claims" cannot distinguish `abstraction-form.md` (normative, current) from `essence-derivation-v1.md` (superseded, historical) without opening the file. The split makes the reader's task trivial: `ls concepts/canonical/` answers the question directly.

This pattern is borrowed from Idris 2's `docs/source/{tutorial,reference,backends,proofs,cookbook,implementation,typedd,updates,faq}/` split, which itself is the operational form of the Diátaxis framework's four-quadrant map.

## When to add a doc here

If your doc answers "what is Planex?" or "why is Planex shaped this way?" (i.e. it's not a tutorial, how-to, reference, or ADR), it belongs somewhere under `concepts/`. Pick the subdirectory by reader intent:

- It states a claim → `canonical/`
- It describes the current state → `state/`
- It proposes something not yet accepted → `speculation/` (or `staging/` if you're not sure)
- It records a superseded derivation → `history/`
- It surveys literature → `background/`

If none fit, put it in [`../staging/`](../staging/) and propose a new subdirectory in an ADR if the gap is structural.
