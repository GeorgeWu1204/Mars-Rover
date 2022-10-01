//fpga vision;
//exploration exploration_map;
//integration path;

//integration integrate;


// 	/* Description of the Grid-
// 	1--> The cell is not blocked
// 	0--> The cell is blocked */
	// int grid[11][17]
	// 	= { 
    //   { 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    //   { 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1},
    //   { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1},
    //   { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    //   { 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    //   { 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    //   { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    //   { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    //   { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    //   { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    //   { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    //   };

// 	// Source is the left-most bottom-most corner
//std::pair<int,int> src = std::make_pair(0, 0);

// 	// Destination is the left-most top-most corner
//std::pair<int, int> dest = std::make_pair(3, 5);

// 	aStarSearch(grid, src, dest);

// 	return (0);
// }


// void setup() {
//   //Serial.begin(115200);
//   //vision.start();
//   modeBegin(1);
  
// }

// void loop() {

  

//     //vision.fpga_loop();
   
//     //integrate.move_to_dest(grid, 10, src, dest);
// }

// WiFiClient client;

// TaskHandle_t movement;

// bool notstarted = true;

// void roverMovement(void * pvParameters) {

//   roverTranslate(0.5);

//   delay(5000);

//   roverStop();

//   roverRotateToTarget(1,0.3);

//   roverWait();

//   roverTranslateToTarget(200,0.8);

//   roverWait();

//   vTaskDelete(movement);

// }