#pragma once

#include <Arduino.h>

void servos_init();

// Низкий уровень — прямая запись угла на серво (0..180)
void servo1_set_angle(uint8_t deg);
void servo2_set_angle(uint8_t deg);

// Башня
void turret_home();
void pan_set(int16_t deg);             // абсолютный угол, обрезается лимитами
void tilt_set(int16_t deg);
void pan_step(int16_t delta_deg);      // относительное смещение
void tilt_step(int16_t delta_deg);
uint8_t pan_get();
uint8_t tilt_get();
