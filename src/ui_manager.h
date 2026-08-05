#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

extern int estado_tela_atual;
extern lv_obj_t * label_debug;

// Declarações públicas de UI
void setup_ui();
void carregar_tela(int id_tela);
void atualizar_ui();

#endif // UI_MANAGER_H