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
const bool debug_mode = CONFIG_DEBUG;

const int servoPin = CONFIG_PIN_SERVO;
const int txPin = BUILTIN_LED; // On-board blue LED
const int ledPin = CONFIG_PIN_LED;

const int fadeSpeed = CONFIG_FADE_SPEED;
const int maxBrightness = 255;
const int numLEDs = CONFIG_NUM_LEDS;
const int ledOffset = 3; // Move the center by this many places. Useful if the ring isn't oriented correctly.

const int servoDown = CONFIG_SERVO_DOWN;
const int servoUp = CONFIG_SERVO_UP;
const int servoDetachDelay = 1000; // in MS

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASS;

const char* mqtt_server = CONFIG_MQTT_HOST;
const char* mqtt_username = CONFIG_MQTT_USER;
const char* mqtt_password = CONFIG_MQTT_PASS;
const char* client_id = CONFIG_MQTT_CLIENT_ID;

const char* mqtt_topic = CONFIG_MQTT_TOPIC;

// Globals
String payloadString = "";
struct CRGB leds[numLEDs];
// Color palette
CRGBPalette16 currentPalette = OceanColors_p;
int colorIndex;
bool dripLeft = true;
bool dripRight = true;

#define GRAVITY 9.81 // Acceleration of gravity in m/s^2
#define h0 4 // Starting height, in meters, of the drip
long lastDripStart; // millis() the last drip started falling
long servoAttachTime;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // pinMode(txPin, OUTPUT);
  // digitalWrite(txPin, HIGH); // Turn off the on-board LED

  delay(1000);

  servo.attach(servoPin);
  servo.write(servoDown);

  LEDS.addLeds<CONFIG_LED_TYPE, ledPin, CONFIG_COLOR_ORDER>(leds, numLEDs).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(maxBrightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);

  if (debug_mode) {
    Serial.begin(115200);
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  servo.attach(servoPin);

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
  // servo.write(newValue);
  Serial.println(scaled);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
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
    ripple();
    FastLED.show();
  }

  // Turn off the servo after a short time. Otherwise, the cheap servo buzzes.
  if (millis() - servoAttachTime > servoDetachDelay) {
    servo.detach();
  }
}

void ripple() {
  int numSideLed = 9;

  fadeToBlackBy(leds, numLEDs, fadeSpeed); // 8 bit, 1 = slow, 255 = fast

  long tCycle = millis() - lastDripStart;

  // A little kinematics equation calculates positon as a function of time and acceleration (gravity)
  float h = 0.5 * GRAVITY * pow(tCycle/1000.0, 2.0);
  int pos = round(h * (numSideLed - 1) / h0); // Map "h" to a "pos" integer index position on the LED strip

  if (pos >= numSideLed + (numSideLed / 3) || pos < 0) {
    // Reset the values for the next drip
    colorIndex = random8();
    dripLeft = randBool();
    dripRight = randBool();
    delay(random(30, 150));
    lastDripStart = millis();
    return;
  } else if (pos >= numSideLed) {
    // Let's overshoot a bit (to let the lights fade)
    return;
  }

  float calcBrightnes = 255.0 / pos; // Drop off the brightness

  if (dripLeft) {
    leds[(numLEDs - pos + ledOffset) % numLEDs] += ColorFromPalette(currentPalette, colorIndex, calcBrightnes, LINEARBLEND);
  }

  if (dripRight) {
    leds[(numLEDs + pos + ledOffset) % numLEDs] += ColorFromPalette(currentPalette, colorIndex, calcBrightnes, LINEARBLEND);
  }
}

bool randBool() {
  return (random(10) < 5); // 5 is 50-50
}
