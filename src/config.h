#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// HARDWARE SETTINGS
// ==========================================
#define CAN_TX_PIN 1 
#define CAN_RX_PIN 2  
#define SCREEN_DIR 1

// ==========================================
// TIMERS AND INTERVALS (MS)
// ==========================================
#define OBD_REQUEST_INTERVAL_MS  50
#define UI_UPDATE_INTERVAL_MS    50
#define DEBUG_UPDATE_INTERVAL_MS 500

// Total screens on system
#define TOTAL_SCREENS 7

// Activates debug options
#define SHOW_DEBUG_OVERLAY 0

#endif // CONFIG_H