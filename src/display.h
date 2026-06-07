#pragma once

#include <Arduino.h>

void display_init();
void display_tick();
bool display_add_message(const char* text);
void display_clear_messages();
uint8_t display_message_count();
bool display_get_message(uint8_t index, char* out, uint8_t out_size);
