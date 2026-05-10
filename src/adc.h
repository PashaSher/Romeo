#pragma once

#include <Arduino.h>

void adc_init();

// Прочитать аналоговый канал и вернуть (raw, mV) с учётом текущего референса.
// raw — 0..1023, mv — миллиВольты.
void adc_read(uint8_t channel, uint16_t& raw, uint32_t& mv);

// Текущее опорное напряжение (мВ), используемое для пересчёта в милливольты.
uint16_t adc_reference_mv();
void adc_set_reference_mv(uint16_t mv);

// Измерить фактическое AVCC через внутренний 1.1В bandgap (ATmega32U4).
// Возвращает мВ (≈ напряжение питания МК).
uint16_t adc_measure_vcc_mv();

// Удобная обёртка: измерить Vcc и сразу принять как референс.
uint16_t adc_calibrate_to_vcc();
