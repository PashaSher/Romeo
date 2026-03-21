#include "servos.h"
#include "config.h"
#include <Servo.h>

static Servo s_servo1;
static Servo s_servo2;

void servos_init() {
  s_servo1.attach(cfg::kServo1Pin);
  s_servo2.attach(cfg::kServo2Pin);
  s_servo1.write(90);
  s_servo2.write(90);
}

void servo1_set_angle(uint8_t deg) {
  s_servo1.write(constrain(deg, 0, 180));
}

void servo2_set_angle(uint8_t deg) {
  s_servo2.write(constrain(deg, 0, 180));
}
