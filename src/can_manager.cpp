#include "can_manager.h"
#include "config.h"
#include <math.h>

// Definições de Telemetria
int rpm_atual = 0;
int rpm_alvo = 0;
int kmh_atual = 0;
int kmh_alvo = 0;
int temp_oleo_simulada = 50;
float boost_calculado = 0.0f;
float maf_gs_atual = 0.0f;
float bateria_volts_atual = 0.0f;

bool ecu_conectada = false;
bool twai_ok = false;
uint32_t pacotes_can_rx = 0;
uint32_t pacotes_obd_rx = 0;

static unsigned long tempo_ultima_mensagem_can = 0;
static unsigned long tempo_ultimo_pedido = 0;
static unsigned long tempo_ultima_recuperacao = 0;

// Lista de PIDs padrão OBD-II
static const uint8_t lista_pids_padrao[] = {0x0C, 0x0D, 0x5C, 0x0B, 0x10, 0x42};
static int indice_pid_atual = 0;

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
        Serial.printf("Erro ao instalar driver TWAI: %d\n", err);
        twai_ok = false;
        return;
    }
    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("Erro ao iniciar TWAI: %d\n", err);
        twai_ok = false;
        return;
    }
    twai_ok = true;
    Serial.println("Driver TWAI iniciado com sucesso!");
}

void verificar_estado_twai() {
    if (!twai_ok) return;
    twai_status_info_t status;
    if (twai_get_status_info(&status) != ESP_OK) return;

    if (status.state == TWAI_STATE_BUS_OFF) {
        ecu_conectada = false;
        if (millis() - tempo_ultima_recuperacao > 1000) {
            tempo_ultima_recuperacao = millis();
            Serial.println("TWAI BUS-OFF: iniciando recuperacao");
            twai_initiate_recovery();
        }
    }
    else if (status.state == TWAI_STATE_STOPPED) {
        Serial.println("TWAI parado: reiniciando");
        twai_start();
    }
}

bool solicitar_pid_obd(uint8_t pid) {
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

void ler_dados_obd() {
    if (!twai_ok) return;
    twai_message_t message = {};

    while (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
        pacotes_can_rx++;
        if (message.rtr) continue;
        if (message.data_length_code < 4) continue;

        tempo_ultima_mensagem_can = millis();

        if (message.identifier >= 0x7E8 && message.identifier <= 0x7EF) {
            pacotes_obd_rx++;
            if (!ecu_conectada) {
                Serial.println("CARRO CONECTADO! Desligando simulador...");
            }
            ecu_conectada = true;
            if (message.data[1] != 0x41) continue;

            uint8_t pid = message.data[2];
            if ((pid == 0x0C || pid == 0x42) && message.data_length_code < 5) continue;

            if (pid == 0x0C) {
                rpm_alvo = ((message.data[3] * 256) + message.data[4]) / 4;
            } else if (pid == 0x0D) {
                kmh_alvo = message.data[3];
            } else if (pid == 0x5C) {
                temp_oleo_simulada = message.data[3] - 40;
            } else if (pid == 0x0B) {
                int map_kpa = message.data[3];
                boost_calculado = (map_kpa - 100) / 100.0f;
                if (boost_calculado < 0.0f) boost_calculado = 0.0f;
            } else if (pid == 0x10) { 
                maf_gs_atual = ((message.data[3] * 256.0f) + message.data[4]) / 100.0f;
            } else if (pid == 0x42) {
                bateria_volts_atual = ((message.data[3] * 256.0f) + message.data[4]) / 1000.0f;
            }
        }
    }
    if (ecu_conectada && millis() - tempo_ultima_mensagem_can > 2000) {
        ecu_conectada = false;
        Serial.println("ECU desconectada");
    }
}

void processar_obd_requests() {
    if (ecu_conectada && (millis() - tempo_ultimo_pedido >= OBD_REQUEST_INTERVAL_MS)) {
        solicitar_pid_obd(lista_pids_padrao[indice_pid_atual]);
        indice_pid_atual = (indice_pid_atual + 1) % sizeof(lista_pids_padrao);
        tempo_ultimo_pedido = millis();
    }
}

void atualizar_simulador_obd() {
    rpm_alvo = 3000 + (sin(millis() / 500.0f) * 2000);
    kmh_alvo = rpm_alvo / 30;
    boost_calculado = 0.5f + (sin(millis() / 500.0f) * 0.5f);
    if (boost_calculado < 0.0f) boost_calculado = 0.0f;
    temp_oleo_simulada = 90 + (sin(millis() / 1000.0f) * 5);
    bateria_volts_atual = 12.5f + (sin(millis() / 1000.0f) * 1.5f);
    maf_gs_atual = 20.0f + (sin(millis() / 500.0f) * 100.0f);
    if (maf_gs_atual < 0.0f) maf_gs_atual = 0.0f;
}