#ifndef SRC__RUNTIME__SIMULATOR__MOUSE_MONITOR__H_
#define SRC__RUNTIME__SIMULATOR__MOUSE_MONITOR__H_

class Renderer;

namespace mouse_monitor {

// Installs an NSEvent local monitor (left mouse down/dragged/up) that drives
// the renderer's orbit-drag state. `contentHeight` is the window content
// height in points, used to ignore clicks in the title bar. Main-thread only;
// call once after the MTKView delegate (and hence the Renderer) exists.
void install(Renderer *pRenderer, double contentHeight);

} // namespace mouse_monitor

#endif // SRC__RUNTIME__SIMULATOR__MOUSE_MONITOR__H_
