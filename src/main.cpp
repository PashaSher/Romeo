#include <Arduino.h>
#include "config.h"
#include "ir_combat.h"
#include "motors.h"
#include "protocol.h"
#include "servos.h"

void setup() {
  motors_init();
  servos_init();
  ir_combat_init();
  protocol_init();
}

void loop() {
  protocol_tick();
  turret_tick();
}
