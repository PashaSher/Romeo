#include "motors.h"
#include "config.h"

static void drive_channel(int16_t speed, uint8_t dir_pin, uint8_t pwm_pin) {
  speed = constrain(speed, -255, 255);
  if (speed == 0) {
    analogWrite(pwm_pin, 0);
    return;
  }
  if (speed > 0) {
    digitalWrite(dir_pin, HIGH);
    analogWrite(pwm_pin, static_cast<uint8_t>(speed));
  } else {
    digitalWrite(dir_pin, LOW);
    analogWrite(pwm_pin, static_cast<uint8_t>(-speed));
  }
}

void motors_init() {
  pinMode(cfg::kMotor1Dir, OUTPUT);
  pinMode(cfg::kMotor2Dir, OUTPUT);
  pinMode(cfg::kMotor1Pwm, OUTPUT);
  pinMode(cfg::kMotor2Pwm, OUTPUT);
  motor1_set(0);
  motor2_set(0);
}

void motor1_set(int16_t speed) {
  drive_channel(speed, cfg::kMotor1Dir, cfg::kMotor1Pwm);
}

void motor2_set(int16_t speed) {
  drive_channel(speed, cfg::kMotor2Dir, cfg::kMotor2Pwm);
}
