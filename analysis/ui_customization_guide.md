# UI Customization Guide (Fonts & Emotes)

## 1. Modifying Fonts
The project uses LVGL fonts.

*   **Location**: `main/display/emote_display.cc` (for Emote Mode) or `lvgl_display.cc` (for Default Mode).
*   **Default Font**: Look for `LV_FONT_DECLARE(BUILTIN_TEXT_FONT);`. This usually links to a font defined in `main/assets` or a standard LVGL font (like `lv_font_montserrat_20`).
*   **How to Change Size/Style**:
    1.  **Generate a new font**: Use the online [LVGL Font Converter](https://lvgl.io/tools/fontconverter) to convert your `.ttf` file to a C array.
    2.  **Add to project**: Save the `.c` file in `main/display/` or `main/assets/`.
    3.  **Declare it**: Add `LV_FONT_DECLARE(my_new_font);` in `emote_display.cc`.
    4.  **Use it**: Find `gfx_label_set_font(g_obj_label_toast, ...)` and replace the argument with `&my_new_font`.

## 2. Customizing "Emotes" (Robot Face)
The robot face animations are driven by **EAF (Easy Animation Format)** files and mapped via JSON.

*   **Mapping File**: `main/boards/esp-box-3/emote.json`
    *   This file maps "server emotions" (e.g., "happy", "sad") to specific animation files (e.g., `Happy.eaf`).
    *   **To change basic behavior**: Edit this JSON. For example, make "excited" use `shocked.eaf`.

*   **Animation Files**: `.eaf` files are binary animation assets stored in the flash partition.
    *   **Replacing Emotions**:
        1.  You need to create new `.eaf` files (likely using a specific tool or converting from GIF/Samsung-EAF).
        2.  Place them in the assets folder that gets packed into the flash image.
        3.  Update `emote.json` to point to the new filename.

## 3. Customizing the Default UI (LVGL)
If you are using the Standard interface (not the robot face):
*   **File**: `main/display/lvgl_display/lvgl_display.cc`
*   **Changing Layout**: The UI is built using standard LVGL widgets (`lv_label`, `lv_obj`).
*   **Example**: To move the status bar, look for `status_label_` creation and change `lv_obj_align`.
