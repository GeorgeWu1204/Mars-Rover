// #ifndef NEW_INTEGRATION_H
// #define NEW_INTEGRATION_H
// #include <Arduino.h>
// #include <SPI.h>
// #include <bitset>
// #include <map>
// #include <bits/stdc++.h>
// #include <string>
// #include <iostream>
// #include <vector>
// #include <math.h>

// #define xBound 11
// #define yBound 17


// void start(void * param);

// typedef std::pair<int, int> Pair;

// typedef std::pair<double, std::pair<int, int>> pPair;

// struct cell {
//     int parent_i, parent_j;
//     // f = g + h
//     double f, g, h;
//     };

// // global variable

// // User

// bool execution_check();

// void modeBegin(int select_message);

// std::pair<std::string, std::vector<double>> getAlien_message();

// std::vector<int> getLeave_position();

// bool getcomplete_task();

// void stopAllTask();

// bool leaving_detected();

// bool get_tower_detected();

// std::map<std::string, std::vector<double>> get_complete_alien_storage();

// void battery_low_return();

// // Vision

// bool fpga_loop(std::map<std::string, std::vector<double>> &colour_map, bool start_detection);

// bool Vision_main_loop(int received, int special_code, std::map<std::string, std::vector<double>> &detected_alien_set, double& continue_rotate_angle, bool start_detection);

// void exploration_loop(void * param);

// void export_alien_location_map(void * param);

// void listen_map_alien(std::vector<int> rover_position, int map[11][17], std::map<std::string, std::vector<double>> &alien_storage, std::vector<std::string> wrong_detect_alien, int current_car_altitude, bool start_detection);

// //A-star

// void move_to_dest(volatile int initial_car_altitude, Pair initial_position, Pair destination);

// void aStar(void * param);

// //Drive part

// void pixel_rotation(int pixel, bool stop);

// void drive_command(int relative_movement);

// int relative_rotation(int original_car_angle, int target_angle);

// void rotate_translate_drive_command(int relative_movement);

// #endif