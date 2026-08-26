# FAQ

---

### Q: Why C? Why not Rust/Zig/C++?

C has zero runtime, zero hidden abstractions, and compiles everywhere. The 4 abstractions (Relation + Estimate + Closure + Perception) are about **design philosophy**, not language features. C forces us to be explicit — no hidden constructors, no RAII, no trait bounds. If the abstractions work in C, they work anywhere.

---

### Q: Why not use callbacks like every other UI library?

Callbacks (`onClick={fn}`) hide the **intent** — what the user wanted to do. Planex uses `px_closure` with a typed `px_intent` (REQUEST, PROMISE, DECLARE, etc.). The intent is a **value**, not a function — it can be serialized, replayed, and audited.

---

### Q: What's the difference between Estimate and React's useState?

| | `useState` | `px_estimate` |
|---|---|---|
| Value | discrete snapshot | continuous (can animate) |
| Time | side-effect (`useEffect`) | built-in (`animate()` + `value()` auto-samples) |
| Derived | manual (`useMemo`) | automatic (`px_derived_new`) |
| Dynamic sources | not supported | `px_derived_add_source` / `remove_source` |
| Observers | manual (`useEffect` deps) | automatic (observer pattern) |

---

### Q: What's the difference between Closure and a regular callback?

| | callback | `px_closure` |
|---|---|---|
| Intent | hidden in function | typed value (ASSERT/REQUEST/PROMISE/...) |
| Goal | none | human-readable string |
| Evaluation | none | auto: false → FAILED + feedback |
| Replay | impossible | `px_closure_last_intent()` |
| Status | none | IDLE/RUNNING/DONE/FAILED |

---

### Q: Can I use Planex without the Relation graph?

Yes. The Relation graph is optional — you can use Estimate + Closure without ever calling `px_declare`. Relation is for advanced use cases (layout, querying, constraint solving).

---

### Q: Does Planex support mobile (iOS/Android)?

Not yet. The Cocoa backend (macOS) code is complete but untested on iOS. Android would need a new backend (probably via NDK + ANativeWindow). This is on the roadmap but not a current priority.

---

### Q: Does Planex support GPU rendering?

Not yet. Current rendering is software rasterization (CPU framebuffer → blit to window). GPU backend (Vulkan/Metal/D3D12) is on the roadmap. The architecture supports it — `px_window_present()` is the only rendering touchpoint, and it's backend-specific.

---

### Q: How is Planex different from Dear ImGui?

| | Dear ImGui | Planex |
|---|---|---|
| Mode | immediate | declarative (build → render → reset) |
| State | external (developer manages) | Estimate (auto-tracking, animated, derived) |
| Interaction | callback | Closure (typed intent, 7-stage) |
| Relations | none | Relation graph (queryable, drives layout) |
| Text input | basic | IME support (XIM/IMM32/NSTextInputClient) |
| Accessibility | none | a11y API (roles, states, announce) |

---

### Q: How is Planex different from clay?

clay is a **layout library** — it computes where elements should go, but doesn't render or handle state. Planex is a **full UI library** — it includes state management (Estimate), interaction (Closure), rendering (framebuffer + fonts), and window management.

---

### Q: Is Planex production-ready?

No. It's an experiment exploring whether 4 abstractions (Relation + Estimate + Closure + Perception) can express all UI patterns. The counter_4abs and multi_perception demos prove they can for basic cases. More testing and real-world use is needed before production.

---

### Q: On Windows, pressing + doesn't work in slider/counter

On Windows, the `+` key and `=` key are the same physical key. Without Shift, Windows sends `=` (0x3D). With Shift, it sends `+` (0x2B). Always accept both:

```c
if (key == '+' || key == '=') { /* increment */ }
if (key == '-' || key == '_') { /* decrement */ }
```
