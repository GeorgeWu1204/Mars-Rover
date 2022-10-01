#ifndef EXPLORATION_H
#define EXPLORATION_H
#include <Arduino.h>
#include <SPI.h>
#include <bitset>
#include <map>
#include <exploration.h>
#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>

exploration::exploration(){     
    #define matrix_size
    #define xBound 11
    #define yBound 17
}

std::vector<double> exploration::locate_alien(std::vector<int> rover_position, std::vector<double> polar_coordinate, int current_car_altitude)
{
    double distance = polar_coordinate[0];
    double tilt_angle = polar_coordinate[1];
    double delta_x = 0.0;
    double delta_y = 0.0;
    delta_x = (distance/20) * sin(PI * tilt_angle / 180);
    delta_y = (distance/20) * cos(PI * tilt_angle / 180);
    std::vector<double> result;
    if(current_car_altitude == 10){
        Serial.println(" ");
        Serial.print("polar distance: ");
        Serial.print(distance);
        Serial.print(", ");
        Serial.println(tilt_angle);
        Serial.println("car altitude up");
        Serial.print("current car location ------>>>>");
        Serial.print(rover_position[0]);
        Serial.print(", ");
        Serial.println(rover_position[1]);
        Serial.print(delta_x);
        Serial.print(", ");
        Serial.println(delta_y);

        if((rover_position[0] + delta_x) <= xBound - 1 && (rover_position[1] + delta_y) <= xBound - 1){
        result.push_back(rover_position[0] + delta_x);
        result.push_back(rover_position[1] + delta_y);
        
        }
        else {
            Serial.println(" will go out of range");
            result.push_back(0);
            result.push_back(0);
        }
    }

    else if(current_car_altitude ==11 ){
        Serial.println("car altitude down 11 down");
        if((rover_position[0] - delta_x) >= 0 && (rover_position[1] - delta_y) >= 0 ){
            result.push_back(rover_position[0] - delta_x);
            result.push_back(rover_position[1] - delta_y);
           
        }
        else {
            Serial.println(" will go out of range");
        }
    }

    else if(current_car_altitude == 12){
        Serial.println("car altitude right 12 right");
        if((rover_position[0] + delta_y) <= xBound - 1 && (rover_position[1] - delta_y) >= 0 ){
            result.push_back(rover_position[0] + delta_y);
            result.push_back(rover_position[1] - delta_x);
          
        }
        else {
            Serial.println(" will go out of range");
        }
    }


    else if(current_car_altitude == 13){
        Serial.println("car altitude left 13 left");
        Serial.println("rover position");
        Serial.print(rover_position[0]);
        Serial.print(", ");
        Serial.println(rover_position[1]);

        if((rover_position[0] - delta_y) >= 0 && (rover_position[1] + delta_x) <= xBound - 1 ){
        result.push_back(rover_position[0] - delta_y);
        result.push_back(rover_position[1] + delta_x);
      
        }
        else {
            Serial.println(" will go out of range");
        }
    }
    
    return result;
}

int exploration::normal_round(double input)
{
    if (input - floor(input) < 0.5)
    {
        return floor(input);
    }
    else
    {
        return ceil(input);
    }
}


std::vector<int> exploration::next_step(int map[xBound][yBound], std::vector<int> xHistory, std::vector<int> yHistory, int& movement)
{
    int original_x = xHistory.back();
    int original_y = yHistory.back();
    int next_x, next_y;
    if (original_x - 1 >= 0 && map[original_x - 1][original_y] == 0)
    {
        // left
        next_x = original_x - 1;
        next_y = original_y;
        movement = 13;
    }
    
    else if (original_y - 1 >= 0 && map[original_x][original_y - 1] == 0)
    {
        // down
        next_x = original_x;
        next_y = original_y - 1;
        movement = 11;
    }
    else if (original_y + 1 < yBound && map[original_x][original_y + 1] == 0) 
    {
        // up
        next_x = original_x;
        next_y = original_y + 1;
        movement = 10;
    }
    else if (original_x + 1 < xBound && map[original_x + 1][original_y] == 0)
    {
        // right
        next_x = original_x + 1;
        next_y = original_y;
        movement = 12;
    }
    else
    {
        int current_priority = 1;
        while (true)
        {
            if (original_y + 1 < yBound && map[original_x][original_y + 1] == current_priority)
            {
                // up
                next_x = original_x;
                next_y = original_y + 1;
                movement = 10 ;
                break;
            }
            else if (original_x - 1 >= 0 && map[original_x - 1][original_y] == current_priority)
            {
                // left
                next_x = original_x - 1;
                next_y = original_y;
                movement = 13;
                break;
            }
            else if (original_y - 1 >= 0 and map[original_x][original_y - 1] == current_priority)
            {
                // down
                next_x = original_x;
                next_y = original_y - 1;
                movement = 11;
                break;
            }
            else if (original_x + 1 < xBound and map[original_x + 1][original_y] == current_priority)
            {
                // right
                next_x = original_x + 1;
                next_y = original_y;
                movement = 12;
                break;
            }
            else
            {
                current_priority += 1;
            }
        }
    }
    std::vector<int> result;
    result.clear();
    result.push_back(next_x);
    result.push_back(next_y);
    Serial.print("Next_X_position:  ");
    Serial.println(next_x);
    Serial.print("Next_Y_position   ");
    Serial.println(next_y);
    return result;
}

bool exploration::FPGA_detection(std::pair<std::string, std::vector<double>> &FPGA_ESP32_input)
{
  return false;
}

 #endif