#include <Wifi.h>
#include <ArduinoWebsockets.h>


#ifndef COMMUNICATION_H
#define COMMUNICATION_H

using namespace websockets;

void init_WiFi();
void reconnect_WiFi(WebsocketsClient& client, unsigned long reconnectWifiPeriod, unsigned long& previousTime, bool& disconnectionHappened);
void rubbish_function_for_wifi_offline_mode();
void rubbish_function_after_server_reconnected(WebsocketsClient& client, bool& disconnectionHappened);

void server_connection(WebsocketsClient& client);
void onMessageCallback(WebsocketsMessage message);
void onEventsCallback(WebsocketsEvent event, String data);

void send_coord_msg(WebsocketsClient& client, float x, float y, float t);
void send_planned_coord_msg(WebsocketsClient& client, int px, int py);
void send_alien_msg(WebsocketsClient& client, int alienIndex, float x, float y, String color, int count);
void send_tower_msg(WebsocketsClient& client, float x, float y, float w, int count);
// #void rubbish_function_for_wifi_online_mode(WebsocketsClient& client);
void wifi_online_mode(WebsocketsClient& client);
#endif