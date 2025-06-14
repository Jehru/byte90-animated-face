// remote_pet_face.ino
// ------------------------------------------------------------------
// Show a big “W” whenever the board is picked up (Δax ≤ THRESHOLD).
// Otherwise loop a continuous blink animation. After the “W” holds on
// screen for ~0.6 s the baseline is re‑calibrated so the new posture is
// treated as neutral for the next event.
// Serial prints ax, delta, and high‑level events for tuning.
// ------------------------------------------------------------------

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "blink.h"   // bitmaps blink_frame_00_delay_0 … _25

// --------------------------- Display setup -------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --------------------------- IMU setup -----------------------------
Adafruit_MPU6050 mpu;
float baselineAx = 0.0f;                   // learned at start‑up or after “W”
const float PICKUP_THRESHOLD_NEG = -0.8f;  // m/s² relative drop ⇒ picked up

// --------------------------- Frame table ---------------------------
const unsigned char *BLINK_FRAMES[] = {
  blink_frame_00_delay_0, blink_frame_01_delay_0, blink_frame_02_delay_0,
  blink_frame_03_delay_0, blink_frame_04_delay_0, blink_frame_05_delay_0,
  blink_frame_06_delay_0, blink_frame_07_delay_0, blink_frame_08_delay_0,
  blink_frame_09_delay_0, blink_frame_10_delay_0, blink_frame_11_delay_0,
  blink_frame_12_delay_0, blink_frame_13_delay_0, blink_frame_14_delay_0,
  blink_frame_15_delay_0, blink_frame_16_delay_0, blink_frame_17_delay_0,
  blink_frame_18_delay_0, blink_frame_19_delay_0, blink_frame_20_delay_0,
  blink_frame_21_delay_0, blink_frame_22_delay_0, blink_frame_23_delay_0,
  blink_frame_24_delay_0, blink_frame_25_delay_0
};
const uint8_t BLINK_COUNT = sizeof(BLINK_FRAMES) / sizeof(BLINK_FRAMES[0]);

// --------------------------- Prototypes ----------------------------
void playAnimation(const unsigned char *frames[], uint8_t frameCount,
                   uint8_t loops, uint16_t frameDelay);
void blinkAnimation();
void showW();
bool pickupDetected();
void calibrateBaseline();

// --------------------------- Setup ----------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("Remote Pet Face – booting…");

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed – halting");
    for (;;) {}
  }
  display.clearDisplay();
  display.display();

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found – halting");
    for (;;) {}
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  calibrateBaseline();
  Serial.print("Baseline ax: ");
  Serial.println(baselineAx);
}

// --------------------------- Main loop ----------------------------
void loop() {
  static bool prevPickup = false;

  bool pickup = pickupDetected();

  if (pickup && !prevPickup) {
    Serial.println("EVENT: PICK‑UP → show W");
    showW();
    delay(600);          // hold the W for visibility
    calibrateBaseline(); // learn new neutral
    Serial.print("New baseline ax: ");
    Serial.println(baselineAx);
  } else if (!pickup) {
    blinkAnimation();    // idle
  }

  prevPickup = pickup;
}

// ------------------------------------------------------------------
// Animation helpers
// ------------------------------------------------------------------
void blinkAnimation() {
  playAnimation(BLINK_FRAMES, BLINK_COUNT, 1, 60);
}

void showW() {
  display.clearDisplay();
  display.setTextSize(5);             // fills most of 128×64
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);           // y‑offset to centre vertically-ish
  display.print("W");
  display.display();
}

void playAnimation(const unsigned char *frames[], uint8_t frameCount,
                   uint8_t loops, uint16_t frameDelay) {
  for (uint8_t l = 0; l < loops; ++l) {
    for (uint8_t i = 0; i < frameCount; ++i) {
      display.clearDisplay();
      display.drawBitmap(0, 0, frames[i], SCREEN_WIDTH, SCREEN_HEIGHT, 1);
      display.display();
      delay(frameDelay);
    }
  }
}

// ------------------------------------------------------------------
// IMU utilities
// ------------------------------------------------------------------
void calibrateBaseline() {
  const uint16_t samples = 200;           // ~1 s at 5 ms each
  float sum = 0.0f;
  sensors_event_t a, g, t;
  for (uint16_t i = 0; i < samples; ++i) {
    mpu.getEvent(&a, &g, &t);
    sum += a.acceleration.x;
    delay(5);
  }
  baselineAx = sum / samples;
}

bool pickupDetected() {
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  float delta = a.acceleration.x - baselineAx;
  Serial.print("ax: ");
  Serial.print(a.acceleration.x, 2);
  Serial.print("  delta: ");
  Serial.println(delta, 2);

  return delta <= PICKUP_THRESHOLD_NEG;
}
