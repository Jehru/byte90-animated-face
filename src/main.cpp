#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "../wink_frame_1.h"
#include "../wink_frame_2.h"
#include "../wink_frame_3.h"
#include "../wink_frame_4.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned char* winkFrames[] = {
  wink_frame_1, wink_frame_2, wink_frame_3, wink_frame_4
};
const int frameCount = sizeof(winkFrames) / sizeof(winkFrames[0]);

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
}

void loop() {
  for (int i = 0; i < frameCount; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, winkFrames[i], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    display.display();
    delay(150);
  }
  delay(1000);
}
