#include <Arduino.h>
#include <SPI.h>
#include <bitset>
#include <map>
#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>

#define xBound 11
#define yBound 17

class exploration
{
public:
    exploration();
    std::vector<double> locate_alien(std::vector<int> rover_position, std::vector<double> polar_coordinate, int current_car_altitude);
    int normal_round(double input);
    bool FPGA_detection(std::pair<std::string, std::vector<double>> &FPGA_ESP32_input);
    std::vector<int> next_step(int map[xBound][yBound], std::vector<int> xHistory, std::vector<int> yHistory, int &movement);
};