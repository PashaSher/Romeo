#include "adc.h"
#include "config.h"

static uint16_t s_reference_mv;

void adc_init() {
  s_reference_mv = cfg::kAdcReferenceMv;
  for (uint8_t ch = 0; ch < cfg::kAdcChannels; ++ch) {
    uint8_t pin = static_cast<uint8_t>(A0 + ch);
    pinMode(pin, INPUT);
    digitalWrite(pin, LOW);  // гарантированно отключить внутреннюю подтяжку
  }
  // analogRead() сам выставит REFS0=1 (AVCC) при первом вызове
}

uint16_t adc_reference_mv() { return s_reference_mv; }

void adc_set_reference_mv(uint16_t mv) {
  if (mv < 500) {
    mv = 500;  // защитный нижний предел
  }
  s_reference_mv = mv;
}

void adc_read(uint8_t channel, uint16_t& raw, uint32_t& mv) {
  uint8_t pin = static_cast<uint8_t>(A0 + channel);
  pinMode(pin, INPUT);
  digitalWrite(pin, LOW);  // если где-то был включён pull-up, выключаем перед чтением

  // Первые чтения «прогревают» ADC mux/sample-and-hold после смены канала.
  (void)analogRead(pin);
  (void)analogRead(pin);

  uint32_t acc = 0;
  for (uint8_t i = 0; i < cfg::kAdcSamples; ++i) {
    acc += static_cast<uint16_t>(analogRead(pin));
  }
  raw = static_cast<uint16_t>((acc + (cfg::kAdcSamples / 2)) / cfg::kAdcSamples);
  mv = (static_cast<uint32_t>(raw) * s_reference_mv) / 1023UL;
}

// ATmega32U4: внутренний 1.1В bandgap доступен как канал MUX[5:0] = 0b011110.
// MUX5 находится в ADCSRB, MUX4..0 — в ADMUX.
uint16_t adc_measure_vcc_mv() {
#if defined(__AVR_ATmega32U4__)
  uint8_t admux_save = ADMUX;
  uint8_t adcsrb_save = ADCSRB;

  // REFS0=1 (AVCC с внешним конденсатором на AREF), MUX[4:0]=0b11110 (bandgap)
  ADMUX = (1 << REFS0) | 0x1E;
  ADCSRB &= ~(1 << MUX5);

  delay(2);                         // дать Vref/мультиплексору устаканиться
  ADCSRA |= (1 << ADSC);            // начать преобразование
  while (ADCSRA & (1 << ADSC)) {    // ждать завершения
  }
  uint16_t adc = ADCL;
  adc |= static_cast<uint16_t>(ADCH) << 8;

  // Vcc(mV) ≈ 1.1V * 1023 * 1000 / adc = 1125300 / adc
  uint32_t vcc_mv = 0;
  if (adc != 0) {
    vcc_mv = 1125300UL / adc;
  }

  ADMUX = admux_save;
  ADCSRB = adcsrb_save;
  delay(1);

  if (vcc_mv > 65535UL) {
    vcc_mv = 65535UL;
  }
  return static_cast<uint16_t>(vcc_mv);
#else
  // Заглушка для неподдерживаемых МК
  return cfg::kAdcReferenceMv;
#endif
}

uint16_t adc_calibrate_to_vcc() {
  uint16_t vcc = adc_measure_vcc_mv();
  if (vcc >= 1500 && vcc <= 7000) {
    s_reference_mv = vcc;
  }
  return vcc;
}
