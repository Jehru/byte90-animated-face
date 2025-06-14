/*********************************************************************
   Simple Wink / Blink animator for 128×64 SSD1306 OLED
   – Tested on Arduino UNO-style boards
 *********************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "wink.h"
#include "blink.h"

// ----------  DISPLAY SET-UP ----------
#define OLED_WIDTH   128
#define OLED_HEIGHT   64
#define OLED_RESET    -1          // Reset pin (-1 = share Arduino reset)
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ----------  ANIMATION META-DATA ----------
// ❶  Put the frame pointers in Flash so we don’t waste RAM.
const uint8_t* const WINK_FRAMES[] PROGMEM = {
  winkframe_03_delay_0, winkframe_04_delay_0,
  winkframe_05_delay_0, winkframe_07_delay_0,
  winkframe_17_delay_0
};
const uint8_t  WINK_COUNT = sizeof(WINK_FRAMES) / sizeof(WINK_FRAMES[0]);

const uint8_t* const BLINK_FRAMES[] PROGMEM = {
  epd_bitmap_frame_03_delay_0, epd_bitmap_frame_04_delay_0,
  epd_bitmap_frame_05_delay_0, epd_bitmap_frame_07_delay_0,
  epd_bitmap_frame_17_delay_0
};
const uint8_t  BLINK_COUNT = sizeof(BLINK_FRAMES) / sizeof(BLINK_FRAMES[0]);

// Default frame timing (ms).  Feel free to tweak or use per-frame tables.
const uint16_t FRAME_DELAY = 80;

// ----------  HELPER  ----------
void drawFrame_P(const uint8_t* frame) {
  // Copy bitmap from Flash to the display’s back-buffer, then show it
  display.clearDisplay();
  display.drawBitmap(0, 0, frame, OLED_WIDTH, OLED_HEIGHT, 1);
  display.display();
}

void playAnimation(const uint8_t* const* frameTable, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    // Read the PROGMEM pointer itself:
    const uint8_t* frame = (const uint8_t*)pgm_read_ptr(&frameTable[i]);
    drawFrame_P(frame);
    delay(FRAME_DELAY);
  }
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // or 0x3D if you soldered ADDR
  display.clearDisplay();
}

void loop() {
  playAnimation(WINK_FRAMES,  WINK_COUNT);   // “wink” once
  delay(600);                                // idle pause
  playAnimation(BLINK_FRAMES, BLINK_COUNT);  // “blink” once
  delay(600);
}
