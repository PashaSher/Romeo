#include "led.h"
#include "config.h"

static bool s_state;

void led_init() {
  pinMode(cfg::kLedPin, OUTPUT);
  led_set(false);
}

void led_set(bool on) {
  s_state = on;
  digitalWrite(cfg::kLedPin, on ? HIGH : LOW);
}

void led_toggle() { led_set(!s_state); }

bool led_get() { return s_state; }
