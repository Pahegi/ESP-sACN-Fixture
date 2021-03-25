#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncE131.h>
#include <ESPAsync_WiFiManager.h> //https://github.com/khoih-prog/ESPAsync_WiFiManager
#include "SevenSeg.hpp"

#define HTTP_PORT 80
AsyncWebServer webServer(HTTP_PORT);
DNSServer dnsServer;
ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer, "ESP32-DMX sAcn Manager");
String ssid = "ESP_" + String(ESP_getChipId(), HEX);
const char *password = "123456789.";

#define UNIVERSE 1       // First DMX Universe to listen for
#define UNIVERSE_COUNT 1 // Total number of Universes to listen for, starting at UNIVERSE

const uint8_t ledPin = GPIO_NUM_12;
const int freq = 5000;
const uint8_t ledChannel = 0;
const uint8_t resolution = 8;

typedef struct
{
    uint16_t start_channel = 0;
    uint16_t channel_range = 1;
} DMX_Config;

ESPAsyncE131 e131(UNIVERSE_COUNT);

//custom seven segment library
void setup()
{
    /********************************
     * 
     *          Pin Setup
     * 
     ********************************/
    Serial.begin(115200);
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(ledPin, ledChannel);
    delay(10);

    /********************************
     * 
     *       Wifi Manager Setup
     * 
     ********************************/

    ESPAsync_wifiManager.setConfigPortalChannel(0);
    ESPAsync_wifiManager.startConfigPortal("ESP32 DMX sAcn", password);

    /********************************
     * 
     *    Config for sAcn Library
     * 
     ********************************/

    // Choose one to begin listening for E1.31 data
    //if (e131.begin(E131_UNICAST))                               // Listen via Unicast
    if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT)) // Listen via Multicast
        Serial.println(F("Listening for data..."));
    else
        Serial.println(F("*** e131.begin failed ***"));
}

// struct freqUpdate{
//     long start;
//     long frequency;
//     void callback();
//     void tick(){
//         if (millis()-start>freq){

//         }
//     }
// };

uint8_t packet_value;
void loop()
{
    long start = micros();
    long start2 = micros();
    while (!e131.isEmpty())
    {
        e131_packet_t packet;
        e131.pull(&packet); // Pull packet from ring buffer

        // Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
        //         htons(packet.universe),                 // The Universe for this packet
        //         htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
        //         e131.stats.num_packets,                 // Packet counter
        //         e131.stats.packet_errors,               // Packet error counter
        //         packet.property_values[1]);             // Dimmer data for Channel 1
        //Serial.println(packet.property_values[1]);
        packet_value = packet.property_values[1];
    }
    long diff = micros() - start;
    Serial.println(diff);
    delayMicroseconds(1600 - diff);
    ledcWrite(ledChannel, packet_value);
    Serial.println(micros() - start2);
}
