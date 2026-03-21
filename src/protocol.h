#pragma once

#include <Arduino.h>

void protocol_init();
// Читает USB Serial, выполняет команды, шлёт EVT HIT из очереди
void protocol_tick();
