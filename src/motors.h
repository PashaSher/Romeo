#pragma once

#include <Arduino.h>

void motors_init();
void motor1_set(int16_t speed);  // -255..255
void motor2_set(int16_t speed);
