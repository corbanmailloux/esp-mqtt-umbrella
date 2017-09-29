/*
 * This is a sample configuration file for the ESP8266 umbrella.
 *
 * Change the settings below and save the file as "config.h"
 * You can then upload the code using the Arduino IDE.
 */

// Pins
#define CONFIG_PIN_SERVO D2
#define CONFIG_PIN_LED D1

// Servo
#define CONFIG_SERVO_DOWN 8
#define CONFIG_SERVO_UP 73
// After setting the servo angle, how long to wait before detaching it in milliseconds.
// Set to -1 to leave the servo attached at all times.
#define CONFIG_SERVO_DETACH_TIME 1000

// FastLED configuration
#define CONFIG_COLOR_ORDER GRB
#define CONFIG_LED_TYPE WS2812B
#define CONFIG_NUM_LEDS 16
#define CONFIG_FADE_SPEED 80 // How fast should the trail fade out? 1=slow, 255=fast
// Move the center of the animation by this many places. Useful if the ring isn't oriented correctly.
// Set to 0 to disable the offset.
#define CONFIG_LED_OFFSET 0

// WiFi
#define CONFIG_WIFI_SSID "{WIFI-SSID}"
#define CONFIG_WIFI_PASS "{WIFI-PASSWORD}"

// MQTT
#define CONFIG_MQTT_HOST "{MQTT-SERVER}"
#define CONFIG_MQTT_PORT 1883 // Usually 1883
#define CONFIG_MQTT_USER "{MQTT-USERNAME}"
#define CONFIG_MQTT_PASS "{MQTT-PASSWORD}"

#define CONFIG_MQTT_CLIENT_ID "umbrella" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC "home/umbrella"

// Enables Serial and print statements
#define CONFIG_DEBUG false
