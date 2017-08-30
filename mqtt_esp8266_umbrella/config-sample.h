/*
 * This is a sample configuration file for the ESP8266 umbrella.
 *
 * Change the settings below and save the file as "config.h"
 * You can then upload the code using the Arduino IDE.
 */

// Pins
#define CONFIG_PIN_SERVO D4
#define CONFIG_PIN_LED 7

// Servo angles
#define CONFIG_SERVO_DOWN 100
#define CONFIG_SERVO_UP 150

// FastLED configuration
#define CONFIG_COLOR_ORDER GRB
#define CONFIG_LED_TYPE WS2812B
#define CONFIG_NUM_LEDS 16
#define CONFIG_FADE_SPEED 80 // How fast should the trail fade out? 1=slow, 255=fast

// WiFi
#define CONFIG_WIFI_SSID "{WIFI-SSID}"
#define CONFIG_WIFI_PASS "{WIFI-PASSWORD}"

// MQTT
#define CONFIG_MQTT_HOST "{MQTT-SERVER}"
#define CONFIG_MQTT_USER "{MQTT-USERNAME}"
#define CONFIG_MQTT_PASS "{MQTT-PASSWORD}"

#define CONFIG_MQTT_CLIENT_ID "umbrella" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC "home/umbrella"

// Enables Serial and print statements
#define CONFIG_DEBUG false
