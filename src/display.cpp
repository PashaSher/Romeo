#include "display.h"
#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <string.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_BUF_SIZE ((SCREEN_WIDTH * SCREEN_HEIGHT) / 8)

class RomeoOled : public Adafruit_SSD1306 {
public:
  RomeoOled()
      : Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 100000,
                         100000) {}

  bool initStatic(uint8_t* buf, uint8_t addr) {
    buffer = buf;
    return begin(SSD1306_SWITCHCAPVCC, addr, false, false);
  }

  bool reinit(uint8_t addr) {
    if (!buffer) {
      return false;
    }
    return begin(SSD1306_SWITCHCAPVCC, addr, false, false);
  }
};

static uint8_t s_oled_buf[OLED_BUF_SIZE];
static RomeoOled s_oled;
static char s_messages[cfg::kMsgCount][cfg::kMsgTextLen];
static char s_scroll_buf[cfg::kMsgTextLen];
static uint8_t s_word_starts[cfg::kMsgMaxWords];
static uint8_t s_msg_head;
static uint8_t s_msg_count;
static uint8_t s_word_count;
static uint8_t s_word_index;
static uint32_t s_last_word_ms;
static bool s_ready;

static void copy_text(char* dst, const char* src) {
  strncpy(dst, src, cfg::kMsgTextLen - 1);
  dst[cfg::kMsgTextLen - 1] = '\0';
}

static bool i2c_probe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

static bool display_hw_ready() {
  if (!s_ready) {
    return false;
  }
  if (i2c_probe(cfg::kOledI2cAddr)) {
    return true;
  }
  return s_oled.reinit(cfg::kOledI2cAddr);
}

static void display_flush_blank() {
  if (!display_hw_ready()) {
    return;
  }
  memset(s_oled_buf, 0, sizeof(s_oled_buf));
  s_oled.clearDisplay();
  s_oled.display();
}

static void parse_words(char* text) {
  s_word_count = 0;
  char* p = text;
  while (*p && s_word_count < cfg::kMsgMaxWords) {
    while (*p == ' ') {
      ++p;
    }
    if (*p == '\0') {
      break;
    }
    s_word_starts[s_word_count++] = static_cast<uint8_t>(p - text);
    while (*p != '\0' && *p != ' ') {
      ++p;
    }
    if (*p == '\0') {
      break;
    }
    *p++ = '\0';
  }
}

static void load_latest_for_scroll() {
  s_word_count = 0;
  s_word_index = 0;
  s_scroll_buf[0] = '\0';

  if (s_msg_count == 0) {
    return;
  }

  const uint8_t latest =
      static_cast<uint8_t>((s_msg_head + cfg::kMsgCount - 1) % cfg::kMsgCount);
  copy_text(s_scroll_buf, s_messages[latest]);
  parse_words(s_scroll_buf);
}

static void display_show_word(uint8_t index) {
  if (!display_hw_ready()) {
    return;
  }

  s_oled.clearDisplay();
  s_oled.setTextSize(cfg::kOledTextSize);
  s_oled.setTextColor(SSD1306_WHITE);
  s_oled.setTextWrap(true);

  if (s_word_count > 0 && index < s_word_count) {
    s_oled.setCursor(0, 0);
    s_oled.print(&s_scroll_buf[s_word_starts[index]]);
  }

  s_oled.display();
}

static void display_redraw() {
  if (s_word_count == 0) {
    display_flush_blank();
    return;
  }
  display_show_word(s_word_index);
}

void display_init() {
  s_msg_head = 0;
  s_msg_count = 0;
  s_word_count = 0;
  s_word_index = 0;
  s_last_word_ms = 0;
  s_ready = false;

  memset(s_oled_buf, 0, sizeof(s_oled_buf));
  Wire.begin();
  Wire.setClock(100000);
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif
  delay(50);

  if (!s_oled.initStatic(s_oled_buf, cfg::kOledI2cAddr)) {
    return;
  }
  s_ready = true;
  display_clear_messages();
  display_add_message("Romeo ready");
}

void display_tick() {
  if (!s_ready || s_word_count <= 1) {
    return;
  }

  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - s_last_word_ms) < cfg::kMsgScrollMs) {
    return;
  }

  s_last_word_ms = now;
  s_word_index = static_cast<uint8_t>((s_word_index + 1) % s_word_count);
  display_show_word(s_word_index);
}

bool display_add_message(const char* text) {
  if (!text) {
    return false;
  }

  copy_text(s_messages[s_msg_head], text);
  s_msg_head = static_cast<uint8_t>((s_msg_head + 1) % cfg::kMsgCount);
  if (s_msg_count < cfg::kMsgCount) {
    ++s_msg_count;
  }

  load_latest_for_scroll();
  s_last_word_ms = millis();
  display_redraw();
  return true;
}

void display_clear_messages() {
  s_msg_head = 0;
  s_msg_count = 0;
  s_word_count = 0;
  s_word_index = 0;
  s_last_word_ms = 0;
  s_scroll_buf[0] = '\0';
  for (uint8_t i = 0; i < cfg::kMsgCount; ++i) {
    s_messages[i][0] = '\0';
  }
  display_flush_blank();
}

uint8_t display_message_count() { return s_msg_count; }

bool display_get_message(uint8_t index, char* out, uint8_t out_size) {
  if (!out || out_size == 0 || index >= s_msg_count) {
    return false;
  }
  uint8_t start =
      static_cast<uint8_t>((s_msg_head + cfg::kMsgCount - s_msg_count) % cfg::kMsgCount);
  uint8_t slot = static_cast<uint8_t>((start + index) % cfg::kMsgCount);
  strncpy(out, s_messages[slot], out_size - 1);
  out[out_size - 1] = '\0';
  return true;
}
