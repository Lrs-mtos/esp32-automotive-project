#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

extern int current_screen_state;
extern lv_obj_t * label_debug;

// Public UI Declarations
void setup_ui();
void load_screen(int screen_id);
void update_ui();

#endif // UI_MANAGER_H