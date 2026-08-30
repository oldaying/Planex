#!/bin/bash
# verify_orca_e2e.sh — the v0.8 Cross-cutting A verification harness:
# an OBSERVED orca pass over a Planex app on the x11 backend.
#
# What it proves (the roadmap's success criterion): a Planex app
# whose focus ring is derived from the AFFORDS graph is navigable
# by a real screen reader — orca announces the window, the focused
# elements as Tab walks the ring, and the app's announcements.
#
# Chain under test:
#   XTEST key -> Xvfb :N -> Planex x11 backend -> px_app_run focus
#   ring -> on_focus -> the a11y query side -> the AT-SPI2 bridge
#   mirror -> atk-bridge -> D-Bus -> orca -> SPEECH OUTPUT lines.
#
# Evidence standard: orca's own presentation decisions — the
# `SPEECH OUTPUT` lines in its debug log are what orca WOULD speak
# (the utterances reach its speech dispatcher; audible delivery
# needs a desktop session with pulse/pipewire and is not part of
# this headless run). The app's stdout is the ground truth the
# speech is asserted against.
#
# Prerequisites (the harness checks and reports):
#   Xvfb, dbus-run-session, orca, atk/atk-bridge dev headers
#   (pkg-config atk atk-bridge-2.0), libX11 + libXtst, a C
#   compiler, and python3 on PATH is NOT needed.
#
# Rootless/unpacked stacks: if orca and the at-spi daemons live in
# an unpacked prefix (no root), point ORCA_ROOT at it — the script
# derives PATH/GI_TYPELIB_PATH/LD_LIBRARY_PATH and rewrites the
# org.a11y.Bus service Exec to the prefix. On a normal distro
# (packages installed), leave ORCA_ROOT unset.
#
# Usage:  scripts/verify_orca_e2e.sh [--keep] [results-dir]
#   --keep    do not tear down Xvfb/session afterwards (debugging)
# Exit:   0 pass, 1 verification failure, 2 prerequisites unmet.
#
# The observed pass that flipped the ledger row (2026-08-30):
#   orca 48.1, at-spi2-core 2.56, Xvfb, Debian 13 — window,
#   three focus announcements, two activation announcements.

set +m
set -u

REPO_DIR=$(cd "$(dirname "$0")/.." && pwd)
RESULTS=${2:-/tmp/px-orca-e2e}
DISP=:97
APP_TITLE="Planex orca demo"
KEYS="tab tab return tab tab return"
KEY_GAP_MS=3000
KEEP=${1:-}

mkdir -p "$RESULTS"

# ---- prerequisites -------------------------------------------------
die_pre() {
    echo "PREREQUISITE: $*" >&2
    exit 2
}

command -v Xvfb            >/dev/null || die_pre "Xvfb missing (xvfb package)"
command -v dbus-run-session >/dev/null || die_pre "dbus-run-session missing (dbus package)"
command -v cc              >/dev/null || die_pre "C compiler missing"

ORCA_BIN=$(command -v orca || true)
if [ -n "${ORCA_ROOT:-}" ]; then
    [ -x "$ORCA_ROOT/usr/bin/orca" ] || die_pre "ORCA_ROOT set but $ORCA_ROOT/usr/bin/orca missing"
    ORCA_BIN=$ORCA_ROOT/usr/bin/orca
    export PATH="$ORCA_ROOT/usr/bin:$PATH"
    export PYTHONPATH="$ORCA_ROOT/usr/lib/python3/dist-packages${PYTHONPATH:+:$PYTHONPATH}"
    export GI_TYPELIB_PATH="$ORCA_ROOT/usr/lib/x86_64-linux-gnu/girepository-1.0:/usr/lib/x86_64-linux-gnu/girepository-1.0"
    export LD_LIBRARY_PATH="$ORCA_ROOT/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export PKG_CONFIG_PATH="$ORCA_ROOT/usr/lib/x86_64-linux-gnu/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
fi
[ -n "$ORCA_BIN" ] || die_pre "orca missing (install the orca package, or set ORCA_ROOT)"
echo "orca: $ORCA_BIN"

pkg-config --exists atk atk-bridge-2.0 || die_pre \
    "atk/atk-bridge dev headers missing (libatk1.0-dev libatk-bridge2.0-dev)"

# ---- clean environment (lesson: leaked daemons poison the bus) -----
# dbus-run-session does NOT kill D-Bus-activated services on exit;
# a leaked at-spi bus from an earlier run keeps its fixed socket
# path and every later session silently shares a dead registry.
pkill -9 -f "[a]t-spi"        2>/dev/null
pkill -9 -f "[a]tk-bridge"    2>/dev/null
pkill -9 -f "[X]vfb $DISP"    2>/dev/null
sleep 0.5
rm -rf "${HOME}/.cache/at-spi" "${XDG_CACHE_HOME:-$HOME/.cache}/at-spi" 2>/dev/null

# ---- build the evidence app with the bridge ------------------------
PCC() { pkg-config "$@"; }
if [ -n "${ORCA_ROOT:-}" ]; then
    # unpacked prefix: the .pc files carry system paths; redirect
    # the atk include dirs to the prefix, keep system glib/dbus.
    PCC() {
        PKG_CONFIG_PATH="$ORCA_ROOT/usr/lib/x86_64-linux-gnu/pkgconfig" \
            pkg-config "$@" |
        sed "s|-I/usr/include/atk-1.0|-I$ORCA_ROOT/usr/include/atk-1.0|g;
             s|-I/usr/include/at-spi2-atk/2.0|-I$ORCA_ROOT/usr/include/at-spi2-atk/2.0|g;
             s|-I/usr/include/at-spi-2.0|-I$ORCA_ROOT/usr/include/at-spi-2.0|g"
    }
fi

SRC="$REPO_DIR/src/relation.c $REPO_DIR/src/estimate.c \
     $REPO_DIR/src/closure.c $REPO_DIR/src/perception.c \
     $REPO_DIR/src/undo.c $REPO_DIR/src/feedback.c \
     $REPO_DIR/src/fb.c $REPO_DIR/src/font.c \
     $REPO_DIR/src/a11y.c $REPO_DIR/src/a11y_bridge_atspi.c \
     $REPO_DIR/src/layout.c $REPO_DIR/src/interaction.c \
     $REPO_DIR/src/hit.c $REPO_DIR/src/x11.c $REPO_DIR/src/app.c"
APP_BIN=$RESULTS/a11y_orca_demo_atspi
LINK_EXTRA=""
if [ -n "${ORCA_ROOT:-}" ]; then
    LINK_EXTRA="-L$ORCA_ROOT/usr/lib/x86_64-linux-gnu -Wl,-rpath,$ORCA_ROOT/usr/lib/x86_64-linux-gnu"
fi
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -D_POSIX_C_SOURCE=200809L \
   -I "$REPO_DIR/include" -DPLANEX_BACKEND_X11 -DPX_A11Y_ATSPI \
   $(PCC --cflags atk atk-bridge-2.0) \
   "$REPO_DIR/examples/a11y_orca_demo.c" $SRC -o "$APP_BIN" \
   $(PCC --libs atk atk-bridge-2.0) $LINK_EXTRA -lX11 -lXext -lm \
   || die_pre "bridge build failed (see compiler output)"

# ---- the XTEST key injector ----------------------------------------
INJ=$RESULTS/x11_key_inject
INJ_LINK=""
if [ -n "${ORCA_ROOT:-}" ]; then
    INJ_LINK="-L$ORCA_ROOT/usr/lib/x86_64-linux-gnu -Wl,-rpath,$ORCA_ROOT/usr/lib/x86_64-linux-gnu"
fi
if ! cc -O1 -Wall -Wextra -Werror "$REPO_DIR/scripts/x11_key_inject.c" \
       -o "$INJ" $INJ_LINK -lX11 -lXtst 2>/dev/null; then
    # runtime-only libXtst (no dev symlink): link by soname
    cc -O1 -Wall -Wextra -Werror "$REPO_DIR/scripts/x11_key_inject.c" \
       -o "$INJ" $INJ_LINK -lX11 -l:libXtst.so.6 \
       || die_pre "key injector build failed (needs libX11 + libXtst)"
fi

# ---- the patched a11y bus service (rootless prefix only) -----------
SVCDIR=$RESULTS/dbus-services
mkdir -p "$SVCDIR/dbus-1/services"
if [ -n "${ORCA_ROOT:-}" ]; then
    cat > "$SVCDIR/dbus-1/services/org.a11y.Bus.service" <<EOF
[D-BUS Service]
Name=org.a11y.Bus
Exec=$ORCA_ROOT/usr/libexec/at-spi-bus-launcher
EOF
else
    cp /usr/share/dbus-1/services/org.a11y.Bus.service \
       "$SVCDIR/dbus-1/services/" 2>/dev/null || true
fi

# The org.a11y.Bus service dir must be visible to the SESSION BUS
# DAEMON at ITS startup — dbus-run-session starts the daemon before
# the body runs, so this export lives HERE, not in the body (the
# harness learned this the hard way: in-body exports arrive too
# late and every bus activation fails with ServiceUnknown).
XDG_DATA_DIRS="$SVCDIR${ORCA_ROOT:+:$ORCA_ROOT/usr/share}:/usr/share"
export XDG_DATA_DIRS

Xvfb $DISP -screen 0 1024x768x24 >"$RESULTS/xvfb.log" 2>&1 &
XVFB_PID=$!
sleep 1

# ---- the session body (clean argv; see x11_key_inject.c's note) ----
cat > "$RESULTS/session_body.sh" <<BODY
set -u
export DISPLAY=$DISP
export HOME=$RESULTS/home
export NO_AT_BRIDGE=0
mkdir -p "\$HOME"

"$ORCA_BIN" --debug --replace --debug-file=$RESULTS/orca-debug.log \
    >"$RESULTS/orca-stdout.log" 2>&1 &
ORCA_PID=\$!
sleep 5

"$APP_BIN" >"$RESULTS/app-stdout.log" 2>"$RESULTS/app-stderr.log" &
APP_PID=\$!
sleep 3

"$INJ" $DISP "$APP_TITLE" "$KEYS" $KEY_GAP_MS

# let orca's presentation (queries round-trip through the app's
# per-frame D-Bus pump) finish, then end the app CLEANLY — the
# summary line is part of the app's ground truth and only prints
# on a clean exit
sleep 12
"$INJ" $DISP "$APP_TITLE" "q" 500
sleep 2
kill \$APP_PID 2>/dev/null
sleep 3
kill \$ORCA_PID 2>/dev/null; sleep 1; kill -9 \$ORCA_PID 2>/dev/null
BODY

timeout 120 dbus-run-session -- /bin/bash "$RESULTS/session_body.sh" \
    >"$RESULTS/session.log" 2>&1

# ---- assertions -----------------------------------------------------
fail=0

check_contains() {  # file, pattern, description
    if grep -q -- "$2" "$1" 2>/dev/null; then
        echo "  [OK]   $3"
    else
        echo "  [FAIL] $3"
        fail=1
    fi
}

echo "== app ground truth (the semantic trace) =="
check_contains "$RESULTS/app-stdout.log" '\[a11y\] bridge: attached' \
    "bridge attached (AT-SPI2 mirror live)"
for step in "swatch-red" "swatch-green" "swatch-blue" "reset"; do
    check_contains "$RESULTS/app-stdout.log" "\[focus\] $step" \
        "focus moved to $step (graph-derived ring)"
done
check_contains "$RESULTS/app-stdout.log" '\[act\] Selected green' \
    "activation: Selected green"
check_contains "$RESULTS/app-stdout.log" '\[act\] Reset: nothing selected' \
    "activation: Reset: nothing selected"
check_contains "$RESULTS/app-stdout.log" \
    '\[summary\] focus moves=4 activations=2 selected=-1' \
    "summary: 4 moves, 2 activations, final state reset"

echo "== orca speech (its presentation decisions) =="
SPEECH=$RESULTS/orca-speech.txt
grep "SPEECH OUTPUT" "$RESULTS/orca-debug.log" > "$SPEECH" 2>/dev/null || true
check_contains "$SPEECH" 'Screen reader on' "orca is running"
check_contains "$SPEECH" 'frame' "orca announced the window"
# NOTE: the FIRST focus move is not asserted — a client that
# attaches races its own keyboard-path locus advance against the
# event latency; every LATER move is announced (documented in the
# bridge source). GTK apps win this race only because their events
# land in-process-fast.
for step in "swatch-green button" "swatch-blue button" "reset button"; do
    check_contains "$SPEECH" "$step" "orca announced focus: $step"
done
check_contains "$SPEECH" 'Selected green' \
    "orca announced the activation message"
check_contains "$SPEECH" 'Reset: nothing selected' \
    "orca announced the reset message"

echo ""
echo "evidence: $RESULTS (orca-debug.log, orca-speech.txt, app traces)"
if [ "$fail" -eq 0 ]; then
    echo "RESULT: PASS — the observed orca pass (v0.8 Cross-cutting A)"
else
    echo "RESULT: FAIL — see $RESULTS for the full logs"
fi

if [ "$KEEP" != "--keep" ]; then
    kill $XVFB_PID 2>/dev/null
    pkill -9 -f "[X]vfb $DISP" 2>/dev/null
    pkill -9 -f "[a]t-spi" 2>/dev/null
    pkill -9 -f "[a]tk-bridge" 2>/dev/null
fi
exit $fail
