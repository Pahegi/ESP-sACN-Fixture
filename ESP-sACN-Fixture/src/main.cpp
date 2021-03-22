#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncE131.h>


#define UNIVERSE 1                      // First DMX Universe to listen for
#define UNIVERSE_COUNT 1                // Total number of Universes to listen for, starting at UNIVERSE

const int ledPin = GPIO_NUM_12;
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

const char ssid[] = "1234";         // Replace with your SSID
const char passphrase[] = "1234";   // Replace with your WPA2 passphrase

ESPAsyncE131 e131(UNIVERSE_COUNT);

void setup() {
    Serial.begin(115200);
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(ledPin, ledChannel);
    delay(10);

    // Make sure you're in station mode    
    WiFi.mode(WIFI_STA);

    Serial.println("");
    Serial.print(F("Connecting to "));
    Serial.print(ssid);

    if (passphrase != NULL)
        WiFi.begin(ssid, passphrase);
    else
        WiFi.begin(ssid);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print(F("Connected with IP: "));
    Serial.println(WiFi.localIP());
    
    // Choose one to begin listening for E1.31 data
    //if (e131.begin(E131_UNICAST))                               // Listen via Unicast
    if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT))   // Listen via Multicast
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
void loop() {
    long start = micros();
    long start2 = micros();
    while (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);     // Pull packet from ring buffer
        
        // Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
        //         htons(packet.universe),                 // The Universe for this packet
        //         htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
        //         e131.stats.num_packets,                 // Packet counter
        //         e131.stats.packet_errors,               // Packet error counter
        //         packet.property_values[1]);             // Dimmer data for Channel 1
        //Serial.println(packet.property_values[1]);
        packet_value = packet.property_values[1];
    }
    long diff = micros()-start;
    Serial.println(diff);
    delayMicroseconds(1600-diff);
    ledcWrite(ledChannel, packet_value);
    Serial.println(micros()-start2);
}
