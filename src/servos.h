#pragma once

#include <Arduino.h>

void servos_init();
void servo1_set_angle(uint8_t deg);  // 0..180
void servo2_set_angle(uint8_t deg);
