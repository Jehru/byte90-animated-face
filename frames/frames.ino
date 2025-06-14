/*********************************************************************
 *  Includes – keep the display library lines you already have
 *********************************************************************/
#include "blink.h"     // new – eyes blink frames 03-07  :contentReference[oaicite:2]{index=2}
#include "wink.h"    // you already have this for the wink frames

/*********************************************************************
 *  Frame lookup tables (lives in PROGMEM just like the bitmaps)
 *********************************************************************/
const unsigned char* const BLINK_FRAMES[] PROGMEM = {
  epd_bitmap_frame_03_delay_0,
  epd_bitmap_frame_04_delay_0,
  epd_bitmap_frame_05_delay_0,
  epd_bitmap_frame_04_delay_0
};

const unsigned char* const WINK_FRAMES[] PROGMEM = {
  wink_frame_32_delay_0,
  wink_frame_33_delay_0,
  wink_frame_34_delay_0,
  wink_frame_35_delay_0
};

const uint8_t WINK_COUNT = sizeof(WINK_FRAMES) / sizeof(WINK_FRAMES[0]);

/*********************************************************************
 *  Behaviour state-machine
 *********************************************************************/
enum AnimState { BLINK, WINK };
AnimState  state        = BLINK;   // default
uint8_t    idxBlink     = 0;
uint8_t    idxWink      = 0;
unsigned long lastFrame = 0;
const unsigned long FRAME_MS = 120; // adjust to taste

/*  If you use an accelerometer, replace this with your real “picked-up”
    test (e.g. threshold on ∣X∣+∣Y∣+∣Z∣).  Here we demo with a pin.       */
const uint8_t SENSOR_PIN = 2;

void setup() {
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  // … display init you already have …
}

void loop() {
  /*******************************************************************
   *  1) state transition – did we just pick the bot up?
   *******************************************************************/
  if (digitalRead(SENSOR_PIN) == LOW && state == BLINK) {   // active-low switch
    state    = WINK;
    idxWink  = 0;
  }

  /*******************************************************************
   *  2) time to advance the current animation?
   *******************************************************************/
  if (millis() - lastFrame < FRAME_MS) return;   // not yet
  lastFrame = millis();

  /*******************************************************************
   *  3) show the next frame & update indices / state
   *******************************************************************/
  const unsigned char* frameToDraw;

  if (state == BLINK) {
    frameToDraw = BLINK_FRAMES[idxBlink++];
    if (idxBlink >= BLINK_COUNT) idxBlink = 0;          // loop forever
  } else { // WINK
    frameToDraw = WINK_FRAMES[idxWink++];
    if (idxWink >= WINK_COUNT) {
      state    = BLINK;                                 // back to blink
      idxBlink = 0;
    }
  }

  // draw the frame exactly the way you already do, e.g.:
  // display.clearDisplay();
  // display.drawBitmap(0, 0, frameToDraw, 128, 64, 1);
  // display.display();
}
