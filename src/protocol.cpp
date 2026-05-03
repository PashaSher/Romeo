#include "protocol.h"
#include "config.h"
#include "ir_combat.h"
#include "motors.h"
#include "servos.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char s_line[cfg::kLineBufSize];
static uint8_t s_len;

static bool streqi(const char* a, const char* b) {
  while (*a && *b) {
    if (tolower(static_cast<unsigned char>(*a)) !=
        tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == *b;
}

static void reply_ok() { Serial.println(F("OK")); }

static void print_help() {
  Serial.println(F("=== Romeo USB — команды ==="));
  Serial.println(F("MF     оба мотора вперёд"));
  Serial.println(F("MB     оба мотора назад"));
  Serial.println(F("MS     стоп обоих моторов"));
  Serial.println(F("STOP   то же, что MS"));
  Serial.println(F("M1 n   мотор 1, скорость -255..255"));
  Serial.println(F("M2 n   мотор 2, скорость -255..255"));
  Serial.println(F("S1 n   серво 1, угол 0..180"));
  Serial.println(F("S2 n   серво 2, угол 0..180"));
  Serial.println(F("FIRE   ИК-импульс (или IR)"));
  Serial.println(F("PING   проверка связи"));
  Serial.println(F("?      эта справка"));
}

static void reply_err_flash(const __FlashStringHelper* msg) {
  Serial.print(F("ERR "));
  Serial.println(msg);
}

static int16_t parse_int(const char* s, bool& ok) {
  char* end = nullptr;
  long v = strtol(s, &end, 10);
  if (end == s) {
    ok = false;
    return 0;
  }
  ok = true;
  return static_cast<int16_t>(constrain(v, -32768L, 32767L));
}

static void handle_line(char* line) {
  while (*line == ' ' || *line == '\t') {
    ++line;
  }
  if (*line == '\0') {
    return;
  }

  char* sp = strchr(line, ' ');
  if (sp) {
    *sp++ = '\0';
  }

  if (streqi(line, "PING")) {
    Serial.println(F("PONG 1"));
    return;
  }

  if (strcmp(line, "?") == 0 || streqi(line, "HELP") || streqi(line, "H")) {
    print_help();
    reply_ok();
    return;
  }

  if (streqi(line, "MF")) {
    motor1_set(cfg::kMotorCruiseSpeed);
    motor2_set(cfg::kMotorCruiseSpeed);
    reply_ok();
    return;
  }

  if (streqi(line, "MB")) {
    motor1_set(static_cast<int16_t>(-cfg::kMotorCruiseSpeed));
    motor2_set(static_cast<int16_t>(-cfg::kMotorCruiseSpeed));
    reply_ok();
    return;
  }

  if (streqi(line, "MS")) {
    motor1_set(0);
    motor2_set(0);
    reply_ok();
    return;
  }

  if (streqi(line, "M1")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("M1_VAL"));
      return;
    }
    motor1_set(v);
    reply_ok();
    return;
  }

  if (streqi(line, "M2")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("M2_VAL"));
      return;
    }
    motor2_set(v);
    reply_ok();
    return;
  }

  if (streqi(line, "S1")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("S1_VAL"));
      return;
    }
    servo1_set_angle(static_cast<uint8_t>(v));
    reply_ok();
    return;
  }

  if (streqi(line, "S2")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("S2_VAL"));
      return;
    }
    servo2_set_angle(static_cast<uint8_t>(v));
    reply_ok();
    return;
  }

  if (streqi(line, "FIRE") || streqi(line, "IR")) {
    ir_fire_pulse();
    reply_ok();
    return;
  }

  if (streqi(line, "STOP")) {
    motor1_set(0);
    motor2_set(0);
    reply_ok();
    return;
  }

  reply_err_flash(F("UNKNOWN"));
}

void protocol_init() {
  s_len = 0;
  Serial.begin(cfg::kUsbBaud);
  Serial.setTimeout(10);
  Serial.println(F("BOOT ROMEO_USB_PROTO 1"));
}

void protocol_tick() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (s_len < sizeof(s_line)) {
        s_line[s_len] = '\0';
      } else {
        s_line[sizeof(s_line) - 1] = '\0';
      }
      handle_line(s_line);
      s_len = 0;
      continue;
    }
    if (s_len < sizeof(s_line) - 1) {
      s_line[s_len++] = c;
    } else {
      s_len = 0;
      reply_err_flash(F("LINE_OVF"));
    }
  }

  uint32_t t;
  uint16_t seq;
  while (ir_hit_pop(t, seq)) {
    Serial.print(F("EVT HIT "));
    Serial.print(seq);
    Serial.write(' ');
    Serial.println(t);
  }
}
