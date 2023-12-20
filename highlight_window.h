#ifndef HIGHLIGHT_WINDOW_H
#define HIGHLIGHT_WINDOW_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h> // XShapeCombineRectangles -lXext
#include <stdlib.h>
#include <thread>
#include <atomic>
#include <mutex>

class HighLightWindow {
public:
    HighLightWindow(Window target_window, int border_width, int border_length, unsigned int border_color);
    ~HighLightWindow();
    void Start();
    void Stop();

private:
    void CreateHighlightWindow();
    void HighlightWindowMoveResizeHandle();
    void RedrawRectangleHandle();
    void ExtendHandle(int &x, int &y, int &width, int &height);
    void UpdateHighLightWindowMapState();

    struct Point {
        int x;
        int y;

        Point() : x(0), y(0) {}
        Point(int x, int y) : x(x), y(y) {}
    };

private:
    Display *display_;
    Window target_window_ = 0; // real window
    Window highlight_window_ = 0;
    bool is_initialized_ = false;
    std::thread event_thread_;
    std::atomic_bool stop_event_monitor_;
    bool is_stopped_ = false;

    // highlight window's attributes
    int highlight_border_width_ = 6;
    int highlight_border_length_ = 120;
    unsigned int highlight_border_color_argb_ = 0xFF29CCA3; // argb

    // highlight window cache state
    bool is_highlight_window_visible_ = true;
};


#endif // HIGHLIGHT_WINDOW_H
