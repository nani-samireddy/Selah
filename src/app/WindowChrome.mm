#include "WindowChrome.h"
#include <QWindow>
#import <AppKit/AppKit.h>

void WindowChrome::configure(QWindow *window) {
    auto *view = reinterpret_cast<NSView *>(window->winId());
    view.window.titleVisibility = NSWindowTitleHidden;
}
