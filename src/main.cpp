#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <AnimatedGIF.h>
#include "../wink_gif.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
AnimatedGIF gif;
static uint8_t gifBuffer[(SCREEN_WIDTH * SCREEN_HEIGHT) + ((SCREEN_WIDTH * SCREEN_HEIGHT)/8)];

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  gif.begin(GIF_PALETTE_1BPP);
  gif.setFrameBuf(gifBuffer);
  gif.setDrawType(GIF_DRAW_COOKED);
}

void loop() {
  if (gif.open((uint8_t*)wink_gif, wink_gif_len, NULL)) {
    while (gif.playFrame(false, NULL)) {
      display.clearDisplay();
      display.drawBitmap(0, 0,
                         &gifBuffer[SCREEN_WIDTH * SCREEN_HEIGHT],
                         SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      display.display();
    }
    gif.close();
  }
  delay(500);
}
