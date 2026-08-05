#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// CONFIGURAÇÕES DO HARDWARE
// ==========================================
#define CAN_TX_PIN 1 
#define CAN_RX_PIN 2  
#define SCREEN_DIR 1

// ==========================================
// TIMERS E INTERVALOS (MS)
// ==========================================
#define OBD_REQUEST_INTERVAL_MS  50
#define UI_UPDATE_INTERVAL_MS    50
#define DEBUG_UPDATE_INTERVAL_MS 500

// Quantidade total de telas no sistema
#define TOTAL_SCREENS 6

#endif // CONFIG_H