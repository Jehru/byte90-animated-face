/*********************************************************************
 * frames.ino — ultra‑simple pickup detector (v6‑b)                
 *                                                                   
 *  CHANGELOG (2025‑06‑14, 2nd pass)                                 
 *  --------------------------------------------------------------   
 *    • Fixed compile error: `StringSumHelper` → `const char*`.       
 *      Added an overload of `logState()` that accepts an Arduino    
 *      `String`, so we can build debug lines with the `+` operator  
 *      yet still reuse the same function.                           
 *    • Minor comments tidy‑up.                                      
 *                                                                   
 *  Behaviour                                                        
 *  --------------------------------------------------------------   
 *    • Display blank most of the time.                              
 *    • Every 3 s: flash **B** for 500 ms as a heartbeat.            
 *    • On pickup (|Δax| ≥ 0.60 m/s² wrt baseline): flash **W**.     
 *********************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

/****************  DISPLAY  ****************/
#define OLED_W      128
#define OLED_H       64
#define OLED_RESET   -1
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, OLED_RESET);

/****************  MPU‑6050  ****************/
Adafruit_MPU6050 mpu;
// ─── tweakables ──────────────────────────────────────────────
const float PICKUP_THRESHOLD       = 0.60f;    // m/s² Δx to qualify as pickup
const unsigned long FLASH_MS       = 500;      // letter dwell time
const unsigned long B_INTERVAL_MS  = 3000;     // gap between B flashes
// ─────────────────────────────────────────────────────────────

static float baselineAx = 0.0f;                // rest x‑axis accel
static unsigned long lastBFlash = 0;           // ms timestamp of last B
static unsigned long lastWFlash = 0;           // ms timestamp of last W

/****************  SERIAL HELPERS  ****************/
static void logAccel(const sensors_event_t& a) {
  Serial.print(F("ACC,"));
  Serial.print(a.acceleration.x, 2); Serial.print(',');
  Serial.print(a.acceleration.y, 2); Serial.print(',');
  Serial.println(a.acceleration.z, 2);
}

// Base version: C‑string
static void logState(const char* s) {
  Serial.print(F("STATE,")); Serial.println(s);
}
// Overload: Arduino String — fixes StringSumHelper compile error
static void logState(const String& s) {
  Serial.print(F("STATE,")); Serial.println(s);
}

/****************  PICKUP DETECTOR  ****************/
static bool pickupDetected() {
  sensors_event_t a, g, t; mpu.getEvent(&a, &g, &t);
  logAccel(a);
  float dx = fabs(a.acceleration.x - baselineAx);
  return dx >= PICKUP_THRESHOLD;
}

/****************  DRAW  ****************/
static void drawLetter(char c) {
  display.clearDisplay();
  display.setTextSize(5);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, 12);
  display.write(c);
  display.display();
}

static void clearScreen() {
  display.clearDisplay();
  display.display();
}

/****************  SETUP  ****************/
void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    logState("INIT_ERR_DISPLAY");
    while (1) delay(100);
  }
  clearScreen();

  if (!mpu.begin()) {
    logState("INIT_ERR_MPU");
    while (1) delay(100);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // establish baseline (board resting flat)
  sensors_event_t a, g, t; mpu.getEvent(&a, &g, &t);
  baselineAx = a.acceleration.x;
  logState(String("BASE,ax=") + String(baselineAx, 2));
  logState("IDLE");
}

/****************  LOOP  ****************/
void loop() {
  unsigned long now = millis();

  // ── pickup? (flash W) ─────────────────────────────────────
  if (pickupDetected()) {
    lastWFlash = now;
    drawLetter('W');
    logState("W");
  }

  // ── heartbeat B every 3 s ─────────────────────────────────
  if (now - lastBFlash >= B_INTERVAL_MS) {
    lastBFlash = now;
    drawLetter('B');
    logState("B");
  }

  // ── clear screen after FLASH_MS ───────────────────────────
  bool letterShowing = (now - lastBFlash < FLASH_MS) || (now - lastWFlash < FLASH_MS);
  static bool screenBlank = false;
  if (!letterShowing && !screenBlank) {
    clearScreen();
    screenBlank = true;
  }
  if (letterShowing) screenBlank = false;
}
