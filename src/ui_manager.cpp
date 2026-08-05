#include "ui_manager.h"
#include "can_manager.h"
#include "config.h"

int estado_tela_atual = 0;
lv_obj_t * label_debug = NULL;

// Pointers Privados Locais das Telas 0 a 5
static lv_obj_t * meter_rpm = NULL; 
static lv_meter_indicator_t * indic_rpm = NULL; 
static lv_obj_t * label_valor_rpm = NULL; 
static lv_obj_t * meter_kmh = NULL; 
static lv_meter_indicator_t * indic_kmh = NULL; 
static lv_obj_t * label_valor_kmh = NULL; 
static lv_obj_t * shift_light = NULL; 
static lv_obj_t * boost_arc = NULL; 
static lv_obj_t * boost_label = NULL;
static lv_obj_t * temp_bar = NULL; 
static lv_obj_t * temp_label = NULL;
static lv_obj_t * meter_boost_bmw = NULL; 
static lv_meter_indicator_t * indic_boost_bmw = NULL; 
static lv_obj_t * label_valor_boost_bmw = NULL; 
static lv_obj_t * meter_temp_bmw = NULL; 
static lv_meter_indicator_t * indic_temp_bmw = NULL; 
static lv_obj_t * label_valor_temp_bmw = NULL;
static lv_obj_t * meter_big_main = NULL; 
static lv_meter_indicator_t * indic_big_main = NULL; 
static lv_obj_t * label_big_main = NULL;
static lv_obj_t * meter_sub_bottom = NULL; 
static lv_meter_indicator_t * indic_sub_bottom = NULL;
static lv_obj_t * meter_peugeot = NULL; 
static lv_meter_indicator_t * indic_peugeot = NULL; 
static lv_obj_t * label_valor_peugeot = NULL;

// Pointers da Tela 5
static lv_obj_t * bar_bat_indicator = NULL; 
static lv_obj_t * label_bat_val = NULL; 
static lv_obj_t * label_bat_status = NULL;
static lv_obj_t * arc_maf = NULL; 
static lv_obj_t * label_maf_val = NULL;
static lv_obj_t * led_ecu_status = NULL;

// ==========================================
// CONFIGURAÇÃO DAS FORMATAÇÕES DE ESCALAS
// ==========================================
static void formatar_escala_rpm_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) {
        int inteiro = dsc->value / 1000; int decimal = (dsc->value % 1000) / 100;
        if (decimal == 0) sprintf(dsc->text, "%d", inteiro); 
        else sprintf(dsc->text, "%d.%d", inteiro, decimal); 
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
        lv_meter_set_indicator_value(meter_kmh, indic_kmh, v); 
        lv_label_set_text_fmt(label_valor_kmh, "%d", v); 
    }
}

static void animar_agulha(lv_meter_indicator_t * indic, lv_anim_exec_xcb_t func_exec, int origem, int destino, int duracao_ms) {
    lv_anim_t a; 
    lv_anim_init(&a); 
    lv_anim_set_var(&a, indic); 
    lv_anim_set_exec_cb(&a, func_exec); 
    lv_anim_set_values(&a, origem, destino); 
    lv_anim_set_time(&a, duracao_ms); 
    lv_anim_start(&a);
}

// ==========================================
// FUNÇÕES PRIVADAS DE CONSTRUÇÃO DE TELA
// ==========================================
static void build_screen_0() { 
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

    shift_light = lv_led_create(lv_scr_act()); lv_obj_set_style_align(shift_light, LV_ALIGN_TOP_MID, 0); lv_obj_set_size(shift_light, 300, 10); lv_led_set_color(shift_light, lv_palette_main(LV_PALETTE_RED)); lv_led_set_brightness(shift_light, 0); lv_obj_align(shift_light, LV_ALIGN_TOP_MID, 0, 5); 
    boost_arc = lv_arc_create(lv_scr_act()); lv_obj_set_size(boost_arc, 60, 60); lv_arc_set_rotation(boost_arc, 135); lv_arc_set_bg_angles(boost_arc, 0, 270); lv_arc_set_range(boost_arc, 0, 150); lv_obj_set_style_arc_color(boost_arc, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR); lv_obj_align(boost_arc, LV_ALIGN_BOTTOM_MID, 0, -10);
    boost_label = lv_label_create(boost_arc); lv_label_set_text(boost_label, "0.0"); lv_obj_align(boost_label, LV_ALIGN_CENTER, 0, 0);
}

static void build_screen_1() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x001122), 0);
    temp_bar = lv_bar_create(lv_scr_act()); lv_obj_set_size(temp_bar, 40, 140); lv_bar_set_range(temp_bar, 50, 120); lv_bar_set_value(temp_bar, 50, LV_ANIM_OFF); lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0x00AAFF), LV_PART_INDICATOR); lv_obj_align(temp_bar, LV_ALIGN_CENTER, 0, 10);
    temp_label = lv_label_create(lv_scr_act()); lv_label_set_text(temp_label, "-- °C (Oleo)"); lv_obj_set_style_text_color(temp_label, lv_color_white(), 0); lv_obj_align_to(temp_label, temp_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

static void build_screen_2() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_color_t cor_ambar = lv_color_hex(0xFF4500); lv_color_t cor_ponteiro = lv_color_hex(0xFF0000);

    meter_boost_bmw = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_boost_bmw, 156, 156); lv_obj_align(meter_boost_bmw, LV_ALIGN_LEFT_MID, 2, 0); lv_obj_set_style_bg_color(meter_boost_bmw, lv_color_black(), 0); lv_obj_set_style_border_width(meter_boost_bmw, 0, 0); lv_obj_set_style_text_color(meter_boost_bmw, cor_ambar, LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_boost_bmw, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_boost_bmw, formatar_escala_boost_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); lv_meter_scale_t * scale_boost_bmw = lv_meter_add_scale(meter_boost_bmw); lv_meter_set_scale_ticks(meter_boost_bmw, scale_boost_bmw, 31, 2, 6, cor_ambar); lv_meter_set_scale_major_ticks(meter_boost_bmw, scale_boost_bmw, 5, 3, 10, cor_ambar, 15); lv_meter_set_scale_range(meter_boost_bmw, scale_boost_bmw, 0, 150, 240, 150); indic_boost_bmw = lv_meter_add_needle_line(meter_boost_bmw, scale_boost_bmw, 4, cor_ponteiro, -10);
    lv_obj_t * unidade_boost_bmw = lv_label_create(meter_boost_bmw); lv_label_set_text(unidade_boost_bmw, "Bar"); lv_obj_set_style_text_color(unidade_boost_bmw, cor_ambar, 0); lv_obj_align(unidade_boost_bmw, LV_ALIGN_CENTER, 0, 45); 
    label_valor_boost_bmw = lv_label_create(meter_boost_bmw); lv_label_set_text(label_valor_boost_bmw, "0.0"); lv_obj_set_style_text_color(label_valor_boost_bmw, lv_color_white(), 0); lv_obj_align(label_valor_boost_bmw, LV_ALIGN_CENTER, 0, 25); 

    meter_temp_bmw = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_temp_bmw, 156, 156); lv_obj_align(meter_temp_bmw, LV_ALIGN_RIGHT_MID, -2, 0); lv_obj_set_style_bg_color(meter_temp_bmw, lv_color_black(), 0); lv_obj_set_style_border_width(meter_temp_bmw, 0, 0); lv_obj_set_style_text_color(meter_temp_bmw, cor_ambar, LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_temp_bmw, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_temp_bmw, formatar_escala_kmh_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); lv_meter_scale_t * scale_temp_bmw = lv_meter_add_scale(meter_temp_bmw); lv_meter_set_scale_ticks(meter_temp_bmw, scale_temp_bmw, 15, 2, 6, cor_ambar); lv_meter_set_scale_major_ticks(meter_temp_bmw, scale_temp_bmw, 2, 3, 10, cor_ambar, 15); lv_meter_set_scale_range(meter_temp_bmw, scale_temp_bmw, 50, 150, 240, 150); indic_temp_bmw = lv_meter_add_needle_line(meter_temp_bmw, scale_temp_bmw, 4, cor_ponteiro, -10); 
    lv_obj_t * unidade_temp_bmw = lv_label_create(meter_temp_bmw); lv_label_set_text(unidade_temp_bmw, "Oleo °C"); lv_obj_set_style_text_color(unidade_temp_bmw, cor_ambar, 0); lv_obj_align(unidade_temp_bmw, LV_ALIGN_CENTER, 0, 45); 
    label_valor_temp_bmw = lv_label_create(meter_temp_bmw); lv_label_set_text(label_valor_temp_bmw, "50"); lv_obj_set_style_text_color(label_valor_temp_bmw, lv_color_white(), 0); lv_obj_align(label_valor_temp_bmw, LV_ALIGN_CENTER, 0, 25); 
}

static void build_screen_3() { 
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
    lv_label_set_text(icone_temp, "°C (Oleo)");
    lv_obj_set_style_text_color(icone_temp, cor_ambar, 0); 
    lv_obj_align_to(icone_temp, meter_sub_bottom, LV_ALIGN_CENTER, 0, 40); 
}

static void build_screen_4() { 
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

static void build_screen_5() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0F111A), 0); 
    
    // Header
    lv_obj_t *lbl_title = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl_title, "DIAGNOSTICS");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x8A99AD), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 10, 8);

    led_ecu_status = lv_led_create(lv_scr_act());
    lv_obj_set_size(led_ecu_status, 10, 10);
    lv_obj_align(led_ecu_status, LV_ALIGN_TOP_RIGHT, -10, 8);
    lv_led_set_color(led_ecu_status, lv_color_hex(0xFF0000));
    lv_led_off(led_ecu_status);

    lv_obj_t *line = lv_obj_create(lv_scr_act());
    lv_obj_set_size(line, 300, 2);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x1F293D), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 26);

    // Bateria Panel
    lv_obj_t *pnl_bat = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pnl_bat, 150, 190);
    lv_obj_align(pnl_bat, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    lv_obj_set_style_bg_color(pnl_bat, lv_color_hex(0x161B26), 0);
    lv_obj_set_style_border_color(pnl_bat, lv_color_hex(0x2A3447), 0);
    lv_obj_set_style_border_width(pnl_bat, 1, 0);

    lv_obj_t *lbl_bat_tag = lv_label_create(pnl_bat);
    lv_label_set_text(lbl_bat_tag, "BATTERY SYS");
    lv_obj_set_style_text_color(lbl_bat_tag, lv_color_hex(0x6C7D93), 0);
    lv_obj_set_style_text_font(lbl_bat_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_bat_tag, LV_ALIGN_TOP_LEFT, 5, 5);

    label_bat_val = lv_label_create(pnl_bat);
    lv_label_set_text(label_bat_val, "--.- V");
    lv_obj_set_style_text_color(label_bat_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_bat_val, &lv_font_montserrat_14, 0);
    lv_obj_align(label_bat_val, LV_ALIGN_CENTER, 0, -15);

    label_bat_status = lv_label_create(pnl_bat);
    lv_label_set_text(label_bat_status, "CHECKING...");
    lv_obj_set_style_text_color(label_bat_status, lv_color_hex(0x8A99AD), 0);
    lv_obj_set_style_text_font(label_bat_status, &lv_font_montserrat_14, 0);
    lv_obj_align(label_bat_status, LV_ALIGN_CENTER, 0, 15);

    bar_bat_indicator = lv_bar_create(pnl_bat);
    lv_obj_set_size(bar_bat_indicator, 120, 8);
    lv_bar_set_range(bar_bat_indicator, 100, 150); 
    lv_obj_align(bar_bat_indicator, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(bar_bat_indicator, lv_color_hex(0x0F111A), LV_PART_MAIN);

    // MAF Panel
    lv_obj_t *pnl_maf = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pnl_maf, 150, 190);
    lv_obj_align(pnl_maf, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
    lv_obj_set_style_bg_color(pnl_maf, lv_color_hex(0x161B26), 0);
    lv_obj_set_style_border_color(pnl_maf, lv_color_hex(0x2A3447), 0);
    lv_obj_set_style_border_width(pnl_maf, 1, 0);

    lv_obj_t *lbl_maf_tag = lv_label_create(pnl_maf);
    lv_label_set_text(lbl_maf_tag, "AIR FLOW");
    lv_obj_set_style_text_color(lbl_maf_tag, lv_color_hex(0x6C7D93), 0);
    lv_obj_set_style_text_font(lbl_maf_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_maf_tag, LV_ALIGN_TOP_LEFT, 5, 5);

    arc_maf = lv_arc_create(pnl_maf);
    lv_obj_set_size(arc_maf, 105, 105);
    lv_arc_set_rotation(arc_maf, 135);
    lv_arc_set_bg_angles(arc_maf, 0, 270);
    lv_arc_set_range(arc_maf, 0, 250);
    lv_arc_set_value(arc_maf, 0);
    lv_obj_align(arc_maf, LV_ALIGN_CENTER, 0, -5);

    lv_obj_set_style_arc_color(arc_maf, lv_color_hex(0x0F111A), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_maf, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_maf, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_maf, 10, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_maf, NULL, LV_PART_KNOB);

    label_maf_val = lv_label_create(pnl_maf);
    lv_label_set_text(label_maf_val, "0.0\ng/s");
    lv_obj_set_style_text_align(label_maf_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_maf_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_maf_val, &lv_font_montserrat_14, 0);
    lv_obj_align(label_maf_val, LV_ALIGN_CENTER, 0, -5);
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
    else if(id_tela == 5) build_screen_5();
}

void setup_ui() {
    carregar_tela(0); 

    label_debug = lv_label_create(lv_layer_top()); 
    lv_obj_set_style_text_font(label_debug, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_debug, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(label_debug, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(label_debug, "DEBUG INICIADO");
}

void atualizar_ui() {
    int valor_boost_inteiro = (int)(boost_calculado * 100);
    int int_part = (int)boost_calculado; 
    int dec_part = (int)((boost_calculado - int_part) * 10);
    int anim_dur = 50;

    if (estado_tela_atual == 0) {
        animar_agulha(indic_rpm, set_indic_rpm_value, rpm_atual, rpm_alvo, anim_dur);
        animar_agulha(indic_kmh, set_indic_kmh_value, kmh_atual, kmh_alvo, anim_dur);
        if (boost_arc != NULL) {
            lv_arc_set_value(boost_arc, valor_boost_inteiro); 
            lv_label_set_text_fmt(boost_label, "%d.%d", int_part, dec_part);
        }
    }
    else if (estado_tela_atual == 1) {
        if (temp_bar != NULL) {
            lv_bar_set_value(temp_bar, temp_oleo_simulada, LV_ANIM_OFF);
            lv_label_set_text_fmt(temp_label, "%d C (Oleo)", temp_oleo_simulada);
            if(temp_oleo_simulada >= 120) lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
            else lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
        }
    }
    else if (estado_tela_atual == 2) {
        if (meter_boost_bmw != NULL) {
            lv_meter_set_indicator_value(meter_boost_bmw, indic_boost_bmw, valor_boost_inteiro);
            lv_label_set_text_fmt(label_valor_boost_bmw, "%d.%d", int_part, dec_part);
            lv_meter_set_indicator_value(meter_temp_bmw, indic_temp_bmw, temp_oleo_simulada);
            lv_label_set_text_fmt(label_valor_temp_bmw, "%d", temp_oleo_simulada);
        }
    }
    else if (estado_tela_atual == 3) {
        if (meter_big_main != NULL) {
            lv_meter_set_indicator_value(meter_big_main, indic_big_main, valor_boost_inteiro);
            lv_label_set_text_fmt(label_big_main, "%d.%d", int_part, dec_part);
            lv_meter_set_indicator_value(meter_sub_bottom, indic_sub_bottom, temp_oleo_simulada);
        }
    }
    else if (estado_tela_atual == 4) { 
        if (meter_peugeot != NULL) {
            lv_meter_set_indicator_value(meter_peugeot, indic_peugeot, valor_boost_inteiro);
            lv_label_set_text_fmt(label_valor_peugeot, "%d.%d", int_part, dec_part);
        }
    }
    else if (estado_tela_atual == 5) {
        if (arc_maf != NULL) {
            lv_arc_set_value(arc_maf, (int)maf_gs_atual);
            lv_label_set_text_fmt(label_maf_val, "%.1f\ng/s", maf_gs_atual);

            lv_label_set_text_fmt(label_bat_val, "%.1f V", bateria_volts_atual);
            lv_bar_set_value(bar_bat_indicator, (int)(bateria_volts_atual * 10), LV_ANIM_OFF);

            if (bateria_volts_atual < 11.8f) {
                lv_obj_set_style_text_color(label_bat_val, lv_color_hex(0xFF0000), 0);
                lv_obj_set_style_text_color(label_bat_status, lv_color_hex(0xFF0000), 0);
                lv_obj_set_style_bg_color(bar_bat_indicator, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
                lv_label_set_text(label_bat_status, "LOW VOLTAGE");
            } else if (bateria_volts_atual >= 13.5f) {
                lv_obj_set_style_text_color(label_bat_val, lv_color_hex(0x00FF00), 0);
                lv_obj_set_style_text_color(label_bat_status, lv_color_hex(0x00FF00), 0);
                lv_obj_set_style_bg_color(bar_bat_indicator, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
                lv_label_set_text(label_bat_status, "CHARGING OK");
            } else {
                lv_obj_set_style_text_color(label_bat_val, lv_color_hex(0xFFB300), 0);
                lv_obj_set_style_text_color(label_bat_status, lv_color_hex(0xFFB300), 0);
                lv_obj_set_style_bg_color(bar_bat_indicator, lv_color_hex(0xFFB300), LV_PART_INDICATOR);
                lv_label_set_text(label_bat_status, "SYSTEM OK");
            }

            extern volatile bool flag_botao_b;
            if (ecu_conectada || flag_botao_b) {
                lv_led_set_color(led_ecu_status, lv_color_hex(0x00FF00));
                lv_led_on(led_ecu_status);
            } else {
                lv_led_set_color(led_ecu_status, lv_color_hex(0xFF0000));
                lv_led_on(led_ecu_status);
            }
        }
    }
    rpm_atual = rpm_alvo;
    kmh_atual = kmh_alvo;
}