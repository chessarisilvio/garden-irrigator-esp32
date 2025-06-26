#include <Arduino.h>
#include <Wire.h>   
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <universaltelegrambot.h>

//---wifi credentials---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

//---telegram bot credentials---
#define BOT_TOKEN "YOUR_BOT_TOKEN"                  // Replace with your bot token from botfather
#define CHAT_ID "YOUR_CHAT_ID"                      // Replace with your chat ID (where you want to receive messages from the bot)

//---DHT sensor settings (pin definition)---
#define DHTPIN 4                                    // Pin where the DHT data sensor is connected
#define DHTTYPE DHT22                               // DHT 22 sensor type (can be changed to DHT11 if needed)

#define SOIL_PIN 34                                 // Pin for soil moisture analog sensor input pin
#define LIGHT_PIN 35                                // Pin for light sensor (LDR) analog input pin
#define WATER_PUMP_PIN 15                           // Pin for water pump control (digital output pin)

//---sensor thresholds and configurations (asdjust as needed)---
#const int SOIL_MOISTURE_THRESHOLD 2500             // higher value means dryer soil (CALIBRATE THIS VALUE)
#const float MAX_TEMPERATURE = 30.0;                // Maximum temperature for watering (in Celsius)
#const int LIGHT_THRESHOLD = 1000;                  // Light threshold for watering (lower value means more light)

//---Watering configuration---
const unsigned long MANUAL_WATERING_DURATION = 30000;       // Duration for manual watering in milliseconds (30 seconds)
const unsigned long AUTOMATIC_WATERING_DURATION_MS = 60000; // Duration for automatic watering in milliseconds (60 seconds)

//---global variables---
DHT dht(DHTPIN, DHTTYPE);                                  // Initialize DHT sensor
WifiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);               // Initialize Telegram bot

//variables for non-blocking timing
unsigned long previousSensorCheckMillidelta = 0;            // Last time sensors were checked
const long SENSOR_CHECK_INTERVAL_MS = 60000;                // Interval for checking sensors (60 seconds)

unsigned long previousTelegramReportMillis = 0;            // Last time Telegram report was sent
const long TELEGRAM_REPORT_INTERVAL_MS = 3600000;          // Interval for sending Telegram report (1 hour)

unsigned long wateringstarttimr = 0;                       // stores the time when watering started
bool isWateringActive = false;                           // Flag to indicate if watering is active
String wateringStatusmessage = "System initialized:";     // message to report via telegram the status of watering

//---function to connect to WiFi---
void connectToWiFi() {
    serial.printIn("\nWi-Fi Connected! IP Address: ");
