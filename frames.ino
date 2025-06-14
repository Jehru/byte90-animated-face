#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "esp_sleep.h"          //  ESP32 deep-sleep API
#include "frames.h"             //  all your PROGMEM bitmaps

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- POINTER ARRAY (exact symbols from frames.h) ----------
const unsigned char* const frames[] PROGMEM = {
  epd_bitmap_frame_00_delay_0, epd_bitmap_frame_01_delay_0,
  epd_bitmap_frame_02_delay_0, epd_bitmap_frame_03_delay_0,
  epd_bitmap_frame_04_delay_0, epd_bitmap_frame_05_delay_0,
  epd_bitmap_frame_06_delay_0, epd_bitmap_frame_07_delay_0,
  epd_bitmap_frame_08_delay_0, epd_bitmap_frame_09_delay_0,
  epd_bitmap_frame_10_delay_0, epd_bitmap_frame_11_delay_0,
  epd_bitmap_frame_12_delay_0, epd_bitmap_frame_13_delay_0,
  epd_bitmap_frame_14_delay_0, epd_bitmap_frame_15_delay_0,
  epd_bitmap_frame_16_delay_0, epd_bitmap_frame_17_delay_0,
  epd_bitmap_frame_18_delay_0, epd_bitmap_frame_19_delay_0,
  epd_bitmap_frame_20_delay_0, epd_bitmap_frame_21_delay_0,
  epd_bitmap_frame_22_delay_0, epd_bitmap_frame_23_delay_0,
  epd_bitmap_frame_24_delay_0, epd_bitmap_frame_25_delay_0,
  epd_bitmap_frame_26_delay_0, epd_bitmap_frame_27_delay_0,
  epd_bitmap_frame_28_delay_0, epd_bitmap_frame_29_delay_0,
  epd_bitmap_frame_30_delay_0, epd_bitmap_frame_31_delay_0,
  epd_bitmap_frame_32_delay_0, epd_bitmap_frame_33_delay_0,
  epd_bitmap_frame_34_delay_0, epd_bitmap_frame_35_delay_0,
  epd_bitmap_frame_36_delay_0, epd_bitmap_frame_37_delay_0,
  epd_bitmap_frame_38_delay_0
};
const int  totalFrames = sizeof(frames) / sizeof(frames[0]);
const int  frameDelay  = 80;              //   ~12 fps, tweak if you like
const uint32_t ANIM_RUNTIME_MS = 30UL * 60UL * 1000UL;   // 30 min

uint32_t startMs;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  startMs = millis();                     //  mark start time

  // OPTIONAL: set a hardware wake-up here if you need it later, e.g.
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // wake on button press
}

void loop() {
  //  ----- play frames -----
  for (int i = 0; i < totalFrames; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0,
      (const uint8_t*) pgm_read_ptr(&frames[i]),
      SCREEN_WIDTH, SCREEN_HEIGHT, 1);
    display.display();
    delay(frameDelay);
  }

  //  ----- 30-minute shutdown check -----
  if (millis() - startMs >= ANIM_RUNTIME_MS) {
    // turn off OLED to save power
    display.ssd1306_command(SSD1306_DISPLAYOFF);

    // sleep for “infinite” time (until reset or configured wake source)
    esp_deep_sleep_start();
    // execution never reaches here
  }
}
