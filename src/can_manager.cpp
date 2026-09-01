#include "can_manager.h"
#include "config.h"
#include <math.h>

// Telemetry Definitions
int current_rpm = 0;
int target_rpm = 0;
int current_kmh = 0;
int target_kmh = 0;
int simulated_oil_temp = 50;
float calculated_boost = 0.0f;
float current_maf_gs = 0.0f;
float current_battery_volts = 0.0f;

bool ecu_connected = false;
bool twai_ok = false;
uint32_t can_rx_packets = 0;
uint32_t obd_rx_packets = 0;

static unsigned long last_can_msg_time = 0;
static unsigned long last_request_time = 0;
static unsigned long last_recovery_time = 0;

// Standard OBD-II PID List
static const uint8_t standard_pids_list[] = {0x0C, 0x0D, 0x5C, 0x0B, 0x10, 0x42};
static int current_pid_index = 0;

void setup_twai() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN,
        (gpio_num_t)CAN_RX_PIN,
        TWAI_MODE_NORMAL
    );
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("Error installing TWAI driver: %d\n", err);
        twai_ok = false;
        return;
    }
    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("Error starting TWAI: %d\n", err);
        twai_ok = false;
        return;
    }
    twai_ok = true;
    Serial.println("TWAI driver started successfully!");
}

void check_twai_state() {
    if (!twai_ok) return;
    twai_status_info_t status;
    if (twai_get_status_info(&status) != ESP_OK) return;

    if (status.state == TWAI_STATE_BUS_OFF) {
        ecu_connected = false;
        if (millis() - last_recovery_time > 1000) {
            last_recovery_time = millis();
            Serial.println("TWAI BUS-OFF: initiating recovery");
            twai_initiate_recovery();
        }
    }
    else if (status.state == TWAI_STATE_STOPPED) {
        Serial.println("TWAI stopped: restarting");
        twai_start();
    }
}

bool request_obd_pid(uint8_t pid) {
    if (!twai_ok) return false;
    twai_message_t message = {};
    message.identifier = 0x7DF;
    message.extd = 0;
    message.rtr = 0;
    message.data_length_code = 8;
    message.data[0] = 0x02;
    message.data[1] = 0x01;
    message.data[2] = pid;

    esp_err_t err = twai_transmit(&message, pdMS_TO_TICKS(10));
    return (err == ESP_OK);
}

void read_obd_data() {
    if (!twai_ok) return;
    twai_message_t message = {};

    while (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
        can_rx_packets++;
        if (message.rtr) continue;
        if (message.data_length_code < 4) continue;

        last_can_msg_time = millis();

        if (message.identifier >= 0x7E8 && message.identifier <= 0x7EF) {
            obd_rx_packets++;
            if (!ecu_connected) {
                Serial.println("CAR CONNECTED! Turning off simulator...");
            }
            ecu_connected = true;
            if (message.data[1] != 0x41) continue;

            uint8_t pid = message.data[2];
            if ((pid == 0x0C || pid == 0x42) && message.data_length_code < 5) continue;

            if (pid == 0x0C) {
                target_rpm = ((message.data[3] * 256) + message.data[4]) / 4;
            } else if (pid == 0x0D) {
                target_kmh = message.data[3];
            } else if (pid == 0x5C) {
                simulated_oil_temp = message.data[3] - 40;
            } else if (pid == 0x0B) {
                int map_kpa = message.data[3];
                calculated_boost = (map_kpa - 100) / 100.0f;
                if (calculated_boost < 0.0f) calculated_boost = 0.0f;
            } else if (pid == 0x10) { 
                current_maf_gs = ((message.data[3] * 256.0f) + message.data[4]) / 100.0f;
            } else if (pid == 0x42) {
                current_battery_volts = ((message.data[3] * 256.0f) + message.data[4]) / 1000.0f;
            }
        }
    }
    if (ecu_connected && millis() - last_can_msg_time > 2000) {
        ecu_connected = false;
        Serial.println("ECU disconnected");
    }
}

void process_obd_requests() {
    if (ecu_connected && (millis() - last_request_time >= OBD_REQUEST_INTERVAL_MS)) {
        request_obd_pid(standard_pids_list[current_pid_index]);
        current_pid_index = (current_pid_index + 1) % sizeof(standard_pids_list);
        last_request_time = millis();
    }
}

void update_obd_simulator() {
    target_rpm = 3000 + (sin(millis() / 500.0f) * 2000);
    target_kmh = target_rpm / 30;
    calculated_boost = 0.5f + (sin(millis() / 500.0f) * 0.5f);
    if (calculated_boost < 0.0f) calculated_boost = 0.0f;
    simulated_oil_temp = 90 + (sin(millis() / 1000.0f) * 5);
    current_battery_volts = 12.5f + (sin(millis() / 1000.0f) * 1.5f);
    current_maf_gs = 20.0f + (sin(millis() / 500.0f) * 100.0f);
    if (current_maf_gs < 0.0f) current_maf_gs = 0.0f;
}