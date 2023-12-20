# linux-x11-highlight-window
 Implement application window highlight based on X11, mainly used in screen sharing process to highlight the shared application window and prompt the user.

## compile

`cmake -B build -DCMAKE_BUILD_TYPE=Release`

`cmake --build build --config Release`


## run test
```
Usage: ./demo <window_id> <border_width> <border_length> <border_color:0xAARRGGBB>
```
1. window_id
The window id of target application window, you can query it by `xwininfo -int`
2. border_width
Highlighted border line width, measured in pixels, default value is 6 pixels.
3. border_length
The length of the highlighted border lines, from any corner of the window (4 corners in total), extending to the length of the adjacent sides. The unit is pix and the default value is 120 pix.Setting it to -1 indicates a fully encompassing highlighted box.
4. border_color
The color of the highlighted border, using the format 0xAARRGGBB, default value is 0xFF29CCA3.