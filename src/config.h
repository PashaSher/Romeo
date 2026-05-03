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

// --- Сервы: пины с ШИМ, не пересекаются с 4–7 ---
constexpr uint8_t kServo1Pin = 9;
constexpr uint8_t kServo2Pin = 10;

// --- ИК: передатчик (мигание несущей ~38 кГц), приёмник (прерывание) ---
constexpr uint8_t kIrLedPin = 11;
constexpr uint8_t kIrReceiverPin = 3;  // INT на Leonardo

constexpr uint16_t kIrCarrierHalfUs = 13;   // ~38 кГц
constexpr uint16_t kIrFireDurationMs = 18;
constexpr uint32_t kIrHitDebounceMs = 100;

constexpr uint8_t kLineBufSize = 96;
constexpr uint8_t kHitQueueDepth = 16;

}  // namespace cfg
