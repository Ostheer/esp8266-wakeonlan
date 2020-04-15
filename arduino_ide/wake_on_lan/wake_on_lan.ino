#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino.h>
#include <SPI.h>
#include "settings.h"

MDNSResponder mdns;
WiFiUDP udp;
ESP8266WebServer server(1337);
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
String pwd = SERVICE_PASSWORD;

void sendWOL(const IPAddress ip, const byte mac[]);
void beginWifi();
void macStringToBytes(const String mac, byte *bytes);

//for posting the public address
HTTPClient http;
String pubipsrv = PUBLICIPSERVER;
String pubipdest = PUBLICIPDESTINATION;
int i;

void setup(void){
  i = 0;
  
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // turn on led
  beginWifi();
  while (!mdns.begin("esp8266", WiFi.localIP())) {}
  udp.begin(9);

  server.on("/wol", [](){
    digitalWrite(LED_BUILTIN, HIGH);
    // example: GET /wol?mac=112233aabbcc&bcast=255&pwd=xxx
    if(server.arg("mac").length() <= 12 && server.arg("pwd").length() <= 200 && server.arg("bcast").length() <= 3) {
      String mac = server.arg("mac");
      String p = server.arg("pwd");
      int bcast = server.arg("bcast").toInt();
  
      if(p == pwd) {
        IPAddress target_ip;
        target_ip = WiFi.localIP();
        target_ip[3] = bcast;
        Serial.println("Sending WOL");
        Serial.println(target_ip);
  
        byte target_mac[6];
        macStringToBytes(mac, target_mac);
    
        sendWOL(target_ip, target_mac);
        server.send(200, "text/plain", "WOL sent to " + target_ip.toString() + " " + mac);
      }
      else {
        server.send(403, "text/plain", "WOL request received...");
      }
    }
    else {
      server.send(403, "text/plain", "Invalid data");
    }
    delay(1000); 
    digitalWrite(LED_BUILTIN, LOW);
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  if (WiFi.status() != WL_CONNECTED)
  {
     ESP.reset(); 
  }
  server.handleClient();
  
  delay(50);

  //every minute, post the public address
  if (i > 1200){
    postPublicAddress();
    i = 0;
  }
  
  i++;
}


void beginWifi() {
  WiFi.disconnect();
  WiFi.hostname("WOL Forwarder");
  WiFi.config(staticIP, gateway, subnet, dns);

  WiFi.begin(ssid, password);
  Serial.println("");
  WiFi.mode(WIFI_STA);
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/*
* Send a Wake-On-LAN packet for the given MAC address, to the given IP
* address. Often the IP address will be the local broadcast.
*/
void sendWOL(const IPAddress ip, const byte mac[]) {
  digitalWrite(LED_BUILTIN, HIGH);
  byte preamble[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  udp.beginPacket(ip, 9);
  udp.write(preamble, 6);
  for (uint8 i = 0; i < 16; i++) {
    udp.write(mac, 6);
  }
  udp.endPacket();
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}


byte valFromChar(char c) {
  if(c >= 'a' && c <= 'f') return ((byte) (c - 'a') + 10) & 0x0F;
  if(c >= 'A' && c <= 'F') return ((byte) (c - 'A') + 10) & 0x0F;
  if(c >= '0' && c <= '9') return ((byte) (c - '0')) & 0x0F;
  return 0;
}

/*
* Very simple converter from a String representation of a MAC address to
* 6 bytes. Does not handle errors or delimiters, but requires very little
* code space and no libraries.
*/
void macStringToBytes(const String mac, byte *bytes) {
  if(mac.length() >= 12) {
    for(int i = 0; i < 6; i++) {
      bytes[i] = (valFromChar(mac.charAt(i*2)) << 4) | valFromChar(mac.charAt(i*2 + 1));
    }
  } else {
    Serial.println("Incorrect MAC format.");
  }
}

int postPublicAddress(){
  //get Public IP
  http.begin(pubipsrv);  
  int httpCode = http.GET();
  Serial.println(httpCode);
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    http.end();
    
    //post Public IP
    http.begin(pubipdest);
    http.addHeader("Content-Type", "text/plain");
    httpCode = http.POST(payload);   //Send the request
    http.end();

    return 0;
  }
  else {
    http.end();
    return 1;
  }
}
