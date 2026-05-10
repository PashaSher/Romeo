#include "protocol.h"
#include "adc.h"
#include "config.h"
#include "ir_combat.h"
#include "led.h"
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
  Serial.println(F("MF       оба мотора вперёд"));
  Serial.println(F("MB       оба мотора назад"));
  Serial.println(F("MS/STOP  стоп обоих моторов"));
  Serial.println(F("TL       разворот на месте влево"));
  Serial.println(F("TR       разворот на месте вправо"));
  Serial.println(F("TANK l r левая и правая гусеница, -255..255 каждая"));
  Serial.println(F("M1 n     мотор 1, скорость -255..255"));
  Serial.println(F("M2 n     мотор 2, скорость -255..255"));
  Serial.println(F("PAN d    башня: поворот, угол с лимитами"));
  Serial.println(F("TILT d   башня: наклон, угол с лимитами"));
  Serial.println(F("TURRET p t  оба угла сразу"));
  Serial.println(F("PANL [s] шаг влево  (по умолчанию kTurretStepDeg)"));
  Serial.println(F("PANR [s] шаг вправо"));
  Serial.println(F("TILTU [s] шаг вверх"));
  Serial.println(F("TILTD [s] шаг вниз"));
  Serial.println(F("PL/PR/TU/TD [v] плавный ход башни (по умолчанию kTurretDefaultRate)"));
  Serial.println(F("TS        стоп плавного хода башни (синоним TSTOP)"));
  Serial.println(F("PANV v   плавный поворот, v град/с (со знаком)"));
  Serial.println(F("TILTV v  плавный наклон, v град/с (со знаком)"));
  Serial.println(F("TURRETV p t   обе скорости сразу"));
  Serial.println(F("TSTOP    остановить плавный ход башни"));
  Serial.println(F("HOME     башня в центр (гасит скорость)"));
  Serial.println(F("POS      текущее: POS PAN <d> TILT <d>"));
  Serial.println(F("S1 n     серво 1 (PAN), 0..180 без лимитов"));
  Serial.println(F("S2 n     серво 2 (TILT), 0..180 без лимитов"));
  Serial.println(F("FIRE/IR  ИК-импульс"));
  Serial.println(F("LON/LOFF/LTG  бортовой светодиод"));
  Serial.println(F("LED ON|OFF|T   то же, развёрнуто; LED без аргумента — статус"));
  Serial.println(F("A0..A5 / A n  АЦП -> A<n> <raw_0_1023> <mV>"));
  Serial.println(F("VBAT     батарея на A1 через 47k/10k -> VBAT <mV> <raw> <pin_mV>"));
  Serial.println(F("VCC      измерить AVCC через bandgap и взять как референс"));
  Serial.println(F("VREF [mv|AUTO]  показать/задать опорное напряжение АЦП"));
  Serial.println(F("PING     проверка связи"));
  Serial.println(F("?        эта справка"));
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

  // --- Танковый поворот на месте ---
  if (streqi(line, "TL") || streqi(line, "TR")) {
    bool turn_right = streqi(line, "TR");
    int16_t v = cfg::kMotorTurnSpeed;
    int16_t left = turn_right ? +v : -v;
    int16_t right = turn_right ? -v : +v;
    if (cfg::kMotor1IsLeft) {
      motor1_set(left);
      motor2_set(right);
    } else {
      motor1_set(right);
      motor2_set(left);
    }
    reply_ok();
    return;
  }

  // TANK <left> <right> — танковая раскладка, каждая гусеница отдельно (-255..255)
  if (streqi(line, "TANK")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    char* sp2 = strchr(sp, ' ');
    if (!sp2) {
      reply_err_flash(F("ARG"));
      return;
    }
    *sp2++ = '\0';
    bool ok1 = false;
    bool ok2 = false;
    int16_t lv = parse_int(sp, ok1);
    int16_t rv = parse_int(sp2, ok2);
    if (!ok1 || !ok2) {
      reply_err_flash(F("TANK_VAL"));
      return;
    }
    if (cfg::kMotor1IsLeft) {
      motor1_set(lv);
      motor2_set(rv);
    } else {
      motor1_set(rv);
      motor2_set(lv);
    }
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

  // --- Башня ---
  if (streqi(line, "HOME")) {
    turret_home();
    reply_ok();
    return;
  }

  if (streqi(line, "POS")) {
    Serial.print(F("POS PAN "));
    Serial.print(pan_get());
    Serial.print(F(" TILT "));
    Serial.println(tilt_get());
    return;
  }

  if (streqi(line, "PAN")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("PAN_VAL"));
      return;
    }
    pan_set(v);
    reply_ok();
    return;
  }

  if (streqi(line, "TILT")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("TILT_VAL"));
      return;
    }
    tilt_set(v);
    reply_ok();
    return;
  }

  if (streqi(line, "TURRET")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    char* sp2 = strchr(sp, ' ');
    if (!sp2) {
      reply_err_flash(F("ARG"));
      return;
    }
    *sp2++ = '\0';
    bool ok1 = false;
    bool ok2 = false;
    int16_t pv = parse_int(sp, ok1);
    int16_t tv = parse_int(sp2, ok2);
    if (!ok1 || !ok2) {
      reply_err_flash(F("TURRET_VAL"));
      return;
    }
    pan_set(pv);
    tilt_set(tv);
    reply_ok();
    return;
  }

  // --- Скоростной режим башни (плавный ход) ---
  if (streqi(line, "PANV")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("PANV_VAL"));
      return;
    }
    pan_set_rate(v);
    reply_ok();
    return;
  }

  if (streqi(line, "TILTV")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok) {
      reply_err_flash(F("TILTV_VAL"));
      return;
    }
    tilt_set_rate(v);
    reply_ok();
    return;
  }

  if (streqi(line, "TURRETV")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    char* sp2 = strchr(sp, ' ');
    if (!sp2) {
      reply_err_flash(F("ARG"));
      return;
    }
    *sp2++ = '\0';
    bool ok1 = false;
    bool ok2 = false;
    int16_t pv = parse_int(sp, ok1);
    int16_t tv = parse_int(sp2, ok2);
    if (!ok1 || !ok2) {
      reply_err_flash(F("TURRETV_VAL"));
      return;
    }
    pan_set_rate(pv);
    tilt_set_rate(tv);
    reply_ok();
    return;
  }

  if (streqi(line, "TSTOP") || streqi(line, "TS")) {
    pan_set_rate(0);
    tilt_set_rate(0);
    reply_ok();
    return;
  }

  // Однобуквенные «джойстиковые» команды плавного хода башни.
  // Без аргумента берут cfg::kTurretDefaultRateDegPerSec.
  // Плавно крутят сервопривод ДО ПРИХОДА ДРУГОЙ КОМАНДЫ (TS / любой PAN/TILT/HOME/STOP).
  if (streqi(line, "PL") || streqi(line, "PR") ||
      streqi(line, "TU") || streqi(line, "TD")) {
    int16_t rate = cfg::kTurretDefaultRateDegPerSec;
    if (sp) {
      bool ok = false;
      int16_t v = parse_int(sp, ok);
      if (!ok || v <= 0) {
        reply_err_flash(F("RATE_VAL"));
        return;
      }
      rate = v;
    }
    if (streqi(line, "PL")) {
      pan_set_rate(static_cast<int16_t>(-rate));
    } else if (streqi(line, "PR")) {
      pan_set_rate(rate);
    } else if (streqi(line, "TU")) {
      tilt_set_rate(rate);
    } else {
      tilt_set_rate(static_cast<int16_t>(-rate));
    }
    reply_ok();
    return;
  }

  if (streqi(line, "PANL") || streqi(line, "PANR") ||
      streqi(line, "TILTU") || streqi(line, "TILTD")) {
    int16_t step = cfg::kTurretStepDeg;
    if (sp) {
      bool ok = false;
      int16_t v = parse_int(sp, ok);
      if (!ok || v <= 0) {
        reply_err_flash(F("STEP_VAL"));
        return;
      }
      step = v;
    }
    if (streqi(line, "PANL")) {
      pan_step(static_cast<int16_t>(-step));
    } else if (streqi(line, "PANR")) {
      pan_step(step);
    } else if (streqi(line, "TILTU")) {
      tilt_step(step);
    } else {
      tilt_step(static_cast<int16_t>(-step));
    }
    reply_ok();
    return;
  }

  if (streqi(line, "FIRE") || streqi(line, "IR")) {
    ir_fire_pulse();
    reply_ok();
    return;
  }

  // --- АЦП: A0..A(kAdcChannels-1) ---
  // Форматы:
  //   "A0", "A1", ... "A5"      — короткая форма
  //   "A 0", "A 3"              — общая форма
  // Ответ:  A<ch> <raw_0_1023> <millivolts>
  if ((line[0] == 'A' || line[0] == 'a') && line[1] >= '0' && line[1] <= '9' &&
      line[2] == '\0') {
    uint8_t ch = static_cast<uint8_t>(line[1] - '0');
    if (ch >= cfg::kAdcChannels) {
      reply_err_flash(F("ADC_CH"));
      return;
    }
    uint16_t raw;
    uint32_t mv;
    adc_read(ch, raw, mv);
    Serial.print(F("A"));
    Serial.print(ch);
    Serial.write(' ');
    Serial.print(raw);
    Serial.write(' ');
    Serial.println(mv);
    return;
  }

  if (streqi(line, "A")) {
    if (!sp) {
      reply_err_flash(F("ARG"));
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok || v < 0 || v >= cfg::kAdcChannels) {
      reply_err_flash(F("ADC_CH"));
      return;
    }
    uint16_t raw;
    uint32_t mv;
    adc_read(static_cast<uint8_t>(v), raw, mv);
    Serial.print(F("A"));
    Serial.print(v);
    Serial.write(' ');
    Serial.print(raw);
    Serial.write(' ');
    Serial.println(mv);
    return;
  }

  // VBAT — напряжение батареи до делителя на A1.
  // Ответ: VBAT <battery_mV> <raw> <pin_mV>
  if (streqi(line, "VBAT")) {
    uint16_t raw;
    uint32_t pin_mv;
    adc_read(cfg::kBatteryAdcChannel, raw, pin_mv);

    uint32_t divider_num = cfg::kBatteryDividerR1Ohm + cfg::kBatteryDividerR2Ohm;
    uint32_t divider_den = cfg::kBatteryDividerR2Ohm;
    uint32_t battery_mv = (pin_mv * divider_num + (divider_den / 2)) / divider_den;
    battery_mv = (battery_mv * cfg::kBatteryCalibrationPpm + 500000UL) / 1000000UL;

    Serial.print(F("VBAT "));
    Serial.print(battery_mv);
    Serial.write(' ');
    Serial.print(raw);
    Serial.write(' ');
    Serial.println(pin_mv);
    return;
  }

  // VCC — измерить фактическое напряжение питания МК через 1.1В bandgap
  // и принять его как опорное для следующих чтений A0..A5.
  if (streqi(line, "VCC")) {
    uint16_t vcc = adc_calibrate_to_vcc();
    Serial.print(F("VCC "));
    Serial.println(vcc);
    return;
  }

  // VREF              — показать текущее опорное напряжение (мВ)
  // VREF <mv>         — задать вручную (например, измеренное мультиметром)
  // VREF AUTO         — то же, что VCC
  if (streqi(line, "VREF")) {
    if (!sp) {
      Serial.print(F("VREF "));
      Serial.println(adc_reference_mv());
      return;
    }
    if (streqi(sp, "AUTO")) {
      uint16_t vcc = adc_calibrate_to_vcc();
      Serial.print(F("VREF "));
      Serial.println(vcc);
      return;
    }
    bool ok = false;
    int16_t v = parse_int(sp, ok);
    if (!ok || v < 500 || v > 7000) {
      reply_err_flash(F("VREF_VAL"));
      return;
    }
    adc_set_reference_mv(static_cast<uint16_t>(v));
    Serial.print(F("VREF "));
    Serial.println(adc_reference_mv());
    return;
  }

  // --- Бортовой светодиод ---
  if (streqi(line, "LON")) {
    led_set(true);
    reply_ok();
    return;
  }
  if (streqi(line, "LOFF")) {
    led_set(false);
    reply_ok();
    return;
  }
  if (streqi(line, "LTG") || streqi(line, "LTOGGLE")) {
    led_toggle();
    reply_ok();
    return;
  }
  if (streqi(line, "LED")) {
    if (!sp) {
      Serial.print(F("LED "));
      Serial.println(led_get() ? F("ON") : F("OFF"));
      return;
    }
    if (streqi(sp, "ON") || strcmp(sp, "1") == 0) {
      led_set(true);
      reply_ok();
      return;
    }
    if (streqi(sp, "OFF") || strcmp(sp, "0") == 0) {
      led_set(false);
      reply_ok();
      return;
    }
    if (streqi(sp, "T") || streqi(sp, "TOGGLE")) {
      led_toggle();
      reply_ok();
      return;
    }
    reply_err_flash(F("LED_VAL"));
    return;
  }

  if (streqi(line, "STOP")) {
    motor1_set(0);
    motor2_set(0);
    pan_set_rate(0);
    tilt_set_rate(0);
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
