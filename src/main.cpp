#include <unihiker_k10.h>
#include <lvgl.h>
#include "config.h"
#include "can_manager.h"
#include "ui_manager.h"

UNIHIKER_K10 k10;

volatile bool flag_button_a = false;
volatile bool flag_button_b = false;

static unsigned long last_debug_time = 0;
static unsigned long connection_attempt_time = 0;
static unsigned long last_update_time = 0;

void onButtonAPressed() { 
    flag_button_a = true; 
}

void onButtonBPressed() { 
    flag_button_b = !flag_button_b; 
    Serial.println("BUTTON B pressed"); 
}

void setup() {
    Serial.begin(115200);
    k10.begin();
    k10.initScreen(SCREEN_DIR);
    k10.creatCanvas();
    k10.buttonA->setPressedCallback(onButtonAPressed);
    k10.buttonB->setPressedCallback(onButtonBPressed);

    setup_twai();
    setup_ui();
}

void loop() {
    check_twai_state();
    
    // 1. Debug Label
    if (millis() - last_debug_time >= DEBUG_UPDATE_INTERVAL_MS) {
        last_debug_time = millis();
        if (twai_ok && label_debug != NULL) {
            twai_status_info_t status;
            twai_get_status_info(&status);
        
            const char *can_state = "OK";
            if (status.state == TWAI_STATE_BUS_OFF) can_state = "BUS-OFF";
            else if (status.state == TWAI_STATE_STOPPED) can_state = "STOPPED";
            else if (status.state == TWAI_STATE_RECOVERING) can_state = "RECOVERING";
        
            lv_label_set_text_fmt(label_debug,
                "ECU: %s\nHW CAN: %s\nCAN RX Packets: %u\nOBD RX Packets: %u\nTX Errors: %d",
                ecu_connected ? "Connected" : "Searching...",
                can_state,
                can_rx_packets,
                obd_rx_packets,
                status.tx_error_counter
            );
        } else if (label_debug != NULL) {
            lv_label_set_text(label_debug, "TWAI not started");
        }
    }
    
    // 2. Screen Switching (6 options)
    if (flag_button_a) {
        int next_screen = (current_screen_state + 1) % TOTAL_SCREENS; 
        load_screen(next_screen);
        flag_button_a = false;
    }

    // 3. Process CAN Bus
    read_obd_data();
  
    // 4. Search Mode vs Simulator Mode (Button B)
    if (!ecu_connected) {
        if ((millis() - connection_attempt_time >= 5000) && !flag_button_b) {
            connection_attempt_time = millis();
            request_obd_pid(0x00);
        }
    
        if (flag_button_b) {
            update_obd_simulator();
        }
    }
    else {
        process_obd_requests();
    }

    // 5. Graphic Update at 20 FPS (50ms)
    if (millis() - last_update_time >= UI_UPDATE_INTERVAL_MS) {
        last_update_time = millis(); 
        update_ui();
    }

    lv_timer_handler(); 
    delay(5);
}