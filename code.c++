#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <UniversalTelegramBot.h>

//---wifi credentials---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

//---telegram bot credentials---
#define BOT_TOKEN "YOUR_BOT_TOKEN"              // Replace with your bot token from botfather
#define CHAT_ID "YOUR_CHAT_ID"                  // Replace with your chat ID (where you want to receive messages from the bot)

//---DHT sensor settings (pin definition)---
#define DHTPIN 4                                // Pin where the DHT data sensor is connected
#define DHTTYPE DHT22                           // DHT 22 sensor type (can be changed to DHT11 if needed)

#define SOIL_PIN 34                             // Pin for soil moisture analog sensor input pin
#define LIGHT_PIN 35                            // Pin for light sensor (LDR) analog input pin
#define RELAY_PIN 15                            // Pin for water pump control (digital output pin)

//---sensor thresholds and configurations (adjust as needed)---
const int SOIL_MOISTURE_THRESHOLD = 2500;       // higher value means dryer soil (CALIBRATE THIS VALUE)
const float MAX_TEMPERATURE = 30.0;             // Maximum temperature for watering (in Celsius)
const int LIGHT_THRESHOLD = 1000;               // Light threshold for watering (lower value means more light)

//---Watering configuration---
const unsigned long MANUAL_WATERING_DURATION_MS = 30000;    // Duration for manual watering in milliseconds (30 seconds)
const unsigned long AUTOMATIC_WATERING_DURATION_MS = 60000; // Duration for automatic watering in milliseconds (60 seconds)

//---global variables---
DHT dht(DHTPIN, DHTTYPE);                           // Initialize DHT sensor
WiFiClientSecure client;                            // Create a secure WiFi client for Telegram bot communication
UniversalTelegramBot bot(BOT_TOKEN, client);        // Initialize Telegram bot

//variables for non-blocking timing
unsigned long previousSensorCheckMillis = 0;        // Last time sensors were checked
const long SENSOR_CHECK_INTERVAL_MS = 60000;        // Interval for checking sensors (60 seconds)

unsigned long previousTelegramReportMillis = 0;     // Last time Telegram report was sent
const long TELEGRAM_REPORT_INTERVAL_MS = 3600000;   // Interval for sending Telegram report (1 hour)

unsigned long wateringStartTime = 0;                // stores the time when watering started
bool isWateringActive = false;                      // Flag to indicate if watering is active
String wateringStatusMessage = "System initialized."; // message to report via telegram the status of watering

//---function to connect to WiFi---
void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi: "); 
    Serial.println(WIFI_SSID); // Print the Wi-Fi SSID to the serial monitor
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // Start the Wi-Fi connection with the provided SSID and password
    unsigned long startTime = millis();   // Record the start time of the connection attempt
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {   // Wait for connection or timeout after 30 seconds
        delay(500);  // Wait for 500 milliseconds before checking the status again
        Serial.print(".");   // Print a dot to indicate that the connection is still in progress
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi!");   // If connected successfully, print success message
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Set the Telegram certificate for secure connection
    } else {
        Serial.println("\nFailed to connect to Wi-Fi."); // If connection fails, print error message
        Serial.println("Please check your Wi-Fi credentials.");
    }
}

//---function to start watering---
void startWatering(unsigned long durationMs, String source) { 
    if (!isWateringActive) {    // only start if the pump is not already active
        digitalWrite(RELAY_PIN, HIGH);  // Turn on the water pump - Corrected name
        wateringStartTime = millis();   // Record the start time of watering - Corrected name
        isWateringActive = true;    // Set the flag to indicate watering is active
        Serial.print("--- Watering started from: "); 
        Serial.print(source);
        Serial.print(" for duration: ");
        Serial.print(durationMs / 1000); 
        Serial.println(" seconds ---"); 
        wateringStatusMessage = "Watering started from: " + source + " for duration: " + String(durationMs / 1000) + " seconds.";
        // immediately send a telegram message about the watering
        if (WiFi.status() == WL_CONNECTED) {
            bot.sendMessage(CHAT_ID, "Watering started (" + source + ") for " + String(durationMs / 1000) + " seconds.", ""); 
        }
    } else {
        Serial.println("Watering is already active. Skipping new command.");
        if (WiFi.status() == WL_CONNECTED) {
            bot.sendMessage(CHAT_ID, "Watering is already active. Please wait or stop the current watering.", ""); 
    }
}

//---function to stop watering---
void stopWatering(String reason) { 
    if (isWateringActive) {
        digitalWrite(RELAY_PIN, LOW); 
        isWateringActive = false;
        wateringStartTime = 0; // Reset the start time 
        Serial.print("--- Stopped watering. Reason: "); 
        Serial.println(reason);    
        wateringStatusMessage = "Stopped watering. Reason: " + reason; 
        if (WiFi.status() == WL_CONNECTED) { 
            bot.sendMessage(CHAT_ID, "Watering stopped. Reason: " + reason, ""); 
        }
    } else {
        Serial.println("Watering is not active. No need to stop.");
        if (WiFi.status() == WL_CONNECTED) {
            bot.sendMessage(CHAT_ID, "Watering is not currently active.", ""); e
        }
    }
}

//---handle incoming Telegram messages ---
void handleNewMessages(int numNewMessages, unsigned long currentMillis) {
    Serial.print("Handling "); 
    Serial.print(numNewMessages);
    Serial.println(" new messages"); 

    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = bot.messages[i].chat_id; // Get the chat ID of the message 
        String text = bot.messages[i].text; // Get the text of the message 

        // Ensure messages come from the authorized chat ID
        if (chat_id != CHAT_ID) {
            bot.sendMessage(chat_id, "Unauthorized access. Please use the authorized chat ID.", ""); 
            Serial.println("Unauthorized message from: " + chat_id); 
            continue; 
        }

        Serial.println("Received message: " + text); 
        String keyboardJson = "[[\"Start Manual Watering\"],[\"Stop Watering\"],[\"Get Status\"]]"; 

        if (text == "/start" || text == "/menu") {
            bot.sendMessageWithReplyKeyboard(chat_id, "Welcome to GardenBot!", keyboardJson, true, true); 
        } else if (text == "Start Manual Watering") { 
            startWatering(MANUAL_WATERING_DURATION_MS, "Telegram Manual Command"); 
        } else if (text == "Stop Watering") {
            stopWatering("Telegram Manual Command");
        } else if (text == "Get Status") {
            // Re-read sensors for current status report
            int soilValue = analogRead(SOIL_PIN);
            int lightValue = analogRead(LIGHT_PIN); 
            float temperature = dht.readTemperature();
            float humidity = dht.readHumidity();

            String statusReport; 
            if (isnan(temperature) || isnan(humidity)) {
                statusReport = "Error: DHT sensor read failed. Cannot get temperature or humidity.";
            } else {
                statusReport = "Current Status:\n";
                statusReport += "Soil Moisture: " + String(soilValue) + "\n";
                statusReport += "Light Level: " + String(lightValue) + "\n";
                statusReport += "Temperature: " + String(temperature, 1) + " °C\n"; 
                statusReport += "Humidity: " + String(humidity, 1) + " %\n"; 
                if (isWateringActive) { 
                    // using currentMillis to calculate remaining watering time
                    unsigned long durationToUse = (wateringStatusMessage.indexOf("Manual") != -1 ? MANUAL_WATERING_DURATION_MS : AUTOMATIC_WATERING_DURATION_MS); // Corrected name and removed trailing parenthesis
                    unsigned long elapsedTime = currentMillis - wateringStartTime;
                    unsigned long remainingTime = 0;
                    if (elapsedTime < durationToUse) {      // prevent negative remaining time
                        remainingTime = durationToUse - elapsedTime;
                    }
                    statusReport += "Watering Active! Remaining: " + String(remainingTime / 1000) + " seconds.\n";
                } else {
                    statusReport += "Watering is not active.\n";
                }
            }
            bot.sendMessage(chat_id, statusReport, ""); // 
        } else {
            bot.sendMessageWithReplyKeyboard(chat_id, "I don't understand that command. Please use the menu options.", keyboardJson, true, true); // Added parse_mode
        }
    }
}

//---setup function---
void setup() {
    Serial.begin(115200); // Initialize serial communication
    connectToWiFi(); // Connect to Wi-Fi
    dht.begin(); // Initialize DHT sensor
    pinMode(RELAY_PIN, OUTPUT); // Set the relay pin for the water pump
    digitalWrite(RELAY_PIN, LOW); // Ensure the water pump is off initially
    pinMode(SOIL_PIN, INPUT); // Set soil moisture sensor pin as input
    pinMode(LIGHT_PIN, INPUT); // Set light sensor pin as input
    Serial.println("--- Autonomous Garden Irrigation System is ready!"); // Print initialization message
    bot.sendMessage(CHAT_ID, "Garden irrigation system started. Initializing sensors and Wi-Fi connection...", ""); // 
}

//---loop function---
void loop() {
    unsigned long currentMillis = millis(); // Get the current time in milliseconds

    //---watering duration Management---
    if (isWateringActive && (currentMillis - wateringStartTime >= (wateringStatusMessage.indexOf("Manual") != -1 ? MANUAL_WATERING_DURATION_MS : AUTOMATIC_WATERING_DURATION_MS))) {
        stopWatering("Duration completed"); 
        // Update wateringStatusMessage for the hourly report after stop
        // Note: For a precise hourly report, consider re-reading sensors just before sending it.
        wateringStatusMessage = "Automatic watering completed. Temp: " + String(dht.readTemperature(), 1) + "°C, Hum: " + String(dht.readHumidity(), 1) + "%, Soil: " + String(analogRead(SOIL_PIN));
    }

    //---check sensors logic periodically---
    if (currentMillis - previousSensorCheckMillis >= SENSOR_CHECK_INTERVAL_MS) {
        previousSensorCheckMillis = currentMillis; // Update the last sensor check time

        // Only run automatic watering logic if manual watering is not active
        if (!isWateringActive) { 
            int soilValue = analogRead(SOIL_PIN);
            int lightValue = analogRead(LIGHT_PIN);
            float temperature = dht.readTemperature();
            float humidity = dht.readHumidity(); 

            if (isnan(temperature) || isnan(humidity)) {
                Serial.println("Error reading DHT sensor. Skipping automatic watering logic."); 
                wateringStatusMessage = "Error reading DHT sensor. Cannot get temperature or humidity.";
            } else {
                Serial.println("\n--- Sensor Readings ---"); 
                Serial.print("Soil Moisture (Analog): "); Serial.println(soilValue); 
                Serial.print("Light Level (Analog): "); Serial.println(lightValue);
                Serial.print("Temperature (Celsius): "); Serial.println(temperature, 1); 
                Serial.print("Humidity (%): "); Serial.println(humidity, 1); 

                bool isSoilDry = soilValue > SOIL_MOISTURE_THRESHOLD; // Check if soil is dry
                bool isNight = lightValue < LIGHT_THRESHOLD; // Check if it's dark (night time)
                bool isTemperatureHigh = temperature > MAX_TEMPERATURE; // Check if temperature is high

                Serial.print("Conditions: Soil Dry? "); Serial.print(isSoilDry ? "Yes" : "No"); 
                Serial.print(" | Is Night? "); Serial.print(isNight ? "Yes" : "No");
                Serial.print(" | Is Temperature High? "); Serial.println(isTemperatureHigh ? "Yes" : "No");

                if (isSoilDry && isNight && !isTemperatureHigh) { 
                    startWatering(AUTOMATIC_WATERING_DURATION_MS, "Automatic Sensor Check");
                } else {
                    Serial.println("--- Conditions not met for automatic watering ---"); 
                    // update status message only if watering is not active
                    if (!isWateringActive) {
                        wateringStatusMessage = "No watering needed.\n"
                                                "Temp: " + String(temperature, 1) + "°C, Hum: " + String(humidity, 1) + "%, Soil: " + String(soilValue);
                    }
                }
                Serial.println("--- Automatic watering logic completed ---"); 
            }
        } else {
            Serial.println("Automatic check skipped: Watering is active."); 
            wateringStatusMessage = "Watering is currently active. Next automatic check will be skipped.";
        }
    }

    //---send hourly report to Telegram---
    if (currentMillis - previousTelegramReportMillis >= TELEGRAM_REPORT_INTERVAL_MS) {
        previousTelegramReportMillis = currentMillis; // update the last report time

        if (WiFi.status() == WL_CONNECTED) {
            //Ensure we have current sensor value for the report
            float temp = dht.readTemperature();
            float hum = dht.readHumidity();
            int soil = analogRead(SOIL_PIN);
            String reportMessage; 

            if (isnan(temp) || isnan(hum)) {
                reportMessage = "Hourly Report (DHT Error):\n" + wateringStatusMessage; 
            } else {
                // hourly report with current sensor values
                reportMessage = "Hourly Report:\n"
                                "Temp: " + String(temp, 1) + "°C, Hum: " + String(hum, 1) + "%, Soil: " + String(soil) + "\n"
                                + wateringStatusMessage;
            }
            bot.sendMessage(CHAT_ID, reportMessage, ""); 
            Serial.println("Telegram report sent."); 
        } else {
            Serial.println("Wi-Fi not connected. Cannot send Telegram report. Trying to reconnect.");
            connectToWiFi();
        }
    }

    // --- Handle incoming Telegram messages ---
    int numNewMessages = bot.getUpdates(bot.lastUpdateID + 1);
    while (numNewMessages) {
        handleNewMessages(numNewMessages, currentMillis); 
        numNewMessages = bot.getUpdates(bot.lastUpdateID + 1);
    }
}