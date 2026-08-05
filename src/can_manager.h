#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <Arduino.h>
#include "driver/twai.h"

// Variáveis de Telemetria Compartilhadas
extern int rpm_atual;
extern int rpm_alvo;
extern int kmh_atual;
extern int kmh_alvo;
extern int temp_oleo_simulada;
extern float boost_calculado;
extern float maf_gs_atual;
extern float bateria_volts_atual;

extern bool ecu_conectada;
extern bool twai_ok;
extern uint32_t pacotes_can_rx;
extern uint32_t pacotes_obd_rx;

// Declarações das funções públicas do CAN
void setup_twai();
void verificar_estado_twai();
bool solicitar_pid_obd(uint8_t pid);
void ler_dados_obd();
void processar_obd_requests();
void atualizar_simulador_obd();

#endif // CAN_MANAGER_H