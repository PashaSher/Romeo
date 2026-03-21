#include "ir_combat.h"
#include "config.h"

struct HitEvent {
  uint32_t time_ms;
  uint16_t seq;
};

static HitEvent s_queue[cfg::kHitQueueDepth];
static volatile uint8_t s_q_head;
static volatile uint8_t s_q_tail;

static volatile uint32_t s_last_edge_us;
static volatile uint16_t s_seq_next = 1;

static void push_hit_isr(uint32_t t_ms, uint16_t seq) {
  uint8_t next = static_cast<uint8_t>((s_q_tail + 1) % cfg::kHitQueueDepth);
  if (next == s_q_head) {
    return;
  }
  s_queue[s_q_tail].time_ms = t_ms;
  s_queue[s_q_tail].seq = seq;
  s_q_tail = next;
}

static void on_ir_edge() {
  uint32_t now = micros();
  if ((uint32_t)(now - s_last_edge_us) < (cfg::kIrHitDebounceMs * 1000UL)) {
    return;
  }
  s_last_edge_us = now;
  uint16_t seq = s_seq_next++;
  push_hit_isr(millis(), seq);
}

bool ir_hit_pop(uint32_t& out_time_ms, uint16_t& out_seq) {
  noInterrupts();
  if (s_q_head == s_q_tail) {
    interrupts();
    return false;
  }
  HitEvent e = s_queue[s_q_head];
  s_q_head = static_cast<uint8_t>((s_q_head + 1) % cfg::kHitQueueDepth);
  interrupts();
  out_time_ms = e.time_ms;
  out_seq = e.seq;
  return true;
}

void ir_fire_pulse() {
  uint32_t end = millis() + cfg::kIrFireDurationMs;
  while (static_cast<int32_t>(millis() - end) < 0) {
    for (uint8_t n = 0; n < 6; n++) {
      digitalWrite(cfg::kIrLedPin, HIGH);
      delayMicroseconds(cfg::kIrCarrierHalfUs);
      digitalWrite(cfg::kIrLedPin, LOW);
      delayMicroseconds(cfg::kIrCarrierHalfUs);
    }
  }
}

void ir_combat_init() {
  s_q_head = s_q_tail = 0;
  s_last_edge_us = 0;

  pinMode(cfg::kIrLedPin, OUTPUT);
  digitalWrite(cfg::kIrLedPin, LOW);

  pinMode(cfg::kIrReceiverPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(cfg::kIrReceiverPin), on_ir_edge, FALLING);
}

void ir_combat_poll() {
  // Резерв: фильтрация/агрегация в главном цикле при необходимости
}
