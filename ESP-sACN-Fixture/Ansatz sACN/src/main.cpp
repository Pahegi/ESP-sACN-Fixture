#include <LittleFS.h>
#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif

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
AsyncWiFiManager wifiManager(&webServer,&dnsServer);


/********************************
*
*           PIN Setup
*
********************************/
const uint8_t ledPinR = 15;
const uint8_t ledPinG = 13;
const uint8_t ledPinB = 12;
const uint8_t ledPinW1 = 14;
const uint8_t ledPinW2 = 16;

/********************************
*
*          DMX Setup
*
********************************/
#define UNIVERSE_COUNT 1 // Total number of Universes to listen for, starting at UNIVERSE
#define CHANNEL_COUNT 5 // Total number of Channels to listen for, starting at START_CHANNEL
uint8_t start_channel = 1;
uint8_t start_universe = 1;
String fixture_name = "sACN Fixture";

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
    fixture_name = (const char*)doc["fixture_name"];

    Serial.print("Loaded start_channel: ");
    Serial.println(start_channel);
    Serial.print("Loaded start_universe: ");
    Serial.println(start_universe);
    Serial.print("Loaded Fixture Name: ");
    Serial.println(fixture_name);
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
    doc["fixture_name"] = fixture_name;

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
*       Webserver Contents
*
********************************/
String getHTML(){
    return
    "<!DOCTYPE html>\
    <html>\
    <style>\
        html {font-family: Arial; display: inline-block; text-align: center}\
    </style>\
    <title>sACN Fixture</title>\
    <body>\
        <h1>" + fixture_name + "</h1>\
        <p>Adresse: " + String(start_channel) + "</p>\
        <p>Universum: " + String(start_universe) + "</p>\
        <form action=\"/\">\
        Name: <input type=\"text\" name=\"Name\" value=\"" + fixture_name + "\"><br>\
        Adresse: <input type=\"text\" name=\"Adresse\" value=\"" + start_channel + "\"><br>\
        Universum: <input type=\"text\" name=\"Universum\" value=\"" + start_universe + "\"><br>\
        <input type=\"submit\" value=\"Speichern\">\
        </form><br>\
        <form action=\"/disconnect\">\
        <input type=\"submit\" value=\"Verbindung trennen\">\
        </form><br>\
    </body>\
    </html>";
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
    AsyncWiFiManagerParameter custom_start_adress("DMX Startadresse", "DMX Startadresse", "", 3);
    AsyncWiFiManagerParameter custom_start_universe("Universum", "Universum", "", 3);
    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    //add all parameter
    wifiManager.addParameter(&custom_start_adress);
    wifiManager.addParameter(&custom_start_universe);
    wifiManager.setAPStaticIPConfig(IPAddress(192,168,0,1), IPAddress(192,168,0,1), IPAddress(255,255,255,0));
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
     *       Webserver Listener
     *
     ********************************/
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String inputMessage1;
        String inputMessage2;
        String inputMessage3;
        const char* PARAM_INPUT_1 = "Adresse";
        const char* PARAM_INPUT_2 = "Universum";   
        const char* PARAM_INPUT_3 = "Name";   
        // GET input1 value on <ESP_IP>/get?Adresse=<inputMessage1>&Universum=<inputMessage2>
        if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2) && request->hasParam(PARAM_INPUT_3)) {
            inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
            inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
            inputMessage3 = request->getParam(PARAM_INPUT_3)->value();
            start_channel = inputMessage1.toInt();
            start_universe = inputMessage2.toInt();
            fixture_name = inputMessage3;
            saveConfig();
        }
        request->send(200, "text/html", getHTML());
    });
    webServer.on("/disconnect", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", "Verbindung wird getrennt...<br>Bitte über Accesspoint verbinden.");
        delay(1000);
        WiFi.disconnect();
        delay(1000);
        wifiManager.autoConnect("DMX Fixture");
        Serial.println("Connected to Wifi");
    });
    webServer.begin();

    /********************************
     *
     *    Config for sACN Library
     *
     ********************************/
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

}