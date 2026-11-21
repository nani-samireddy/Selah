#pragma once
class QWindow;

namespace WindowChrome {
// Keep the document title for accessibility and window switching, but hide its visual caption.
void configure(QWindow *window);
}
