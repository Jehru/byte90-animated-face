/*****************************************************************
   Animated-eye demo for 128×64 OLED + MPU6050 accelerometer
   – idle loop  : blinks forever
   – pick-up    : plays wink once, then reverts to blink
   – no delay() : animation and sensor run concurrently
 *****************************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1        // shared reset line not used
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// ------------ FRAME DATA ------------
#include "blink.h"          // epd_bitmap_frame_XX_delay_0 symbols
#include "wink.h"   // wink_frame_XX_delay_0 symbols (NO clashes)

// BLINK animation (4-frame loop)
const unsigned char* const BLINK_FRAMES[] PROGMEM = {
  epd_bitmap_frame_03_delay_0,
  epd_bitmap_frame_04_delay_0,
  epd_bitmap_frame_05_delay_0,
  epd_bitmap_frame_04_delay_0    // reopen
};
const uint8_t BLINK_COUNT  = sizeof(BLINK_FRAMES) / sizeof(BLINK_FRAMES[0]);
const uint16_t BLINK_DELAY = 120;      // ms per blink frame

// WINK animation (plays once)
const unsigned char* const WINK_FRAMES[] PROGMEM = {
  winkframe_32_delay_0, 
  winkframe_33_delay_0,
  winkframe_34_delay_0,
  winkframe_35_delay_0
};
const uint8_t  WINK_COUNT  = sizeof(WINK_FRAMES) / sizeof(WINK_FRAMES[0]);
const uint16_t WINK_DELAY  = 80;

// ------------ STATE MACHINE ------------
enum AnimState { ANIM_BLINK, ANIM_WINK };
AnimState      anim        = ANIM_BLINK;
uint8_t        frameIdx    = 0;
uint32_t       lastFrameMs = 0;
const float    PICKUP_G    = 1.3;      // > 1.3 g considered “picked up”

// ------------ SETUP ------------
void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  if (!mpu.begin()) {
    // MPU not found – keep running animation anyway
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  }
}

// ------------ MAIN LOOP ------------
void loop() {
  uint32_t now = millis();

  /* -------- read accelerometer -------- */
  sensors_event_t a, g, temp;
  if (mpu.getEvent(&a, &g, &temp)) {
    // magnitude of acceleration vector (in g)
    float mag = sqrt(a.acceleration.x * a.acceleration.x +
                     a.acceleration.y * a.acceleration.y +
                     a.acceleration.z * a.acceleration.z) / 9.80665;
    if (mag > PICKUP_G && anim == ANIM_BLINK) {   // start wink once
      anim      = ANIM_WINK;
      frameIdx  = 0;
      lastFrameMs = now;
    }
  }

  /* -------- advance animation -------- */
  uint16_t delayMs = (anim == ANIM_BLINK) ? BLINK_DELAY : WINK_DELAY;
  if (now - lastFrameMs >= delayMs) {
    const unsigned char* bmp;
    if (anim == ANIM_BLINK) {
      bmp = (const uint8_t*)pgm_read_ptr(&BLINK_FRAMES[frameIdx]);
      frameIdx = (frameIdx + 1) % BLINK_COUNT;
    } else { // WINK
      bmp = (const uint8_t*)pgm_read_ptr(&WINK_FRAMES[frameIdx]);
      frameIdx++;
      if (frameIdx >= WINK_COUNT) {          // wink finished
        anim      = ANIM_BLINK;
        frameIdx  = 0;
      }
    }

    display.clearDisplay();
    display.drawBitmap(0, 0, bmp, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
    display.display();
    lastFrameMs = now;
  }
}
