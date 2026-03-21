#pragma once

#include <Arduino.h>

void ir_combat_init();
void ir_combat_poll();
bool ir_hit_pop(uint32_t& out_time_ms, uint16_t& out_seq);
void ir_fire_pulse();
