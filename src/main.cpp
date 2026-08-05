#include <unihiker_k10.h>
#include <lvgl.h>
#include "config.h"
#include "can_manager.h"
#include "ui_manager.h"

UNIHIKER_K10 k10;

volatile bool flag_botao_a = false;
volatile bool flag_botao_b = false;

static unsigned long tempo_ultimo_debug = 0;
static unsigned long tempo_tentativa_conexao = 0;
static unsigned long tempo_ultima_atualizacao = 0;

void onButtonAPressed() { 
    flag_botao_a = true; 
}

void onButtonBPressed() { 
    flag_botao_b = !flag_botao_b; 
    Serial.println("BOTAO B pressionado"); 
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
    verificar_estado_twai();
    
    // 1. Label de Debug
    if (millis() - tempo_ultimo_debug >= DEBUG_UPDATE_INTERVAL_MS) {
        tempo_ultimo_debug = millis();
        if (twai_ok && label_debug != NULL) {
            twai_status_info_t status;
            twai_get_status_info(&status);
        
            const char *estado_can = "OK";
            if (status.state == TWAI_STATE_BUS_OFF) estado_can = "BUS-OFF";
            else if (status.state == TWAI_STATE_STOPPED) estado_can = "PARADO";
            else if (status.state == TWAI_STATE_RECOVERING) estado_can = "RECUPERANDO";
        
            lv_label_set_text_fmt(label_debug,
                "ECU: %s\nHW CAN: %s\nPacotes CAN RX: %u\nPacotes OBD RX: %u\nErros TX: %d",
                ecu_conectada ? "Conectada" : "Buscando...",
                estado_can,
                pacotes_can_rx,
                pacotes_obd_rx,
                status.tx_error_counter
            );
        } else if (label_debug != NULL) {
            lv_label_set_text(label_debug, "TWAI nao iniciado");
        }
    }
    
    // 2. Troca de Telas (6 opções)
    if (flag_botao_a) {
        int proxima_tela = (estado_tela_atual + 1) % TOTAL_SCREENS; 
        carregar_tela(proxima_tela);
        flag_botao_a = false;
    }

    // 3. Processa CAN Bus
    ler_dados_obd();
  
    // 4. Modo Busca vs Modo Simulador (Botão B)
    if (!ecu_conectada) {
        if ((millis() - tempo_tentativa_conexao >= 5000) && !flag_botao_b) {
            tempo_tentativa_conexao = millis();
            solicitar_pid_obd(0x00);
        }
    
        if (flag_botao_b) {
            atualizar_simulador_obd();
        }
    }
    else {
        processar_obd_requests();
    }

    // 5. Atualização Gráfica a 20 FPS (50ms)
    if (millis() - tempo_ultima_atualizacao >= UI_UPDATE_INTERVAL_MS) {
        tempo_ultima_atualizacao = millis(); 
        atualizar_ui();
    }

    lv_timer_handler(); 
    delay(5);
}