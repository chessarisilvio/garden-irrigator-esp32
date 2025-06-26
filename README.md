# Garden irrigator esp32:
This project empowers you to care for your beloved plants with the magic of electronics and code, using an ESP32, it sends you real-time updates via a Telegram bot about your garden's status, including relevant temperatures, humidity, and more. Who said technology kills plants? Quite the opposite – it helps them thrive!

## What you need:
- esp 32 board: An ESP32 is the microcontroller we will need (i am using a esp32-32u) for better wifi range and stabilty (using the 2.4GHz 802.11b/g/n protocol), 
  i higly reccomend an ESP32 model with an external antenna, such as the a ESP32-WROOM-32U or similar, especially if your garden is far from your router or the hotspot/wifi acces point (you could also give the wifi from your phone). Higher flash and RAM memory are always a plus for future expansions. i also reccomend a 38pin esp 32 or you could even buy an expansion board.
- DFRobot Gravity v2 (i2c or analog): An analog soil moisture sensor to detect the dryness of your soil.
- DHT11 or DHT22: Temperature & humidity sensor to give you those values, but a dht22 is way better.
- LDR or BH1750: a light sensor LDR (light dependent resistor) or a more precise BH1750 sensor to determine light levels, if its day or night.
- MOSFET DRIVER or 5V RELAY MODULE: to safely switch the higher current/voltage required by the pump using the ESP32's lower voltage signals.
- Mini water pump or DC motor: the actual device that moves water to your platns. Ensure it's suitable for the volume of water you need.
- 3.7v Lithium Battery or powerban with USB-A cable: for portable power. If using a raw battery, ensure you have proper charging and protection circuitry.
- solar pannel (recommended): To make the system truly autonomus and self-sustaining, especially if battery-powered, i recommend using one that gives you minus - 
  and positive + so you can connect them to an eventual converter DC step-up from 3.7v/3.3v to 5v.
- Breadboard and jumper wires: For prototyping and making connections.

## The connections:
These are the core connections for your ESP32-based irrigation system:
- esp32 (gio 34) > Soil moisture sensor: connect the analog output of your soil moisture sensor to gpio 34 on the esp32.
- esp32 (gpio 15) > Relay module (control pin): coneect gpio 15 to the control input (example: IN1) of your 5v realy module. Ensure the realy's VCC connected to a 
  5V source (if required by your realy module) and GND to ESP32'S GND.
- esp32 (gpio 4) > DHT sensor (data pin): connect the data pin of your DHT11 / DHT22 sensor to gpio 4 on the ESP32. Rembember to power the FHT sensor with 3.3 or  
  5v as per its specifications (check the snesor datasheet). A 10k Ihm pull-up resistor between the data pin and VCC might be needed for some DHT setups.
- esp32 (gpio 35) > Light sensor (LDR): connect the analog output of your LDR (typically part of a voltage divider circuit) to gpio 35 on the esp32.
- mini water pump > relay module: connect your mini water pump ( or DC motor) to the appropriate screw terminals on the relay module (usually NO - normally Open and COM - COMMON) to allow the esp32 switch its power. Ensure the pump's power source (battery/power bank) is also correctly wired trought the relay.

## The code and setup: 
We will be coding in C++ using the arduino IDE

1. Arduino IDE setup:
   - dowload arduino IDE setup from the official website: https://www.arduino.cc/en/software/

2. Add ESP32 Board support: 
   - Open arduino IDE.
   - Go to file > Preferences (or Arduino > settings on MacOS).
   - In the "Additional boards manager URLs" field, paste this URL: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   - Click "OK".
   - Go to Tools > board > board manager...
   - search for "esp32" and install the "esp32 by Espressif System" package (install the latest version).

3. Install Usb Drivers:
   Your computer needs drivers to communicate with the ESP32's USB-to-serial chip. Its best to install them all to be safe, as different 
   esp32 boards use different chips:
   - CP210X DRIVER: https://www.silabs.com/documents/public/software/CP210x_Universal_Windows_Driver.zip
   - CH340 DRIVER: https://sparks.gogo.co.nz/assets/site/downloads/CH34x_Install_Windows_v3_4.zip
   - FTDI DRIVER: https://ftdichip.com/drivers/vcp-drivers/ (Find the correct installer for your OS).

4. Select the correct COM port:
   -plug your ESP32 directly into a USB on your computer (avoid USB hubs, which can sometimes cause confusion to your pc).
   -in the arduino IDE, go to Tools > Port and select the COM port that corresponds to your ESP32. it often apperars as "COM X (ESP32 dev module) or similar, to identify iy better I also suggest to disconnect any unwanted usb device.

5. Required Libraries for the code:
   You will need two additional libraries for telegram communication. Install them via the Arduino IDE Library Manager:
    1. UniversalTelegramBot:
       - Go to Sketch > Include Library > Manage Libraries...
       - Search for "UniversalTelegram" and install the latest version.
    2. ArduinoJson. (This is a dependency for UniversalTelegramBot)
       - In the same "Library Manager", search for "ArduinoJson" and install the latest stable version.

6. Telegram bot setup:

   To enable your ESP32 to communicate with telegram, you will need to setup your bot:
    1. Open telegram and search for @botfather.
    2. Send the command /newbot.
    3. Follow the promts to choose a name (example "gardenbot") and a username (example "mygardenirrigation_bot)
    4. bothfather will provide you with an unique BOT_TOKEN. this is a long string of characters (example 123456789:ABCDEF1234567890abcdef1234567890). Copy this token carefully.

   FIND YOUR CHAT_ID:
    1. search for your newly created bot by its username (example @mygradenirrigation_bot) in telegram and start a chat by sending it any message (example "hi")
    2. Open your web browser and go to this URL, replacing YOUR_BOT_TOKEN with the actual token you have receveied from botfather: https://api.telegram.org/botYOUR_BOT_TOKEN/getUpdates
    3. In the JSON output, look for "chat":["id":...]. The number nect to "id": is your CHAT_ID. This can be positive number on negative number, copy that number.

7. Prepare the code: 
   Before uploading, you must configure the code with your specific wi-fi and telegram details.
   1. open the provided C++ code in arduino IDE.
   2. Locate these lines and replace the placeholder values with your actual credentials:

   const char* WIFI_SSID = "YOUR_WIFI_SSID";            // Your Wi-Fi network name
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";    // Your Wi-Fi password
   #define BOT_TOKEN "YOUR_BOT_TOKEN";                  // The token from BotFather
   #define CHAT_ID "YOUR_CHAT_ID";                      // The chat ID you found

8. Upload the code:
   1. In the arduino IDE, with your ESP32 board and COM port selected, click the "upload" button (right arrow icon).
   2. The arduino IDE will compile the code
   3. When you see "connecting..." in the console, press and hold "BOOT" button on the esp32 board.
   4. Keep it pressed until you see the upload percentage start (example "writing at 10%)
   5. Once the upload starts, you can release the "BOOT" button.
   6. If it fails to connect, try disconnectiong the ESP32, holding down the BOOT button, plugging it back into the PC, and  
      then immediately starting the upload while still holding BOOT, release once the upload begins.
   7. Once uploaded, the ESP32 will reset and start running the program. Open the Serial Monitor (Tools > Serial Monitor) in the arduino IDE to see debug messages.

## How your garden irrigator works:
   Your ESP32 is now a smart, connected graden assistant with a noth automatic and manual control!
    1. Automatic monitoring (every minute)
       -it continusly reads data from your soil moisture, light, and DHT (temperature/humidity) sensors.
       -it autoatically activates the pump for 1 minute if the soil is drym it's nighttime and the temperature is not excessively high (to prevent evaporation).
       -it prints all the sensor readings and its decisions to the serial monitor for debugging.
    2. Hourly telegram reports:
       -every hour it sends a detailed status update to your telegram bot. This message includes:
         1. wheter it irrigated in the past hour (why / why not)
         2. current temperature, humidity, soil moisture and light levels.
         3. It also sends instant notifications for system startup, sensor errors, or when manual watering is initiated or   
            stopped.
    3. Manual control via Telegram:
        -send " /start " or " /menu " to your bot in telegram. It will reply with an interactive keyboard at the bottom of   
         your chat.
        -"start manual watering": Tap this to activate the pump for a 30-second burst, regardless of sensor readings.
        -"stop watering": Tap this for an instant, real time report of all sensor and the current watering status.
    4. Intelligent Pump Management:
        -The pump will run only for the specified duration (1 minute for auto, 30 seconds for manual).
        -it wont start a new watering cycle if the pump is alredy active, preventing accindetal overwatering or conflicts.
        -If the wi-fi connection is lost, it will attemp to reconnect to ensure telegram communication can resume.

        
This project empowers you with remote control and automated care for your plants, ensuring they get the water they need when they need it. Happy gardening!