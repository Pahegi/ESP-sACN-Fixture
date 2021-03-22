#include <WiFiManager.h>
#include <ArduinoOTA.h>

WiFiServer server(80);
String header;

void setup() {
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
    server.begin();
}

void loop() {
  ArduinoOTA.handle();
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {            // wiederholen so lange der Client verbunden ist
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();
      client.println("ESP8266 Web Server OTA");
      client.println();
      break;
    }
    client.stop();
  }
}