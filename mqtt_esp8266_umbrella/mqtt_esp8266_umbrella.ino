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

// Color palette
const CRGBPalette16 palette = CRGBPalette16(
  CRGB::White, CRGB(0,18,109), CRGB(161,219,236), CRGB(220,240,247),
  CRGB(12,194,221), CRGB(48,146,206), CRGB(0,83,146), CRGB(223,239,240),
  CRGB::White, CRGB(0,18,109), CRGB(161,219,236), CRGB(220,240,247),
  CRGB(12,194,221), CRGB(48,146,206), CRGB(0,83,146), CRGB(223,239,240)
);

// Globals
struct CRGB leds[numLEDs];
#define GRAVITY 9.81 // Acceleration of gravity in m/s^2
#define h0 3 // Starting height, in meters, of the drip
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

  // Drop the brighness of the red and green to make up for blue.
  // This makes it very dim and the blue still doesn't really come through.
  // TODO: Revisit this.
  LEDS.addLeds<CONFIG_LED_TYPE, ledPin, CONFIG_COLOR_ORDER>(leds, numLEDs).setCorrection(0x1177FF);

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
  static String payloadString = "";
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
void raindrop_side(byte side) {
  // Set up persistent variables. These are only evaluated the first time.
  static const int numSideLed = 8;
  static int colorIndex[2];
  static bool dripping[2] = {false, false};
  static long lastDripStart[2]; // millis() the last drip started falling

  if (!dripping[side]) {
    // Delay some time before a new drip
    if (randBool()) {
      // Reset the values for the next drip
      colorIndex[side] = random8();
      lastDripStart[side] = millis();
      dripping[side] = true;
    } else {
      return;
    }
  }

  long tCycle = millis() - lastDripStart[side];

  // Calculate positon as a function of time and acceleration (gravity)
  float h = 0.5 * GRAVITY * pow(tCycle/1000.0, 2.0);
  int pos = round(h * (numSideLed - 1) / h0); // Map "h" to a "pos" integer index position on the LED strip
  // TODO: Couldn't this just use the built-in `map` function?

  if (pos >= numSideLed) {
    dripping[side] = false;
    return;
  }

  float calcBrightnes = 255.0 / pos; // Drop off the brightness
  int ledIndex;
  if (side == LEFT) {
    ledIndex = numLEDs - pos - 1; // -1 to separate the sides at the top
  } else {
    ledIndex = numLEDs + pos;
  }

  leds[(ledIndex + ledOffset) % numLEDs] += ColorFromPalette(palette, colorIndex[side], calcBrightnes, LINEARBLEND);
}

void raindrops() {
  fadeToBlackBy(leds, numLEDs, fadeSpeed); // 8 bit, 1 = slow, 255 = fast
  raindrop_side(LEFT);
  raindrop_side(RIGHT);
}

bool randBool() {
  return (random(100) < 4);
}
