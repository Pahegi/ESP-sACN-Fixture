#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ESPAsyncE131.h>

#define UNIVERSE 1                      // First DMX Universe to listen for
#define UNIVERSE_COUNT 1                // Total number of Universes to listen for, starting at UNIVERSE
const int ledPin1 = 15;
const int ledPin2 = 12;
const int ledPin3 = 4;
const int ledPin4 = 14;

// WiFiServer server(80);
String header;
ESPAsyncE131 e131(UNIVERSE_COUNT);

void setup() {
    pinMode (ledPin1, OUTPUT);
    pinMode (ledPin2, OUTPUT);
    pinMode (ledPin3, OUTPUT);
    pinMode (ledPin4, OUTPUT);

    WiFi.mode(WIFI_STA);
    Serial.begin(115200);
    WiFiManager wm;
    bool res;
    res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    else {
        Serial.println("connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    ArduinoOTA.begin();
    // server.begin();

    if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT))   // Listen via Multicast
        Serial.println(F("Listening for data..."));
    else 
        Serial.println(F("*** e131.begin failed ***"));
}

void loop() {
  ArduinoOTA.handle();
  // WiFiClient client = server.available();
  // if (client) {
  //   while (client.connected()) {            // wiederholen so lange der Client verbunden ist
  //     client.println("HTTP/1.1 200 OK");
  //     client.println("Content-type:text/html");
  //     client.println("Connection: close");
  //     client.println();
  //     client.println("ESP8266 Web Server OTA");
  //     client.println();
  //     break;
  //   }
  //   client.stop();
  // }
   if (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);     // Pull packet from ring buffer
        
        // Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
        //         htons(packet.universe),                 // The Universe for this packet
        //         htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
        //         e131.stats.num_packets,                 // Packet counter
        //         e131.stats.packet_errors,               // Packet error counter
        //         packet.property_values[1]);             // Dimmer data for Channel 1

        analogWrite(ledPin1, packet.property_values[1]);
        analogWrite(ledPin2, packet.property_values[2]);
        analogWrite(ledPin3, packet.property_values[3]);
        analogWrite(ledPin4, packet.property_values[4]);

    }
}