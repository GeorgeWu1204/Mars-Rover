#include <Arduino.h>
#include <SPI.h>
#include <bitset>
#include <map>
#include <rover_drive_v2.h>
#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>
#include <new_integration.h>
#include <../fpga/fpga.h>
#include <../exploration/exploration.h>
#include <../A_star/A_star.h>
//#include <../Communication/Communication.h>
//#include "Wifi.h"

SPIClass *hspi = NULL;
exploration explore;
fpga fpga_module;
A_star a_star_module;
//Communication communication_module;

int received;
bool FPGA_ready = false;
// define VSPI_SS  SS

int special_code, previous_special_code;
int distance_messaage_count = 0;
int loop_start = 10;
int loop_end = 17;
bool distance_bool;
std::map<std::string, std::vector<double>> alien_location_storage;
std::vector<std::string> error_alien_detected;
int aStar_map[11][17]; 
int explore_map[11][17];
// Read in core 0 Write in core 1
int user_initial_car_altitude;
Pair user_initial_position;
Pair user_destination;
std::vector<int> leave_position;
std::pair<std::string, std::vector<double>> send_alien_message;  // varible that would be rechieved by core 1 storing colour + [distance, angle, count]
volatile bool start_leaving;
bool leaving_status;
bool original_leaving_status;
bool current_alien_detected_status;
bool original_alien_detected_status;
bool complete_task;
bool task_start = false;
SemaphoreHandle_t IntegrateSemaphore;
SemaphoreHandle_t movingSemaphore;
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCK 14
#define HSPI_SS 15
#define xBound 11
#define yBound 17

TaskHandle_t startTask;
TaskHandle_t modeTask;

// std::pair<int,int> leftDownCorner (0,0); 
// std::pair<int,int> rightDownCorner (10,0);
// std::pair<int,int> rightUpCorner (10,16);
// std::pair<int,int> leftUpCorner (0,16);

int leftDownCorner = 0; 
int rightDownCorner = 1; 
int rightUpCorner = 2;
int leftUpCorner = 3;

//------------------------------------------------
// User 
//------------------------------------------------
bool execution_check(){
    if(task_start == false){
        return false;
    }
    else{
        return true;
    }
}
void modeBegin(int select_message){

    xTaskCreatePinnedToCore(start, "start", 10000, NULL, 0, &startTask, 0);

    delay(2000); // try with this Jeffrey v
  
    switch(select_message){
        case 0:
            Serial.println("Case 0 ");
            xTaskCreatePinnedToCore(exploration_loop, "Exploration", 10000, (void*)&leftDownCorner, 0, &modeTask, 0); // loop task
            xTaskCreatePinnedToCore(export_alien_location_map, "Export_map", 10000, NULL, 0, &modeTask, 0); // loop task
            Serial.println("done 0 ");
            break;
        case 1:
            Serial.println("Case 1 ");
            xTaskCreatePinnedToCore(exploration_loop, "Exploration", 10000, (void*)&rightDownCorner, 0, &modeTask, 0); // loop task
            xTaskCreatePinnedToCore(export_alien_location_map, "Export_map", 10000, NULL, 0, &modeTask, 0); // loop task
            Serial.println("done 1 ");
            break;
        case 2:
            Serial.println("Case 2 ");
            xTaskCreatePinnedToCore(exploration_loop, "Exploration", 10000, (void*)&rightUpCorner, 0, &modeTask, 0); // loop task
            xTaskCreatePinnedToCore(export_alien_location_map, "Export_map", 10000, NULL, 0, &modeTask, 0); // loop task
            break;
        case 3:
            Serial.println("Case 3 ");
            xTaskCreatePinnedToCore(exploration_loop, "Exploration", 10000, (void*)&leftUpCorner, 0, &modeTask, 0); // loop task
            xTaskCreatePinnedToCore(export_alien_location_map, "Export_map", 10000, NULL, 0, &modeTask, 0); // loop task
            break;
        case 4:
            xTaskCreatePinnedToCore(export_alien_location_map, "Export_map", 10000, NULL, 0, &modeTask, 0); // loop task
            xTaskCreatePinnedToCore(aStar, "A_star", 10000, NULL, 0, &modeTask, 0); // loop task
            break;
        }
    }


std::pair<std::string, std::vector<double>> getAlien_message(){
    // alien_storage instaantious map
    while (true) {

        if (xSemaphoreTake(IntegrateSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

        break;  // exit the waiting loop

        }
    }
    xSemaphoreGive(IntegrateSemaphore);
    return send_alien_message;
    }
std::vector<int> getLeave_position(){
    while (true) {

        if (xSemaphoreTake(movingSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free
        break;  // exit the waiting loop
        }
    }
    xSemaphoreGive(movingSemaphore);
    Serial.print("return success");
    return leave_position;
    }

bool getcomplete_task(){

    return complete_task;
    }

void stopAllTask(){
    vTaskDelete(modeTask);
    }



bool leaving_detected(){
    if (original_leaving_status != leaving_status){
        original_leaving_status = leaving_status;
        return true;
    }
    else{
        return false;
        }
    }   



std::map<std::string, std::vector<double>> get_complete_alien_storage(){
    return alien_location_storage;
    }



//------------------------------------------------
// Private
//------------------------------------------------

void start(void * param) {
    IntegrateSemaphore = xSemaphoreCreateBinary();
    movingSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(IntegrateSemaphore);
    xSemaphoreGive(movingSemaphore);
    Serial.begin(115200);
    task_start = true;
    Serial.print("<------plplplplplpl----------->");
    roverBegin();
    Serial.print("<------plplplplplpl----pppoooooo------->");
    hspi = new SPIClass(HSPI);
    hspi->begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
    special_code = loop_start;
    pinMode(hspi->pinSS(), OUTPUT);
    while (!FPGA_ready){
        hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
        digitalWrite(hspi->pinSS(), LOW);
        Serial.print("<------------Transfer----------->");
        Serial.println(70);
        received = hspi->transfer16(70);
        Serial.println(received);
        digitalWrite(hspi->pinSS(), HIGH);
        hspi->endTransaction();
        if (received == 60){
            FPGA_ready = true;
            Serial.println("<=========================****************===========================+>");
        }
    }
    vTaskDelete(startTask);
    // initialize wifi module
    //WiFi.disconnect(true);
    //communication_module.init_WiFi();
}

//------------------------------------
// Vision part
//------------------------------------

bool fpga_loop(std::map<std::string, std::vector<double>> &colour_map, bool start_detection){ // loop for one node detection :: calling Vision_main_loop multiple time {returning all the detected objects colour map}
    bool all_object_is_detected = false;
    double continue_angle;
    while (true)
    {
        int colour, distance;

        hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
        digitalWrite(hspi->pinSS(), LOW);
        // SOS
        Serial.print("<------------Transfer----------->");
        Serial.println(special_code);
        received = hspi->transfer16(special_code);
        std::string received_in_binary = std::bitset<16>(received).to_string();
        digitalWrite(hspi->pinSS(), HIGH);
        hspi->endTransaction();

        std::string type;
        Serial.print("previous special_code: ");
        Serial.println(previous_special_code);
        switch (previous_special_code)
        {
        // case 10:
        //     type = "Distance-R    ||";
        //     break;
        // case 11:
        //     type = "RED-R    ||";
        //     break;
        // case 12:
        //     type = "Formate valid    ||";
        //     break;
        case 10:
            type = "slot    ||";
            break;
        case 11:
            type = "Valid    ||";
            break;
        case 12:
            type = "Select   ||";
            break;
        case 13:
            type = "MOVING   ||";
            break;
        case 14:
            type = "c_8      ||";
            break;
        case 15:
            type = "data_colour Distance_black ||";
            break;
        case 16:
            type = "pixel avg_black ||";
            break;
        case 17:
            type = "Distance ||";
            break;
        default:
            type = "False";
        }

        Serial.print(type.c_str());
        Serial.print("  ");
        Serial.println(received_in_binary.c_str());
        delay(100);
        // Distance code has to be sent twice
        if (special_code != loop_end)
        {
            previous_special_code = special_code;
            special_code++;
            distance_bool = false;
        }
        else
        { // sent(13) count =0;
            if (distance_messaage_count == 1)
            {
                previous_special_code = special_code;
                special_code = loop_start;
                distance_messaage_count = 0;
                distance_bool = true;
            }
            else
            {
                previous_special_code = special_code;
                special_code = loop_end;
                distance_messaage_count = 1;
                distance_bool = true;
            }
        }
        // TODO: .h 
        all_object_is_detected = Vision_main_loop(received, special_code, colour_map, continue_angle, start_detection); //------> all objects are detected
        if (all_object_is_detected)
        {
            if (colour_map.size() == 0)
                return false;
            else
                return true;
        }
    }
}

bool Vision_main_loop(int received, int special_code, std::map<std::string, std::vector<double>> &detected_alien_set, double& continue_rotate_angle, bool start_detection){ // loop for one node detection :: calling Vision_main_loop multiple time {returning all the detected objects colour map}
    bool all_objects_are_detected = false;
    std::pair<std::string, std::vector<double>> colour_map;
    bool stop;
    int colour_first;
    int pixel_first, distance_first;
    int distance_final;
    std::map<int, int> distance_count;
    std::string received_in_binary = std::bitset<16>(received).to_string();

    if (received != 0 && received != 0b1111111111111111)
    {
        // Message is valid
        Serial.print("Distance_bool: ");
        Serial.print(distance_bool);
        Serial.print(", special_code: ");
        Serial.print(special_code);
        Serial.print(", distance_message_c: ");
        Serial.println(distance_messaage_count);
        if (received_in_binary.at(0) == '0' && distance_messaage_count == 0 && distance_bool == true)
        {
            // Message is either distance type or special case
            // meaning full distance information
            fpga_module.distance_decode(received_in_binary, colour_first, distance_first);
            if (special_code == 10)
            {
                // if(start_detection){
                //     roverStop();
                //     rover.measure();
                //     double brake_angle = rover.phideg;
                //     // <- ++ positive angle ++ || -- negative angle -- -> //
                //     if(brake_angle < 0){
                //     continue_rotate_angle = -90 - brake_angle;
                //     }
                //     else {
                //     continue_rotate_angle = 90 - brake_angle;
                //     }    
                // }
                //Message is distance type
                int received_tmp, distance_tmp, colour_tmp;
                Serial.println("<----------------------MEANSURE DISTANCE------------------->");
                Serial.print("COLOUR FIRST: ");
                Serial.print(colour_first);
                Serial.println("<----------------------MEANSURE DISTANCE------------------->");

                for (int i = 0; i < 20; i++){
                    // try to stabilize the distance inform

                    hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                    digitalWrite(hspi->pinSS(), LOW);
                    Serial.print("<------------Transfer----------->");
                    Serial.println(4);
                    received_tmp = hspi->transfer16(17);
                    previous_special_code = 17;
                    std::string distance_tmp_in_binary = std::bitset<16>(received_tmp).to_string();

                    digitalWrite(hspi->pinSS(), HIGH);
                    hspi->endTransaction();
                    delay(100);

                    fpga_module.distance_decode(distance_tmp_in_binary, colour_tmp, distance_tmp);
                    // Serial.print(i);
                    // Serial.print("  Distance Mesaage : ");
                    // Serial.println(distance_tmp_in_binary.c_str());

                    if (received_in_binary.at(0) == '0' && colour_tmp == colour_first){
                        // the message is belone to the same colour
                        Serial.print(i);
                        Serial.print(" Colour: ");
                        Serial.print(colour_tmp);
                        Serial.print(" Distance: ");
                        Serial.println(distance_tmp);
                        if (distance_tmp != 0){
                            std::map<int, int>::iterator it = distance_count.find(distance_tmp);
                            if (it != distance_count.end()){
                                it->second++;
                            }
                            else{
                                distance_count.insert(std::make_pair(distance_tmp, 1));
                            }
                        }
                    }
                    else{
                        // unexpect warning;
                        if (colour_tmp != colour_first){
                            Serial.print(i);
                            Serial.print(" Unexpected behaviour, Colour Change changed :: ");
                            Serial.print("  Distance Mesaage : ");
                            Serial.println(distance_tmp_in_binary.c_str());
                            
                        }
                        else
                        Serial.print(i);
                        Serial.println("  Unexpected behaviour, received message should be distance type");
                    }
                }
                // stablization done
                int max_key, max_number = 0;
                std::map<int, int>::iterator it;
                for (it = distance_count.begin(); it != distance_count.end(); it++)
                {
                    if (max_number < it->second)
                    {
                        max_number = it->second;
                        max_key = it->first;
                    }
                }
                if (max_number < 10)
                {
                    Serial.println("XXXXXXXXXXXXXXXXXXX Invalid distance count XXXXXXXXXXXXXXXXXXX");
                    // TODO: Rotate back AND THEN Transmit Unknow.
                }
                else
                {
                    int tower_diameter_binary;
                    int scale_r = 1;
                    int sum_binary_to_decimal_r = 0;
                    distance_final = max_key;
                    int select_message;
                    std::string tmp;
                    std::string block_colour;
                    switch(colour_first){
                        case 0:
                            // red
                            select_message = 30;
                            block_colour = "r";
                            break;
                        case 1:
                            select_message = 31;
                            block_colour = "p";
                            break;
                        case 10:
                            select_message = 32;
                            block_colour = "g";
                            break;
                        case 11:
                            select_message = 33;
                            block_colour = "w";
                            break;
                        case 100:
                        //TODO:: bllack
                            select_message = 34;
                            block_colour = "t";

                            hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                            digitalWrite(hspi->pinSS(), LOW);
                            Serial.print("<------------Transfer----------->");
                            Serial.println(18);
                            received_tmp = hspi->transfer16(18);
                            previous_special_code = 18;
                            digitalWrite(hspi->pinSS(), HIGH);
                            hspi->endTransaction();

                            hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0)); // receving distance message
                            digitalWrite(hspi->pinSS(), LOW);
                            Serial.print("<------------Transfer----------->");
                            Serial.println(17);
                            received_tmp = hspi->transfer16(17);
                            previous_special_code = 17;
                            digitalWrite(hspi->pinSS(), HIGH);
                            hspi->endTransaction();
                           
                            tmp = std::bitset<16>(received_tmp).to_string();
                            Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                            Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                            Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                            Serial.println(tmp.c_str());
                            tower_diameter_binary = std::stoi(std::bitset<16>(received_tmp).to_string().substr(5,15));
                            while (true)
                            {
                                if (tower_diameter_binary % 10 == 1)
                                {
                                    sum_binary_to_decimal_r += 1 * scale_r;
                                }
                                scale_r *= 2;
                                if(tower_diameter_binary / 10 == 0){
                                    break;
                                }
                                else{
                                    tower_diameter_binary /= 10;
                                }
                            }
                            Serial.print(tower_diameter_binary);
                            Serial.print(" -------> CONVERT TO ----------->");
                            Serial.println(sum_binary_to_decimal_r);
                            break;
                        case 101:
                            select_message = 35;
                            block_colour = "y";
                            break;
                        case 110:
                            select_message = 36;
                            block_colour = "c";
                            break;
                        case 111:
                            select_message = 37;
                            block_colour = "b";
                            break;
                    }
                    hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                    digitalWrite(hspi->pinSS(), LOW);
                    Serial.print("<------------Transfer----------->");
                    Serial.println(select_message);
                    received = hspi->transfer16(select_message);
                    previous_special_code = select_message;
                    digitalWrite(hspi->pinSS(), HIGH);
                    hspi->endTransaction();
                    delay(100);

                    Serial.println("********************* SUCCESS SUCCESS SUCCESS *********************");
                    Serial.print("Finalized Distance :: ");
                    Serial.println(distance_final);
                    int scale = 1;
                    int sum_binary_to_decimal = 0;
 
                    while (true)
                    {
                        if (distance_final % 10 == 1)
                        {
                            sum_binary_to_decimal += 1 * scale;
                        }
                        scale *= 2;
                        if(distance_final / 10 == 0){
                            break;
                        }
                        else{
                            distance_final /= 10;
                        }
                    }

                    Serial.print(distance_final);
                    Serial.print(" -------> CONVERT TO ----------->");
                    Serial.println(sum_binary_to_decimal);

                    std::vector<double> position_detail; // position detail 
                    double angle = -getRoverPhi(true);
                    position_detail.push_back(sum_binary_to_decimal + sum_binary_to_decimal_r/2);
                    position_detail.push_back(angle);

                    position_detail.push_back(sum_binary_to_decimal_r); //radius 

                    colour_map = std::make_pair(block_colour, position_detail); // combined with colour 
                    detected_alien_set.insert(colour_map);



                    Serial.println("<------------------------------------ BLOCKING ----------------------------------->");
                    Serial.println("<------------------------------------ BLOCKING ----------------------------------->");
                    Serial.println("<------------------------------------ BLOCKING ----------------------------------->");
                    Serial.print("Block: ");
                    Serial.println(block_colour.c_str());
                    // TODO: Rotate back AND THEN Transmit block.
                    Serial.println(getRoverPhi(true));
                    roverRotateBack(0.2);
                    Serial.println("################################### ROTATE BACK ########################################");
                }
                // END stablization finish
                // Transmitting BLOCK signal;
            }
            else
            {
                // Handling special case

                // std::string type;
                // Serial.print("");
                // switch(special_code - 1){
                //   case 9:  type = "Distance ||"; break;
                //   case 10: type = "Lock     ||"; break;
                //   case 11: type = "Valid    ||"; break;
                //   case 12: type = "Select   ||"; break;
                //   default : "False" ;
                // };
                // Serial.print(type.c_str());
                // Serial.println(distance_first);
            }
        }
        else if (received_in_binary.at(0) == '1')
        {
            // pixel message
            Serial.println("Done1");
            fpga_module.pixel_decode(received_in_binary, colour_first, pixel_first);
            Serial.println("Done2");
            if (special_code == 10)
            {
                
                // roverStop();
                // rover.measure();
                // double brake_angle = rover.phideg;
                // // <- ++ positive angle ++ || -- negative angle -- -> //
                
                // if(brake_angle < 0){
                //    continue_rotate_angle = -90 - brake_angle;
                // }
                // else {
                //    continue_rotate_angle = 90 - brake_angle;
                // }
                
                stop = false;
                Serial.print("Colour: ");
                Serial.print(colour_first);
                Serial.print(" pixel: ");
                Serial.println(pixel_first);

                Serial.println("Rotation start");
                pixel_rotation(pixel_first, stop);

                int out = 1;
                int received_tmp;
                int non_detected_count = 0;
                hspi->endTransaction();
                while (out)
                {
                    hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                    digitalWrite(hspi->pinSS(), LOW);
                    Serial.print("<------------Transfer----------->");
                    Serial.println(17);
                    received_tmp = hspi->transfer16(17);
                    previous_special_code = 17;
                    std::string distance_tmp_in_binary = std::bitset<16>(received_tmp).to_string();
                    digitalWrite(hspi->pinSS(), HIGH); // pull ss high to signify end of data transfer
                    hspi->endTransaction();
                    delay(100);

                    Serial.print("<------------WHILE CHECK----------->");
                    Serial.print("<------------1------------>");
                    Serial.print("<------------2------------>");
                    Serial.print("Rotating message: ");
                    Serial.println(distance_tmp_in_binary.c_str());

                    if (distance_tmp_in_binary.at(0) == '0')
                    {
                        out = 0;
                        Serial.println("<><><><><><><><><><><><><><>BRAKE<><><><><><><><><><><><><><>");
                        roverStop();
                        Serial.print("<-------------OUT------------>: ");
                        Serial.println(distance_tmp_in_binary.c_str());
                        Serial.println("<><><><><><><><><><><><><><>BRAKE<><><><><><><><><><><><><><>");
                    }
                    // TODO: check if this is still needed? if you r with in this "if" meaning lock is on? so 1111 wonldnt be the case
                    // else if (distance_tmp_in_binary == "1111111111111111")
                    // {
                    //   //target lost in detection
                    //   non_detected_count++;
                    //   if (non_detected_count > 10)
                    //   {
                    //     Serial.println("<-----------Target Lost---------->");
                    //     roverStop();
                    //     break;
                    //   }
                    // }
                }

                if (non_detected_count > 10)
                {
                    Serial.println("Fail return back");
                    // TODO: LOCK? 
                }
                else
                {
                    Serial.println("Rotation done");
                }
                delay(100);
            }
            Serial.println("Done");
        }
    }
    else
    {
        // Message not started
        if (received == 0)
        {
            Serial.println("NOT started");
        }
        else if (received == 0b1111111111111111)
        {
            if (special_code == 10)
            {
                int received_not_detected;
                int received_not_detected_count = 0;
                Serial.println("enter nothing is detected state");
                for (int i = 0; i < 5; i++)
                {
                    hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                    digitalWrite(hspi->pinSS(), LOW);
                    Serial.print("<------------Transfer----------->");
                    Serial.println(17);
                    received_not_detected = hspi->transfer16(17);
                    previous_special_code = 17;
                    // std::string distance_tmp_in_binary = std::bitset<16>(received_not_detected).to_string();

                    digitalWrite(hspi->pinSS(), HIGH);
                    hspi->endTransaction();
                    
                   // Serial.println(" ");
                    
                    // hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                    // digitalWrite(hspi->pinSS(), LOW);
                    // int cross_check;
                    // Serial.print("<------------Transfer----------->");
                    // Serial.println(12);
                    // cross_check = hspi->transfer16(12);
                    // previous_special_code = 12;
                    // digitalWrite(hspi->pinSS(), HIGH);
                    // hspi->endTransaction();

                //     hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                //     digitalWrite(hspi->pinSS(), LOW);
                //     Serial.print("<------------Transfer----------->");
                //     Serial.println(17);
                //    // int cross_check;
                //     cross_check = hspi->transfer16(17);
                //     previous_special_code = 17;
                //     std::string not_ok = std::bitset<16>(cross_check).to_string();
                //     Serial.print("select: ");
                //     Serial.println(not_ok.c_str());
                //     digitalWrite(hspi->pinSS(), HIGH);
                //     hspi->endTransaction();

                    Serial.print(i);
                    delay(10);
                    if (received_not_detected == 0b1111111111111111)
                    {
                        Serial.println(" :: NONE");
                        received_not_detected_count++;
                    }
                }
                if (received_not_detected_count > 3)
                {
                    // Indeed theres nothing there
                    hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
                    digitalWrite(hspi->pinSS(), LOW);
                    // Transmit moving forward signal
                    Serial.print("<------------Transfer----------->");
                    Serial.println(50);
                    received_not_detected = hspi->transfer16(50);
                    previous_special_code = 50;
                    digitalWrite(hspi->pinSS(), HIGH);
                    hspi->endTransaction();
                    Serial.println("~~~~~****~~~~~~~MOVING FORWARD~~~~~~*****~~~~~~");
                    Serial.println("~~~~~****~~~~~~~MOVING FORWARD~~~~~~*****~~~~~~");
                    Serial.println("~~~~~****~~~~~~~MOVING FORWARD~~~~~~*****~~~~~~");
                    Serial.println("~~~~~****~~~~~~~MOVING FORWARD~~~~~~*****~~~~~~");
                    //roverRotateToTarget(continue_rotate_angle, 0.5);
                    all_objects_are_detected = true;
                    
                    
                }
            }
        }
    }
    return all_objects_are_detected;
}

//------------------------------------
// Exploration part
//------------------------------------
void exploration_loop(void * param){ //  Exploration mode with defined starting position. Expected to be call for once;

    complete_task = 0;
    Serial.println("inside exploration");
    std::vector<double> alien_posi;
    std::string colour;
    //std::map<std::string, std::vector<double>> alien_set;
    explore_map[xBound][yBound] = {0};
    std::pair<std::string, std::vector<double>> FPGA_ESP32_input; // colour, distance, angle

    std::vector<int> current_rover_position;
    // std::vector<int> next_rover_position;
    int movement; // movement of rover in next step
    // initialize position
    bool exploration_complete;
    //std::vector<std::string> wrong_detected_alien;
    bool step_taken = false;
     std::vector<int> pre_next_rover_position, next_rover_position;
    std::vector<int> xHistory, yHistory;
    int current_car_altitude;
    switch(*((int*)param)){

        case 0: xHistory.push_back(0);
                yHistory.push_back(0);
                current_rover_position.push_back(0);
                current_rover_position.push_back(0);
                explore_map[0][0] = 1;
                current_car_altitude = 10;
                pre_next_rover_position.push_back(0);
                pre_next_rover_position.push_back(0);
                break;

        case 1: xHistory.push_back(10);   //[10,0]
                yHistory.push_back(0);
                current_rover_position.push_back(10);
                current_rover_position.push_back(0);
                explore_map[10][0] = 1;
                current_car_altitude = 10;
                pre_next_rover_position.push_back(10);
                pre_next_rover_position.push_back(0);
                break;
               
                break;
        case 2: xHistory.push_back(10);    //[10,16]
                yHistory.push_back(16);
                current_rover_position.push_back(10);
                current_rover_position.push_back(16);
                explore_map[10][16] = 1;
                current_car_altitude = 11;
                pre_next_rover_position.push_back(0);
                pre_next_rover_position.push_back(0);
                 current_car_altitude = 10;
                pre_next_rover_position.push_back(10);
                pre_next_rover_position.push_back(16);
                break;
        case 3: xHistory.push_back(0);
                yHistory.push_back(16);
                current_rover_position.push_back(0);
                current_rover_position.push_back(16);
                explore_map[0][16] = 1;
                 current_car_altitude = 11;
                pre_next_rover_position.push_back(0);
                pre_next_rover_position.push_back(16);
                break;
    }
   
   
    while (true)
    {
        // step 1: detect alien and refresh map
        // TODO: method to reduce possible rotation
        Serial.println("pre");
        std::vector<int> tmp = explore.next_step(explore_map, xHistory, yHistory, movement);
        Serial.println("gg");
        pre_next_rover_position[0] = tmp[0];
        pre_next_rover_position[1] = tmp[1]; 
        Serial.println("<----------------Pre Next Rover Position------------------>");
        Serial.print(pre_next_rover_position[0]);
        Serial.print(" : ");
        Serial.println(pre_next_rover_position[1]);
        Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        int gg = relative_rotation(current_car_altitude, movement);
        Serial.print(gg);
        Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        drive_command(relative_rotation(current_car_altitude, movement));
        bool start_detection = false;
        current_car_altitude = movement;
        while (!step_taken)
        {
            current_rover_position[0] = xHistory.back();
            current_rover_position[1] = yHistory.back();
            //
            int detection_code;
            hspi->beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
            digitalWrite(hspi->pinSS(), LOW);
            // Transmit moving forward signal
            Serial.print("<------------Transfer----------->");
            Serial.println(100);
            detection_code = hspi->transfer16(100);
            previous_special_code = 100;
            digitalWrite(hspi->pinSS(), HIGH);
            hspi->endTransaction();
            listen_map_alien(current_rover_position, explore_map, alien_location_storage, error_alien_detected, current_car_altitude, start_detection);
            Serial.println("<--------------------MAP------------------->");
            for (int i = 0; i < xBound; i++){
                for (int j = 0; j < yBound; j++)
                {
                    Serial.print(explore_map[i][j]);
                    Serial.print(" ");
                }
                Serial.println("");
            }
            Serial.println("<--------------------MAP------------------->");

            // step 2: calculate next step position
            Serial.println("<-------------------xhistory content after 1------------------>");
            for (int m = 0; m < xHistory.size(); m++){
                Serial.print(xHistory[m]);
                Serial.print(", ");
            }
            Serial.println("<-------------------yhistory content after 1------------------>");
            for (int n = 0; n < yHistory.size(); n++){
                Serial.println("<-------yHistory size------>");
                Serial.println(yHistory.size());
                Serial.print(yHistory[n]);
                Serial.print(", ");    
            }
            Serial.println("End y history");
            next_rover_position = explore.next_step(explore_map, xHistory, yHistory, movement);
            Serial.println("<----------------Next Rover Position------------------>");
            Serial.print("next_rover_position: ");
            Serial.print(next_rover_position[0]);
            Serial.print(" : ");
            Serial.println(next_rover_position[1]);
             while (true) {

                if (xSemaphoreTake(movingSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

                    break;  // exit the waiting loop

                }
            }
            leave_position = next_rover_position;
            xSemaphoreGive(movingSemaphore);
            Serial.println("<----------------Pre Rover Position------------------>");
            Serial.print("pre_rover_position: ");
           // Serial.print(pre_next_rover_position.size());
            Serial.print(pre_next_rover_position[0]);
            Serial.print(" : ");
            Serial.println(pre_next_rover_position[1]);

            Serial.println("<----------------Pre Rover Position------------------>");


            if (next_rover_position[0] == pre_next_rover_position[0] && next_rover_position[1] == pre_next_rover_position[1]){
                
                step_taken = true;
                Serial.println("<--------------STEP TAKEN------------->");
                xHistory.push_back(next_rover_position[0]);
                Serial.println("*******Next_Rover_Position*******");
                Serial.println(next_rover_position[1]);
                yHistory.push_back(next_rover_position[1]);
                explore_map[next_rover_position[0]][next_rover_position[1]] += 1;
                leaving_status = !leaving_status;
            }
            else{
                // TODO: rotate
                Serial.println("<----------------ROTATE-------------->");
                Serial.print("current_car_altitude:  ");
                Serial.println(current_car_altitude);
                Serial.print("movement:  ");
                Serial.println(movement);
                Serial.println(relative_rotation(current_car_altitude, movement));
                drive_command(relative_rotation(current_car_altitude, movement));
                pre_next_rover_position = next_rover_position;
                current_car_altitude = movement;
            }
        }
        step_taken = false;
        // step 3: move to the position
        Serial.println("316");
        std::string direction;
        switch (movement)
        {
        case 10:
            direction = "UP";
            break;
        case 11:
            direction = "DOWN";
            break;
        case 12:
            direction = "LEFT";
            break;
        case 13:
            direction = "RIGHT";
            break;
        }
        Serial.println(direction.c_str());
       // roverTranslateToTarget(200,0.8);
       roverMoveToTarget(next_rover_position[1]*200,-next_rover_position[0]*200, 0.8, 0.5);
        roverWait();
        // step 4: check if the map is completely detected
        exploration_complete = false;
        for (int i = 0; i < xBound; i++)
        {
            Serial.print("Row ");
            Serial.println(i);
            for (int g = 0; g < yBound; g++)
            {
                Serial.print(explore_map[i][g]);
                Serial.print(", ");
                if (explore_map[i][g] == 0)
                {
                    exploration_complete = false;
                }
            }
            Serial.println(" ");
        }
        Serial.println("<----------END MAP-------->");
        if (exploration_complete)
        {
            Serial.println("Exploration Completed!");
            break;
        }
    }
    complete_task = 1;
    vTaskDelete(modeTask);
}

void export_alien_location_map(void * param){
    for(int i=0; i<11; i++){
        for(int j=0; j<17; j++){
            if(explore_map[i][j]==900){
                aStar_map[i][j] = 0;
            }else if(explore_map[i][j] == 0){
                aStar_map[i][j] = 0; // unknow area set as unreachable;
            }else{
                aStar_map[i][j] = 1;
            }
        }
    }
    vTaskDelete(modeTask);
}

void listen_map_alien(std::vector<int> rover_position, int map[11][17], std::map<std::string, std::vector<double>> &alien_storage, std::vector<std::string> wrong_detect_alien, int current_car_altitude, bool start_detection){
    // alien_storage key:color value: [0] x_coordinate [1] y_coordinate [2] count_number
    //  2*2 block
    Serial.println("inside listen_map_alien");
    Serial.println("inside listen_map_alien");
    Serial.println("inside listen_map_alien");
    std::map<std::string, std::vector<double>> node_tmp_colour_map;
    double alienx, alieny;
    double diameter, compliment;
    int xLow, xHigh, yLow, yHigh, xCenter, yCenter;

    // pair already exited in the loop
    std::map<std::string, std::vector<double>>::iterator it_1;
    if (fpga_loop(node_tmp_colour_map, start_detection))
    {
        Serial.println("------------------- OBJECT DETECTED ---------------");
        std::map<std::string, std::vector<double>>::iterator it_2;

        for (it_2 = node_tmp_colour_map.begin(); it_2 != node_tmp_colour_map.end(); it_2++)
        {
            double alienx = explore.locate_alien(rover_position, it_2->second, current_car_altitude)[0];
            double alieny = explore.locate_alien(rover_position, it_2->second, current_car_altitude)[1];
            Serial.println("<----------------------ALIEN POSITION----------------------->");
            Serial.println(alienx);
            Serial.println(alieny);
            Serial.println("<----------------------ALIEN POSITION----------------------->");
          
            // alien_position double tile
            std::vector<double> alien_message_prepare; // tmp varible storing [distance, angle, count]

            if(alienx == 0 && alieny == 0){ // Out of range 
                it_1 = alien_storage.find(it_2->first); // it_2->first colour
                if (it_1 != alien_storage.end())
                { 
                    while (true) {

                        if (xSemaphoreTake(IntegrateSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

                        break;  // exit the waiting loop

                        }
                    }
                    it_1->second[2] -= 1;
                    alien_message_prepare.push_back(alienx*20);
                    alien_message_prepare.push_back(alieny*20);
                    alien_message_prepare.push_back(it_1->second[2]);
                    Serial.println("COUNT: "); 
                    Serial.print(it_1->second[2]); 
                    send_alien_message = std::make_pair(it_1->first, alien_message_prepare);
                    xSemaphoreGive(IntegrateSemaphore);

                    Serial.println("OUT OF RANGE");
                    Serial.print("color: ");
                    Serial.println(it_1->first.c_str());
                    
                    if(it_1->second[2] == 0){
                        wrong_detect_alien.push_back(it_1->first);
                        Serial.print("erasing it from the stable list");
                        alien_storage.erase(it_1);
                    }
                }      
            }
            else{
                if(it_2->first != "t"){
                    xLow = explore.normal_round(alienx - 0.25);
                    xHigh = explore.normal_round(alienx + 0.25);
                    yLow = explore.normal_round(alieny - 0.25);
                    yHigh = explore.normal_round(alieny + 0.25);
                    xCenter = explore.normal_round(alienx);
                    yCenter = explore.normal_round(alieny);
                }else{
                    Serial.println("<---------------------alien tower detected------------------>");
                    Serial.print("diameter: ");
                    diameter = it_2->second[2];
                    Serial.println(diameter);
                    compliment = ((diameter/40) < 1)? 0.6 : diameter/40 + 0.1;
                    Serial.print("compliment factor: ");
                    Serial.println(compliment);
                    xLow = explore.normal_round(alienx - compliment);
                    xHigh = explore.normal_round(alienx + compliment);
                    yLow = explore.normal_round(alieny - compliment);
                    yHigh = explore.normal_round(alieny + compliment);

                    xCenter = explore.normal_round(alienx);
                    yCenter = explore.normal_round(alieny);
                }
                Serial.println("<========map location==========>");
                Serial.print("xlow ");
                Serial.print(xLow);
                Serial.print("xHigh ");
                Serial.print(xHigh);
                Serial.print("yLow ");
                Serial.print(yLow);
                Serial.print("yHigh ");
                Serial.println(yHigh);
                Serial.print("yCenter ");
                Serial.print(yCenter);
                Serial.print("xCenter ");
                Serial.println(xCenter);
                Serial.println("<========map location==========>");

                if(it_2->first != "t"){ //case non tower
                    it_1 = alien_storage.find(it_2->first); // it_2->first colour
                    if (it_1 != alien_storage.end()){
                        // alien already detected.
                        // update map
                        if (((xLow - 1) * 20 < it_1->second[0] < (xHigh + 1) * 20) || ((yLow - 1) * 20 < it_1->second[1] < (yLow + 1) * 20)){
                            Serial.println("Same alien detected ");
                            Serial.print("color: ");
                            Serial.print(it_1->first.c_str());
                        
                            it_1->second[0] = (alienx + it_1->second[0])/2;
                            it_1->second[1] = (alieny + it_1->second[1])/2;
                            it_1->second[2] += 1;
                            while (true) {

                                if (xSemaphoreTake(IntegrateSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

                                break;  // exit the waiting loop

                                }
                            }
                            alien_message_prepare.push_back((alienx*20 + it_1->second[0])/2);
                            alien_message_prepare.push_back((alieny*20 + it_1->second[1])/2);
                            alien_message_prepare.push_back(it_1->second[2]);
                            Serial.println("COUNT: "); 
                            Serial.print(it_1->second[2]); 
                            xSemaphoreGive(IntegrateSemaphore);
                            send_alien_message = std::make_pair(it_1->first, alien_message_prepare);
                            
                            if (0 <= xLow < xBound && 0 <= yLow < yBound)
                            {
                                map[xLow][yLow] = 900;
                            }
                            
                            if (0 <= xLow < xBound && 0 <= yHigh < yBound)
                            {
                                map[xLow][yHigh] = 900;
                            }
                            if (0 <= xHigh < xBound && 0 <= yLow < yBound)
                            {
                                map[xHigh][yLow] = 900;
                            }
                            if (0 <= xHigh < xBound && 0 <= yHigh < yBound)
                            {
                                map[xHigh][yHigh] = 900;
                            }

                            //maping center
                            if (0 <= xCenter < xBound && 0 <= yLow < yBound)
                            {
                                map[xCenter][yLow] = 900;
                            }
                            if (0 <= xCenter < xBound && 0 <= yHigh < yBound)
                            {
                                map[xCenter][yHigh] = 900;
                            }
                            if (0 <= xHigh < xBound && 0 <= yCenter < yBound)
                            {
                                map[xHigh][yCenter] = 900;
                            }
                            if (0 <= xHigh < xBound && 0 <= yCenter < yBound)
                            {
                                map[xHigh][yCenter] = 900;
                            }
                            if(0 <= xCenter < xBound && 0 <= yCenter < yBound)
                            {
                                map[xCenter][yCenter] = 900;
                            }
                            return;
                        
                        }
                        else{
                            while (true) {

                                if (xSemaphoreTake(IntegrateSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

                                break;  // exit the waiting loop

                                }
                            }
                            it_1->second[2] -= 1;
                            alien_message_prepare.push_back(alienx*20);
                            alien_message_prepare.push_back(alieny*20);
                            alien_message_prepare.push_back(it_1->second[2]);
                            Serial.println("COUNT: "); 
                            Serial.print(it_1->second[2]); 
                            xSemaphoreGive(IntegrateSemaphore);
                            send_alien_message = std::make_pair(it_1->first, alien_message_prepare);

                            Serial.println("Wrong alien detected, position change");
                            Serial.print("color: ");
                            Serial.println(it_1->first.c_str());
                            
                            if(it_1->second[2] == 0){
                                wrong_detect_alien.push_back(it_1->first);
                                Serial.print("erasing it from the stable list");
                                alien_storage.erase(it_1);
                            } 
                        }
                    }
                    else{
                        Serial.println(" New alien detected ");
                        while (true) {

                            if (xSemaphoreTake(IntegrateSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

                            break;  // exit the waiting loop

                            }
                        }
                        alien_message_prepare.push_back(alienx*20);
                        alien_message_prepare.push_back(alieny*20);
                        alien_message_prepare.push_back(1);    
                        send_alien_message = std::make_pair(it_2->first, alien_message_prepare); // it_1 storage slot
                        xSemaphoreGive(IntegrateSemaphore);
                        alien_storage.insert(std::make_pair(it_2->first, alien_message_prepare));// it_2 tmp slot
                        Serial.println("exec 6");
                        if (0 <= xLow < xBound && 0 <= yLow < yBound)
                        {
                            map[xLow][yLow] = 900;
                        }
                        if (0 <= xLow < xBound && 0 <= yHigh < yBound)
                        {
                            map[xLow][yHigh] = 900;
                        }
                        if (0 <= xHigh < xBound && 0 <= yLow < yBound)
                        {
                            map[xHigh][yLow] = 900;
                        }
                        if (0 <= xHigh < xBound && 0 <= yHigh < yBound)
                        {
                            map[xHigh][yHigh] = 900;
                        }

                        Serial.println("Detected Alien Map");
                        Serial.print("(Low Low) ");
                        Serial.print(xLow);
                        Serial.print(", ");
                        Serial.println(yLow);

                        Serial.print("(Low High) ");
                        Serial.print(xLow);
                        Serial.print(", ");
                        Serial.println(yHigh);

                        Serial.print("(High Low) ");
                        Serial.print(xHigh);
                        Serial.print(", ");
                        Serial.println(yLow);

                        Serial.print("(High High) ");
                        Serial.print(xHigh);
                        Serial.print(", ");
                        Serial.println(yHigh);
                    }
                }else{   //case tower                   
                    Serial.println(" tower detected ");
                    while (true) {

                        if (xSemaphoreTake(IntegrateSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

                        break;  // exit the waiting loop

                        }
                    }
                    alien_message_prepare.push_back(alienx*20);
                    alien_message_prepare.push_back(alieny*20);
                    alien_message_prepare.push_back(1); 
                    alien_message_prepare.push_back(diameter);
                    send_alien_message = std::make_pair(it_2->first, alien_message_prepare); // it_1 storage slot
                    xSemaphoreGive(IntegrateSemaphore);
                    alien_storage.insert(std::make_pair(it_2->first, alien_message_prepare));// it_2 tmp slot

                    if (0 <= xLow < xBound && 0 <= yLow < yBound)
                    {
                        map[xLow][yLow] = 900;
                    }
                    if (0 <= xLow < xBound && 0 <= yHigh < yBound)
                    {
                        map[xLow][yHigh] = 900;
                    }
                    if (0 <= xHigh < xBound && 0 <= yLow < yBound)
                    {
                        map[xHigh][yLow] = 900;
                    }
                    if (0 <= xHigh < xBound && 0 <= yHigh < yBound)
                    {
                        map[xHigh][yHigh] = 900;
                    }

                    Serial.println("Detected Alien Map");
                    Serial.print("(Low Low) ");
                    Serial.print(xLow);
                    Serial.print(", ");
                    Serial.println(yLow);

                    Serial.print("(Low High) ");
                    Serial.print(xLow);
                    Serial.print(", ");
                    Serial.println(yHigh);

                    Serial.print("(High Low) ");
                    Serial.print(xHigh);
                    Serial.print(", ");
                    Serial.println(yLow);

                    Serial.print("(High High) ");
                    Serial.print(xHigh);
                    Serial.print(", ");
                    Serial.println(yHigh);
                }
            }
        }
    } // TODO: CHECK Bound
}

//------------------------------------
// A_star part
//------------------------------------
void aStar(void * param){
    complete_task = 0; 
    move_to_dest(user_initial_car_altitude, user_initial_position, user_destination);   
    complete_task = 1;
    vTaskDelete(modeTask); 
}

void move_to_dest(int initial_car_altitude, Pair initial_position, Pair destination){
    std::stack<Pair> path;
    // export_map is the global map
    path = a_star_module.aStarSearch(aStar_map, initial_position, destination);
    std::pair<int, int> current_location;
    current_location = path.top();
    std::pair<int, int> next_location;
    int current_car_altitude = initial_car_altitude;
    int delta_x, delta_y;
    int relative_movement;
    path.pop();
    while (!path.empty())
    {
        next_location = path.top();
        path.pop();
        Serial.println("Path Content -------");
        Serial.print(next_location.first);
        Serial.print(", ");
        Serial.println(next_location.second);
        delta_x = next_location.first - current_location.first;
        delta_y = next_location.second - current_location.second;
        Serial.print("<-----------Current Car Location ----------->  ");
        Serial.print(current_location.first);
        Serial.print(", ");
        Serial.println(current_location.second);
        Serial.print("<-----------Next Car Location----------->  ");
        Serial.print(next_location.first);
        Serial.print(", ");
        Serial.println(next_location.second);

        // up
        if (delta_x == 0 && delta_y == 1)
        {
            relative_movement = relative_rotation(current_car_altitude, 10);
            current_car_altitude = 10;
            leaving_status = ! leaving_status;
        }
        // down
        else if (delta_x == 0 && delta_y == -1)
        {
            relative_movement = relative_rotation(current_car_altitude, 11);
            current_car_altitude = 11;
            leaving_status = ! leaving_status;

        }
        // right
        else if (delta_x == 1 && delta_y == 0)
        {
            relative_movement = relative_rotation(current_car_altitude, 12);
            current_car_altitude = 12;
            leaving_status = ! leaving_status;
        }
        // left
        else if (delta_x == -1 && delta_y == 0)
        {
            relative_movement = relative_rotation(current_car_altitude, 13);
            current_car_altitude = 13;
            leaving_status = ! leaving_status;
        }
        else if (delta_x == 0 && delta_y == 0)
        {
            Serial.println("<-------------error message------------>");
            Serial.print("current");
            Serial.print(current_location.first);
            Serial.println(current_location.second);
            Serial.print("next");
            Serial.print(next_location.first);
            Serial.print(next_location.second);
            Serial.println("<-------------error message------------>");
        }
        else
        {
            Serial.print("invalid algorithm");
            return;
        }

        rotate_translate_drive_command(relative_movement);
        current_location = next_location;
    }
}

//------------------------------------
// Drive part
//------------------------------------
void drive_command(int relative_movement){ //  :: Exploration
    Serial.println("<inside drive commaand>");
    Serial.println(relative_movement);
    if (relative_movement == 10)
    {
        // 90 up
        Serial.println("Remain Still");
    }
    else if (relative_movement == 11)
    {
        // -90 down
        Serial.println("11");
        roverRotateToTarget(M_PI, 0.5);
        roverWait();
        Serial.println("Go to opposite direction");
    }
    else if (relative_movement == 12)
    {
        // 0 right
        Serial.println("12");
        roverRotateToTarget(-M_PI / 2, 0.5);
        roverWait();
        Serial.println("Rotate right by 90 degree done");
    }
    else if (relative_movement == 13)
    {
        // 180 left
        Serial.println("13");
        roverRotateToTarget( M_PI / 2, 0.5);
        roverWait();
        // rover.translateToTargt(100);
        Serial.println("Rotate left by 90 degree done");
    }
}

int relative_rotation(int original_car_angle, int target_angle){
    Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
    if (original_car_angle == 10)
    {
        // 0
        return target_angle;
    }
    else if (original_car_angle == 11)
    {
        // originally at -90
        if (target_angle == 10)
        {
            return 11;
        }
        else if (target_angle == 11)
        {
            return 10;
        }
        else if (target_angle == 12)
        {
            return 13;
        }
        else if (target_angle == 13)
        {
            return 12;
        }
        else
        {
            return 0;
        }
    }

    else if (original_car_angle == 12)
    {
        // originally toward right
        if (target_angle == 10)
        {
            return 13;
        }
        else if (target_angle == 11)
        {
            return 12;
        }
        else if (target_angle == 12)
        {
            return 10;
        }
        else if (target_angle == 13)
        {
            return 11;
        }
        else
        {
            return 0;
        }
    }

    else if (original_car_angle == 13)
    {
        // originally at -90
        if (target_angle == 10)
        {
            return 12;
        }
        else if (target_angle == 11)
        {
            return 13;
        }
        else if (target_angle == 12)
        {
            return 11;
        }
        else if (target_angle == 13)
        {
            return 10;
        }
        else
        {
            return 0;
        }
    }
}

void pixel_rotation(int pixel, bool stop){
    if (pixel < 100101100)
    {
        Serial.println("<><><><><><><><><><><><><><><>Rotate Left<><><><><><><><><><><><><><><>");
        roverRotate(0.2);
    }
    else if (pixel > 101010100)
    {
        Serial.println("<><><><><><><><><><><><><><><>Rotate Right<><><><><><><><><><><><><><><>");
        roverRotate(-0.2);
    }
}

void rotate_translate_drive_command(int relative_movement){ // :: Astar
    if (relative_movement == 10)
    {
        // 90 up
        roverTranslateToTarget(200,0.8);
        roverWait();
        Serial.println("Remain Still");
    }
    else if (relative_movement == 11)
    {
        // -90 down
        roverRotateToTarget(M_PI, 0.5);
        roverWait();
        roverTranslateToTarget(200,0.8);
        roverWait();
        Serial.println("Go to opposite direction");
    }
    else if (relative_movement == 12)
    {
        // 0 right
        roverRotateToTarget(-M_PI / 2, 0.5);
        roverWait();
        roverTranslateToTarget(200,0.8);
        roverWait();
        Serial.println("Rotate right by 90 degree done");
    }
    else if (relative_movement == 13)
    {
        // 180 left
        roverRotateToTarget(M_PI / 2, 0.5);
        roverWait();
        roverTranslateToTarget(200,0.8);
        roverWait();
        Serial.println("Rotate left by 90 degree done");
    }
}
