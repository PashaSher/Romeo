#include "servos.h"
#include "config.h"
#include <Servo.h>

static Servo s_pan;   // servo1, пин 9 — поворот башни
static Servo s_tilt;  // servo2, пин 10 — наклон

static uint8_t s_pan_deg;
static uint8_t s_tilt_deg;

static uint8_t clamp_u8(int16_t v, uint8_t lo, uint8_t hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return static_cast<uint8_t>(v);
}

void servos_init() {
  s_pan.attach(cfg::kServo1Pin);
  s_tilt.attach(cfg::kServo2Pin);
  turret_home();
}

void servo1_set_angle(uint8_t deg) {
  uint8_t v = constrain(deg, 0, 180);
  s_pan.write(v);
  s_pan_deg = v;
}

void servo2_set_angle(uint8_t deg) {
  uint8_t v = constrain(deg, 0, 180);
  s_tilt.write(v);
  s_tilt_deg = v;
}

void turret_home() {
  pan_set(cfg::kPanHomeDeg);
  tilt_set(cfg::kTiltHomeDeg);
}

void pan_set(int16_t deg) {
  s_pan_deg = clamp_u8(deg, cfg::kPanMinDeg, cfg::kPanMaxDeg);
  s_pan.write(s_pan_deg);
}

void tilt_set(int16_t deg) {
  s_tilt_deg = clamp_u8(deg, cfg::kTiltMinDeg, cfg::kTiltMaxDeg);
  s_tilt.write(s_tilt_deg);
}

void pan_step(int16_t delta_deg) {
  pan_set(static_cast<int16_t>(s_pan_deg) + delta_deg);
}

void tilt_step(int16_t delta_deg) {
  tilt_set(static_cast<int16_t>(s_tilt_deg) + delta_deg);
}

uint8_t pan_get() { return s_pan_deg; }
uint8_t tilt_get() { return s_tilt_deg; }
