#pragma once

#include <Arduino.h>

// Romeo V2.2 (DFR0225), USB CDC = Serial (к Raspberry Pi и т.п.)
namespace cfg {

constexpr uint32_t kUsbBaud = 115200;

// --- DC motors, режим PWM (DFR0225 pin table) ---
constexpr uint8_t kMotor1Dir = 4;
constexpr uint8_t kMotor1Pwm = 5;
constexpr uint8_t kMotor2Pwm = 6;
constexpr uint8_t kMotor2Dir = 7;
/** Скорость для команд MF/MB (оба мотора), модуль 1…255 */
constexpr int16_t kMotorCruiseSpeed = 220;
/** Скорость для команд TL/TR (поворот на месте), модуль 1…255 */
constexpr int16_t kMotorTurnSpeed = 200;
/**
 * Если true — TR/TL для «левый = M1, правый = M2».
 * Поставьте false, если у вас наоборот по проводке.
 */
constexpr bool kMotor1IsLeft = true;

// --- Сервы башни (MG996R), пины с ШИМ, не пересекаются с моторами 4–7 ---
constexpr uint8_t kServo1Pin = 9;   // PAN (поворот башни)
constexpr uint8_t kServo2Pin = 10;  // TILT (вверх/вниз)

// Лимиты и «дом» для башни (правьте под свою механику)
constexpr uint8_t kPanMinDeg = 0;
constexpr uint8_t kPanMaxDeg = 180;
constexpr uint8_t kPanHomeDeg = 90;

constexpr uint8_t kTiltMinDeg = 30;
constexpr uint8_t kTiltMaxDeg = 150;
constexpr uint8_t kTiltHomeDeg = 90;

constexpr uint8_t kTurretStepDeg = 5;

// Скоростной режим башни (PANV/TILTV) — макс. угл. скорость, град/с.
// MG996R комфортно крутится 60–180 град/с; больше — рывки.
constexpr int16_t kTurretMaxRateDegPerSec = 240;
// Скорость по умолчанию для PL/PR/TU/TD (без аргумента), град/с.
constexpr int16_t kTurretDefaultRateDegPerSec = 90;
// Если loop «застрянет» дольше этого — клампим dt, чтобы не было прыжка.
constexpr uint16_t kTurretMaxDtMs = 100;

// Диапазон импульсов сервоприводов (для writeMicroseconds).
// MG996R: типично 500..2500 мкс. Если сервоприводы упираются раньше лимита,
// сузьте этот диапазон или подвиньте лимиты углов выше.
constexpr uint16_t kServoMinPulseUs = 500;
constexpr uint16_t kServoMaxPulseUs = 2500;

// --- Бортовой светодиод (на Leonardo это пин 13) ---
constexpr uint8_t kLedPin = LED_BUILTIN;

// --- ИК: передатчик (мигание несущей ~38 кГц), приёмник (прерывание) ---
constexpr uint8_t kIrLedPin = 11;
constexpr uint8_t kIrReceiverPin = 3;  // INT на Leonardo

constexpr uint16_t kIrCarrierHalfUs = 13;   // ~38 кГц
constexpr uint16_t kIrFireDurationMs = 18;
constexpr uint32_t kIrHitDebounceMs = 100;

constexpr uint8_t kLineBufSize = 96;
constexpr uint8_t kHitQueueDepth = 16;

}  // namespace cfg
