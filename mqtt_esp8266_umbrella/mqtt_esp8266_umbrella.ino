/*
 * ESP8266 MQTT Umbrella to indicate the chance of rain for Home Assistant.
 */

// Set configuration options for pins, WiFi, and MQTT in the following file:
#include "config.h"

#include <ESP8266WiFi.h>
#include <Servo.h>
Servo servo;

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

const bool debug_mode = CONFIG_DEBUG;

const int servoPin = CONFIG_PIN_SERVO;
const int txPin = BUILTIN_LED; // On-board blue LED

const int servoDown = CONFIG_SERVO_DOWN;
const int servoUp = CONFIG_SERVO_UP;

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASS;

const char* mqtt_server = CONFIG_MQTT_HOST;
const char* mqtt_username = CONFIG_MQTT_USER;
const char* mqtt_password = CONFIG_MQTT_PASS;
const char* client_id = CONFIG_MQTT_CLIENT_ID;

const char* mqtt_topic = CONFIG_MQTT_TOPIC;

String payloadString = "";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // pinMode(redPin, OUTPUT);

  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, HIGH); // Turn off the on-board LED

  servo.attach(servoPin);
  servo.write(servoDown);

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
}
