/*
 * cocoa.c - macOS Cocoa backend for Planex (Stage 7+)
 *
 * Uses NSWindow + NSBitmapImageRep for software rasterization.
 *
 * Stage 14: IME support via NSTextInputClient protocol.
 * - PlanexView conforms to NSTextInputClient
 * - insertText: → PX_EV_IME_COMMIT (committed UTF-8 text)
 * - setMarkedText: → PX_EV_IME_COMPOSE (preedit, Stage 15+ rendering)
 * - doCommandBySelector: → handles special keys (backspace, arrows, etc.)
 *
 * Build:
 *   clang -std=c17 -Iinclude -DPLANEX_BACKEND_COCOA \
 *     -framework Cocoa -framework Foundation \
 *     src/relation.c src/estimate.c src/closure.c src/fb.c src/font.c \
 *     src/font_ttf.c src/app.c src/cocoa.m examples/counter_perception_window.c \
 *     -o counter
 *
 * Note: this file must be compiled as Objective-C (.m extension).
 * On macOS, rename to cocoa.m. The .c extension here is for
 * Makefile pattern rules — it must be compiled with -x objective-c.
 */

#if defined(__APPLE__) && defined(__MACH__)

#import <Cocoa/Cocoa.h>
#include "planex/window.h"
#include "planex/fb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Stage 14: PlanexView with NSTextInputClient protocol
 *
 * This view receives key events and IME input. It converts
 * Cocoa's NSString/NSDictionary text to UTF-8 and queues
 * px_event structs for the app loop to poll.
 * ============================================================ */

@interface PlanexView : NSView <NSTextInputClient> {
@public
    px_fb*       fb;
    NSBitmapImageRep* bitmapRep;
    /* Event queue (simple 1-slot buffer for poll_event) */
    px_event     queued_event;
    bool         has_queued;
    /* IME state */
    NSString*    marked_text;     /* current preedit text */
    NSRange     marked_range;
}
- (void)drawRect:(NSRect)dirtyRect;
- (void)setFb:(px_fb*)f;
- (void)queueEvent:(px_event)ev;
@end

@implementation PlanexView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        bitmapRep = nil;
        has_queued = false;
        marked_text = nil;
        marked_range = NSMakeRange(0, 0);
    }
    return self;
}

- (void)dealloc {
    if (bitmapRep) [bitmapRep release];
    if (marked_text) [marked_text release];
    [super dealloc];
}

- (void)setFb:(px_fb*)f {
    fb = f;
    if (bitmapRep) [bitmapRep release];
    int w = px_fb_width(fb);
    int h = px_fb_height(fb);
    bitmapRep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL
                      pixelsWide:w
                      pixelsHigh:h
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSCalibratedRGBColorSpace
                     bytesPerRow:w * 4
                    bitsPerPixel:32];
}

- (void)queueEvent:(px_event)ev {
    queued_event = ev;
    has_queued = true;
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    if (!fb || !bitmapRep) return;

    const uint32_t* src = px_fb_pixels(fb);
    uint8_t* dst = [bitmapRep bitmapData];
    int w = px_fb_width(fb);
    int h = px_fb_height(fb);
    memcpy(dst, src, (size_t)w * h * 4);

    [NSGraphicsContext saveGraphicsState];
    [bitmapRep drawInRect:NSMakeRect(0, 0, w, h)];
    [NSGraphicsContext restoreGraphicsState];
}

/* ============================================================
 * NSResponder overrides (key events)
 * ============================================================ */

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    /* Stage 14: Cocoa sends key events to the input context first.
     * The input context will call our NSTextInputClient methods
     * (insertText:, setMarkedText:, etc.) appropriately.
     * We just need to pass the event to the input context. */
    [self interpretKeyEvents:[NSArray arrayWithObject:event]];
}

- (void)mouseDown:(NSEvent*)event {
    NSPoint p = [event locationInWindow];
    px_event ev = {0};
    ev.kind = PX_EV_MOUSE_DOWN;
    ev.x = (int)p.x;
    ev.y = (int)(px_fb_height(fb) - p.y);  /* flip Y */
    ev.button = 1;
    [self queueEvent:ev];
}

/* ============================================================
 * Stage 14: NSTextInputClient protocol
 * ============================================================ */

/* insertText: — Called when IME commits text (or regular typing).
 * The text may be a single ASCII char or multi-byte CJK. */
- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    (void)replacementRange;

    NSString* text = nil;
    if ([string isKindOfClass:[NSString class]]) {
        text = (NSString*)string;
    } else if ([string isKindOfClass:[NSAttributedString class]]) {
        text = [(NSAttributedString*)string string];
    }

    if (!text || [text length] == 0) return;

    /* Clear any marked (preedit) text */
    if (marked_text) {
        [marked_text release];
        marked_text = nil;
        marked_range = NSMakeRange(0, 0);
    }

    /* Convert NSString to UTF-8 */
    const char* utf8 = [text UTF8String];
    if (!utf8) return;

    size_t len = strlen(utf8);
    if (len == 0) return;

    /* Check if it's a single ASCII char */
    if (len == 1 && (unsigned char)utf8[0] < 0x80) {
        px_event ev = {0};
        ev.kind = PX_EV_KEY_DOWN;
        ev.key_char = utf8[0];
        [self queueEvent:ev];
    } else {
        /* Multi-byte — IME commit */
        px_event ev = {0};
        ev.kind = PX_EV_IME_COMMIT;
        strncpy(ev.ime_text, utf8, sizeof(ev.ime_text) - 1);
        ev.ime_text[sizeof(ev.ime_text) - 1] = 0;
        [self queueEvent:ev];
    }
}

/* setMarkedText: — Called during IME composition (preedit).
 * The text is intermediate, not yet committed. */
- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange
        replacementRange:(NSRange)replacementRange {
    (void)selectedRange;
    (void)replacementRange;

    NSString* text = nil;
    if ([string isKindOfClass:[NSString class]]) {
        text = (NSString*)string;
    } else if ([string isKindOfClass:[NSAttributedString class]]) {
        text = [(NSAttributedString*)string string];
    }

    if (marked_text) [marked_text release];
    marked_text = text ? [text copy] : nil;

    if (marked_text && [marked_text length] > 0) {
        /* Fire PX_EV_IME_COMPOSE with preedit text */
        const char* utf8 = [marked_text UTF8String];
        if (utf8) {
            px_event ev = {0};
            ev.kind = PX_EV_IME_COMPOSE;
            strncpy(ev.ime_text, utf8, sizeof(ev.ime_text) - 1);
            ev.ime_text[sizeof(ev.ime_text) - 1] = 0;
            [self queueEvent:ev];
        }
    }

    marked_range = NSMakeRange(0, marked_text ? [marked_text length] : 0);
    [self setNeedsDisplay:YES];
}

/* unmarkText: — Called when IME cancels composition. */
- (void)unmarkText {
    if (marked_text) {
        [marked_text release];
        marked_text = nil;
        marked_range = NSMakeRange(0, 0);
    }
    [self setNeedsDisplay:YES];
}

/* hasMarkedText — Return whether there's preedit text. */
- (BOOL)hasMarkedText {
    return marked_text != nil && [marked_text length] > 0;
}

/* markedRange — Range of marked text within the "input buffer". */
- (NSRange)markedRange {
    if (!marked_text || [marked_text length] == 0) {
        return NSMakeRange(NSNotFound, 0);
    }
    return marked_range;
}

/* selectedRange — We don't maintain a selection. */
- (NSRange)selectedRange {
    return NSMakeRange(NSNotFound, 0);
}

/* validAttributesForMarkedText — Return empty array (no attributes). */
- (NSArray*)validAttributesForMarkedText {
    return [NSArray array];
}

/* attributedSubstringForProposedRange: — We don't support this
 * (no text buffer to extract from). Return nil. */
- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
    (void)range;
    (void)actualRange;
    return nil;
}

/* doCommandBySelector: — Called for special keys (backspace, arrows,
 * enter, escape, etc.) that aren't handled by insertText. */
- (void)doCommandBySelector:(SEL)selector {
    /* Map common selectors to key events */
    if (selector == @selector(insertBackspace:)) {
        px_event ev = {0};
        ev.kind = PX_EV_KEY_DOWN;
        ev.key_char = 127;  /* ASCII DEL — treated as backspace by app */
        [self queueEvent:ev];
    } else if (selector == @selector(insertNewline:)) {
        px_event ev = {0};
        ev.kind = PX_EV_KEY_DOWN;
        ev.key_char = '\n';
        [self queueEvent:ev];
    } else if (selector == @selector(cancel:)) {
        /* Escape key */
        px_event ev = {0};
        ev.kind = PX_EV_KEY_DOWN;
        ev.key_char = 27;  /* ESC */
        [self queueEvent:ev];
    }
    /* Other selectors (moveLeft:, moveRight:, etc.) are silently
     * ignored. Stage 15+ can add arrow key handling. */
}

/* ============================================================
 * End of NSTextInputClient
 * ============================================================ */

@end

/* ============================================================
 * px_window struct and API
 * ============================================================ */

struct px_window {
    NSWindow*    window;
    PlanexView*   view;
    int          width;          /* logical (CSS) */
    int          height;         /* logical (CSS) */
    double       scale;          /* backingScaleFactor: 1.0 or 2.0 */
    px_fb*       fb;             /* physical size */
    bool         should_close;
    bool         visible;
    px_resize_callback on_resize_cb;
    void*              on_resize_user;
};

@interface PlanexWindowDelegate : NSObject <NSWindowDelegate>
@property (assign) px_window* planexWindow;
@end

@implementation PlanexWindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)sender {
    (void)sender;
    if (self.planexWindow) {
        self.planexWindow->should_close = true;
    }
    return YES;
}
- (void)windowDidResize:(NSNotification*)notification {
    (void)notification;
    if (!self.planexWindow) return;
    NSRect frame = [self.planexWindow->view bounds];
    int new_w = (int)frame.size.width;
    int new_h = (int)frame.size.height;
    if (new_w > 0 && new_h > 0 &&
        (new_w != self.planexWindow->width ||
         new_h != self.planexWindow->height)) {
        px_fb_resize(self.planexWindow->fb, new_w, new_h);
        self.planexWindow->width = new_w;
        self.planexWindow->height = new_h;
        [self.planexWindow->view setFb:self.planexWindow->fb];
        if (self.planexWindow->on_resize_cb) {
            self.planexWindow->on_resize_cb(self.planexWindow, new_w, new_h,
                                             self.planexWindow->on_resize_user);
        }
    }
}
@end

static PlanexWindowDelegate* s_delegate = nil;

const char* px_window_backend_name(void) {
    return "cocoa";
}

px_window* px_window_new(int width, int height, const char* title) {
    if (!NSApp) {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    }

    px_window* w = (px_window*)calloc(1, sizeof(px_window));
    if (!w) return NULL;
    w->width  = width;
    w->height = height;
    w->should_close = false;
    w->visible = false;
    w->scale = 1.0;  /* will be updated after window creation */

    NSRect frame = NSMakeRect(100, 100, width, height);
    NSUInteger style = NSWindowStyleMaskTitled |
                       NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable |
                       NSWindowStyleMaskResizable;
    w->window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:style
                    backing:NSBackingStoreBuffered
                      defer:NO];
    if (title) {
        [w->window setTitle:[NSString stringWithUTF8String:title]];
    }

    /* Create custom view with NSTextInputClient support */
    w->view = [[PlanexView alloc] initWithFrame:frame];
    [w->view setFb:w->fb];
    [w->window setContentView:w->view];

    if (!s_delegate) {
        s_delegate = [[PlanexWindowDelegate alloc] init];
    }
    s_delegate.planexWindow = w;
    [w->window setDelegate:s_delegate];

    [w->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    /* Stage 15: detect backing scale factor (1.0 = normal, 2.0 = Retina) */
    w->scale = [w->window backingScaleFactor];
    if (w->scale < 1.0) w->scale = 1.0;

    /* Allocate fb at physical resolution */
    int pw = (int)(w->width * w->scale);
    int ph = (int)(w->height * w->scale);
    w->fb = px_fb_new(pw, ph);

    /* Create custom view with NSTextInputClient support */
    w->view = [[PlanexView alloc] initWithFrame:frame];
    [w->view setFb:w->fb];
    [w->window setContentView:w->view];

    return w;
}

void px_window_free(px_window* w) {
    if (!w) return;
    if (w->window) [w->window release];
    if (w->view) [w->view release];
    if (w->fb) px_fb_free(w->fb);
    free(w);
}

void px_window_on_resize(px_window* w, px_resize_callback cb, void* user) {
    if (!w) return;
    w->on_resize_cb   = cb;
    w->on_resize_user = user;
}

int px_window_width(px_window* w)  { return w ? w->width  : 0; }
int px_window_height(px_window* w) { return w ? w->height : 0; }
double px_window_scale(px_window* w) { return w ? w->scale : 1.0; }
int px_window_fb_width(px_window* w) {
    return w ? (int)(w->width * w->scale) : 0;
}
int px_window_fb_height(px_window* w) {
    return w ? (int)(w->height * w->scale) : 0;
}

px_fb* px_window_fb(px_window* w) {
    return w ? w->fb : NULL;
}

int px_window_show(px_window* w) {
    if (!w) return -1;
    [w->window makeKeyAndOrderFront:nil];
    w->visible = true;
    return 0;
}

px_event px_window_poll_event(px_window* w) {
    px_event ev = { .kind = PX_EV_NONE };
    if (!w) return ev;

    /* Check if view has a queued event (from NSTextInputClient) */
    if (w->view->has_queued) {
        ev = w->view->queued_event;
        w->view->has_queued = false;
        return ev;
    }

    /* Process one event from the Cocoa event loop */
    NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:[NSDate distantPast]
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];
    if (event) {
        [NSApp sendEvent:event];
        /* Check if the view queued something */
        if (w->view->has_queued) {
            ev = w->view->queued_event;
            w->view->has_queued = false;
            return ev;
        }
    }

    return ev;
}

void px_window_present(px_window* w) {
    if (!w || !w->fb) return;
    [w->view setNeedsDisplay:YES];
    [w->view display];
}

void px_window_close(px_window* w) {
    if (!w) return;
    w->should_close = true;
    [w->window close];
}

bool px_window_should_close(px_window* w) {
    return w ? w->should_close : true;
}

#endif /* __APPLE__ && __MACH__ */
