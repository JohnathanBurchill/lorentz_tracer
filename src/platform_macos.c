/* macOS-specific window fixes via objc_msgSend. Pure C, no .m needed. */
#ifdef __APPLE__
#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>

void macos_fix_window_switching(void *nswindow)
{
    if (!nswindow) return;
    id win = (id)nswindow;
    /* NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorFullScreenAuxiliary */
    unsigned long behavior = (1 << 2) | (1 << 8);
    ((void (*)(id, SEL, unsigned long))objc_msgSend)(
        win, sel_registerName("setCollectionBehavior:"), behavior);
    /* NSNormalWindowLevel = 0 */
    ((void (*)(id, SEL, long))objc_msgSend)(
        win, sel_registerName("setLevel:"), 0L);
}

/* Hide the menu bar and Dock while the app is frontmost (on = 1), so a
 * borderless window sized to the screen covers all of it, or show them
 * again. Unlike a monitor-bound fullscreen window, this never changes the
 * display mode. */
void macos_set_fullscreen_presentation(int on)
{
    id app = ((id (*)(id, SEL))objc_msgSend)(
        (id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
    /* NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar */
    unsigned long opts = on ? ((1UL << 1) | (1UL << 3)) : 0UL;
    ((void (*)(id, SEL, unsigned long))objc_msgSend)(
        app, sel_registerName("setPresentationOptions:"), opts);
}
#endif
