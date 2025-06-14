// remote_pet_face.ino
// ------------------------------------------------------------------
//  ▸ Idle   : blink animation (eyes open/close)
//  ▸ Pick‑up (Δax ≤ −0.8 m/s²) : one wink animation
//  ▸ Shake  (Δ‖a‖ ≥ +4  m/s²) : one dizzy animation
//  After every event we re‑calibrate the baseline so the new posture
//  becomes neutral. Serial prints raw data + event banners for tuning.
// ------------------------------------------------------------------

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#include "blink.h"   // blink_frame_00_delay_0 … _25
#include "wink.h"    // wink_frame_00_delay_0  … _25
#include "dizzy.h"   // dizzy_frame_00_delay_0 … _17

// --------------------------- Display ------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --------------------------- IMU ----------------------------------
Adafruit_MPU6050 mpu;
float baselineAx = 0.0f;
float baselineMag = 9.81f;               // learned at runtime

const float PICKUP_THRESHOLD_NEG = -0.8f; // m/s² ↓ along X  ⇒ wink
const float SHAKE_THRESHOLD_MAG =  4.0f;  // m/s² Δ‖a‖       ⇒ dizzy

// --------------------------- Frame tables -------------------------
const unsigned char *BLINK_FRAMES[] = {
  blink_frame_00_delay_0, blink_frame_01_delay_0, blink_frame_02_delay_0,
  blink_frame_03_delay_0, blink_frame_04_delay_0, blink_frame_05_delay_0,
  blink_frame_06_delay_0, blink_frame_07_delay_0, blink_frame_08_delay_0,
  blink_frame_09_delay_0, blink_frame_10_delay_0, blink_frame_11_delay_0,
  blink_frame_12_delay_0, blink_frame_13_delay_0, blink_frame_14_delay_0,
  blink_frame_15_delay_0, blink_frame_16_delay_0, blink_frame_17_delay_0,
  blink_frame_18_delay_0, blink_frame_19_delay_0, blink_frame_20_delay_0,
  blink_frame_21_delay_0, blink_frame_22_delay_0, blink_frame_23_delay_0,
  blink_frame_24_delay_0, blink_frame_25_delay_0 };
const uint8_t BLINK_COUNT = sizeof(BLINK_FRAMES) / sizeof(BLINK_FRAMES[0]);

const unsigned char *WINK_FRAMES[]  = {
  wink_frame_00_delay_0, wink_frame_01_delay_0, wink_frame_02_delay_0,
  wink_frame_03_delay_0, wink_frame_04_delay_0, wink_frame_05_delay_0,
  wink_frame_06_delay_0, wink_frame_07_delay_0, wink_frame_08_delay_0,
  wink_frame_09_delay_0, wink_frame_10_delay_0, wink_frame_11_delay_0,
  wink_frame_12_delay_0, wink_frame_13_delay_0, wink_frame_14_delay_0,
  wink_frame_15_delay_0, wink_frame_16_delay_0, wink_frame_17_delay_0,
  wink_frame_18_delay_0, wink_frame_19_delay_0, wink_frame_20_delay_0,
  wink_frame_21_delay_0, wink_frame_22_delay_0, wink_frame_23_delay_0,
  wink_frame_24_delay_0, wink_frame_25_delay_0 };
const uint8_t WINK_COUNT = sizeof(WINK_FRAMES) / sizeof(WINK_FRAMES[0]);

const unsigned char *DIZZY_FRAMES[] = {
  dizzy_frame_00_delay_0, dizzy_frame_01_delay_0, dizzy_frame_02_delay_0,
  dizzy_frame_03_delay_0, dizzy_frame_04_delay_0, dizzy_frame_05_delay_0,
  dizzy_frame_06_delay_0, dizzy_frame_07_delay_0, dizzy_frame_08_delay_0,
  dizzy_frame_09_delay_0, dizzy_frame_10_delay_0, dizzy_frame_11_delay_0,
  dizzy_frame_12_delay_0, dizzy_frame_13_delay_0, dizzy_frame_14_delay_0,
  dizzy_frame_15_delay_0, dizzy_frame_16_delay_0, dizzy_frame_17_delay_0 };
const uint8_t DIZZY_COUNT = sizeof(DIZZY_FRAMES) / sizeof(DIZZY_FRAMES[0]);

// --------------------------- Prototypes ---------------------------
void playAnimation(const unsigned char **frames, uint8_t cnt, uint16_t delayMs);
void blinkAnim();
void winkAnim();
void dizzyAnim();

bool pickupDetected();
bool shakeDetected();
void calibrateBaseline();

// --------------------------- Setup -------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("Remote Pet Face – booting…");

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 fail");
    for (;;) {}
  }
  display.clearDisplay(); display.display();

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found");
    for (;;) {}
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  calibrateBaseline();
  Serial.print("Baseline ax: "); Serial.println(baselineAx);
  Serial.print("Baseline |a|: "); Serial.println(baselineMag);
}

// --------------------------- Loop -------------------------------
void loop() {
  static bool prevPickup=false, prevShake=false;

  bool shake  = shakeDetected();
  bool pickup = pickupDetected();

  if (shake && !prevShake) {
    Serial.println("EVENT: SHAKE → dizzy");
    dizzyAnim();
    calibrateBaseline();
  } else if (pickup && !prevPickup) {
    Serial.println("EVENT: PICK‑UP → wink");
    winkAnim();
    calibrateBaseline();
  } else if (!shake && !pickup) {
    blinkAnim();
  }

  prevShake  = shake;
  prevPickup = pickup;
}

// --------------------------- Animations --------------------------
void blinkAnim() { playAnimation(BLINK_FRAMES, BLINK_COUNT, 60); }
void winkAnim()  { playAnimation(WINK_FRAMES,  WINK_COUNT, 60); }
void dizzyAnim() { playAnimation(DIZZY_FRAMES, DIZZY_COUNT, 60); }

void playAnimation(const unsigned char **frames, uint8_t cnt, uint16_t delayMs) {
  for (uint8_t i=0; i<cnt; ++i) {
    display.clearDisplay();
    display.drawBitmap(0,0, frames[i], SCREEN_WIDTH, SCREEN_HEIGHT, 1);
    display.display();
    delay(delayMs);
  }
}

// --------------------------- Baseline ----------------------------
void calibrateBaseline() {
  const uint16_t N=200; float sumX=0, sumMag=0; sensors_event_t a,g,t;
  for (uint16_t i=0;i<N;++i){ mpu.getEvent(&a,&g,&t);
    sumX   += a.acceleration.x;
    sumMag += sqrtf(a.acceleration.x*a.acceleration.x + a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z);
    delay(5);
  }
  baselineAx  = sumX/N;
  baselineMag = sumMag/N;
}

// --------------------------- Detection ---------------------------
bool pickupDetected() {
  sensors_event_t a,g,t; mpu.getEvent(&a,&g,&t);
  float deltaX = a.acceleration.x - baselineAx;
  Serial.print("dx:"); Serial.print(deltaX,2); Serial.print(" | ");
  return deltaX <= PICKUP_THRESHOLD_NEG;
}

bool shakeDetected() {
  sensors_event_t a,g,t; mpu.getEvent(&a,&g,&t);
  float mag = sqrtf(a.acceleration.x*a.acceleration.x + a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z);
  float deltaMag = fabsf(mag - baselineMag);
  Serial.print("d|a|:"); Serial.println(deltaMag,2);
  return deltaMag >= SHAKE_THRESHOLD_MAG;
}
