#include <LittleFS.h>
#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif

#include <ArduinoOTA.h>
#include <ESPAsyncE131.h>
#include <ESPAsyncWiFiManager.h> //https://github.com/khoih-prog/ESPAsync_WiFiManager
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

bool loadConfig();
bool saveConfig();

/********************************
*
*       Wifi Manager Setup
*
********************************/
bool shouldSaveConfig = false;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

#define HTTP_PORT 80
AsyncWebServer webServer(HTTP_PORT);
DNSServer dnsServer;


/********************************
*
*           PIN Setup
*
********************************/
const uint8_t ledPinR = 15;
const uint8_t ledPinG = 13;
const uint8_t ledPinB = 12;
const uint8_t ledPinW1 = 14;
const uint8_t ledPinW2 = 4;

/********************************
*
*          DMX Setup
*
********************************/
#define UNIVERSE_COUNT 1 // Total number of Universes to listen for, starting at UNIVERSE
#define CHANNEL_COUNT 5 // Total number of Channels to listen for, starting at START_CHANNEL
uint8_t start_channel = 1;
uint8_t start_universe = 1;

ESPAsyncE131 e131(UNIVERSE_COUNT);

/********************************
*
*          Load Config
*
********************************/
bool loadConfig() {
    Serial.println("Loading config...");
    if (!LittleFS.exists("/config.json")) {
        Serial.println("Config file does not exist. Creating new config file...");
        saveConfig();
        return true;
    }
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return false;
    }
    size_t size = configFile.size();
    if (size > 1024) {
        Serial.println("Config file size is too large");
        return false;
    }

    std::unique_ptr<char[]> buf(new char[size]);

    configFile.readBytes(buf.get(), size);
    StaticJsonDocument<200> doc;
    auto error = deserializeJson(doc, buf.get());
    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }
    start_channel = doc["start_channel"];
    start_universe = doc["start_universe"];

    Serial.print("Loaded start_channel: ");
    Serial.println(start_channel);
    Serial.print("Loaded start_universe: ");
    Serial.println(start_universe);
    return true;
}

/********************************
*
*          Save Config
*
********************************/
bool saveConfig() {
    Serial.println("Saving config...");
    StaticJsonDocument<200> doc;
    doc["start_channel"] = start_channel;
    doc["start_universe"] = start_universe;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    serializeJson(doc, configFile);
    Serial.println("Successfully saved configfile");
    return true;
}

/********************************
*
*         Debug Methods
*
********************************/
void readFile(const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println();
  file.close();
}



/********************************
*
*          MAIN SETUP
*
********************************/
void setup() {
    /********************************
     *
     *          Pin Setup
     *
     ********************************/
    Serial.begin(115200);
    Serial.println();
    pinMode(ledPinR, OUTPUT);
    pinMode(ledPinG, OUTPUT);
    pinMode(ledPinB, OUTPUT);
    pinMode(ledPinW1, OUTPUT);
    pinMode(ledPinW2, OUTPUT);
    delay(1000);

    /********************************
     *
     *       Debug Methods
     *
     ********************************/
    // Serial.println("Deleting WiFi Credentials...");
    // WiFi.disconnect();
    // delay(1000);

    /********************************
     *
     *       Handle Config File
     *
     ********************************/
    Serial.println("Mounting FS...");

    if (!LittleFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    }
    loadConfig();
    // readFile("/config.json");

    /********************************
     *
     *       Wifi Manager Setup
     *
     ********************************/
    Serial.println("Starting WiFi Manager...");
    AsyncWiFiManagerParameter custom_start_adress("DMX Startadresse", "DMX Startadresse", "", 3); //Crashing right here TODO
    AsyncWiFiManagerParameter custom_start_universe("Universum", "Universum", "", 3);
    AsyncWiFiManager wifiManager(&webServer,&dnsServer);
    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    //add all parameter
    wifiManager.addParameter(&custom_start_adress);
    wifiManager.addParameter(&custom_start_universe);
    wifiManager.autoConnect("DMX Fixture");
    Serial.println("Connected to Wifi");
    //store updated values if necessary
    if (shouldSaveConfig) {
        start_channel = String(custom_start_adress.getValue()).toInt();
        start_universe = String(custom_start_universe.getValue()).toInt();
        Serial.printf("Updated values start_channel %u and start_universe %u.\n", start_channel, start_universe);
        saveConfig();
    }

    /********************************
     *
     *          OTA Setup
     *
     ********************************/
    Serial.println("Starting OTA Setup...");
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();

    /********************************
     *
     *    Config for sACN Library
     *
     ********************************/
    // start_channel = 1;
    // start_universe = 1;
    // Choose one to begin listening for E1.31 data
    // if (e131.begin(E131_UNICAST))                               // Listen via Unicast
    if (e131.begin(E131_MULTICAST, start_universe, UNIVERSE_COUNT)) // Listen via Multicast
        Serial.printf("Listening for E1.31 data on Universe %u starting at Channel %u...\n", start_universe, start_channel);
    else
        Serial.println(F("*** e131.begin failed ***"));

    Serial.println("Setup complete. Starting Loop...");
}

/********************************
*
*         LOOP SECTION
*
********************************/
void loop() {
    //Timing options
    long start = micros();

    //OTA Handling
    ArduinoOTA.handle();

    //Incoming Packet Handling
    while (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet); // Pull packet from ring buffer
        analogWrite(ledPinR, packet.property_values[start_channel + 0]*4);
        analogWrite(ledPinG, packet.property_values[start_channel + 1]*4);
        analogWrite(ledPinB, packet.property_values[start_channel + 2]*4);
        analogWrite(ledPinW1, packet.property_values[start_channel + 3]*4);
        analogWrite(ledPinW2, packet.property_values[start_channel + 4]*4);
        Serial.printf("R-%u G-%u B-%u W1-%u W2-%u \n",
            packet.property_values[start_channel + 0]*4,
            packet.property_values[start_channel + 1]*4,
            packet.property_values[start_channel + 2]*4,
            packet.property_values[start_channel + 3]*4,
            packet.property_values[start_channel + 4]*4
        );
    }

    //more timing
    long diff = micros() - start;
    delayMicroseconds(1600 - diff);
}
