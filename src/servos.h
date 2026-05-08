#pragma once

#include <Arduino.h>

void servos_init();
// Должна вызываться часто (каждый loop) — обеспечивает плавное движение в скоростном режиме
void turret_tick();

// Низкий уровень — прямая запись угла на серво (0..180)
void servo1_set_angle(uint8_t deg);
void servo2_set_angle(uint8_t deg);

// Башня — позиционные команды (останавливают скоростной режим)
void turret_home();
void pan_set(int16_t deg);
void tilt_set(int16_t deg);
void pan_step(int16_t delta_deg);
void tilt_step(int16_t delta_deg);

// Башня — скоростной режим: задать угловую скорость, плата сама плавно крутит
// до новой команды или до 0. Положительное значение PAN — увеличение угла.
void pan_set_rate(int16_t deg_per_sec);
void tilt_set_rate(int16_t deg_per_sec);

uint8_t pan_get();
uint8_t tilt_get();
int16_t pan_rate_get();
int16_t tilt_rate_get();
