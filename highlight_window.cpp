
#include "highlight_window.h"
#include <iostream>

HighLightWindow::HighLightWindow(Window target_window, int border_width, int border_length, unsigned int border_color)
: stop_event_monitor_(false)
, highlight_border_width_(border_width)
, highlight_border_length_(border_length)
, highlight_border_color_argb_(border_color) {
    display_ = XOpenDisplay(NULL);
    if (display_ == NULL) {
        std::cout << "Failed to open display";
        is_initialized_ = false;
    }

    target_window_ = target_window;
    is_initialized_ = true;
}


HighLightWindow::~HighLightWindow() {
    if (!is_initialized_) {
        return;
    }
    Stop();
    if (highlight_window_ > 0) {
        XDestroyWindow(display_, highlight_window_);
    }
    if (display_) {
        XCloseDisplay(display_);
    }
}

void HighLightWindow::Start() {
    if (!is_initialized_) {
        return;
    }

    CreateHighlightWindow();
    
    // 订阅目标窗口事件
    XSelectInput(display_, target_window_, StructureNotifyMask | ExposureMask);

    // 订阅高亮窗口事件
    XSelectInput(display_, highlight_window_, ExposureMask);

    event_thread_ = std::thread([this]() {
        XEvent event;
        while (!stop_event_monitor_) {
            if (XPending(display_)) {
                do {
                    XNextEvent(display_, &event);
                    if (event.type == ConfigureNotify) {
                        if (event.xconfigure.window == target_window_) { 
                            HighlightWindowMoveResizeHandle();
                        }
                    } else if (event.type == Expose) {
                        if (event.xexpose.window == target_window_) {
                            HighlightWindowMoveResizeHandle();
                        } else if (event.xexpose.window == highlight_window_) {
                            RedrawRectangleHandle();
                        }
                    }
                } while (XPending(display_));
            }
            UpdateHighLightWindowMapState();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void HighLightWindow::Stop() {
    if (!is_initialized_) {
        return;
    }

    if (is_stopped_) {
        return;
    }
    // 停止事件循环
    stop_event_monitor_ = true;

    if (event_thread_.joinable()) {
        event_thread_.join();
    }
    is_stopped_ = true;
}

void HighLightWindow::UpdateHighLightWindowMapState() {
    if (!is_initialized_) {
        return;
    }

    // 获取目标窗口状态
    Atom property = XInternAtom(display_, "WM_STATE", False);
    const int kBitsPerByte = 8;
    Atom actual_type;
    int actual_format;
    unsigned long bytes_after;  // NOLINT: type required by XGetWindowProperty
    unsigned long size = 0;  // NOLINT: type required by XGetWindowProperty
    unsigned char* data = nullptr;
    int status = XGetWindowProperty(
      display_, target_window_, property, 0L, ~0L, False, AnyPropertyType,
      &actual_type, &actual_format, &size, &bytes_after, &data);
    if (status == Success && actual_format == 32) {
        int32_t state = *reinterpret_cast<int32_t*>(data);
        if (state == IconicState && is_highlight_window_visible_) {
            // Window is in minimized. unmap highlith window
            is_highlight_window_visible_ = false;
            XUnmapWindow(display_, highlight_window_);
        } else if (state == NormalState && is_highlight_window_visible_ == false) {
            // Window is in normal. map highlith window
            is_highlight_window_visible_ = true;
            XMapWindow(display_, highlight_window_);
        }
    }

    if (data) {
      XFree(data);
    }
}

void HighLightWindow::CreateHighlightWindow() {
    if (!is_initialized_) {
        return;
    }

    XWindowAttributes target_attrs;
    XGetWindowAttributes(display_, target_window_, &target_attrs);
    XVisualInfo vinfo;
    XMatchVisualInfo(display_, DefaultScreen(display_), 32, TrueColor, &vinfo);
    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(display_, DefaultRootWindow(display_), vinfo.visual, AllocNone);
    attr.border_pixel = 0; 
    attr.background_pixel = 0; // 透明背景
    unsigned long mask = CWColormap | CWBorderPixel | CWBackPixel;
    highlight_window_ = XCreateWindow(display_, DefaultRootWindow(display_), target_attrs.x, target_attrs.y,
            target_attrs.width, target_attrs.height, 0, vinfo.depth, InputOutput, vinfo.visual, mask, &attr);
    // 设置窗口name
    Atom wm_name_atom = XInternAtom(display_, "_NET_WM_NAME", False);
    XTextProperty wm_name_property;
    const char* wm_name = "Highlight-Window"; // note: don't rename, it used by other
    XStringListToTextProperty((char**)&wm_name, 1, &wm_name_property);
    XSetTextProperty(display_, highlight_window_, &wm_name_property, wm_name_atom);
    // 鼠标事件穿透
    XShapeCombineRectangles(display_, highlight_window_, ShapeInput, 0, 0, NULL, 0, ShapeSet, YXBanded);
    // 设置窗口始终悬浮在最上层 - Dock Window Type
    Atom atom = XInternAtom(display_, "_NET_WM_WINDOW_TYPE", False);
    Atom type = XInternAtom(display_, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(display_, highlight_window_, atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&type, 1);
    // 显示高亮窗口
    XMapWindow(display_, highlight_window_);
    
    std::cout << "CreateHighlightWindow, highlight_window_:" << highlight_window_ << std::endl;

    HighlightWindowMoveResizeHandle();
    RedrawRectangleHandle();
}

void HighLightWindow::ExtendHandle(int &x, int &y, int &width, int &height) {
    x = x - highlight_border_width_ / 2;
    y = y - highlight_border_width_ / 2;
    width = width + highlight_border_width_;
    height = height + highlight_border_width_;
}

void HighLightWindow::HighlightWindowMoveResizeHandle() {
    if (!is_initialized_) {
        return;
    }

    // 获取目标窗口属性
    XWindowAttributes target_attrs;
    XGetWindowAttributes(display_, target_window_, &target_attrs);
    int return_x = 0, return_y = 0;
    int return_w = target_attrs.width, return_h = target_attrs.height;
    Window child;
    // 坐标转换
    XTranslateCoordinates(display_, target_window_, DefaultRootWindow(display_), 0, 0, &return_x, &return_y, &child);
    // Extend handle
    ExtendHandle(return_x, return_y, return_w, return_h);
    // 窗口移动和重绘
    XMoveResizeWindow(display_, highlight_window_, return_x, return_y, return_w, return_h);
}

void HighLightWindow::RedrawRectangleHandle()
{
    XWindowAttributes attr;
    XGetWindowAttributes(display_, target_window_, &attr);
    XGCValues gcValues;
    gcValues.foreground = highlight_border_color_argb_; // argb
    gcValues.line_width = highlight_border_width_;
    unsigned long valuemask = GCForeground | GCLineWidth;
    GC gc = XCreateGC(display_, highlight_window_, valuemask, &gcValues);
    XClearWindow(display_, highlight_window_);

    // Extend handle: offset
    int x_offset = highlight_border_width_ / 2;
    int y_offset = highlight_border_width_ / 2;

    Point left_top = Point(x_offset, y_offset);
    Point left_bottom = Point(x_offset, y_offset + attr.height);
    Point right_top = Point(x_offset + attr.width, y_offset);
    Point right_bottom = Point(x_offset + attr.width, y_offset + attr.height);
        
    if (highlight_border_length_ == -1) {
        XDrawRectangle(display_, highlight_window_, gc, x_offset, y_offset, attr.width, attr.height);
    } else {
        // left top
        XDrawLine(display_, highlight_window_, gc, left_top.x - highlight_border_width_ / 2, left_top.y, left_top.x + std::min(highlight_border_length_, attr.width), left_top.y);
        XDrawLine(display_, highlight_window_, gc, left_top.x, left_top.y, left_top.x, left_top.y + std::min(highlight_border_length_, attr.height));
        // left bottom
        XDrawLine(display_, highlight_window_, gc,  left_bottom.x - highlight_border_width_ / 2, left_bottom.y, left_bottom.x + std::min(highlight_border_length_, attr.width), left_bottom.y);
        XDrawLine(display_, highlight_window_, gc,  left_bottom.x, left_bottom.y, left_bottom.x, left_bottom.y - std::min(highlight_border_length_, attr.height));
        // right top
        XDrawLine(display_, highlight_window_, gc, right_top.x + highlight_border_width_ / 2, right_top.y, right_top.x - std::min(highlight_border_length_, attr.width), right_top.y);
        XDrawLine(display_, highlight_window_, gc, right_top.x, right_top.y, right_top.x, right_top.y + std::min(highlight_border_length_, attr.height));
        // right bottom
        XDrawLine(display_, highlight_window_, gc,  right_bottom.x + highlight_border_width_ / 2, right_bottom.y, right_bottom.x - std::min(highlight_border_length_, attr.width), right_bottom.y);
        XDrawLine(display_, highlight_window_, gc,  right_bottom.x, right_bottom.y, right_bottom.x, right_bottom.y - std::min(highlight_border_length_, attr.height));
    }

    XFreeGC(display_, gc);
}
