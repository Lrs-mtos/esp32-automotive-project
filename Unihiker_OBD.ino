#include <unihiker_k10.h>
#include <lvgl.h>
#include <math.h> 
#include "driver/twai.h" 

UNIHIKER_K10 k10;
uint8_t screen_dir = 1; 

// ==========================================
// CONFIGURAÇÕES DO BARRAMENTO CAN
// ==========================================
#define CAN_TX_PIN 1 
#define CAN_RX_PIN 2  

// PIDs OBD-II
// Padrão RPM, Velocidade, Óleo, Turbo
uint8_t lista_pids_padrao[] = {0x0C, 0x0D, 0x5C, 0x0B}; 

int indice_pid_atual = 0;

volatile bool flag_botao_a = false;
volatile bool flag_botao_b = false;
int estado_tela_atual = 0; 
 

lv_obj_t * meter_rpm; lv_meter_indicator_t * indic_rpm; lv_obj_t * label_valor_rpm; 
lv_obj_t * meter_kmh; lv_meter_indicator_t * indic_kmh; lv_obj_t * label_valor_kmh; 
lv_obj_t * shift_light; lv_obj_t * boost_arc; lv_obj_t * boost_label;
lv_obj_t * temp_bar; lv_obj_t * temp_label;
lv_obj_t * meter_boost_bmw; lv_meter_indicator_t * indic_boost_bmw; lv_obj_t * label_valor_boost_bmw; 
lv_obj_t * meter_temp_bmw; lv_meter_indicator_t * indic_temp_bmw; lv_obj_t * label_valor_temp_bmw;
lv_obj_t * meter_big_main; lv_meter_indicator_t * indic_big_main; lv_obj_t * label_big_main;
lv_obj_t * meter_sub_bottom; lv_meter_indicator_t * indic_sub_bottom;
lv_obj_t * meter_peugeot; lv_meter_indicator_t * indic_peugeot; lv_obj_t * label_valor_peugeot;


int rpm_atual = 0; int rpm_alvo = 0; 
int kmh_atual = 0; int kmh_alvo = 0; 
int temp_oleo_simulada = 50; 
float boost_calculado = 0.0;

bool ecu_conectada = false;
unsigned long tempo_ultima_mensagem_can = 0;
unsigned long tempo_tentativa_conexao = 0;
unsigned long tempo_ultimo_pedido = 0;
unsigned long tempo_ultima_atualizacao = 0;

lv_obj_t * label_debug;
uint32_t pacotes_can_rx = 0;

bool twai_ok = false;

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

static unsigned long tempo_ultima_recuperacao = 0;

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
    message.data[3] = 0x00;
    message.data[4] = 0x00;
    message.data[5] = 0x00;
    message.data[6] = 0x00;
    message.data[7] = 0x00;

    esp_err_t err = twai_transmit(&message, pdMS_TO_TICKS(10));

    if (err != ESP_OK) {
        Serial.printf("Erro ao transmitir PID 0x%02X: %d\n", pid, err);
        return false;
    }

    return true;
}

uint32_t pacotes_obd_rx = 0;

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

            if ((pid == 0x0C || pid == 0x42) && message.data_length_code < 5) {
                continue;
            }

            if (pid == 0x0C) {
                rpm_alvo = ((message.data[3] * 256) + message.data[4]) / 4;
            }
            else if (pid == 0x0D) {
                kmh_alvo = message.data[3];
            }
            else if (pid == 0x5C) {
                temp_oleo_simulada = message.data[3] - 40;
            }
            else if (pid == 0x0B) {
                int map_kpa = message.data[3];
                boost_calculado = (map_kpa - 100) / 100.0;

                if (boost_calculado < 0) {
                    boost_calculado = 0.0;
                }
            }
        }
    }
}

static void formatar_escala_rpm_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) {
        int inteiro = dsc->value / 1000; int decimal = (dsc->value % 1000) / 100;
        if (decimal == 0) sprintf(dsc->text, "%d", inteiro); else sprintf(dsc->text, "%d.%d", inteiro, decimal); 
    }
}
static void formatar_escala_kmh_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) { sprintf(dsc->text, "%d", dsc->value); }
}
static void formatar_escala_boost_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) {
        int inteiro = dsc->value / 100; int decimal = (dsc->value % 100) / 10;
        sprintf(dsc->text, "%d.%d", inteiro, decimal); 
    }
}

static void set_indic_rpm_value(void * obj, int32_t v) {
    if(estado_tela_atual == 0 && meter_rpm != NULL) {
        lv_meter_set_indicator_value(meter_rpm, indic_rpm, v);
        lv_label_set_text_fmt(label_valor_rpm, "%d", v); 
        if(v >= 5500) lv_led_set_brightness(shift_light, 255); 
        else if (v >= 4000) lv_led_set_brightness(shift_light, 120); 
        else lv_led_set_brightness(shift_light, 0);
    }
}
static void set_indic_kmh_value(void * obj, int32_t v) {
    if(estado_tela_atual == 0 && meter_kmh != NULL) {
        lv_meter_set_indicator_value(meter_kmh, indic_kmh, v); lv_label_set_text_fmt(label_valor_kmh, "%d", v); 
    }
}

void animar_agulha(lv_meter_indicator_t * indic, lv_anim_exec_xcb_t func_exec, int origem, int destino, int duracao_ms) {
    lv_anim_t a; lv_anim_init(&a); lv_anim_set_var(&a, indic); lv_anim_set_exec_cb(&a, func_exec); lv_anim_set_values(&a, origem, destino); lv_anim_set_time(&a, duracao_ms); lv_anim_start(&a);
}


void build_screen_0() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    meter_rpm = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_rpm, 156, 156); lv_obj_align(meter_rpm, LV_ALIGN_LEFT_MID, 2, 0); 
    lv_obj_set_style_bg_color(meter_rpm, lv_color_black(), 0); lv_obj_set_style_border_width(meter_rpm, 3, 0); lv_obj_set_style_border_color(meter_rpm, lv_palette_main(LV_PALETTE_RED), 0); lv_obj_set_style_text_color(meter_rpm, lv_color_white(), LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_rpm, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_rpm, formatar_escala_rpm_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_rpm = lv_meter_add_scale(meter_rpm);
    lv_meter_set_scale_ticks(meter_rpm, scale_rpm, 36, 2, 8, lv_color_white()); lv_meter_set_scale_major_ticks(meter_rpm, scale_rpm, 5, 3, 10, lv_color_white(), 12); lv_meter_set_scale_range(meter_rpm, scale_rpm, 0, 7000, 240, 150);
    indic_rpm = lv_meter_add_needle_line(meter_rpm, scale_rpm, 4, lv_palette_main(LV_PALETTE_RED), -10);
    lv_obj_t * txt_unidade_rpm = lv_label_create(meter_rpm); lv_label_set_text(txt_unidade_rpm, "x1000"); lv_obj_set_style_text_color(txt_unidade_rpm, lv_color_hex(0x666666), 0); lv_obj_align(txt_unidade_rpm, LV_ALIGN_CENTER, 0, 45); 
    label_valor_rpm = lv_label_create(meter_rpm); lv_label_set_text(label_valor_rpm, "0"); lv_obj_set_style_text_color(label_valor_rpm, lv_color_white(), 0); lv_obj_align(label_valor_rpm, LV_ALIGN_CENTER, 0, 25); 

    meter_kmh = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_kmh, 156, 156); lv_obj_align(meter_kmh, LV_ALIGN_RIGHT_MID, -2, 0); 
    lv_obj_set_style_bg_color(meter_kmh, lv_color_black(), 0); lv_obj_set_style_border_width(meter_kmh, 3, 0); lv_obj_set_style_border_color(meter_kmh, lv_palette_main(LV_PALETTE_CYAN), 0); lv_obj_set_style_text_color(meter_kmh, lv_color_white(), LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_kmh, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_kmh, formatar_escala_kmh_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_kmh = lv_meter_add_scale(meter_kmh);
    lv_meter_set_scale_ticks(meter_kmh, scale_kmh, 21, 2, 8, lv_color_white()); lv_meter_set_scale_major_ticks(meter_kmh, scale_kmh, 4, 3, 10, lv_color_white(), 12); lv_meter_set_scale_range(meter_kmh, scale_kmh, 0, 200, 240, 150);
    indic_kmh = lv_meter_add_needle_line(meter_kmh, scale_kmh, 4, lv_palette_main(LV_PALETTE_CYAN), -10);
    lv_obj_t * txt_unidade_kmh = lv_label_create(meter_kmh); lv_label_set_text(txt_unidade_kmh, "km/h"); lv_obj_set_style_text_color(txt_unidade_kmh, lv_color_hex(0x666666), 0); lv_obj_align(txt_unidade_kmh, LV_ALIGN_CENTER, 0, 45); 
    label_valor_kmh = lv_label_create(meter_kmh); lv_label_set_text(label_valor_kmh, "0"); lv_obj_set_style_text_color(label_valor_kmh, lv_color_white(), 0); lv_obj_align(label_valor_kmh, LV_ALIGN_CENTER, 0, 25); 

    shift_light = lv_led_create(lv_scr_act()); lv_obj_set_size(shift_light, 300, 10); lv_led_set_color(shift_light, lv_palette_main(LV_PALETTE_RED)); lv_led_set_brightness(shift_light, 0); lv_obj_align(shift_light, LV_ALIGN_TOP_MID, 0, 5); 
    boost_arc = lv_arc_create(lv_scr_act()); lv_obj_set_size(boost_arc, 60, 60); lv_arc_set_rotation(boost_arc, 135); lv_arc_set_bg_angles(boost_arc, 0, 270); lv_arc_set_range(boost_arc, 0, 150); lv_obj_set_style_arc_color(boost_arc, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR); lv_obj_align(boost_arc, LV_ALIGN_BOTTOM_MID, 0, -10);
    boost_label = lv_label_create(boost_arc); lv_label_set_text(boost_label, "0.0"); lv_obj_align(boost_label, LV_ALIGN_CENTER, 0, 0);
}

void build_screen_1() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x001122), 0);
    temp_bar = lv_bar_create(lv_scr_act()); lv_obj_set_size(temp_bar, 40, 140); lv_bar_set_range(temp_bar, 50, 120); lv_bar_set_value(temp_bar, 50, LV_ANIM_OFF); lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0x00AAFF), LV_PART_INDICATOR); lv_obj_align(temp_bar, LV_ALIGN_CENTER, 0, 10);
    temp_label = lv_label_create(lv_scr_act()); lv_label_set_text(temp_label, "-- °C (Óleo)"); lv_obj_set_style_text_color(temp_label, lv_color_white(), 0); lv_obj_align_to(temp_label, temp_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void build_screen_2() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_color_t cor_ambar = lv_color_hex(0xFF4500); lv_color_t cor_ponteiro = lv_color_hex(0xFF0000);

    meter_boost_bmw = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_boost_bmw, 156, 156); lv_obj_align(meter_boost_bmw, LV_ALIGN_LEFT_MID, 2, 0); lv_obj_set_style_bg_color(meter_boost_bmw, lv_color_black(), 0); lv_obj_set_style_border_width(meter_boost_bmw, 0, 0); lv_obj_set_style_text_color(meter_boost_bmw, cor_ambar, LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_boost_bmw, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_boost_bmw, formatar_escala_boost_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); lv_meter_scale_t * scale_boost_bmw = lv_meter_add_scale(meter_boost_bmw); lv_meter_set_scale_ticks(meter_boost_bmw, scale_boost_bmw, 31, 2, 6, cor_ambar); lv_meter_set_scale_major_ticks(meter_boost_bmw, scale_boost_bmw, 5, 3, 10, cor_ambar, 15); lv_meter_set_scale_range(meter_boost_bmw, scale_boost_bmw, 0, 150, 240, 150); indic_boost_bmw = lv_meter_add_needle_line(meter_boost_bmw, scale_boost_bmw, 4, cor_ponteiro, -10);
    lv_obj_t * unidade_boost_bmw = lv_label_create(meter_boost_bmw); lv_label_set_text(unidade_boost_bmw, "Bar"); lv_obj_set_style_text_color(unidade_boost_bmw, cor_ambar, 0); lv_obj_align(unidade_boost_bmw, LV_ALIGN_CENTER, 0, 45); 
    label_valor_boost_bmw = lv_label_create(meter_boost_bmw); lv_label_set_text(label_valor_boost_bmw, "0.0"); lv_obj_set_style_text_color(label_valor_boost_bmw, lv_color_white(), 0); lv_obj_align(label_valor_boost_bmw, LV_ALIGN_CENTER, 0, 25); 

    meter_temp_bmw = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_temp_bmw, 156, 156); lv_obj_align(meter_temp_bmw, LV_ALIGN_RIGHT_MID, -2, 0); lv_obj_set_style_bg_color(meter_temp_bmw, lv_color_black(), 0); lv_obj_set_style_border_width(meter_temp_bmw, 0, 0); lv_obj_set_style_text_color(meter_temp_bmw, cor_ambar, LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_temp_bmw, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_temp_bmw, formatar_escala_kmh_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); lv_meter_scale_t * scale_temp_bmw = lv_meter_add_scale(meter_temp_bmw); lv_meter_set_scale_ticks(meter_temp_bmw, scale_temp_bmw, 15, 2, 6, cor_ambar); lv_meter_set_scale_major_ticks(meter_temp_bmw, scale_temp_bmw, 2, 3, 10, cor_ambar, 15); lv_meter_set_scale_range(meter_temp_bmw, scale_temp_bmw, 50, 150, 240, 150); indic_temp_bmw = lv_meter_add_needle_line(meter_temp_bmw, scale_temp_bmw, 4, cor_ponteiro, -10); 
    lv_obj_t * unidade_temp_bmw = lv_label_create(meter_temp_bmw); lv_label_set_text(unidade_temp_bmw, "Óleo °C"); lv_obj_set_style_text_color(unidade_temp_bmw, cor_ambar, 0); lv_obj_align(unidade_temp_bmw, LV_ALIGN_CENTER, 0, 45); 
    label_valor_temp_bmw = lv_label_create(meter_temp_bmw); lv_label_set_text(label_valor_temp_bmw, "50"); lv_obj_set_style_text_color(label_valor_temp_bmw, lv_color_white(), 0); lv_obj_align(label_valor_temp_bmw, LV_ALIGN_CENTER, 0, 25); 
}

void build_screen_3() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_color_t cor_ambar = lv_color_hex(0xFF4500); 
    lv_color_t cor_ponteiro = lv_color_hex(0xFF0000);

    meter_big_main = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_big_main, 240, 240); 
    lv_obj_align(meter_big_main, LV_ALIGN_CENTER, 0, 0); 
    lv_obj_set_style_bg_color(meter_big_main, lv_color_black(), 0); 
    lv_obj_set_style_border_width(meter_big_main, 0, 0); 
    lv_obj_set_style_text_color(meter_big_main, cor_ambar, LV_PART_TICKS); 
    lv_obj_set_style_bg_opa(meter_big_main, 255, LV_PART_INDICATOR); 

    meter_sub_bottom = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_sub_bottom, 210, 210); 
    lv_obj_align(meter_sub_bottom, LV_ALIGN_BOTTOM_MID, 0, 0); 
    lv_obj_set_style_bg_opa(meter_sub_bottom, 0, 0); 
    lv_obj_set_style_border_width(meter_sub_bottom, 0, 0); 
    lv_obj_set_style_text_color(meter_sub_bottom, cor_ambar, LV_PART_TICKS); 
    lv_obj_set_style_bg_opa(meter_sub_bottom, 0, LV_PART_INDICATOR); 

    lv_obj_add_event_cb(meter_sub_bottom, formatar_escala_kmh_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_sub = lv_meter_add_scale(meter_sub_bottom);
    lv_meter_set_scale_ticks(meter_sub_bottom, scale_sub, 15, 2, 5, cor_ambar);
    lv_meter_set_scale_major_ticks(meter_sub_bottom, scale_sub, 7, 3, 10, cor_ambar, 15);
    lv_meter_set_scale_range(meter_sub_bottom, scale_sub, 50, 150, 90, 45); 
    indic_sub_bottom = lv_meter_add_needle_line(meter_sub_bottom, scale_sub, 3, cor_ponteiro, -5);

    lv_obj_t * mascara_centro = lv_obj_create(lv_scr_act());
    lv_obj_set_size(mascara_centro, 100, 100); 
    lv_obj_align_to(mascara_centro, meter_sub_bottom, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(mascara_centro, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(mascara_centro, 255, 0); 
    lv_obj_set_style_border_width(mascara_centro, 0, 0);
    lv_obj_set_style_radius(mascara_centro, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(mascara_centro, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(meter_big_main, formatar_escala_boost_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_big = lv_meter_add_scale(meter_big_main);
    lv_meter_set_scale_ticks(meter_big_main, scale_big, 41, 3, 10, cor_ambar); 
    lv_meter_set_scale_major_ticks(meter_big_main, scale_big, 5, 4, 18, cor_ambar, 22); 
    lv_meter_set_scale_range(meter_big_main, scale_big, 0, 150, 240, 150); 
    
    lv_meter_indicator_t * faixa_turbo = lv_meter_add_arc(meter_big_main, scale_big, 2, cor_ambar, -18);
    lv_meter_set_indicator_start_value(meter_big_main, faixa_turbo, 0);
    lv_meter_set_indicator_end_value(meter_big_main, faixa_turbo, 150);
    indic_big_main = lv_meter_add_needle_line(meter_big_main, scale_big, 4, cor_ponteiro, -15);
    
    lv_obj_t * texto_turbo = lv_label_create(lv_scr_act());
    lv_label_set_text(texto_turbo, "TURBO"); 
    lv_obj_set_style_text_color(texto_turbo, cor_ambar, 0); 
    lv_obj_align(texto_turbo, LV_ALIGN_CENTER, 0, -45); 

    label_big_main = lv_label_create(lv_scr_act());
    lv_label_set_text(label_big_main, "0.0"); 
    lv_obj_set_style_text_color(label_big_main, lv_color_white(), 0); 
    lv_obj_align(label_big_main, LV_ALIGN_CENTER, 0, -25); 

    lv_obj_t * texto_bar = lv_label_create(lv_scr_act());
    lv_label_set_text(texto_bar, "Bar"); 
    lv_obj_set_style_text_color(texto_bar, cor_ambar, 0); 
    lv_obj_align(texto_bar, LV_ALIGN_CENTER, 0, -10); 

    lv_obj_t * icone_temp = lv_label_create(lv_scr_act()); 
    lv_label_set_text(icone_temp, "°C (Óleo)");
    lv_obj_set_style_text_color(icone_temp, cor_ambar, 0); 
    lv_obj_align_to(icone_temp, meter_sub_bottom, LV_ALIGN_CENTER, 0, 40); 
}

void build_screen_4() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_color_t cor_vermelho_peugeot = lv_color_hex(0xFF0000); 
    lv_color_t cor_branca = lv_color_hex(0xFFFFFF);
    
    meter_peugeot = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_peugeot, 270, 270); 
    lv_obj_align(meter_peugeot, LV_ALIGN_CENTER, 0, 30); 
    lv_obj_set_style_bg_color(meter_peugeot, lv_color_black(), 0); 
    lv_obj_set_style_border_width(meter_peugeot, 0, 0); 
    lv_obj_set_style_text_color(meter_peugeot, cor_branca, LV_PART_TICKS); 
    lv_obj_set_style_bg_opa(meter_peugeot, 0, LV_PART_INDICATOR); 

    lv_obj_set_style_pad_all(meter_peugeot, 15, 0);

    lv_obj_add_event_cb(meter_peugeot, formatar_escala_boost_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_peugeot = lv_meter_add_scale(meter_peugeot);
    lv_meter_set_scale_range(meter_peugeot, scale_peugeot, 0, 150, 240, 150); 
    
    lv_meter_set_scale_ticks(meter_peugeot, scale_peugeot, 31, 2, 8, cor_branca); 
    lv_meter_set_scale_major_ticks(meter_peugeot, scale_peugeot, 5, 3, 15, cor_branca, 20); 
    
    lv_meter_indicator_t * aro_vermelho = lv_meter_add_arc(meter_peugeot, scale_peugeot, 6, cor_vermelho_peugeot, 15);
    lv_meter_set_indicator_start_value(meter_peugeot, aro_vermelho, 0);
    lv_meter_set_indicator_end_value(meter_peugeot, aro_vermelho, 150);

    indic_peugeot = lv_meter_add_needle_line(meter_peugeot, scale_peugeot, 3, cor_branca, -10);

    lv_obj_t * miolo_ponteiro = lv_obj_create(meter_peugeot);
    lv_obj_set_size(miolo_ponteiro, 40, 40); 
    lv_obj_align(miolo_ponteiro, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(miolo_ponteiro, lv_color_hex(0x151515), 0); 
    lv_obj_set_style_bg_opa(miolo_ponteiro, 255, 0); 
    lv_obj_set_style_border_width(miolo_ponteiro, 2, 0);
    lv_obj_set_style_border_color(miolo_ponteiro, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(miolo_ponteiro, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(miolo_ponteiro, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * texto_turbo = lv_label_create(lv_scr_act());
    lv_label_set_text(texto_turbo, "TURBO"); 
    lv_obj_set_style_text_color(texto_turbo, cor_branca, 0); 
    lv_obj_align(texto_turbo, LV_ALIGN_CENTER, 0, -30); 

    label_valor_peugeot = lv_label_create(lv_scr_act());
    lv_label_set_text(label_valor_peugeot, "0.0"); 
    lv_obj_set_style_text_color(label_valor_peugeot, cor_branca, 0); 
    lv_obj_align(label_valor_peugeot, LV_ALIGN_CENTER, 0, 30); 

    lv_obj_t * texto_bar = lv_label_create(lv_scr_act());
    lv_label_set_text(texto_bar, "Bar"); 
    lv_obj_set_style_text_color(texto_bar, lv_color_hex(0x888888), 0); 
    lv_obj_align(texto_bar, LV_ALIGN_CENTER, 0, 80); 
}

void carregar_tela(int id_tela) {
    lv_anim_del(NULL, NULL);    
    lv_obj_clean(lv_scr_act()); 

    estado_tela_atual = id_tela;
    if(id_tela == 0) build_screen_0();
    else if(id_tela == 1) build_screen_1();
    else if(id_tela == 2) build_screen_2();
    else if(id_tela == 3) build_screen_3();
    else if(id_tela == 4) build_screen_4(); 
}

void onButtonAPressed() { flag_botao_a = true; }
void onButtonBPressed() { flag_botao_b = !flag_botao_b; Serial.println("BOTAO B pressionado"); }

void setup() {
    Serial.begin(115200);
    k10.begin();
    k10.initScreen(screen_dir);
    k10.creatCanvas();
    k10.buttonA->setPressedCallback(onButtonAPressed);
    k10.buttonB->setPressedCallback(onButtonBPressed);

    
    
    setup_twai();

    carregar_tela(0); 

    label_debug = lv_label_create(lv_layer_top()); 
    lv_obj_set_style_text_font(label_debug, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_debug, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(label_debug, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(label_debug, "DEBUG INICIADO");
}

void loop() {
    verificar_estado_twai();
    static unsigned long tempo_ultimo_debug = 0;
    if (millis() - tempo_ultimo_debug >= 500) {
        tempo_ultimo_debug = millis();
        if (twai_ok) {
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
        } else {
            lv_label_set_text(label_debug, "TWAI nao iniciado");
        }
    }
    
    if (flag_botao_a) {
        int proxima_tela = (estado_tela_atual + 1) % 5; 
        carregar_tela(proxima_tela);
        flag_botao_a = false;
        indice_pid_atual = 0;
    }

    ler_dados_obd();

    if (ecu_conectada && millis() - tempo_ultima_mensagem_can > 2000) {
        ecu_conectada = false;
        Serial.println("ECU desconectada");
    }
  
    if (!ecu_conectada) {
        if ((millis() - tempo_tentativa_conexao >= 5000) && !flag_botao_b) {
            tempo_tentativa_conexao = millis();
            solicitar_pid_obd(0x00);
        }
    
        if (flag_botao_b) {
            rpm_alvo = 3000 + (sin(millis() / 500.0) * 2000);
            kmh_alvo = rpm_alvo / 30;
            boost_calculado = 0.5 + (sin(millis() / 500.0) * 0.5);
    
            if (boost_calculado < 0) {
                boost_calculado = 0;
            }
    
            temp_oleo_simulada = 90 + (sin(millis() / 1000.0) * 5);
        }
    }
    else {
        if (millis() - tempo_ultimo_pedido >= 50) {
            solicitar_pid_obd(lista_pids_padrao[indice_pid_atual]);
    
            indice_pid_atual = (indice_pid_atual + 1) % sizeof(lista_pids_padrao);
            tempo_ultimo_pedido = millis();
        }
    }

    if (millis() - tempo_ultima_atualizacao >= 50) {
        tempo_ultima_atualizacao = millis(); 
        
        int valor_boost_inteiro = (int)(boost_calculado * 100);
        int int_part = (int)boost_calculado; 
        int dec_part = (int)((boost_calculado - int_part) * 10);

        int anim_dur = 50;

        if (estado_tela_atual == 0) {
            animar_agulha(indic_rpm, set_indic_rpm_value, rpm_atual, rpm_alvo, anim_dur);
            animar_agulha(indic_kmh, set_indic_kmh_value, kmh_atual, kmh_alvo, anim_dur);
            lv_arc_set_value(boost_arc, valor_boost_inteiro); 
            lv_label_set_text_fmt(boost_label, "%d.%d", int_part, dec_part);
        }
        else if (estado_tela_atual == 1) {
            lv_bar_set_value(temp_bar, temp_oleo_simulada, LV_ANIM_OFF);
            lv_label_set_text_fmt(temp_label, "%d C (Óleo)", temp_oleo_simulada);
            if(temp_oleo_simulada >= 120) lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
            else lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
        }
        else if (estado_tela_atual == 2) {
            lv_meter_set_indicator_value(meter_boost_bmw, indic_boost_bmw, valor_boost_inteiro);
            lv_label_set_text_fmt(label_valor_boost_bmw, "%d.%d", int_part, dec_part);
            lv_meter_set_indicator_value(meter_temp_bmw, indic_temp_bmw, temp_oleo_simulada);
            lv_label_set_text_fmt(label_valor_temp_bmw, "%d", temp_oleo_simulada);
        }
        else if (estado_tela_atual == 3) {
            lv_meter_set_indicator_value(meter_big_main, indic_big_main, valor_boost_inteiro);
            lv_label_set_text_fmt(label_big_main, "%d.%d", int_part, dec_part);
            lv_meter_set_indicator_value(meter_sub_bottom, indic_sub_bottom, temp_oleo_simulada);
        }
        else if (estado_tela_atual == 4) { 
            lv_meter_set_indicator_value(meter_peugeot, indic_peugeot, valor_boost_inteiro);
            lv_label_set_text_fmt(label_valor_peugeot, "%d.%d", int_part, dec_part);
        }
        rpm_atual = rpm_alvo;
        kmh_atual = kmh_alvo;
    }

    lv_timer_handler(); 
    delay(5);
}
