#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>

// --- Wi-Fi Credentials ---
const char* WIFI_SSID = "YOUR_WIFI_SSID";     // Replace with your Wi-Fi SSID
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"; // Replace with your Wi-Fi Password

// --- Telegram Bot Credentials ---
#define BOT_TOKEN "YOUR_BOT_TOKEN" // Replace with your Bot Token from BotFather
#define CHAT_ID "YOUR_CHAT_ID"     // Replace with your Chat ID (where the bot will send messages)

// --- Pin Definitions ---
#define DHTPIN 4            // DHT sensor data pin
#define DHTTYPE DHT22       // DHT sensor type (change to DHT11 if you use that)

#define SOIL_PIN 34         // Soil moisture sensor analog input pin
#define LIGHT_PIN 35        // Light sensor (LDR) analog input pin
#define RELAY_PIN 15        // Relay control pin for the watering pump

// --- Sensor Thresholds and Configuration (Adjust these values!) ---
const int SOIL_DRY_THRESHOLD = 2500; // Higher value means drier soil (CALIBRATE THIS!)
const float MAX_TEMPERATURE = 30.0;  // Maximum temperature in Celsius for watering
const int LIGHT_THRESHOLD = 1000;    // Lower value means darker conditions (e.g., night)

// --- Watering Durations ---
const unsigned long MANUAL_WATERING_DURATION_MS = 30000; // Manual watering duration: 30 seconds
const unsigned long AUTOMATIC_WATERING_DURATION_MS = 60000; // Automatic watering duration: 60 seconds (1 minute)

// --- Global Variables ---
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Variables for non-blocking timing
unsigned long previousSensorCheckMillis = 0;
const long SENSOR_CHECK_INTERVAL_MS = 60000; // Check sensors every 60 seconds (1 minute)

unsigned long previousTelegramReportMillis = 0;
const long TELEGRAM_REPORT_INTERVAL_MS = 3600000; // Send Telegram report every 1 hour (3600 * 1000 ms)

unsigned long wateringStartTime = 0; // Stores the time when watering started
bool isWateringActive = false;       // Flag to track if watering is currently active
String wateringStatusMessage = "System initialized."; // Message to report via Telegram

// --- Function to connect to Wi-Fi ---
void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) { // Try for 30 seconds
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi Connected! IP Address: ");
        Serial.println(WiFi.localIP());
        client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Use Telegram's root certificate for security
    } else {
        Serial.println("\nFailed to connect to Wi-Fi.");
        // Consider deeper sleep or reset if Wi-Fi continually fails
    }
}

// --- Function to start watering ---
void startWatering(unsigned long durationMs, String source) {
    if (!isWateringActive) { // Only start if not already watering
        digitalWrite(RELAY_PIN, HIGH);
        wateringStartTime = millis();
        isWateringActive = true;
        Serial.print("--- Starting watering from ");
        Serial.print(source);
        Serial.print(" for ");
        Serial.print(durationMs / 1000);
        Serial.println(" seconds. ---");
        wateringStatusMessage = "Started watering (" + source + ").";
        // Immediately send a Telegram message about activation
        if (WiFi.status() == WL_CONNECTED) {
            bot.sendMessage(CHAT_ID, "Watering started (" + source + ") for " + String(durationMs / 1000) + " seconds.", "");
        }
    } else {
        Serial.println("Watering is already active. Skipping new command.");
        if (WiFi.status() == WL_CONNECTED) {
            bot.sendMessage(CHAT_ID, "Watering is already active. Please wait or stop current watering.", "");
        }
    }
}

// --- Function to stop watering ---
void stopWatering(String reason) {
    if (isWateringActive) {
        digitalWrite(RELAY_PIN, LOW);
        isWateringActive = false;
        wateringStartTime = 0;
        Serial.print("--- Stopped watering. Reason: ");
        Serial.println(reason);
        wateringStatusMessage = "Stopped watering. Reason: " + reason;
        if (WiFi.status() == WL_CONNECTED) {
            bot.sendMessage(CHAT_ID, "Watering stopped. Reason: " + reason, "");
        }
    } else {
        Serial.println("Watering is not active. No need to stop.");
        if (WiFi.status() == WL_CONNECTED) {
            bot.sendMessage(CHAT_ID, "Watering is not currently active.", "");
        }
    }
}

// --- Handle incoming Telegram messages ---
void handleNewMessages(int numNewMessages) {
    Serial.print("Handling ");
    Serial.print(numNewMessages);
    Serial.println(" new messages");

    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = bot.messages[i].chat_id;
        String text = bot.messages[i].text;

        // Ensure messages come from the authorized CHAT_ID
        if (chat_id != CHAT_ID) {
            bot.sendMessage(chat_id, "Unauthorized user.", "");
            Serial.println("Unauthorized message from: " + chat_id);
            continue;
        }

        Serial.println("Received message: " + text);

        String keyboardJson = "[[\"Start Manual Watering\"],[\"Stop Watering\"],[\"Get Status\"]]";

        if (text == "/start" || text == "/menu") {
            bot.sendMessageWithReplyKeyboard(chat_id, "Welcome to GardenBot! Choose an option:", keyboardJson, true, true);
        } else if (text == "Start Manual Watering") {
            startWatering(MANUAL_WATERING_DURATION_MS, "Telegram Manual");
        } else if (text == "Stop Watering") {
            stopWatering("Telegram Command");
        } else if (text == "Get Status") {
            // Re-read sensors for current status report
            int soilValue = analogRead(SOIL_PIN);
            int lightValue = analogRead(LIGHT_PIN);
            float temperature = dht.readTemperature();
            float humidity = dht.readHumidity();

            String statusReport;
            if (isnan(temperature) || isnan(humidity)) {
                statusReport = "Error: DHT sensor read failed. Cannot provide full status.";
            } else {
                statusReport = "Current Status:\n"
                               "Temp: " + String(temperature, 1) + "°C\n"
                               "Humidity: " + String(humidity, 1) + "%\n"
                               "Soil Moisture: " + String(soilValue) + "\n"
                               "Light Level: " + String(lightValue) + "\n";
                if (isWateringActive) {
                    unsigned long remainingTime = (wateringStatusMessage.indexOf("Manual") != -1 ? MANUAL_WATERING_DURATION_MS : AUTOMATIC_WATERING_DURATION_MS) - (currentMillis - wateringStartTime);
                    statusReport += "Watering Active! Remaining: " + String(remainingTime / 1000) + " seconds.\n";
                } else {
                    statusReport += "Watering Inactive.\n";
                }
            }
            bot.sendMessage(chat_id, statusReport, "");
        } else {
            bot.sendMessageWithReplyKeyboard(chat_id, "I don't understand that command. Use /menu to see options.", keyboardJson, true, true);
        }
    }
}

void setup() {
    Serial.begin(115200);

    connectToWiFi();

    dht.begin();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Ensure relay is off initially

    pinMode(SOIL_PIN, INPUT);
    pinMode(LIGHT_PIN, INPUT);

    Serial.println("--- Autonomous Garden Irrigation System Started ---");
    bot.sendMessage(CHAT_ID, "Garden irrigation system started. Initializing...", "");
}

void loop() {
    unsigned long currentMillis = millis();

    // --- Watering Duration Management ---
    if (isWateringActive && (currentMillis - wateringStartTime >= (wateringStatusMessage.indexOf("Manual") != -1 ? MANUAL_WATERING_DURATION_MS : AUTOMATIC_WATERING_DURATION_MS))) {
        stopWatering("Duration completed");
        // Update wateringStatusMessage for the hourly report after stop
        wateringStatusMessage = "Automatic watering completed. Temp: " + String(dht.readTemperature(), 1) + "°C, Hum: " + String(dht.readHumidity(), 1) + "%, Soil: " + String(analogRead(SOIL_PIN));
    }

    // --- Automatic Sensor Check Logic (every SENSOR_CHECK_INTERVAL_MS) ---
    if (currentMillis - previousSensorCheckMillis >= SENSOR_CHECK_INTERVAL_MS) {
        previousSensorCheckMillis = currentMillis;

        // Only run automatic watering logic if manual watering is not active
        if (!isWateringActive) {
            int soilValue = analogRead(SOIL_PIN);
            int lightValue = analogRead(LIGHT_PIN);
            float temperature = dht.readTemperature();
            float humidity = dht.readHumidity();

            if (isnan(temperature) || isnan(humidity)) {
                Serial.println("Error: Failed to read from DHT sensor!");
                wateringStatusMessage = "Error: DHT sensor read failed. Cannot determine watering needs.";
            } else {
                Serial.println("\n---- Sensor Readings ----");
                Serial.print("Soil Moisture (Analog): "); Serial.println(soilValue);
                Serial.print("Light Level (Analog): "); Serial.println(lightValue);
                Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
                Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");

                bool isSoilDry = soilValue > SOIL_DRY_THRESHOLD;
                bool isNight = lightValue < LIGHT_THRESHOLD;
                bool isTemperatureHigh = temperature > MAX_TEMPERATURE;

                Serial.print("Conditions: Soil Dry? "); Serial.print(isSoilDry ? "Yes" : "No");
                Serial.print(" | Is Night? "); Serial.print(isNight ? "Yes" : "No");
                Serial.print(" | Temp High? "); Serial.println(isTemperatureHigh ? "Yes" : "No");

                if (isSoilDry && isNight && !isTemperatureHigh) {
                    startWatering(AUTOMATIC_WATERING_DURATION_MS, "Automatic");
                    // wateringStatusMessage updated inside startWatering()
                } else {
                    Serial.println("--- No automatic watering needed. ---");
                    // Update status message only if watering isn't active (or just completed)
                    if (!isWateringActive) {
                        wateringStatusMessage = "No watering needed.\n"
                                                "Temp: " + String(temperature, 1) + "°C, Hum: " + String(humidity, 1) + "%, Soil: " + String(soilValue);
                    }
                }
                Serial.println("---------------------------\n");
            }
        } else {
            Serial.println("Automatic check skipped: Watering is active.");
            wateringStatusMessage = "Watering is currently active. Next automatic check will be skipped.";
        }
    }

    // --- Telegram Report Logic
    if (currentMillis - previousTelegramReportMillis >= TELEGRAM_REPORT_INTERVAL_MS) {
        previousTelegramReportMillis = currentMillis;

        if (WiFi.status() == WL_CONNECTED) {
            // Ensure we have current sensor values for the report
            float temp = dht.readTemperature();
            float hum = dht.readHumidity();
            int soil = analogRead(SOIL_PIN);
            String reportMessage;

            if (isnan(temp) || isnan(hum)) {
                 reportMessage = "Hourly Report (DHT Error):\n" + wateringStatusMessage;
            } else {
                 reportMessage = "Hourly Report:\n" + wateringStatusMessage;
            }
            bot.sendMessage(CHAT_ID, reportMessage, "");
            Serial.println("Telegram report sent.");
        } else {
            Serial.println("Wi-Fi not connected. Cannot send Telegram report. Reconnecting...");
            connectToWiFi();
        }
    }

    // --- Handle incoming Telegram messages ---
    // This is crucial for interactive control!
    int numNewMessages = bot.getUpdates(bot.lastUpdateID + 1);
    while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.lastUpdateID + 1);
    }
}