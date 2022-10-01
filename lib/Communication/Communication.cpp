#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <rover_drive_v2.h>
#include <new_integration.h>
using namespace websockets;

// const char* ssid = "BT-HWC2G8";
// const char* password = "RMtXdgLqxDRh3e";
const char* ssid = "xxx";
const char* password = "hahaha010101";
// const char* ssid = "mengyuan";
// const char* password = "22222222";

std::vector<std::pair<int,int>> offline_path;
std::pair<int,int> previousPosition;
std::pair<int,int> currentPlace;



const char* websockets_server = "ws://18.215.182.75:14000"; //server adress and port
int status = -1;

// --------------------------------------------------------------- WIFI ----------------------------------------------------------------//

void init_WiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println("Connected");
}

void rubbish_function_for_wifi_offline_mode(){
    // currentPlace = returnCurrentPosition();

    // if(currentPlace != previousCurrentPlace){
    //     offline_path.push_back(currentPlace);
    // }
        //TODO::offline
    Serial.println("hahahahah");
    delay(100);
}

void reconnect_WiFi(WebsocketsClient& client, unsigned long reconnectWifiPeriod, unsigned long& previousTime, bool& disconnectionWifiHappened){
    unsigned long currentTime = millis(); // number of milliseconds since the upload

    // checking for WIFI connection
    if (WiFi.status() != WL_CONNECTED){
        disconnectionWifiHappened = true;
        if (currentTime - previousTime >= reconnectWifiPeriod){
            Serial.println("Try Reconnect to WIFI network");
            WiFi.disconnect();
            WiFi.reconnect();
            previousTime = currentTime;
        }
        else{
            rubbish_function_for_wifi_offline_mode();
        

        }
    }         
}

void rubbish_function_after_server_reconnected(WebsocketsClient& client, bool& disconnectionHappened){
    Serial.println("Sending offline mode collected information to server");
    String reconnectInfo = "restart";
    client.send(reconnectInfo); 
    disconnectionHappened = false;
}

// --------------------------------------------------------------- SERVER ----------------------------------------------------------------//

void server_connection(WebsocketsClient& client) {
  if (!client.connect(websockets_server)) {

    Serial.println("Connection to server failed");
    delay(100);
    return;
  }
    Serial.println("Connected to server successful!");
}

void onMessageCallback(WebsocketsMessage message) {
    String received = message.data();
    Serial.println(received);
    if (received == "!start$"){
        status = 1;
        //roverResetGlobalCoords();
        //roverTranslate(0.5);
        modeBegin(1);
    }
    else if (received == "!end$"){
        status = 0;
        stopAllTask();
        //roverStop();
    }
}
void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
        Serial.println();
        
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}
// --------------------------------------------------------------- SEND ----------------------------------------------------------------//

void send_coord_msg(WebsocketsClient& client, float x, float y, float t){
    String send_info = "!cx" + String(x,2) + ",y" + String(y,2) + ",t" + String(t,2) + "$";
    client.send(send_info);
}

void send_planned_coord_msg(WebsocketsClient& client, int px, int py){
    String send_info = "!px" + String(px) + ",y" + String(py) + "$";
    client.send(send_info);
}

void send_alien_msg(WebsocketsClient& client, int alienIndex, float x, float y, String color, int count){
    // color is single letter
    // x and y coordinates rounded to 2dp
    String send_info = "!a" + String(alienIndex) + color + "x" + String(x,2) + ",y" + String(y,2)+ ",c"+String(count)+"$";
    client.send(send_info);
}

void send_tower_msg(WebsocketsClient& client, float x, float y, float w, int count){
    // color is single letter
    // x and y coordinates rounded to 2dp
    String send_info = "!tx" + String(x,2) + ",y" + String(y,2) + ",w " + String(w, 2) + ",c" + String(count)+"$";
    client.send(send_info);
}
// without rover
// void rubbish_function_for_wifi_online_mode(WebsocketsClient& client){
//     // Serial.println(status);
//     if (status == 1){
//         send_coord_msg(client, x * 5.842, y * 5.847, 0);
//         Serial.println("sent data");
//         if ((i%10)==0){
//             send_planned_coord_msg(client, px , py);
//             px += 1;
//         }
        
//         if ((i%30) == 0){
//             send_alien_msg(client, 1, 35.26 * 5.842 , 75 * 5.847, "g", 1);
//         }
//         else if ((i%70) == 0){
//             send_alien_msg(client, 2, 67 * 5.842 , 15 * 5.847, "r", 1);
//         }
//         else if ((i%90) == 0){
//             send_alien_msg(client, 1, 36 * 5.842 , 77 * 5.847, "g", 2);
//         }
        
//         else if ((i%100)==0){
//             send_tower_msg(client, 50 * 5.842, 100 * 5.847, 1);
//         }

//         x += 1;
//         y += 1;
//         i += 1;
//     }
//     else if (status == 0){

//         client.send("E");
//         status = -1;
//     }
// }

// with rover
void wifi_online_mode(WebsocketsClient& client){
    float x = getRoverX();
    float y = getRoverY();
    float t = getRoverTheta(true);
    send_coord_msg(client, x, y, t);
}