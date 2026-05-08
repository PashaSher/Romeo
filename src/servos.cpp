#include "servos.h"
#include "config.h"
#include <Servo.h>

static Servo s_pan;   // servo1, пин 9 — PAN
static Servo s_tilt;  // servo2, пин 10 — TILT

// Текущая позиция в милли-градусах (1 deg = 1000). Это даёт плавное накопление
// без float: delta_milideg = rate_deg_per_sec * dt_ms.
static int32_t s_pan_milideg;
static int32_t s_tilt_milideg;

static int16_t s_pan_rate;   // град/с (со знаком)
static int16_t s_tilt_rate;
static uint32_t s_last_tick_ms;

static int16_t clamp_i16(int32_t v, int16_t lo, int16_t hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return static_cast<int16_t>(v);
}

static int16_t clamp_rate(int16_t v) {
  return clamp_i16(v, static_cast<int16_t>(-cfg::kTurretMaxRateDegPerSec),
                   cfg::kTurretMaxRateDegPerSec);
}

// Перевод позиции (милли-градусы) в длину импульса (мкс) — даёт под-градусную точность
static uint16_t milideg_to_us(int32_t milideg) {
  if (milideg < 0) {
    milideg = 0;
  }
  if (milideg > 180000L) {
    milideg = 180000L;
  }
  uint32_t span = static_cast<uint32_t>(cfg::kServoMaxPulseUs - cfg::kServoMinPulseUs);
  uint32_t us = cfg::kServoMinPulseUs +
                (static_cast<uint32_t>(milideg) * span) / 180000UL;
  return static_cast<uint16_t>(us);
}

static void apply_pan_limits_and_write() {
  int32_t lo = static_cast<int32_t>(cfg::kPanMinDeg) * 1000;
  int32_t hi = static_cast<int32_t>(cfg::kPanMaxDeg) * 1000;
  if (s_pan_milideg < lo) {
    s_pan_milideg = lo;
    s_pan_rate = 0;
  } else if (s_pan_milideg > hi) {
    s_pan_milideg = hi;
    s_pan_rate = 0;
  }
  s_pan.writeMicroseconds(milideg_to_us(s_pan_milideg));
}

static void apply_tilt_limits_and_write() {
  int32_t lo = static_cast<int32_t>(cfg::kTiltMinDeg) * 1000;
  int32_t hi = static_cast<int32_t>(cfg::kTiltMaxDeg) * 1000;
  if (s_tilt_milideg < lo) {
    s_tilt_milideg = lo;
    s_tilt_rate = 0;
  } else if (s_tilt_milideg > hi) {
    s_tilt_milideg = hi;
    s_tilt_rate = 0;
  }
  s_tilt.writeMicroseconds(milideg_to_us(s_tilt_milideg));
}

void servos_init() {
  s_pan.attach(cfg::kServo1Pin, cfg::kServoMinPulseUs, cfg::kServoMaxPulseUs);
  s_tilt.attach(cfg::kServo2Pin, cfg::kServoMinPulseUs, cfg::kServoMaxPulseUs);
  s_pan_rate = 0;
  s_tilt_rate = 0;
  s_last_tick_ms = millis();
  turret_home();
}

void servo1_set_angle(uint8_t deg) {
  uint8_t v = constrain(deg, 0, 180);
  s_pan_milideg = static_cast<int32_t>(v) * 1000;
  s_pan_rate = 0;
  s_pan.writeMicroseconds(milideg_to_us(s_pan_milideg));
}

void servo2_set_angle(uint8_t deg) {
  uint8_t v = constrain(deg, 0, 180);
  s_tilt_milideg = static_cast<int32_t>(v) * 1000;
  s_tilt_rate = 0;
  s_tilt.writeMicroseconds(milideg_to_us(s_tilt_milideg));
}

void turret_home() {
  pan_set(cfg::kPanHomeDeg);
  tilt_set(cfg::kTiltHomeDeg);
}

void pan_set(int16_t deg) {
  int16_t v = clamp_i16(deg, cfg::kPanMinDeg, cfg::kPanMaxDeg);
  s_pan_milideg = static_cast<int32_t>(v) * 1000;
  s_pan_rate = 0;
  s_pan.writeMicroseconds(milideg_to_us(s_pan_milideg));
}

void tilt_set(int16_t deg) {
  int16_t v = clamp_i16(deg, cfg::kTiltMinDeg, cfg::kTiltMaxDeg);
  s_tilt_milideg = static_cast<int32_t>(v) * 1000;
  s_tilt_rate = 0;
  s_tilt.writeMicroseconds(milideg_to_us(s_tilt_milideg));
}

void pan_step(int16_t delta_deg) {
  pan_set(static_cast<int16_t>(s_pan_milideg / 1000) + delta_deg);
}

void tilt_step(int16_t delta_deg) {
  tilt_set(static_cast<int16_t>(s_tilt_milideg / 1000) + delta_deg);
}

void pan_set_rate(int16_t deg_per_sec) {
  // не сбрасываем s_last_tick_ms: турект-тик сам корректно посчитает dt
  s_pan_rate = clamp_rate(deg_per_sec);
}

void tilt_set_rate(int16_t deg_per_sec) {
  s_tilt_rate = clamp_rate(deg_per_sec);
}

uint8_t pan_get() { return static_cast<uint8_t>(s_pan_milideg / 1000); }
uint8_t tilt_get() { return static_cast<uint8_t>(s_tilt_milideg / 1000); }
int16_t pan_rate_get() { return s_pan_rate; }
int16_t tilt_rate_get() { return s_tilt_rate; }

void turret_tick() {
  uint32_t now = millis();
  uint32_t dt = now - s_last_tick_ms;
  s_last_tick_ms = now;

  if (s_pan_rate == 0 && s_tilt_rate == 0) {
    return;
  }
  if (dt == 0) {
    return;
  }
  if (dt > cfg::kTurretMaxDtMs) {
    dt = cfg::kTurretMaxDtMs;
  }

  if (s_pan_rate != 0) {
    s_pan_milideg += static_cast<int32_t>(s_pan_rate) * static_cast<int32_t>(dt);
    apply_pan_limits_and_write();
  }
  if (s_tilt_rate != 0) {
    s_tilt_milideg += static_cast<int32_t>(s_tilt_rate) * static_cast<int32_t>(dt);
    apply_tilt_limits_and_write();
  }
}
