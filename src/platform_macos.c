/* macOS-specific window fixes via objc_msgSend. Pure C, no .m needed. */
#ifdef __APPLE__
#include <objc/objc.h>
#include <objc/message.h>

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
#endif
