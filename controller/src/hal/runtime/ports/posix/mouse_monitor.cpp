#include "mouse_monitor.h"

#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
#include <objc/message.h>
#include <objc/runtime.h>

#include "renderer.h"

namespace {
// NSEventType values / masks from AppKit's NSEvent.h, which has no
// metal-cpp-extensions wrapper: LeftMouseDown=1, LeftMouseUp=2,
// LeftMouseDragged=6. The mask is 1 << type.
constexpr unsigned long long kTypeLeftMouseDown = 1;
constexpr unsigned long long kTypeLeftMouseUp = 2;
constexpr unsigned long long kTypeLeftMouseDragged = 6;
constexpr unsigned long long kMask =
    (1ULL << kTypeLeftMouseDown) | (1ULL << kTypeLeftMouseUp) | (1ULL << kTypeLeftMouseDragged);
} // namespace

void mouse_monitor::install(Renderer *pRenderer, double contentHeight) {
  id (^handler)(id) = ^id(id evt) {
    auto type = ((unsigned long long (*)(id, SEL))objc_msgSend)(evt, sel_registerName("type"));
    if (type == kTypeLeftMouseDown) {
      // locationInWindow has a bottom-left origin, so anything above
      // contentHeight is the title bar; let those clicks move the window.
      CGPoint loc = ((CGPoint (*)(id, SEL))objc_msgSend)(evt, sel_registerName("locationInWindow"));
      if (loc.y >= 0 && loc.y <= contentHeight) {
        pRenderer->beginDrag();
      }
    } else if (type == kTypeLeftMouseDragged) {
      auto dx = ((CGFloat (*)(id, SEL))objc_msgSend)(evt, sel_registerName("deltaX"));
      auto dy = ((CGFloat (*)(id, SEL))objc_msgSend)(evt, sel_registerName("deltaY"));
      pRenderer->dragBy(static_cast<float>(dx), static_cast<float>(dy));
    } else if (type == kTypeLeftMouseUp) {
      pRenderer->endDrag();
    }
    return evt; // pass the event through to normal dispatch
  };

  // The returned monitor token must stay alive for the app's lifetime, so it
  // is intentionally leaked — same pattern as the delegate wrapper that
  // MTKView.hpp::setDelegate leaks.
  ((id (*)(Class, SEL, unsigned long long, id))objc_msgSend)(
      objc_lookUpClass("NSEvent"),
      sel_registerName("addLocalMonitorForEventsMatchingMask:handler:"), kMask, (id)handler);
}
