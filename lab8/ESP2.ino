#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "DNot gonna Tell you";
const char* password = "Still not gonna tell you";
unsigned int localUdpPort = 4220;  // local port to listen on
char incomingPacket[255];
WiFiUDP Udp;
IPAddress thisESP;
IPAddress ip(10,0,1,14);
IPAddress ip3(10,0,1,3);

byte mac[6];
char MAC_char[18];
void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(ssid);
//  WiFi.config(ipcurrent, gateway, subnet);
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);


}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize){
  int len = Udp.read(incomingPacket, 255);
  if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.print(incomingPacket);
  }
  if(Serial.available()){
    int playerPosition = Serial.read();
    Udp.beginPacket(ip3, 4210);
    Udp.write(playerPosition);
    Udp.endPacket();

  }
  }
