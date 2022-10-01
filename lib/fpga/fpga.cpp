
#ifndef FPGA_H
#define FPGA_H

#include <Arduino.h>
#include <SPI.h>
#include <bitset>
#include <map>
#include <bits/stdc++.h>
#include<fpga.h>
#include <string>


// decode logic for distance :: "0"
void fpga::distance_decode(std::string received_message, int &colour, int &distance)
{
  colour = std::stoi(received_message.substr(1, 4));
  if (received_message.at(5) == '1')
  {
    // distance case :: nothing is detected;
    distance = 0;
  }
  else
  {
    distance = std::stoi(received_message.substr(5, 15));
  }
}
// decode logic for pixel ::  "1"
void fpga::pixel_decode(std::string received_message, int &colour, int &pixel)
{
  Serial.println("PIXEL");
  colour = std::stoi(received_message.substr(1, 4));
  Serial.println(colour);
  Serial.println(received_message.substr(5, 15).c_str());
  pixel = std::stoi(received_message.substr(5, 15));
  Serial.println(pixel);
  Serial.println("finish pixel_decode ");
}



#endif