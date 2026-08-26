# How-to: Create a Button

> A clickable button with visual feedback.

---

## Basic button

```c
typedef struct {
    int btn_x, btn_y, btn_w, btn_h;
} App;

static void on_render(px_fb* fb, void* user) {
    App* a = user;
    int W = px_fb_width(fb);

    px_fb_clear(fb, PX_BG);

    /* Button background */
    px_fb_fill_rect(fb, 12, 40, 100, 24, PX_ACCENT);

    /* Button label (centered) */
    const char* label = "Click Me";
    int label_w = strlen(label) * 8;
    px_fb_draw_text(fb, 12 + (100 - label_w) / 2, 44, label, PX_TEXT);

    /* Store rect for hit testing */
    a->btn_x = 12; a->btn_y = 40;
    a->btn_w = 100; a->btn_h = 24;
}

static bool on_click(int x, int y, void* user) {
    App* a = user;
    if (x >= a->btn_x && x < a->btn_x + a->btn_w &&
        y >= a->btn_y && y < a->btn_y + a->btn_h) {
        printf("Button clicked!\n");
        return true;
    }
    return false;
}
```

---

## Button with Closure

Use `px_closure` for the button's action — this makes the intent serializable and the goal auditable:

```c
px_closure* submit = px_closure_new(
    "submit form",              // goal
    PX_INTENT_REQUEST,           // intent type
    on_submit,                   // action
    NULL,                        // perception (optional)
    eval_form_valid,             // evaluation (auto-sets FAILED if false)
    &app);

/* In on_click: */
if (hit_test(x, y, btn_rect)) {
    px_closure_trigger(submit, NULL, 0);
    /* If eval returns false → status = FAILED + feedback auto-generated */
    return true;
}
```

---

## Button with state feedback

Show the closure's status (IDLE/RUNNING/DONE/FAILED) as color:

```c
px_closure_status st = px_closure_get_status(submit);
uint32_t bg;
switch (st) {
    case PX_CLOSURE_IDLE:    bg = PX_SURFACE; break;
    case PX_CLOSURE_RUNNING: bg = PX_WARNING; break;
    case PX_CLOSURE_DONE:    bg = PX_SUCCESS; break;
    case PX_CLOSURE_FAILED:  bg = PX_DANGER;  break;
}
px_fb_fill_rect(fb, x, y, w, h, bg);
```

---

## Button using layout helpers

Instead of hardcoding coordinates:

```c
/* First button */
px_rect btn1 = { 12, 40, 100, 24 };
px_fb_fill_rect(fb, btn1.x, btn1.y, btn1.w, btn1.h, PX_ACCENT);

/* Second button beside it */
px_rect btn2 = px_layout_beside(btn1, 100, 8);
px_fb_fill_rect(fb, btn2.x, btn2.y, btn2.w, btn2.h, PX_SURFACE);
px_fb_draw_rect(fb, btn2.x, btn2.y, btn2.w, btn2.h, PX_BORDER);
```

---

## Tips

- Always store button rects in the app struct so `on_click` can hit-test them
- Use `PX_ACCENT` for primary buttons, `PX_SURFACE` for secondary
- Use `PX_DANGER` for destructive buttons (delete, reset)
- On Windows, `=` key = `+` key without Shift — accept both in `on_key`
