#include <unihiker_k10.h>
#include <lvgl.h>

UNIHIKER_K10 k10;
uint8_t screen_dir = 1; // Tela deitada (Horizontal 320x240)

// ==========================================
// BANDEIRAS DE CONTROLE DOS BOTÕES
// ==========================================
volatile bool flag_botao_a = false;
volatile bool flag_botao_b = false;

// ==========================================
// VARIÁVEIS DOS MOSTRADORES
// ==========================================
lv_obj_t * meter_rpm;
lv_meter_indicator_t * indic_rpm;
lv_obj_t * txt_unidade_rpm;
lv_obj_t * label_valor_rpm; 
int rpm_atual = 0; 
int rpm_alvo = 0; 

lv_obj_t * meter_kmh;
lv_meter_indicator_t * indic_kmh;
lv_obj_t * txt_unidade_kmh;
lv_obj_t * label_valor_kmh; 
int kmh_atual = 0; 
int kmh_alvo = 0; 

lv_obj_t * shift_light;
lv_obj_t * boost_arc;
lv_obj_t * boost_label;

lv_obj_t * temp_bar;
lv_obj_t * temp_label;
lv_obj_t * titulo_diag;

// ==========================================
// DADOS DE SIMULAÇÃO (Seu CSV)
// ==========================================
int valores_csv_simulacao[] = {700, 707, 1168, 1120, 1579, 1361, 1109, 1264, 1260, 1113, 974, 1090, 1388, 1606, 1809, 2337, 2787, 1976, 1821, 1641, 
                              1537, 1556, 1590, 1641, 1710, 1748, 1741, 1710, 1724, 1706, 2091, 2380, 2404, 2516, 2426, 2223, 1847, 1703, 1589, 1671, 
                              1867, 1702, 982, 952, 944, 1114, 1211, 1412, 1612, 1799, 1875, 1255, 974, 1322, 1713, 1775, 1454, 1023, 949, 874, 976, 958, 
                              947, 957, 918, 955, 863, 719, 695, 703, 957, 968, 933, 944, 829, 1166, 909, 1054, 905, 786, 697, 691, 705, 709, 695, 702, 703, 
                              983, 1205, 2238, 1198, 700, 701, 707, 696, 692, 701, 706, 705, 1052, 1557, 1691, 1182, 1374, 1575, 1777, 1448, 1325, 1871, 3183, 
                              4497, 5792, 4105, 3403, 7000, 2498, 1926, 2050, 1642, 1637, 1641, 1633, 1732, 1820, 1855, 1859, 1802, 1730, 1673, 1617, 1573, 
                              1603, 1634, 1662, 1678, 1693, 1712, 1772, 1940, 2053, 1892, 1681, 1445, 1550, 1345, 955, 853, 812, 791, 822, 780, 794};
int total_valores_csv = 152;
int indice_csv = 100;

unsigned long tempo_ultimo_rpm = 0;
int tempo_simulado[] = {1500, 1517, 1486, 1584, 1512, 1595, 1418, 1586, 1349, 1516, 1387, 1642, 1392, 1586, 1484, 1519, 1418, 1586, 1483, 1609, 1461, 
                        3038, 1620, 1447, 1621, 1452, 1613, 1492, 1619, 1416, 1583, 1419, 1586, 1449, 1621, 1486, 1552, 1486, 1586, 1450, 1550, 1451, 
                        1584, 1454, 1548, 1453, 1547, 1459, 1582, 1519, 1620, 1450, 1688, 1451, 1618, 1454, 1617, 1453, 1619, 1483, 1583, 1456, 1550, 
                        1450, 1553, 1417, 1653, 1417, 1619, 1282, 1587, 1449, 1589, 1414, 1619, 1484, 1622, 1418, 1550, 1408, 1563, 1553, 1077, 1217, 
                        1472, 1227, 2833, 2125, 1347, 1487, 1351, 1480, 2191, 545, 1349, 1351, 1211, 1486, 1484, 1485, 1485, 1486, 1474, 1495, 1482, 
                        1484, 1485, 1485, 1483, 1521, 1474, 1596, 1483, 1518, 1486, 1486, 1483, 1584, 1520, 1488, 1481, 1485, 1483, 1521, 1451, 1486, 
                        1448, 1485, 1518, 1487, 1517, 1482, 1486, 1486, 1450, 1485, 1551, 1485, 1481, 1490, 1641, 1599, 1547, 1522, 1485, 1476, 1492, 
                        1486, 1482, 1485, 1475, 1493};

int temp_agua_simulada = 75; 
unsigned long tempo_ultima_temp = 0;

/* ====================================================================
   FUNÇÕES DO LVGL (Animação e Formatação)
   ==================================================================== */
static void set_indic_rpm_value(void * obj, int32_t v) {
    lv_meter_set_indicator_value(meter_rpm, indic_rpm, v);
    lv_label_set_text_fmt(label_valor_rpm, "%d", v); 
    
    // Shift Light - (Simulado)
    if(v >= 5500) {
        lv_led_set_brightness(shift_light, 255); 
    } else if (v >= 4000) {
        lv_led_set_brightness(shift_light, 120); 
    } else {
        lv_led_set_brightness(shift_light, 0);
    }
}

static void set_indic_kmh_value(void * obj, int32_t v) {
    lv_meter_set_indicator_value(meter_kmh, indic_kmh, v);
    lv_label_set_text_fmt(label_valor_kmh, "%d", v); 
}

void animar_agulha(lv_meter_indicator_t * indic, lv_anim_exec_xcb_t func_exec, int origem, int destino, int duracao_ms) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, indic); 
    lv_anim_set_exec_cb(&a, func_exec); 
    lv_anim_set_values(&a, origem, destino); 
    lv_anim_set_time(&a, duracao_ms); 
    lv_anim_start(&a);
}

static void formatar_escala_rpm_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) {
        float valor_real = dsc->value / 1000.0f;
        if (dsc->value % 1000 == 0) sprintf(dsc->text, "%.0f", valor_real); 
        else sprintf(dsc->text, "%.1f", valor_real); 
    }
}

static void formatar_escala_kmh_cb(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part == LV_PART_TICKS && dsc->text) {
        sprintf(dsc->text, "%d", dsc->value); 
    }
}

/* ====================================================================
   CONSTRUÇÃO DA INTERFACE (MÉTODO PLANO - ROOT SCREEN)
   ==================================================================== */
void setup_manometro() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    // ==========================================
    // ELEMENTOS: PERFORMANCE
    // ==========================================
    meter_rpm = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_rpm, 156, 156); 
    lv_obj_align(meter_rpm, LV_ALIGN_LEFT_MID, 2, 0); 
    lv_obj_set_style_bg_color(meter_rpm, lv_color_black(), 0);
    lv_obj_set_style_border_width(meter_rpm, 3, 0); 
    lv_obj_set_style_border_color(meter_rpm, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_color(meter_rpm, lv_color_white(), LV_PART_TICKS); 

    lv_obj_add_event_cb(meter_rpm, formatar_escala_rpm_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_rpm = lv_meter_add_scale(meter_rpm);
    lv_meter_set_scale_ticks(meter_rpm, scale_rpm, 36, 2, 8, lv_color_white());
    lv_meter_set_scale_major_ticks(meter_rpm, scale_rpm, 5, 3, 10, lv_color_white(), 12);
    lv_meter_set_scale_range(meter_rpm, scale_rpm, 0, 7000, 240, 150);
    indic_rpm = lv_meter_add_needle_line(meter_rpm, scale_rpm, 4, lv_palette_main(LV_PALETTE_RED), -10);

    txt_unidade_rpm = lv_label_create(meter_rpm);
    lv_label_set_text(txt_unidade_rpm, "x1000"); 
    lv_obj_set_style_text_color(txt_unidade_rpm, lv_color_hex(0x666666), 0); 
    lv_obj_align(txt_unidade_rpm, LV_ALIGN_CENTER, 0, 45); 

    label_valor_rpm = lv_label_create(meter_rpm);
    lv_label_set_text(label_valor_rpm, "0"); 
    lv_obj_set_style_text_color(label_valor_rpm, lv_color_white(), 0);
    lv_obj_align(label_valor_rpm, LV_ALIGN_CENTER, 0, 25); 

    meter_kmh = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter_kmh, 156, 156); 
    lv_obj_align(meter_kmh, LV_ALIGN_RIGHT_MID, -2, 0); 
    lv_obj_set_style_bg_color(meter_kmh, lv_color_black(), 0);
    lv_obj_set_style_border_width(meter_kmh, 3, 0); 
    lv_obj_set_style_border_color(meter_kmh, lv_palette_main(LV_PALETTE_CYAN), 0); 
    lv_obj_set_style_text_color(meter_kmh, lv_color_white(), LV_PART_TICKS); 

    lv_obj_add_event_cb(meter_kmh, formatar_escala_kmh_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_meter_scale_t * scale_kmh = lv_meter_add_scale(meter_kmh);
    lv_meter_set_scale_ticks(meter_kmh, scale_kmh, 21, 2, 8, lv_color_white());
    lv_meter_set_scale_major_ticks(meter_kmh, scale_kmh, 4, 3, 10, lv_color_white(), 12);
    lv_meter_set_scale_range(meter_kmh, scale_kmh, 0, 200, 240, 150);
    indic_kmh = lv_meter_add_needle_line(meter_kmh, scale_kmh, 4, lv_palette_main(LV_PALETTE_CYAN), -10);

    txt_unidade_kmh = lv_label_create(meter_kmh);
    lv_label_set_text(txt_unidade_kmh, "km/h"); 
    lv_obj_set_style_text_color(txt_unidade_kmh, lv_color_hex(0x666666), 0); 
    lv_obj_align(txt_unidade_kmh, LV_ALIGN_CENTER, 0, 45); 

    label_valor_kmh = lv_label_create(meter_kmh);
    lv_label_set_text(label_valor_kmh, "0"); 
    lv_obj_set_style_text_color(label_valor_kmh, lv_color_white(), 0);
    lv_obj_align(label_valor_kmh, LV_ALIGN_CENTER, 0, 25); 

    shift_light = lv_led_create(lv_scr_act());
    lv_obj_set_size(shift_light, 300, 10); 
    lv_led_set_color(shift_light, lv_palette_main(LV_PALETTE_RED));
    lv_led_set_brightness(shift_light, 0);
    lv_obj_align(shift_light, LV_ALIGN_TOP_MID, 0, 5); 

    boost_arc = lv_arc_create(lv_scr_act());
    lv_obj_set_size(boost_arc, 60, 60);
    lv_arc_set_rotation(boost_arc, 135);
    lv_arc_set_bg_angles(boost_arc, 0, 270);
    lv_arc_set_range(boost_arc, 0, 150); 
    lv_obj_set_style_arc_color(boost_arc, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR);
    lv_obj_align(boost_arc, LV_ALIGN_BOTTOM_MID, 0, -10);

    boost_label = lv_label_create(boost_arc);
    lv_label_set_text(boost_label, "0.0");
    lv_obj_align(boost_label, LV_ALIGN_CENTER, 0, 0);

    // ==========================================
    // ELEMENTOS: DIAGNÓSTICO
    // ==========================================
    titulo_diag = lv_label_create(lv_scr_act());
    lv_label_set_text(titulo_diag, "SAUDE DO BLOCO");
    lv_obj_set_style_text_color(titulo_diag, lv_color_white(), 0);
    lv_obj_align(titulo_diag, LV_ALIGN_TOP_MID, 0, 20);

    temp_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(temp_bar, 40, 140);
    lv_bar_set_range(temp_bar, 50, 120);
    lv_bar_set_value(temp_bar, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
    lv_obj_align(temp_bar, LV_ALIGN_CENTER, 0, 10);

    temp_label = lv_label_create(lv_scr_act());
    lv_label_set_text(temp_label, "-- C");
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
    lv_obj_align_to(temp_label, temp_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Garante que o painel inicie focado na performance
    lv_obj_add_flag(titulo_diag, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(temp_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(temp_label, LV_OBJ_FLAG_HIDDEN);
}

// ==========================================
// CALLBACKS DE BOTÃO SEGUROS
// ==========================================
void onButtonAPressed() { flag_botao_a = true; }
void onButtonBPressed() { flag_botao_b = true; }

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    Serial.println("===============================");
    Serial.println("Iniciando Setup do Unihiker...");
    
    k10.begin();
    k10.initScreen(screen_dir);
    k10.creatCanvas();
    
    k10.buttonA->setPressedCallback(onButtonAPressed);
    k10.buttonB->setPressedCallback(onButtonBPressed);
    
    Serial.println("Construindo Interface LVGL...");
    setup_manometro();
    
    Serial.println("Setup Concluido! Entrando no loop.");
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================
void loop() {
    // 1. CHAVEAMENTO DE TELA
    Serial.println("Pedro Loop");
    delay(100);
    if (flag_botao_a) {
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
        
        lv_obj_clear_flag(meter_rpm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(meter_kmh, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(shift_light, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(boost_arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(boost_label, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_flag(titulo_diag, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(temp_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(temp_label, LV_OBJ_FLAG_HIDDEN);
        
        flag_botao_a = false;
        Serial.println("Tela: Performance");
    }
    
    if (flag_botao_b) {
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x001122), 0);
        
        lv_obj_add_flag(meter_rpm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(meter_kmh, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(shift_light, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(boost_arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(boost_label, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(titulo_diag, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(temp_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(temp_label, LV_OBJ_FLAG_HIDDEN);
        
        flag_botao_b = false;
        Serial.println("Tela: Garagem");
    }

    // 2. SIMULAÇÃO DO SEU CSV
    if (millis() - tempo_ultimo_rpm >= tempo_simulado[indice_csv]) {
        tempo_ultimo_rpm = millis(); 
        
        rpm_atual = rpm_alvo;
        rpm_alvo = valores_csv_simulacao[indice_csv];
        animar_agulha(indic_rpm, set_indic_rpm_value, rpm_atual, rpm_alvo, tempo_simulado[indice_csv]);

        kmh_atual = kmh_alvo;
        kmh_alvo = valores_csv_simulacao[indice_csv] / 30; 
        animar_agulha(indic_kmh, set_indic_kmh_value, kmh_atual, kmh_alvo, tempo_simulado[indice_csv]);
        
        float boost_calculado = ((float)rpm_alvo - 800.0) / (6000.0 - 800.0) * 1.2;
        if (boost_calculado < 0) boost_calculado = 0;
        
        lv_arc_set_value(boost_arc, (int)(boost_calculado * 100)); 
        lv_label_set_text_fmt(boost_label, "%.1f", boost_calculado); 

        indice_csv++;
        if (indice_csv >= total_valores_csv) { indice_csv = 0; }
    }

    // 3. SIMULAÇÃO DA ÁGUA
    if (millis() - tempo_ultima_temp >= 1000) {
        tempo_ultima_temp = millis();
        
        temp_agua_simulada += 1;
        if (temp_agua_simulada > 110) temp_agua_simulada = 75;

        lv_bar_set_value(temp_bar, temp_agua_simulada, LV_ANIM_ON);
        lv_label_set_text_fmt(temp_label, "%d C", temp_agua_simulada);

        if(temp_agua_simulada >= 105) {
            lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(temp_bar, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
        }
    }

    // 4. MOTOR LVGL
    lv_timer_handler(); 
    delay(5);
}
