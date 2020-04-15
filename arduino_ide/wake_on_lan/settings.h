#ifndef __SETINGS_H
#define __SETINGS_H

#define WIFI_SSID "your wifi ssid"
#define WIFI_PASSWORD "your wifi password"
#define SERVICE_PASSWORD "password to protect from unwanted wol, pick a long one"

//Static IP address configuration
IPAddress staticIP(10, 0, 0, 5);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(208,67,222,222);

//Public IP Server
#define PUBLICIPSERVER "http://api.ipify.org/?format=json"
#define PUBLICIPDESTINATION "http://www.your_own_server.com/example"

#endif
