#include <ArduinoWebsockets.h>
#include <Arduino.h>
#include <SPI.h>
#include <bitset>
#include <map>
#include <../lib/rover_drive_v2/rover_drive_v2.h> 
#include <../lib/fpga/fpga.h> 
#include <../lib/exploration/exploration.h>
#include <new_integration.h>
#include <../lib/A_star/A_star.h>
#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>
#include <WiFi.h>
#include <../lib/Communication/Communication.h>

using namespace websockets;

WebsocketsClient client;

std::pair<std::string,std::vector<double>> received_alien_message;
std::pair<std::string, std::vector<double>> new_detected_alien_message;
void setup() {
  Serial.begin(115200);
  modeBegin(0);
  // init_WiFi();
  // // delay(1000);
  // client.onMessage(onMessageCallback);
  // client.onEvent(onEventsCallback);
  // // Connect to Wifi
  // //init_WiFi();
  // pinMode(32, OUTPUT);
 
  // roverBegin();

}

// // These are useless stuff for fake data
// int i = 0;
// float xi = 10.0;
// float yi = 30.0;
// int pxi = 1;
// int pyi = 1;

// // These three things need to be defined outside loop() function
// bool serverConnected = false;
// String received = "";
// int receivedInfo[3] = {-1,0,0};

// // These three things are used for reconnection to Wifi
unsigned long previousTime = 0;
unsigned long reconnectWifiPeriod = 2000;  // Try to reconnect Wifi once every 2 seconds 
bool disconnectionHappened = false;

int x_leave_position;
int y_leave_position;
double received_alien_message_x;
double received_alien_message_y;
double received_alien_message_count;
double received_tower_diameter;
std::vector<int> tmp_store_leave_message;
std::string received_alien_message_colour;

void loop() {

  // if (!client.available() && WiFi.status() == WL_CONNECTED){
  //   server_connection(client);
  // }
  // else if (disconnectionHappened == true && WiFi.status() == WL_CONNECTED){
  //     rubbish_function_after_server_reconnected(client, disconnectionHappened);
  // }
  // else {
  //   if (client.available()){
  //     client.poll();
  //     wifi_online_mode(client);
  //     Serial.println("line 71");
  //     if(execution_check() == true){
  //         if(leaving_detected() == true){
  //         // Serial.println("----------------Leaving detected ");
  //         // Serial.print(getLeave_position()[0]);
  //         // Serial.print(" , ");
  //         // Serial.println(getLeave_position()[1]);
  //         tmp_store_leave_message.clear();
  //         tmp_store_leave_message = getLeave_position();
  //         Serial.println("line 84");
  //         x_leave_position = tmp_store_leave_message[0];
  //         Serial.println("line 85");
  //         y_leave_position = tmp_store_leave_message[1];
  //         Serial.println("line 87");
  //         send_planned_coord_msg(client, y_leave_position, x_leave_position);
  //         digitalWrite(32, HIGH);
  //         delay(100);
  //         digitalWrite(32, LOW);
  //       }
  //         Serial.println ("line 85");
          // new_detected_alien_message = getAlien_message();
  //         Serial.println ("line 86");
          // if(received_alien_message != new_detected_alien_message){
          //     received_alien_message = new_detected_alien_message;
          //     received_alien_message_x =  received_alien_message.second[0];
          //     received_alien_message_y =  received_alien_message.second[1];
          //     received_alien_message_colour =  received_alien_message.first.c_str();
          //     if(received_alien_message_colour == "t") {
          //       received_tower_diameter =  received_alien_message.second[3];
          //       send_tower_msg(client, received_alien_message_y, received_alien_message_x,received_tower_diameter, received_alien_message_count);
          //     }else{
          //       send_alien_msg(client, 1,received_alien_message_y, received_alien_message_x, received_alien_message_colour.c_str(), received_alien_message_count);
          //     }
    
          // }
          
  //     }
      
  //     delay(100);
  //   }
  // }
  // reconnect_WiFi(client, reconnectWifiPeriod, previousTime, disconnectionHappened);
}
  