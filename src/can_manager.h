#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <Arduino.h>
#include "driver/twai.h"

// Shared Telemetry Variables
extern int current_rpm;
extern int target_rpm;
extern int current_kmh;
extern int target_kmh;
extern int simulated_oil_temp;
extern float calculated_boost;
extern float current_maf_gs;
extern float current_battery_volts;

extern bool ecu_connected;
extern bool twai_ok;
extern uint32_t can_rx_packets;
extern uint32_t obd_rx_packets;

// Public CAN Function Declarations
void setup_twai();
void check_twai_state();
bool request_obd_pid(uint8_t pid);
void read_obd_data();
void process_obd_requests();
void update_obd_simulator();

#endif // CAN_MANAGER_H