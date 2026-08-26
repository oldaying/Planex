# API Reference

> Every public function: parameters, return values, usage.

---

## Table of Contents

1. [Basic Types](#basic-types)
2. [Estimate](#estimate)
3. [Derived Estimate](#derived-estimate)
4. [Closure](#closure)
5. [Relation](#relation)
6. [Layout](#layout)
7. [Framebuffer](#framebuffer)
8. [Font](#font)
9. [Window](#window)
10. [App Loop](#app-loop)
11. [Accessibility](#accessibility)

---

## Basic Types

### px_rect

```c
typedef struct { float x, y, w, h; } px_rect;
```

A 2D rectangle. `(x, y)` is the top-left corner. `w` and `h` are width and height.

```c
px_rect r = { 10, 20, 100, 50 };  // x=10, y=20, w=100, h=50
```

### px_rect_make

```c
static inline px_rect px_rect_make(float x, float y, float w, float h);
```

Create a rect. Convenience constructor.

### px_color

```c
typedef struct { float r, g, b, a; } px_color;
```

sRGB color, each channel 0..1.

### Built-in colors

```c
PX_BG          // #0c0d12 — dark background
PX_SURFACE     // #1f2029 — surface
PX_TEXT        // #ebebf0 — primary text
PX_TEXT_DIM    // #8c9099 — dim text
PX_BORDER      // #333642 — border
PX_ACCENT      // #6690f0 — blue accent
PX_SUCCESS     // #4ec9b0 — green
PX_WARNING     // #d19a66 — orange
PX_DANGER      // #f48787 — red
```

---

## Estimate

### px_estimate_new

```c
px_estimate* px_estimate_new(double value, double confidence);
```

Create a new estimate with an initial value and confidence (0..1).

**Parameters:**
- `value` — initial value
- `confidence` — 0.0 = no confidence, 1.0 = fully confident

**Returns:** pointer to the new estimate, or NULL on failure.

```c
px_estimate* count = px_estimate_new(0, 1.0);
```

### px_estimate_free

```c
void px_estimate_free(px_estimate* e);
```

Free an estimate and all its observers.

### px_estimate_value

```c
double px_estimate_value(px_estimate* e);
```

Get the current value. **Auto-samples animation** — if the estimate is animating, returns the interpolated value at the current time. If the animation finished, finalizes it (sets value = target, fires observers).

```c
double v = px_estimate_value(count);  // always current
```

### px_estimate_set

```c
void px_estimate_set(px_estimate* e, double value, double confidence);
```

Set the value. Cancels any ongoing animation. Notifies all observers.

### px_estimate_animate

```c
void px_estimate_animate(px_estimate* e, double target, double duration_ms);
```

Begin animating from the current value to `target` over `duration_ms` milliseconds. Uses ease-out curve. The value is auto-sampled by `px_estimate_value()` each time it's called.

```c
px_estimate_animate(count, 100, 1000);  // animate to 100 over 1 second
```

### px_estimate_now

```c
double px_estimate_now(px_estimate* e);
```

Explicitly sample the animated value. Usually unnecessary — `px_estimate_value()` does this automatically.

### px_estimate_is_animating

```c
bool px_estimate_is_animating(px_estimate* e);
```

Returns true if the estimate is currently animating. Also finalizes the animation if it has completed.

### px_estimate_observe

```c
void px_estimate_observe(px_estimate* e, px_estimate_observer fn, void* user);
```

Subscribe to value changes. `fn` is called every time `px_estimate_set()` is called (or animation finalizes).

```c
void on_count_changed(px_estimate* e, void* user) {
    printf("count = %.0f\n", px_estimate_value(e));
}
px_estimate_observe(count, on_count_changed, NULL);
```

### px_now_ms

```c
double px_now_ms(void);
```

Get current monotonic time in milliseconds. Used as the time source for animations.

---

## Derived Estimate

### px_derived_new

```c
px_estimate* px_derived_new(px_derive_fn fn, void* user,
                             px_estimate** sources, int n_sources);
```

Create a derived estimate from a fixed set of sources. When any source changes, the derived value auto-recomputes.

**Parameters:**
- `fn` — the derivation function: `double fn(px_estimate* const* sources, int n, void* user)`
- `user` — user data passed to fn
- `sources` — array of source estimates
- `n_sources` — number of sources

```c
double sum_fn(px_estimate* const* srcs, int n, void* user) {
    double s = 0;
    for (int i = 0; i < n; i++) s += px_estimate_value(srcs[i]);
    return s;
}

px_estimate* srcs[] = { a, b, c };
px_estimate* total = px_derived_new(sum_fn, NULL, srcs, 3);
```

### px_derived_new_dynamic

```c
px_estimate* px_derived_new_dynamic(px_derive_fn fn, void* user);
```

Create a derived estimate with no sources initially. Add/remove sources at runtime via `px_derived_add_source` / `px_derived_remove_source`. Ideal for dynamic lists (e.g., todo items).

```c
px_estimate* remaining = px_derived_new_dynamic(count_remaining, NULL);
```

### px_derived_add_source

```c
int px_derived_add_source(px_estimate* derived, px_estimate* source);
```

Add a source to a dynamic derived estimate. Automatically subscribes to the source and recomputes.

**Returns:** 0 on success, -1 on failure.

```c
// Adding a new todo
px_derived_add_source(remaining, todo->done);
// remaining auto-updates — no manual recompute
```

### px_derived_remove_source

```c
int px_derived_remove_source(px_estimate* derived, px_estimate* source);
```

Remove a source from a dynamic derived estimate. Automatically recomputes.

**Note:** The source estimate should be freed AFTER calling this (to clear observer lists).

**Returns:** 0 on success, -1 if source not found.

### px_derived_recompute

```c
void px_derived_recompute(px_estimate* derived);
```

Manually trigger a recompute. Usually unnecessary — derived values auto-update when sources change.

---

## Closure

### px_closure_new

```c
px_closure* px_closure_new(
    const char*      goal,
    px_intent_kind   intent_kind,
    px_action_fn     action,
    px_eval_fn       evaluation,
    void*            user);
```

Create a closure representing a 7-stage interaction.

**Parameters:**
- `goal` — human-readable description ("increment counter")
- `intent_kind` — one of `PX_INTENT_ASSERT`, `PX_INTENT_REQUEST`, `PX_INTENT_PROMISE`, `PX_INTENT_DECLARE`, `PX_INTENT_EXPRESS`
- `action` — function called on trigger: `void fn(px_intent intent, void* user)`
- `perception` — function returning visible state: `void* fn(void* user)` (may be NULL)
- `evaluation` — function checking goal achievement: `bool fn(void* user)` (may be NULL)
- `user` — user data passed to all callbacks

```c
px_closure* inc = px_closure_new(
    "increment counter", PX_INTENT_REQUEST,
    on_inc, eval_nonneg, &app);
```

### px_closure_trigger

```c
void px_closure_trigger(px_closure* c, void* payload, size_t size);
```

Trigger the closure. Copies `payload` (if any), calls action, perception, and evaluation. If evaluation returns false, auto-sets status to `PX_CLOSURE_FAILED` with feedback text.

```c
px_closure_trigger(inc, NULL, 0);

int idx = 2;
px_closure_trigger(toggle, &idx, sizeof(idx));
```

### px_closure_last_intent

```c
px_intent px_closure_last_intent(px_closure* c);
```

Get the last triggered intent (for replay, logging, undo/redo).

### px_closure_feedback

```c
const char* px_closure_feedback(px_closure* c);
```

Get the current feedback text — what the machine tells the user.

### px_closure_get_status

```c
px_closure_status px_closure_get_status(px_closure* c);
```

Get current status: `PX_CLOSURE_IDLE`, `PX_CLOSURE_RUNNING`, `PX_CLOSURE_DONE`, `PX_CLOSURE_FAILED`.

### px_closure_promise / declare / fail

```c
void px_closure_promise(px_closure* c, const char* message);
void px_closure_declare(px_closure* c, const char* message);
void px_closure_fail(px_closure* c, const char* message);
```

Machine-initiated status changes:
- `promise` — "I will do X" (status = RUNNING)
- `declare` — "X is done" (status = DONE)
- `fail` — "X failed" (status = FAILED)

---

## Relation

### px_graph_new / px_graph_free

```c
px_graph* px_graph_new(void);
void     px_graph_free(px_graph* g);
```

Create/free a relation graph.

### px_declare

```c
px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind, void* b);
```

Declare a relation between two nodes (opaque pointers).

```c
px_declare(graph, button, PX_REL_TRIGGERS, counter);
px_declare(graph, &app, PX_REL_CONTAINS, counter);
```

### px_has_relation

```c
bool px_has_relation(px_graph* g, void* a, px_rel_kind kind, void* b);
```

Check if a specific relation exists.

### px_query

```c
px_node_list px_query(px_graph* g, void* node, px_rel_kind kind);
void         px_node_list_free(px_node_list* list);
```

Find all nodes related to `node` via `kind`.

---

## Layout

### px_layout_beside

```c
px_rect px_layout_beside(px_rect prev, int width, int gap);
```

Place a rect to the right of `prev` with `gap` pixels between them.

### px_layout_below

```c
px_rect px_layout_below(px_rect prev, int height, int gap);
```

Place a rect below `prev` with `gap` pixels between them.

### px_layout_center

```c
px_rect px_layout_center(px_rect container, int w, int h);
```

Center a rect of size `w × h` inside `container`.

---

## Framebuffer

### px_fb_new / px_fb_free

```c
px_fb* px_fb_new(int width, int height);
void   px_fb_free(px_fb* fb);
```

### px_fb_width / px_fb_height

```c
int px_fb_width(px_fb* fb);
int px_fb_height(px_fb* fb);
```

### Drawing primitives

```c
void px_fb_clear(px_fb* fb, uint32_t rgba);
void px_fb_fill_rect(px_fb* fb, int x, int y, int w, int h, uint32_t rgba);
void px_fb_draw_rect(px_fb* fb, int x, int y, int w, int h, uint32_t rgba);
void px_fb_draw_hline(px_fb* fb, int x, int y, int len, uint32_t rgba);
void px_fb_draw_vline(px_fb* fb, int x, int y, int len, uint32_t rgba);
void px_fb_set_pixel(px_fb* fb, int x, int y, uint32_t rgba);
```

### Text rendering (8x16 bitmap font, ASCII only)

```c
int px_fb_draw_char(px_fb* fb, int x, int y, char c, uint32_t fg_rgba);
int px_fb_draw_text(px_fb* fb, int x, int y, const char* s, uint32_t fg_rgba);
int px_fb_draw_text_bg(px_fb* fb, int x, int y, const char* s,
                        uint32_t fg_rgba, uint32_t bg_rgba);
```

### Text rendering (TTF, requires FreeType)

```c
px_font* px_font_load(const char* ttf_path, int pixel_size);
px_font* px_font_find(const char* family_name, int pixel_size);
px_font* px_font_default(void);
void     px_font_free(px_font* font);
int      px_font_add_fallback(px_font* font, const char* ttf_path);
int      px_font_add_fallback_named(px_font* font, const char* family_name);
int      px_fb_draw_text_utf8(px_fb* fb, int x, int y, const char* utf8_str,
                               px_font* font, uint32_t fg_rgba);
```

### BMP output

```c
int  px_fb_save_bmp(px_fb* fb, const char* path);
void px_fb_dump_ascii(px_fb* fb);
```

---

## Window

### px_window_new / px_window_free

```c
px_window* px_window_new(int width, int height, const char* title);
void       px_window_free(px_window* w);
```

### px_window_show

```c
int px_window_show(px_window* w);
```

### px_window_poll_event

```c
px_event px_window_poll_event(px_window* w);
```

Non-blocking event poll. Returns `PX_EV_NONE` if no event.

### px_window_present

```c
void px_window_present(px_window* w);
```

Copy framebuffer to the window surface.

### px_window_scale

```c
double px_window_scale(px_window* w);
```

DPI scale factor (1.0 = normal, 2.0 = Retina).

---

## App Loop

### px_app_run

```c
int px_app_run(const px_app_desc* desc);
```

Run the application loop. Handles window creation, event polling, 60fps rendering, animation ticking.

**Parameters (px_app_desc):**
- `width`, `height` — logical window size
- `title` — window title
- `render` — called every frame to draw to fb
- `on_click` — called on mouse click
- `on_key` — called on key press
- `on_tick` — called every ~16ms (60fps)
- `on_resize` — called when window is resized
- `on_ime_commit` — called when IME commits text
- `animated_estimates` — estimates to monitor for animation
- `user` — user data passed to all callbacks

```c
px_app_desc desc = {
    .width = 400,
    .height = 300,
    .title = "My App",
    .perception = render_pixels,  /* Phase 2: perception-driven */
    .on_click = on_click,
    .on_key = on_key,
    .user = &app,
};
px_app_run(&desc);
```

---

## Accessibility

### px_a11y_new / px_a11y_free

```c
px_a11y* px_a11y_new(px_window* w);
void     px_a11y_free(px_a11y* a);
```

### Properties

```c
void px_a11y_set_role(px_a11y* a, px_a11y_role role);
void px_a11y_set_name(px_a11y* a, const char* name);
void px_a11y_set_value(px_a11y* a, const char* value);
void px_a11y_set_state(px_a11y* a, unsigned state);
```

### Actions

```c
void px_a11y_announce(px_a11y* a, const char* message);
void px_a11y_focus(px_a11y* a);
```
