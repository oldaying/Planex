# Getting Started

> From zero to your first Planex window in 10 minutes.

---

## 1. Install

### Windows (Visual Studio)

```bat
git clone https://github.com/oldaying/Planex.git
cd Planex
cmake -B build
cmake --build build --config Release
```

Requires: Visual Studio 2022+ (CMake support included).

### Linux (GCC/Clang)

```bash
git clone https://github.com/oldaying/Planex.git
cd Planex
sudo apt install libx11-dev libxext-dev    # X11 backend
cmake -B build
cmake --build build
```

### macOS (Clang)

```bash
git clone https://github.com/oldaying/Planex.git
cd Planex
cmake -B build
cmake --build build
```

---

## 2. Run your first app

```bash
./build/counter_4abs     # Linux/macOS
.\build\Release\counter_4abs.exe   # Windows
```

A window opens with a working todo application:
- Type text + Enter to add a todo
- Click a todo to toggle done
- Click [x] to delete
- Click All/Active/Completed to filter

---

## 3. Write your own app

Create `myapp.c`:

```c
#include "planex/planex.h"
#include "planex/app.h"

/* State */
typedef struct {
    px_estimate* count;
} App;

/* Action: increment count */
static void on_inc(px_intent intent, void* user) {
    (void)intent;
    App* a = user;
    double v = px_estimate_value(a->count);
    px_estimate_set(a->count, v + 1, 1.0);
}

/* Render: draw to framebuffer */
static void on_render(px_fb* fb, void* user) {
    App* a = user;
    int W = px_fb_width(fb);

    px_fb_clear(fb, PX_BG);
    px_fb_draw_rect(fb, 4, 4, W - 8, 100, PX_BORDER);

    /* Title */
    px_fb_fill_rect(fb, 4, 4, W - 8, 20, PX_SURFACE);
    px_fb_draw_text(fb, 12, 8, "My First Planex App", PX_TEXT);

    /* Count display */
    char buf[32];
    snprintf(buf, sizeof(buf), "Count: %.0f", px_estimate_value(a->count));
    px_fb_draw_text(fb, 12, 32, buf, PX_TEXT);

    /* Button */
    px_fb_fill_rect(fb, 12, 56, 80, 24, PX_ACCENT);
    px_fb_draw_text(fb, 28, 60, "+ Inc", PX_TEXT);
}

/* Handle clicks */
static bool on_click(int x, int y, void* user) {
    App* a = user;
    if (y >= 56 && y < 80 && x >= 12 && x < 92) {
        double v = px_estimate_value(a->count);
        px_estimate_set(a->count, v + 1, 1.0);
        return true;
    }
    return false;
}

static bool on_key(char key, void* user) {
    (void)user;
    if (key == 'q' || key == 27) return false;
    return false;
}

int main(void) {
    App app = {0};
    app.count = px_estimate_new(0, 1.0);

    px_app_desc desc = {
        .width = 256,
        .height = 128,
        .title = "My First Planex App",
        .render = on_render,
        .on_click = on_click,
        .on_key = on_key,
        .user = &app,
    };

    int rc = px_app_run(&desc);
    px_estimate_free(app.count);
    return rc;
}
```

Compile:

```bash
cmake -B build    # if not already done
cmake --build build --config Release

# Or manually (Linux):
cc -std=c17 -Iinclude myapp.c -Lbuild -lplanex_lib -o myapp -lX11 -lXext -lm
```

Run:

```bash
./myapp
```

---

## 4. What just happened

You used all 3 abstractions:

| Abstraction | Where in the code | What it did |
|---|---|---|
| **Estimate** | `px_estimate_new(0, 1.0)` | Held the count value |
| **Closure** | `px_app_run(&desc)` | Ran the interaction loop (render + input) |
| **Relation** | `px_fb_draw_rect / draw_text` | Spatial relations between elements |

When you clicked the button:
1. `on_click` detected the click
2. `px_estimate_set` changed the count
3. `px_app_run` saw the change → called `on_render`
4. `on_render` drew the new count

No manual re-render. No state machine. No callback registration. The app loop handles it.

---

## 5. Next steps

- [Why four abstractions?](../concepts/canonical/why-four-abstractions.md) — understand the design philosophy
- [API Reference](../reference/api.md) — every function documented
- [How-to: create a button](../how-to/create-a-button.md) — common patterns
- [How-to: use derived estimates](../how-to/derived-estimates.md) — auto-tracking state
- [FAQ](../faq.md) — common questions
