#include "ui_manager.h"
#include "can_manager.h"
#include "config.h"

int current_screen_state = 0;
lv_obj_t * label_debug = NULL;

// Global simulation external variables
extern volatile bool flag_button_b;

// Private Local Pointers for Screens 0 to 5
static lv_obj_t * meter_rpm = NULL; 
static lv_meter_indicator_t * indic_rpm = NULL; 
static lv_obj_t * label_rpm_val = NULL; 
static lv_obj_t * meter_kmh = NULL; 
static lv_meter_indicator_t * indic_kmh = NULL; 
static lv_obj_t * label_kmh_val = NULL; 
static lv_obj_t * shift_light = NULL; 
static lv_obj_t * boost_arc = NULL; 
static lv_obj_t * boost_label = NULL;
static lv_obj_t * temp_bar = NULL; 
static lv_obj_t * temp_label = NULL;
static lv_obj_t * meter_boost_bmw = NULL; 
static lv_meter_indicator_t * indic_boost_bmw = NULL; 
static lv_obj_t * label_boost_bmw_val = NULL; 
static lv_obj_t * meter_temp_bmw = NULL; 
static lv_meter_indicator_t * indic_temp_bmw = NULL; 
static lv_obj_t * label_temp_bmw_val = NULL;
static lv_obj_t * meter_big_main = NULL; 
static lv_meter_indicator_t * indic_big_main = NULL; 
static lv_obj_t * label_big_main = NULL;
static lv_obj_t * meter_sub_bottom = NULL; 
static lv_meter_indicator_t * indic_sub_bottom = NULL;
static lv_obj_t * meter_peugeot = NULL; 
static lv_meter_indicator_t * indic_peugeot = NULL; 
static lv_obj_t * label_peugeot_val = NULL;

// Screen 5 Pointers
static lv_obj_t * bar_bat_indicator = NULL; 
static lv_obj_t * label_bat_val = NULL; 
static lv_obj_t * label_bat_status = NULL;
static lv_obj_t * arc_maf = NULL; 
static lv_obj_t * label_maf_val = NULL;
static lv_obj_t * led_ecu_status = NULL;

static void reset_screen_pointers() {
    meter_rpm = NULL; 
    indic_rpm = NULL; 
    label_rpm_val = NULL; 
    meter_kmh = NULL; 
    indic_kmh = NULL; 
    label_kmh_val = NULL; 
    shift_light = NULL; 
    boost_arc = NULL; 
    boost_label = NULL;
    temp_bar = NULL; 
    temp_label = NULL;
    meter_boost_bmw = NULL; 
    indic_boost_bmw = NULL; 
    label_boost_bmw_val = NULL; 
    meter_temp_bmw = NULL; 
    indic_temp_bmw = NULL; 
    label_temp_bmw_val = NULL;
    meter_big_main = NULL; 
    indic_big_main = NULL; 
    label_big_main = NULL;
    meter_sub_bottom = NULL; 
    indic_sub_bottom = NULL;
    meter_peugeot = NULL; 
    indic_peugeot = NULL; 
    label_peugeot_val = NULL;
    bar_bat_indicator = NULL; 
    label_bat_val = NULL; 
    label_bat_status = NULL;
    arc_maf = NULL; 
    label_maf_val = NULL;
    led_ecu_status = NULL;
}

// ==========================================
// SCALE FORMATTING CONFIGURATIONS
// ==========================================
static void format_rpm_scale_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) {
        int integer_part = dsc->value / 1000; int decimal_part = (dsc->value % 1000) / 100;
        if (decimal_part == 0) sprintf(dsc->text, "%d", integer_part); 
        else sprintf(dsc->text, "%d.%d", integer_part, decimal_part); 
    }
}
static void format_kmh_scale_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) { sprintf(dsc->text, "%d", dsc->value); }
}
static void format_boost_scale_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) {
        int integer_part = dsc->value / 100; int decimal_part = (dsc->value % 100) / 10;
        sprintf(dsc->text, "%d.%d", integer_part, decimal_part); 
    }
}

static void set_rpm_indicator_value(void * obj, int32_t v) {
    if(current_screen_state == 0 && meter_rpm != NULL) {
        lv_meter_set_indicator_value(meter_rpm, indic_rpm, v);
        lv_label_set_text_fmt(label_rpm_val, "%d", v); 
        if(v >= 5500) lv_led_set_brightness(shift_light, 255); 
        else if (v >= 4000) lv_led_set_brightness(shift_light, 120); 
        else lv_led_set_brightness(shift_light, 0);
    }
}
static void set_kmh_indicator_value(void * obj, int32_t v) {
    if(current_screen_state == 0 && meter_kmh != NULL) {
        lv_meter_set_indicator_value(meter_kmh, indic_kmh, v); 
        lv_label_set_text_fmt(label_kmh_val, "%d", v); 
    }
}

static void animate_needle(lv_meter_indicator_t * indic, lv_anim_exec_xcb_t func_exec, int origin, int destination, int duration_ms) {
    lv_anim_t a; 
    lv_anim_init(&a); 
    lv_anim_set_var(&a, indic); 
    lv_anim_set_exec_cb(&a, func_exec); 
    lv_anim_set_values(&a, origin, destination); 
    lv_anim_set_time(&a, duration_ms); 
    lv_anim_start(&a);
}

static lv_obj_t * create_oil_icon(lv_obj_t * parent, lv_color_t color) {
    // Oil icon container
    lv_obj_t * container = lv_obj_create(parent);
    lv_obj_set_size(container, 32, 18);
    lv_obj_set_style_bg_opa(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Galon body
    lv_obj_t * body = lv_obj_create(container);
    lv_obj_set_size(body, 16, 10);
    lv_obj_align(body, LV_ALIGN_BOTTOM_RIGHT, -4, 0);
    lv_obj_set_style_bg_color(body, color, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_radius(body, 2, 0);

    // Galon handle
    lv_obj_t * handle = lv_obj_create(container);
    lv_obj_set_size(handle, 6, 8);
    lv_obj_align_to(handle, body, LV_ALIGN_OUT_RIGHT_MID, -1, 0); // Inset 1px to blend with body
    lv_obj_set_style_bg_opa(handle, 0, 0);
    lv_obj_set_style_border_color(handle, color, 0);
    lv_obj_set_style_border_width(handle, 2, 0);
    lv_obj_set_style_radius(handle, 2, 0);

    // Spout tip (fully horizontal)
    lv_obj_t * spout = lv_obj_create(container);
    lv_obj_set_size(spout, 10, 2);
    lv_obj_set_style_bg_color(spout, color, 0);
    lv_obj_set_style_border_width(spout, 0, 0);
    lv_obj_align_to(spout, body, LV_ALIGN_OUT_LEFT_TOP, 1, 2); // Snapped to top-left corner

    // Lid
    lv_obj_t * cap = lv_obj_create(container);
    lv_obj_set_size(cap, 4, 2);
    lv_obj_align_to(cap, body, LV_ALIGN_OUT_TOP_MID, 0, 1); // Centered on top of body
    lv_obj_set_style_bg_color(cap, color, 0);
    lv_obj_set_style_border_width(cap, 0, 0);

    // Oil drop
    lv_obj_t * drop = lv_obj_create(container);
    lv_obj_set_size(drop, 3, 3);
    lv_obj_set_style_radius(drop, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(drop, color, 0);
    lv_obj_set_style_border_width(drop, 0, 0);
    lv_obj_align_to(drop, spout, LV_ALIGN_OUT_BOTTOM_LEFT, -1, 2); // Aligned just below spout tip

    return container;
}

// ==========================================
// PRIVATE SCREEN CONSTRUCTION FUNCTIONS
// ==========================================
static void build_screen_0() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    meter_rpm = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_rpm, 156, 156); lv_obj_align(meter_rpm, LV_ALIGN_LEFT_MID, 2, 0); 
    lv_obj_set_style_bg_color(meter_rpm, lv_color_black(), 0); lv_obj_set_style_border_width(meter_rpm, 3, 0); lv_obj_set_style_border_color(meter_rpm, lv_palette_main(LV_PALETTE_RED), 0); lv_obj_set_style_text_color(meter_rpm, lv_color_white(), LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_rpm, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_rpm, format_rpm_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_rpm = lv_meter_add_scale(meter_rpm);
    lv_meter_set_scale_ticks(meter_rpm, scale_rpm, 36, 2, 8, lv_color_white()); lv_meter_set_scale_major_ticks(meter_rpm, scale_rpm, 5, 3, 10, lv_color_white(), 12); lv_meter_set_scale_range(meter_rpm, scale_rpm, 0, 7000, 240, 150);
    indic_rpm = lv_meter_add_needle_line(meter_rpm, scale_rpm, 4, lv_palette_main(LV_PALETTE_RED), -10);
    lv_obj_t * txt_unidade_rpm = lv_label_create(meter_rpm); lv_label_set_text(txt_unidade_rpm, "x1000"); lv_obj_set_style_text_color(txt_unidade_rpm, lv_color_hex(0x666666), 0); lv_obj_align(txt_unidade_rpm, LV_ALIGN_CENTER, 0, 45); 
    label_rpm_val = lv_label_create(meter_rpm); lv_label_set_text(label_rpm_val, "0"); lv_obj_set_style_text_color(label_rpm_val, lv_color_white(), 0); lv_obj_align(label_rpm_val, LV_ALIGN_CENTER, 0, 25); 

    meter_kmh = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_kmh, 156, 156); lv_obj_align(meter_kmh, LV_ALIGN_RIGHT_MID, -2, 0); 
    lv_obj_set_style_bg_color(meter_kmh, lv_color_black(), 0); lv_obj_set_style_border_width(meter_kmh, 3, 0); lv_obj_set_style_border_color(meter_kmh, lv_palette_main(LV_PALETTE_CYAN), 0); lv_obj_set_style_text_color(meter_kmh, lv_color_white(), LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_kmh, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_kmh, format_kmh_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_kmh = lv_meter_add_scale(meter_kmh);
    lv_meter_set_scale_ticks(meter_kmh, scale_kmh, 21, 2, 8, lv_color_white()); lv_meter_set_scale_major_ticks(meter_kmh, scale_kmh, 4, 3, 10, lv_color_white(), 12); lv_meter_set_scale_range(meter_kmh, scale_kmh, 0, 200, 240, 150);
    indic_kmh = lv_meter_add_needle_line(meter_kmh, scale_kmh, 4, lv_palette_main(LV_PALETTE_CYAN), -10);
    lv_obj_t * txt_unidade_kmh = lv_label_create(meter_kmh); lv_label_set_text(txt_unidade_kmh, "km/h"); lv_obj_set_style_text_color(txt_unidade_kmh, lv_color_hex(0x666666), 0); lv_obj_align(txt_unidade_kmh, LV_ALIGN_CENTER, 0, 45); 
    label_kmh_val = lv_label_create(meter_kmh); lv_label_set_text(label_kmh_val, "0"); lv_obj_set_style_text_color(label_kmh_val, lv_color_white(), 0); lv_obj_align(label_kmh_val, LV_ALIGN_CENTER, 0, 25); 

    shift_light = lv_led_create(lv_scr_act()); lv_obj_set_style_align(shift_light, LV_ALIGN_TOP_MID, 0); lv_obj_set_size(shift_light, 300, 10); lv_led_set_color(shift_light, lv_palette_main(LV_PALETTE_RED)); lv_led_set_brightness(shift_light, 0); lv_obj_align(shift_light, LV_ALIGN_TOP_MID, 0, 5); 
    boost_arc = lv_arc_create(lv_scr_act()); lv_obj_set_size(boost_arc, 60, 60); lv_arc_set_rotation(boost_arc, 135); lv_arc_set_bg_angles(boost_arc, 0, 270); lv_arc_set_range(boost_arc, 0, 150); lv_obj_set_style_arc_color(boost_arc, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR); lv_obj_align(boost_arc, LV_ALIGN_BOTTOM_MID, 0, -10);
    boost_label = lv_label_create(boost_arc); lv_label_set_text(boost_label, "0.0"); lv_obj_align(boost_label, LV_ALIGN_CENTER, 0, 0);
}

static void build_screen_1() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0A0F1D), 0); // Fundo espacial escuro de alto contraste
    
    // Title
    lv_obj_t *lbl_title = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl_title, "OIL TEMPERATURE");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x8A99AD), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 15);
    
    // Barra vertical de temperatura
    temp_bar = lv_bar_create(lv_scr_act()); 
    lv_obj_set_size(temp_bar, 30, 140); 
    lv_bar_set_range(temp_bar, 50, 120); 
    lv_bar_set_value(temp_bar, 50, LV_ANIM_OFF); 
    lv_obj_align(temp_bar, LV_ALIGN_CENTER, 5, 10);
    
    // Estilo do fundo da barra (pista cinza escura)
    lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0x1F293D), LV_PART_MAIN);
    
    // Indicador em Degradê Vertical Nativo (Fogo/Vermelho no topo, Gelo/Azul na base)
    lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0xFF3300), LV_PART_INDICATOR);       // Cor do topo (Quente)
    lv_obj_set_style_bg_grad_color(temp_bar, lv_color_hex(0x0055FF), LV_PART_INDICATOR);  // Cor da base (Frio)
    lv_obj_set_style_bg_grad_dir(temp_bar, LV_GRAD_DIR_VER, LV_PART_INDICATOR);          // Direção vertical

    // Escala espectral de temperatura (5 pontos dinâmicos à esquerda da barra)
    for (int i = 0; i < 5; i++) {
        lv_obj_t * dot = lv_obj_create(lv_scr_act());
        lv_obj_set_size(dot, 6, 6);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        
        lv_color_t dot_color;
        if (i == 0) dot_color = lv_color_hex(0xFF3300);      // Topo: Fogo
        else if (i == 1) dot_color = lv_color_hex(0xFF8800);  // Alto-médio: Laranja
        else if (i == 2) dot_color = lv_color_hex(0xFFD600);  // Médio: Amarelo
        else if (i == 3) dot_color = lv_color_hex(0x00D5FF);  // Baixo-médio: Ciano
        else dot_color = lv_color_hex(0x0055FF);             // Base: Gelo
        
        lv_obj_set_style_bg_color(dot, dot_color, 0);
        
        // Distribui uniformemente os pontos ao longo dos 140px de altura da barra
        int y_offset = 10 + (i * 30);
        lv_obj_align_to(dot, temp_bar, LV_ALIGN_OUT_LEFT_TOP, -10, y_offset);
    }

    // Labels de COLD (Gelo) e HOT (Fogo)
    lv_obj_t * lbl_hot = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl_hot, "HOT");
    lv_obj_set_style_text_color(lbl_hot, lv_color_hex(0xFF3300), 0);
    lv_obj_set_style_text_font(lbl_hot, &lv_font_montserrat_14, 0);
    lv_obj_align_to(lbl_hot, temp_bar, LV_ALIGN_OUT_LEFT_TOP, -50, 5);

    lv_obj_t * lbl_cold = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl_cold, "COLD");
    lv_obj_set_style_text_color(lbl_cold, lv_color_hex(0x0055FF), 0);
    lv_obj_set_style_text_font(lbl_cold, &lv_font_montserrat_14, 0);
    lv_obj_align_to(lbl_cold, temp_bar, LV_ALIGN_OUT_LEFT_BOTTOM, -50, -5);

    // Move o ícone de óleo para a lateral superior direita da barra
    lv_obj_t * oil_icon = create_oil_icon(lv_scr_act(), lv_color_hex(0xFFCC00)); // Cor amarelo-ouro clássica de óleo de motor
    lv_obj_align_to(oil_icon, temp_bar, LV_ALIGN_OUT_RIGHT_TOP, 15, 10);

    // Valor numérico em formato digital na parte inferior da barra
    temp_label = lv_label_create(lv_scr_act()); 
    lv_label_set_text(temp_label, "-- °C"); 
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0); 
    lv_obj_align_to(temp_label, temp_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
}

static void build_screen_2() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_color_t amber_color = lv_color_hex(0xFF4500); lv_color_t needle_color = lv_color_hex(0xFF0000);

    // Title
    lv_obj_t *lbl_title = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl_title, "PERFORMANCE MONITOR");
    lv_obj_set_style_text_color(lbl_title, amber_color, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    meter_boost_bmw = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_boost_bmw, 156, 156); lv_obj_align(meter_boost_bmw, LV_ALIGN_LEFT_MID, 2, 0); lv_obj_set_style_bg_color(meter_boost_bmw, lv_color_black(), 0); lv_obj_set_style_border_width(meter_boost_bmw, 0, 0); lv_obj_set_style_text_color(meter_boost_bmw, amber_color, LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_boost_bmw, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_boost_bmw, format_boost_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); lv_meter_scale_t * scale_boost_bmw = lv_meter_add_scale(meter_boost_bmw); lv_meter_set_scale_ticks(meter_boost_bmw, scale_boost_bmw, 31, 2, 6, amber_color); lv_meter_set_scale_major_ticks(meter_boost_bmw, scale_boost_bmw, 5, 3, 10, amber_color, 15); lv_meter_set_scale_range(meter_boost_bmw, scale_boost_bmw, 0, 150, 240, 150); indic_boost_bmw = lv_meter_add_needle_line(meter_boost_bmw, scale_boost_bmw, 4, needle_color, -10);
    lv_obj_t * unidade_boost_bmw = lv_label_create(meter_boost_bmw); lv_label_set_text(unidade_boost_bmw, "Bar"); lv_obj_set_style_text_color(unidade_boost_bmw, amber_color, 0); lv_obj_align(unidade_boost_bmw, LV_ALIGN_CENTER, 0, 45); 
    label_boost_bmw_val = lv_label_create(meter_boost_bmw); lv_label_set_text(label_boost_bmw_val, "0.0"); lv_obj_set_style_text_color(label_boost_bmw_val, lv_color_white(), 0); lv_obj_align(label_boost_bmw_val, LV_ALIGN_CENTER, 0, 25); 

    meter_temp_bmw = lv_meter_create(lv_scr_act()); lv_obj_set_size(meter_temp_bmw, 156, 156); lv_obj_align(meter_temp_bmw, LV_ALIGN_RIGHT_MID, -2, 0); lv_obj_set_style_bg_color(meter_temp_bmw, lv_color_black(), 0); lv_obj_set_style_border_width(meter_temp_bmw, 0, 0); lv_obj_set_style_text_color(meter_temp_bmw, amber_color, LV_PART_TICKS); lv_obj_set_style_bg_opa(meter_temp_bmw, 0, LV_PART_INDICATOR); 
    lv_obj_add_event_cb(meter_temp_bmw, format_kmh_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); lv_meter_scale_t * scale_temp_bmw = lv_meter_add_scale(meter_temp_bmw); lv_meter_set_scale_ticks(meter_temp_bmw, scale_temp_bmw, 15, 2, 6, amber_color); lv_meter_set_scale_major_ticks(meter_temp_bmw, scale_temp_bmw, 2, 3, 10, amber_color, 15); lv_meter_set_scale_range(meter_temp_bmw, scale_temp_bmw, 50, 150, 240, 150); indic_temp_bmw = lv_meter_add_needle_line(meter_temp_bmw, scale_temp_bmw, 4, needle_color, -10); 

    lv_obj_t * unidade_temp_bmw = lv_label_create(meter_temp_bmw); lv_label_set_text(unidade_temp_bmw, "Oil °C"); lv_obj_set_style_text_color(unidade_temp_bmw, amber_color, 0); lv_obj_align(unidade_temp_bmw, LV_ALIGN_CENTER, 0, 45); 
    label_temp_bmw_val = lv_label_create(meter_temp_bmw); lv_label_set_text(label_temp_bmw_val, "50"); lv_obj_set_style_text_color(label_temp_bmw_val, lv_color_white(), 0); lv_obj_align(label_temp_bmw_val, LV_ALIGN_CENTER, 0, 25); 
}

static void build_screen_3() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_color_t amber_color = lv_color_hex(0xFF4500); 
    lv_color_t needle_color = lv_color_hex(0xFF0000);

    meter_big_main = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_big_main, 240, 240); 
    lv_obj_align(meter_big_main, LV_ALIGN_CENTER, 0, 0); 
    lv_obj_set_style_bg_color(meter_big_main, lv_color_black(), 0); 
    lv_obj_set_style_border_width(meter_big_main, 0, 0); 
    lv_obj_set_style_text_color(meter_big_main, amber_color, LV_PART_TICKS); 
    lv_obj_set_style_bg_opa(meter_big_main, 255, LV_PART_INDICATOR); 

    meter_sub_bottom = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_sub_bottom, 210, 210); 
    lv_obj_align(meter_sub_bottom, LV_ALIGN_BOTTOM_MID, 0, 0); 
    lv_obj_set_style_bg_opa(meter_sub_bottom, 0, 0); 
    lv_obj_set_style_border_width(meter_sub_bottom, 0, 0); 
    lv_obj_set_style_text_color(meter_sub_bottom, amber_color, LV_PART_TICKS); 
    lv_obj_set_style_bg_opa(meter_sub_bottom, 0, LV_PART_INDICATOR); 

    lv_obj_add_event_cb(meter_sub_bottom, format_kmh_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_sub = lv_meter_add_scale(meter_sub_bottom);
    lv_meter_set_scale_ticks(meter_sub_bottom, scale_sub, 15, 2, 5, amber_color);
    lv_meter_set_scale_major_ticks(meter_sub_bottom, scale_sub, 7, 3, 10, amber_color, 15);
    lv_meter_set_scale_range(meter_sub_bottom, scale_sub, 50, 150, 90, 45); 
    indic_sub_bottom = lv_meter_add_needle_line(meter_sub_bottom, scale_sub, 3, needle_color, -5);

    lv_obj_t * center_mask = lv_obj_create(lv_scr_act());
    lv_obj_set_size(center_mask, 100, 100); 
    lv_obj_align_to(center_mask, meter_sub_bottom, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(center_mask, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(center_mask, 255, 0); 
    lv_obj_set_style_border_width(center_mask, 0, 0);
    lv_obj_set_style_radius(center_mask, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(center_mask, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(meter_big_main, format_boost_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_big = lv_meter_add_scale(meter_big_main);
    lv_meter_set_scale_ticks(meter_big_main, scale_big, 41, 3, 10, amber_color); 
    lv_meter_set_scale_major_ticks(meter_big_main, scale_big, 5, 4, 18, amber_color, 22); 
    lv_meter_set_scale_range(meter_big_main, scale_big, 0, 150, 240, 150); 
    
    lv_meter_indicator_t * faixa_turbo = lv_meter_add_arc(meter_big_main, scale_big, 2, amber_color, -18);
    lv_meter_set_indicator_start_value(meter_big_main, faixa_turbo, 0);
    lv_meter_set_indicator_end_value(meter_big_main, faixa_turbo, 150);
    indic_big_main = lv_meter_add_needle_line(meter_big_main, scale_big, 4, needle_color, -15);
    
    lv_obj_t * texto_turbo = lv_label_create(lv_scr_act());
    lv_label_set_text(texto_turbo, "TURBO"); 
    lv_obj_set_style_text_color(texto_turbo, amber_color, 0); 
    lv_obj_align(texto_turbo, LV_ALIGN_CENTER, 0, -45); 

    label_big_main = lv_label_create(lv_scr_act());
    lv_label_set_text(label_big_main, "0.0"); 
    lv_obj_set_style_text_color(label_big_main, lv_color_white(), 0); 
    lv_obj_align(label_big_main, LV_ALIGN_CENTER, 0, -25); 

    lv_obj_t * texto_bar = lv_label_create(lv_scr_act());
    lv_label_set_text(texto_bar, "Bar"); 
    lv_obj_set_style_text_color(texto_bar, amber_color, 0); 
    lv_obj_align(texto_bar, LV_ALIGN_CENTER, 0, -10); 

    lv_obj_t * icone_temp = lv_label_create(lv_scr_act()); 
    lv_label_set_text(icone_temp, "Oil °C");
    lv_obj_set_style_text_color(icone_temp, amber_color, 0); 
    lv_obj_align_to(icone_temp, meter_sub_bottom, LV_ALIGN_CENTER, 0, 40); 
}

static void build_screen_4() { 
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_color_t peugeot_red_color = lv_color_hex(0xFF0000); 
    lv_color_t white_color = lv_color_hex(0xFFFFFF);
    
    // Title
    lv_obj_t *lbl_title = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl_title, "BOOST PRESSURE");
    lv_obj_set_style_text_color(lbl_title, white_color, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);
    
    meter_peugeot = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_peugeot, 270, 270); 
    lv_obj_align(meter_peugeot, LV_ALIGN_CENTER, 0, 30); 
    lv_obj_set_style_bg_color(meter_peugeot, lv_color_black(), 0); 
    lv_obj_set_style_border_width(meter_peugeot, 0, 0); 
    lv_obj_set_style_text_color(meter_peugeot, white_color, LV_PART_TICKS); 
    lv_obj_set_style_bg_opa(meter_peugeot, 0, LV_PART_INDICATOR); 
    lv_obj_set_style_pad_all(meter_peugeot, 15, 0);

    lv_obj_add_event_cb(meter_peugeot, format_boost_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_peugeot = lv_meter_add_scale(meter_peugeot);
    lv_meter_set_scale_range(meter_peugeot, scale_peugeot, 0, 150, 240, 150); 
    
    lv_meter_set_scale_ticks(meter_peugeot, scale_peugeot, 31, 2, 8, white_color); 
    lv_meter_set_scale_major_ticks(meter_peugeot, scale_peugeot, 5, 3, 15, white_color, 20); 
    
    lv_meter_indicator_t * red_ring = lv_meter_add_arc(meter_peugeot, scale_peugeot, 6, peugeot_red_color, 15);
    lv_meter_set_indicator_start_value(meter_peugeot, red_ring, 0);
    lv_meter_set_indicator_end_value(meter_peugeot, red_ring, 150);

    indic_peugeot = lv_meter_add_needle_line(meter_peugeot, scale_peugeot, 3, white_color, -10);

    lv_obj_t * needle_center = lv_obj_create(meter_peugeot);
    lv_obj_set_size(needle_center, 40, 40); 
    lv_obj_align(needle_center, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(needle_center, lv_color_hex(0x151515), 0); 
    lv_obj_set_style_bg_opa(needle_center, 255, 0); 
    lv_obj_set_style_border_width(needle_center, 2, 0);
    lv_obj_set_style_border_color(needle_center, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(needle_center, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(needle_center, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * texto_turbo = lv_label_create(lv_scr_act());
    lv_label_set_text(texto_turbo, "TURBO"); 
    lv_obj_set_style_text_color(texto_turbo, white_color, 0); 
    lv_obj_align(texto_turbo, LV_ALIGN_CENTER, 0, -30); 

    label_peugeot_val = lv_label_create(lv_scr_act());
    lv_label_set_text(label_peugeot_val, "0.0"); 
    lv_obj_set_style_text_color(label_peugeot_val, white_color, 0); 
    lv_obj_align(label_peugeot_val, LV_ALIGN_CENTER, 0, 30); 

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

    // Battery Panel
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

static void wheel_rotate_anim_cb(void * var, int32_t val) {
    lv_arc_set_rotation((lv_obj_t *)var, val);
}

static void cloud_anim_cb(void * var, int32_t val) {
    lv_obj_set_x((lv_obj_t *)var, val);
}

static void build_screen_6() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0A0F1D), 0); // Dark space background

    // Main Project Title (Line 1)
    lv_obj_t * title1 = lv_label_create(lv_scr_act());
    lv_label_set_text(title1, "UNIHIKER");
    lv_obj_set_style_text_color(title1, lv_color_hex(0x00D5FF), 0); // Sporty bright cyan
    lv_obj_set_style_text_font(title1, &lv_font_montserrat_14, 0);
    lv_obj_align(title1, LV_ALIGN_TOP_MID, 0, 20);

    // Main Project Title (Line 2)
    lv_obj_t * title2 = lv_label_create(lv_scr_act());
    lv_label_set_text(title2, "AUTOMOTIVE PROJECT");
    lv_obj_set_style_text_color(title2, lv_color_white(), 0);
    lv_obj_set_style_text_font(title2, &lv_font_montserrat_14, 0);
    lv_obj_align_to(title2, title1, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // Cloud 1: Larger, lower, and more opaque (passing behind chassis)
    lv_obj_t * cloud1 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cloud1, 50, 14);
    lv_obj_align(cloud1, LV_ALIGN_TOP_LEFT, -80, 105);
    lv_obj_set_style_bg_color(cloud1, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(cloud1, LV_OPA_50, 0); // 50% opacity
    lv_obj_set_style_border_width(cloud1, 0, 0);
    lv_obj_set_style_radius(cloud1, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(cloud1, LV_OBJ_FLAG_SCROLLABLE);

    // Cloud 2: Smaller, higher, slower (depth/parallax effect)
    lv_obj_t * cloud2 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cloud2, 35, 10);
    lv_obj_align(cloud2, LV_ALIGN_TOP_LEFT, -80, 80);
    lv_obj_set_style_bg_color(cloud2, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(cloud2, LV_OPA_30, 0); // 30% opacity
    lv_obj_set_style_border_width(cloud2, 0, 0);
    lv_obj_set_style_radius(cloud2, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(cloud2, LV_OBJ_FLAG_SCROLLABLE);

    // Horizontal sliding animation setup
    lv_anim_t a_cloud1, a_cloud2;
    lv_anim_init(&a_cloud1);
    lv_anim_set_var(&a_cloud1, cloud1);
    lv_anim_set_exec_cb(&a_cloud1, cloud_anim_cb);
    lv_anim_set_values(&a_cloud1, 260, -80); // Enter from right (260) and fully exit left (-80)
    lv_anim_set_time(&a_cloud1, 8000); // 8-second transit
    lv_anim_set_repeat_count(&a_cloud1, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a_cloud1);

    lv_anim_init(&a_cloud2);
    lv_anim_set_var(&a_cloud2, cloud2);
    lv_anim_set_exec_cb(&a_cloud2, cloud_anim_cb);
    lv_anim_set_values(&a_cloud2, 260, -80);
    lv_anim_set_time(&a_cloud2, 13000); // 13 seconds (different speed for realistic parallax effect)
    lv_anim_set_repeat_count(&a_cloud2, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a_cloud2);

    // Footer / Instruction
    lv_obj_t * subtitle = lv_label_create(lv_scr_act());
    lv_label_set_text(subtitle, "Press Button A to Start");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x4A5568), 0); // Subtle matte grey
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
    lv_obj_align(subtitle, LV_ALIGN_BOTTOM_MID, 0, -25);

    // Central container for vector car drawing
    lv_obj_t * car_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(car_container, 180, 90);
    lv_obj_align(car_container, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_opa(car_container, 0, 0);
    lv_obj_set_style_border_width(car_container, 0, 0);
    lv_obj_clear_flag(car_container, LV_OBJ_FLAG_SCROLLABLE);

    // Drawing: Road surface line
    lv_obj_t * road = lv_obj_create(car_container);
    lv_obj_set_size(road, 160, 2);
    lv_obj_align(road, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_color(road, lv_color_hex(0x1F293D), 0);
    lv_obj_set_style_border_width(road, 0, 0);

    // Drawing: Main car body (Chassis)
    lv_obj_t * body = lv_obj_create(car_container);
    lv_obj_set_size(body, 120, 22);
    lv_obj_align(body, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(body, lv_color_hex(0xFF69B4), 0); // Pink body (Hot Pink)
    lv_obj_set_style_radius(body, 6, 0); // Aerodynamic rounded corners
    lv_obj_set_style_border_width(body, 0, 0);

    // Drawing: Cabin / Windows
    lv_obj_t * cabin = lv_obj_create(car_container);
    lv_obj_set_size(cabin, 55, 18);
    lv_obj_align(cabin, LV_ALIGN_BOTTOM_MID, -8, -38);
    lv_obj_set_style_bg_color(cabin, lv_color_hex(0x0A0F1D), 0); // Same background tone (simulating tinted glass)
    lv_obj_set_style_border_color(cabin, lv_color_hex(0xFF69B4), 0);
    lv_obj_set_style_border_width(cabin, 2, 0);
    lv_obj_set_style_radius(cabin, 5, 0);

    // Drawing: Left Wheel (Rear)
    lv_obj_t * wheel_l = lv_arc_create(car_container);
    lv_obj_set_size(wheel_l, 20, 20);
    lv_obj_align(wheel_l, LV_ALIGN_BOTTOM_LEFT, 45, -6);
    lv_arc_set_bg_angles(wheel_l, 0, 360);
    lv_arc_set_angles(wheel_l, 0, 180); // Hubcap segment for visual rotation
    lv_obj_remove_style(wheel_l, NULL, LV_PART_KNOB);
    lv_obj_set_style_pad_all(wheel_l, 0, 0);
    lv_obj_set_style_arc_color(wheel_l, lv_color_hex(0x111622), LV_PART_MAIN); // Tire
    lv_obj_set_style_arc_width(wheel_l, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_color(wheel_l, lv_color_hex(0xE2E8F0), LV_PART_INDICATOR); // Rotating silver hubcap
    lv_obj_set_style_arc_width(wheel_l, 3, LV_PART_INDICATOR);

    // Drawing: Right Wheel (Front)
    lv_obj_t * wheel_r = lv_arc_create(car_container);
    lv_obj_set_size(wheel_r, 20, 20);
    lv_obj_align(wheel_r, LV_ALIGN_BOTTOM_RIGHT, -45, -6);
    lv_arc_set_bg_angles(wheel_r, 0, 360);
    lv_arc_set_angles(wheel_r, 0, 180);
    lv_obj_remove_style(wheel_r, NULL, LV_PART_KNOB);
    lv_obj_set_style_pad_all(wheel_r, 0, 0);
    lv_obj_set_style_arc_color(wheel_r, lv_color_hex(0x111622), LV_PART_MAIN);
    lv_obj_set_style_arc_width(wheel_r, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_color(wheel_r, lv_color_hex(0xE2E8F0), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(wheel_r, 3, LV_PART_INDICATOR);

    // Initialize smooth continuous wheel rotation
    lv_anim_t a_wheel_l, a_wheel_r;
    lv_anim_init(&a_wheel_l);
    lv_anim_set_var(&a_wheel_l, wheel_l);
    lv_anim_set_exec_cb(&a_wheel_l, wheel_rotate_anim_cb);
    lv_anim_set_values(&a_wheel_l, 0, 359);
    lv_anim_set_time(&a_wheel_l, 600); // 600ms per full revolution (high-speed effect)
    lv_anim_set_repeat_count(&a_wheel_l, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a_wheel_l);

    lv_anim_init(&a_wheel_r);
    lv_anim_set_var(&a_wheel_r, wheel_r);
    lv_anim_set_exec_cb(&a_wheel_r, wheel_rotate_anim_cb);
    lv_anim_set_values(&a_wheel_r, 0, 359);
    lv_anim_set_time(&a_wheel_r, 600);
    lv_anim_set_repeat_count(&a_wheel_r, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a_wheel_r);

    // Drawing: Front Headlight (Yellow glow at the front edge of the car)
    lv_obj_t * headlight = lv_obj_create(car_container);
    lv_obj_set_size(headlight, 6, 3);
    lv_obj_align(headlight, LV_ALIGN_BOTTOM_LEFT, 26, -32);
    lv_obj_set_style_bg_color(headlight, lv_color_hex(0xFFEE00), 0);
    lv_obj_set_style_border_width(headlight, 0, 0);
    lv_obj_set_style_radius(headlight, 1, 0);
}

void load_screen(int screen_id) {
    lv_anim_del(NULL, NULL);    
    lv_obj_clean(lv_scr_act()); 
    reset_screen_pointers();

    current_screen_state = screen_id;
    if(screen_id == 0) build_screen_0();
    else if(screen_id == 1) build_screen_1();
    else if(screen_id == 2) build_screen_2();
    else if(screen_id == 3) build_screen_3();
    else if(screen_id == 4) build_screen_4(); 
    else if(screen_id == 5) build_screen_5();
    else if(screen_id == 6) build_screen_6();
}

void setup_ui() {
    load_screen(6);

    label_debug = lv_label_create(lv_layer_top()); 
    lv_obj_set_style_text_font(label_debug, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_debug, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(label_debug, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(label_debug, "DEBUG STARTED");
#if !SHOW_DEBUG_OVERLAY
    lv_obj_add_flag(label_debug, LV_OBJ_FLAG_HIDDEN);
#endif
}

void update_ui() {
    int integer_boost_value = (int)(calculated_boost * 100);
    int int_part = (int)calculated_boost; 
    int dec_part = (int)((calculated_boost - int_part) * 10);
    int anim_dur = 50;

    if (current_screen_state == 0) {
        animate_needle(indic_rpm, set_rpm_indicator_value, current_rpm, target_rpm, anim_dur);
        animate_needle(indic_kmh, set_kmh_indicator_value, current_kmh, target_kmh, anim_dur);
        if (boost_arc != NULL) {
            lv_arc_set_value(boost_arc, integer_boost_value); 
            lv_label_set_text_fmt(boost_label, "%d.%d", int_part, dec_part);
        }
    }
    else if (current_screen_state == 1) {
        if (temp_bar != NULL) {
            lv_bar_set_value(temp_bar, simulated_oil_temp, LV_ANIM_OFF);
            lv_label_set_text_fmt(temp_label, "%d °C (Oil)", simulated_oil_temp);
        }
    }
    else if (current_screen_state == 2) {
        if (meter_boost_bmw != NULL) {
            lv_meter_set_indicator_value(meter_boost_bmw, indic_boost_bmw, integer_boost_value);
            lv_label_set_text_fmt(label_boost_bmw_val, "%d.%d", int_part, dec_part);
            lv_meter_set_indicator_value(meter_temp_bmw, indic_temp_bmw, simulated_oil_temp);
            lv_label_set_text_fmt(label_temp_bmw_val, "%d", simulated_oil_temp);
        }
    }
    else if (current_screen_state == 3) {
        if (meter_big_main != NULL) {
            lv_meter_set_indicator_value(meter_big_main, indic_big_main, integer_boost_value);
            lv_label_set_text_fmt(label_big_main, "%d.%d", int_part, dec_part);
            lv_meter_set_indicator_value(meter_sub_bottom, indic_sub_bottom, simulated_oil_temp);
        }
    }
    else if (current_screen_state == 4) { 
        if (meter_peugeot != NULL) {
            lv_meter_set_indicator_value(meter_peugeot, indic_peugeot, integer_boost_value);
            lv_label_set_text_fmt(label_peugeot_val, "%d.%d", int_part, dec_part);
        }
    }
    else if (current_screen_state == 5) {
        if (arc_maf != NULL) {
            lv_arc_set_value(arc_maf, (int)current_maf_gs);
            lv_label_set_text_fmt(label_maf_val, "%.1f\ng/s", current_maf_gs);

            lv_label_set_text_fmt(label_bat_val, "%.1f V", current_battery_volts);
            lv_bar_set_value(bar_bat_indicator, (int)(current_battery_volts * 10), LV_ANIM_OFF);

            if (current_battery_volts < 11.8f) {
                lv_obj_set_style_text_color(label_bat_val, lv_color_hex(0xFF0000), 0);
                lv_obj_set_style_text_color(label_bat_status, lv_color_hex(0xFF0000), 0);
                lv_obj_set_style_bg_color(bar_bat_indicator, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
                lv_label_set_text(label_bat_status, "LOW VOLTAGE");
            } else if (current_battery_volts >= 13.5f) {
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

            if (ecu_connected || flag_button_b) {
                lv_led_set_color(led_ecu_status, lv_color_hex(0x00FF00));
                lv_led_on(led_ecu_status);
            } else {
                lv_led_set_color(led_ecu_status, lv_color_hex(0xFF0000));
                lv_led_on(led_ecu_status);
            }
        }
    }
    current_rpm = target_rpm;
    current_kmh = target_kmh;
}