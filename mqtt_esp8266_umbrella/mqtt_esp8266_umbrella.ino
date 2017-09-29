/*
 * ESP8266 MQTT Umbrella to indicate the chance of rain for Home Assistant.
 */

#include "FastLED.h" // FastLED library: https://github.com/FastLED/FastLED
#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

// Set configuration options for pins, WiFi, and MQTT in the following file:
#include "config.h"

#include <ESP8266WiFi.h>
#include <Servo.h>
Servo servo;

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

// Bring in the config.h options
const bool debugMode = CONFIG_DEBUG;

const int servoPin = CONFIG_PIN_SERVO;
const int ledPin = CONFIG_PIN_LED;

const int fadeSpeed = CONFIG_FADE_SPEED;
const int maxBrightness = 255;
const int numLEDs = CONFIG_NUM_LEDS;
const int ledOffset = CONFIG_LED_OFFSET;

const int servoDown = CONFIG_SERVO_DOWN;
const int servoUp = CONFIG_SERVO_UP;
const int servoDetachDelay = CONFIG_SERVO_DETACH_TIME;

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASS;

const char* mqttServer = CONFIG_MQTT_HOST;
const int mqttPort = CONFIG_MQTT_PORT;
const char* mqttUsername = CONFIG_MQTT_USER;
const char* mqttPassword = CONFIG_MQTT_PASS;
const char* mqttClientId = CONFIG_MQTT_CLIENT_ID;

const char* mqttTopic = CONFIG_MQTT_TOPIC;

// Globals
String payloadString = "";
struct CRGB leds[numLEDs];
// Color palette
CRGBPalette16 currentPalette = OceanColors_p;
int colorIndex[2];
bool dripping[2] = {false, false};

#define GRAVITY 9.81 // Acceleration of gravity in m/s^2
#define h0 4 // Starting height, in meters, of the drip
long lastDripStart[2]; // millis() the last drip started falling
#define LEFT 0
#define RIGHT 1

long servoAttachTime;
bool servoAttached = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  if (debugMode) {
    Serial.begin(115200);
  }

  setupWifi();

  servo.attach(servoPin);
  servoAttached = true;
  servo.write(servoDown);

  LEDS.addLeds<CONFIG_LED_TYPE, ledPin, CONFIG_COLOR_ORDER>(leds, numLEDs).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(maxBrightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void setupWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  servo.attach(servoPin);
  servoAttached = true;

  for (int i = 0; i < length; i++) {
    if (isDigit(payload[i])) {
      payloadString += (char)payload[i];
    }
  }

  Serial.println(payloadString);
  int newValue = constrain(payloadString.toInt(), 0, 100);
  payloadString = "";
  int scaled = map(newValue, 0, 100, servoDown, servoUp);
  servo.write(scaled);
  servoAttachTime = millis();
  Serial.println(scaled);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqttClientId, mqttUsername, mqttPassword)) {
      Serial.println("connected");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  EVERY_N_MILLISECONDS(30) { // Non-blocking rate-limiting for the updates.
    raindrops();
    FastLED.show();
  }

  // Turn off the servo after a short time. Otherwise, the cheap servo buzzes.
  if ((servoDetachDelay >= 0) && (servoAttached) && (millis() - servoAttachTime > servoDetachDelay)) {
    servoAttached = false;
    servo.detach();
  }
}

// 0 == left, 1 == right
// Returns true if the drip is still falling, and false if it's done.
bool raindrop_side(byte side) {
  const int numSideLed = 9;
  long tCycle = millis() - lastDripStart[side];

  // Calculate positon as a function of time and acceleration (gravity)
  float h = 0.5 * GRAVITY * pow(tCycle/1000.0, 2.0);
  int pos = round(h * (numSideLed - 1) / h0); // Map "h" to a "pos" integer index position on the LED strip
  // TODO: Couldn't this just use the built-in `map` function?

  if (pos >= numSideLed + (numSideLed / 3) || pos < 0) {
    // Reset the values for the next drip
    colorIndex[side] = random8();
    lastDripStart[side] = millis();
    return false;
  } else if (pos >= numSideLed) {
    // Let's overshoot a bit (to let the lights fade)
    return true;
  }

  float calcBrightnes = 255.0 / pos; // Drop off the brightness
  int ledIndex;
  if (side == LEFT) {
    ledIndex = numLEDs - pos;
  } else {
    ledIndex = numLEDs + pos;
  }

  leds[(ledIndex + ledOffset) % numLEDs] += ColorFromPalette(currentPalette, colorIndex[side], calcBrightnes, LINEARBLEND);
  return true;
}

void raindrops() {
  fadeToBlackBy(leds, numLEDs, fadeSpeed); // 8 bit, 1 = slow, 255 = fast

  dripping[LEFT] = raindrop_side(LEFT);
  dripping[RIGHT] = raindrop_side(RIGHT);

  // TODO: This is currently handled in the raindrop_side function.
  // if (!dripping[LEFT]) {
  //   colorIndex[LEFT] = random8();
  //   lastDripStart[LEFT] = millis();
  // }
  // if (!dripping[RIGHT]) {
  //   colorIndex[RIGHT] = random8();
  //   lastDripStart[RIGHT] = millis();
  // }

  // long tCycle = millis() - lastDripStart;
  //
  // // A little kinematics equation calculates positon as a function of time and acceleration (gravity)
  // float h = 0.5 * GRAVITY * pow(tCycle/1000.0, 2.0);
  // int pos = round(h * (numSideLed - 1) / h0); // Map "h" to a "pos" integer index position on the LED strip
  //
  // if (pos >= numSideLed + (numSideLed / 3) || pos < 0) {
  //   // Reset the values for the next drip
  //   colorIndex = random8();
  //   dripLeft = randBool();
  //   dripRight = randBool();
  //   delay(random(30, 150));
  //   lastDripStart = millis();
  //   return;
  // } else if (pos >= numSideLed) {
  //   // Let's overshoot a bit (to let the lights fade)
  //   return;
  // }
  //
  // float calcBrightnes = 255.0 / pos; // Drop off the brightness
  //
  // if (dripLeft) {
  //   leds[(numLEDs - pos + ledOffset) % numLEDs] += ColorFromPalette(currentPalette, colorIndex, calcBrightnes, LINEARBLEND);
  // }
  //
  // if (dripRight) {
  //   leds[(numLEDs + pos + ledOffset) % numLEDs] += ColorFromPalette(currentPalette, colorIndex, calcBrightnes, LINEARBLEND);
  // }
}

bool randBool() {
  return (random(10) < 5); // 5 is 50-50
}
