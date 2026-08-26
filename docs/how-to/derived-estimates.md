# How-to: Use Derived Estimates

> Auto-tracking state — no manual recompute.

---

## Fixed sources (form validation)

When you have a fixed set of estimates, use `px_derived_new`:

```c
/* Each field has a validity estimate */
px_estimate* username_valid = px_estimate_new(0, 1.0);
px_estimate* email_valid    = px_estimate_new(0, 1.0);
px_estimate* password_valid = px_estimate_new(0, 1.0);
px_estimate* confirm_valid  = px_estimate_new(0, 1.0);

/* Derivation: all_valid = AND of all validities */
double all_valid_fn(px_estimate* const* srcs, int n, void* user) {
    for (int i = 0; i < n; i++)
        if (px_estimate_value(srcs[i]) < 0.5) return 0;
    return 1;
}

px_estimate* srcs[] = { username_valid, email_valid,
                        password_valid, confirm_valid };
px_estimate* all_valid = px_derived_new(all_valid_fn, NULL, srcs, 4);

/* Now whenever any validity changes: */
px_estimate_set(username_valid, 1, 1.0);
/* all_valid auto-updates — no manual recompute */
```

---

## Dynamic sources (todo list)

When items are added/removed at runtime, use `px_derived_new_dynamic`:

```c
/* Create with 0 sources */
px_estimate* remaining = px_derived_new_dynamic(count_remaining, NULL);

double count_remaining(px_estimate* const* srcs, int n, void* user) {
    int count = 0;
    for (int i = 0; i < n; i++)
        if (px_estimate_value(srcs[i]) < 0.5) count++;
    return (double)count;
}

/* Add a new todo */
px_estimate* todo_done = px_estimate_new(0, 1.0);  /* not done */
px_derived_add_source(remaining, todo_done);
/* remaining auto-updates to include this todo */

/* Toggle done */
px_estimate_set(todo_done, 1, 1.0);
/* remaining auto-decrements — no manual recompute */

/* Delete todo */
px_derived_remove_source(remaining, todo_done);
px_estimate_free(todo_done);  /* free AFTER removing from derived */
/* remaining auto-decrements */
```

---

## Chained derived (derived of derived)

```c
/* sum = a + b */
px_estimate* srcs_ab[] = { a, b };
px_estimate* sum = px_derived_new(sum_fn, NULL, srcs_ab, 2);

/* doubled = sum * 2 */
double double_fn(px_estimate* const* srcs, int n, void* user) {
    return px_estimate_value(srcs[0]) * 2;
}
px_estimate* srcs_sum[] = { sum };
px_estimate* doubled = px_derived_new(double_fn, NULL, srcs_sum, 1);

/* Change a → sum updates → doubled updates */
px_estimate_set(a, 10, 1.0);
/* doubled = (10 + b) * 2, all automatic */
```

---

## Common pitfalls

1. **Free sources AFTER removing from derived** — if you free first, the derived's observer list has a dangling pointer.

2. **Don't use px_derived_new for dynamic lists** — it copies the source array at creation time. Adding/removing items later won't work. Use `px_derived_new_dynamic` instead.

3. **Derived functions must be pure** — they read source values and return a result. They must NOT call `px_estimate_set` on the derived estimate (causes infinite loop).
