#include "FastLED.h" // FastLED library: https://github.com/FastLED/FastLED

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_PIN 7  // LED data pin
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B
#define NUM_LEDS 16

uint8_t max_bright = 100;

struct CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette = OceanColors_p;

uint8_t color;
uint8_t fadeval = 80;
bool dripLeft = true;
bool dripRight = true;

#define GRAVITY 9.81 // Acceleration of gravity in m/s^2
#define h0 4 // Starting height, in meters, of the ball (strip length)

long tLast; // Time since the drop started falling.

void setup() {
  // Serial.begin(57600);
  delay(1000);

  LEDS.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(max_bright);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
}

void loop () {
  EVERY_N_MILLISECONDS(30) { // Non-blocking rate-limiting for the updates.
    ripple();
    FastLED.show();
  }
}

void ripple() {
  int numSideLed = 9;

  fadeToBlackBy(leds, NUM_LEDS, fadeval); // 8 bit, 1 = slow, 255 = fast

  long tCycle = millis() - tLast;

  // A little kinematics equation calculates positon as a function of time and acceleration (gravity)
  float h = 0.5 * GRAVITY * pow(tCycle/1000.0, 2.0);
  int pos = round(h * (numSideLed - 1) / h0); // Map "h" to a "pos" integer index position on the LED strip

  if (pos >= numSideLed + (numSideLed / 3) || pos < 0) {
    color = random8();
    dripLeft = randBool();
    dripRight = randBool();
    delay(random(30, 150));
    tLast = millis();
    return;
  } else if (pos >= numSideLed) {
    // Let's overshoot a bit (to let the lights fade)
    return;
  }

  float calcBrightnes = 255.0 / pos;

  if (dripLeft) {
    leds[(NUM_LEDS - pos) % NUM_LEDS] += ColorFromPalette(currentPalette, color, calcBrightnes, LINEARBLEND);
  }

  if (dripRight) {
    leds[(NUM_LEDS + pos) % NUM_LEDS] += ColorFromPalette(currentPalette, color, calcBrightnes, LINEARBLEND);
  }
}

bool randBool() {
  return (random(10) < 5); // 5 is 50-50
}
