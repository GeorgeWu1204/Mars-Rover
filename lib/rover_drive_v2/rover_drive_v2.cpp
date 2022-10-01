//------------------------
// DRIVE SUBSYSTEM LIBRARY
//------------------------

#include <Arduino.h>
#include <SPI.h>
#include <rover_drive_v2.h>
#include <cmath>

enum roverstate { // rover state machine

  idle,                 // brake rover and wait for next instruction
  paused,               // pause current instruction and remember it
  resume,               // resume previous paused instruction
  rotation,             // rotate rover until stop requested
  translation,          // translate rover until stop requested
  rotationToTarget,     // rotate rover to given angle
  translationToTarget,  // translate rover to given distance
  rotationBack,         // rotate rover back to previous angle
  movementToTarget      // move rover to xy-coordinate

};

volatile roverstate state = idle; // rover state machine state variable
volatile roverstate pausestate;   // state variable to remember previous instruction

volatile float x = 0;     // rover global x coordinate, never reset unless requested
volatile float y = 0;     // rover global y coordinate, never reset unless requested
volatile float theta = 0; // rover global theta angle, never reset unless requested

volatile float r = 0;     // rover local r distance, reset at the end of each movement
volatile float phi = 0;   // rover local phi angle, reset at the end of each movement

volatile float r0;        // variable to remember previous r distance
volatile float phi0;      // variable to remember previous phi angle

float dx;   // rover global x coordinate change
float dy;   // rover global y coordinate change

float dr;   // rover local r distance change
float dphi; // rover local phi angle change

int squal;  // surface quality seen by optical flow sensor

float opticalFlowScale = 0.2; // conversion factor from unit optical flow to millimeters
float distanceToCenter = 120; // distance from optical flow sensor to rover rotation center

float Pphi = 1;   // P gain for angle control during translation
float Pr = 0.01;  // P gain for distance control during rotation

volatile int sgn;       // sign of direction of rover movement
volatile float target;  // movement target
volatile float speed;   // movement speed

volatile float target2; // second target variable
volatile float speed2;  // second speed variable
bool firstStep = true;  // true during first step

TaskHandle_t driveStartTask;  // drive subsystem setup task handle
TaskHandle_t driveTask;       // drive subsystem loop task handle

SemaphoreHandle_t driveSemaphore; // semaphore handle to check for movement completion

int Ts = 20; // sampling time in milliseconds

#define SQUALPIN 26 // debug pin, LED lights up when surface quality is low
#define STOPPIN 25  // debug pin, all rover movements are stopped when button is pressed
#define TIMEPIN 33  // debug pin, goes high when drive subsystem us using core resources

float principalAngle(float angle) { // convert angle to principal angle

  return std::atan2(std::sin(angle), std::cos(angle));

}

//---------------------------------------------------------------------------
// ESP32 pins, parameters and addresses of optical flow sensor, DO NOT CHANGE
//---------------------------------------------------------------------------

#define PIN_SS      5
#define PIN_MISO    19
#define PIN_MOSI    23
#define PIN_SCK     18

#define PIN_MOUSECAM_RESET  22
#define PIN_MOUSECAM_CS     5

#define ADNS3080_PIXELS_X   30
#define ADNS3080_PIXELS_Y   30

#define ADNS3080_PRODUCT_ID                     0x00
#define ADNS3080_REVISION_ID                    0x01
#define ADNS3080_MOTION                         0x02
#define ADNS3080_DELTA_X                        0x03
#define ADNS3080_DELTA_Y                        0x04
#define ADNS3080_SQUAL                          0x05
#define ADNS3080_PIXEL_SUM                      0x06
#define ADNS3080_MAXIMUM_PIXEL                  0x07
#define ADNS3080_CONFIGURATION_BITS             0x0a
#define ADNS3080_EXTENDED_CONFIG                0x0b
#define ADNS3080_DATA_OUT_LOWER                 0x0c
#define ADNS3080_DATA_OUT_UPPER                 0x0d
#define ADNS3080_SHUTTER_LOWER                  0x0e
#define ADNS3080_SHUTTER_UPPER                  0x0f
#define ADNS3080_FRAME_PERIOD_LOWER             0x10
#define ADNS3080_FRAME_PERIOD_UPPER             0x11
#define ADNS3080_MOTION_CLEAR                   0x12
#define ADNS3080_FRAME_CAPTURE                  0x13
#define ADNS3080_SROM_ENABLE                    0x14
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_LOWER   0x19
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_UPPER   0x1a
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_LOWER   0x1b
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_UPPER   0x1c
#define ADNS3080_SHUTTER_MAX_BOUND_LOWER        0x1d
#define ADNS3080_SHUTTER_MAX_BOUND_UPPER        0x1e
#define ADNS3080_SROM_ID                        0x1f
#define ADNS3080_OBSERVATION                    0x3d
#define ADNS3080_INVERSE_PRODUCT_ID             0x3f
#define ADNS3080_PIXEL_BURST                    0x40
#define ADNS3080_MOTION_BURST                   0x50
#define ADNS3080_SROM_LOAD                      0x60

int motion;
int squalreg;
int dxreg;
int dyreg;
int shutter;
int max_pix;

//--------------------------------------------------------
// ESP32 pins of H-bridge for motor control, DO NOT CHANGE
//--------------------------------------------------------

#define PWMA  17
#define AI1   21
#define AI2   16

#define PWMB  2
#define BI1   4
#define BI2   27

//--------------------------------------------------------------
// Optical flow sensor functions already provided, DO NOT CHANGE
//--------------------------------------------------------------

int convTwosComp(int b) {

  if (b & 0x80) {
      b = -1 * ((b ^ 0xff) + 1);
  }
  return b;

}

void mousecam_reset() {

  digitalWrite(PIN_MOUSECAM_RESET,HIGH);
  delay(1);
  digitalWrite(PIN_MOUSECAM_RESET,LOW);
  delay(35);

}

void mousecam_init() {

  pinMode(PIN_MOUSECAM_RESET,OUTPUT);
  pinMode(PIN_MOUSECAM_CS,OUTPUT);

  digitalWrite(PIN_MOUSECAM_CS,HIGH);

  mousecam_reset();

}

void mousecam_write_reg(int reg, int val) {

  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg | 0x80);
  SPI.transfer(val);
  digitalWrite(PIN_MOUSECAM_CS,HIGH);
  delayMicroseconds(50);

}

int mousecam_read_reg(int reg) {

  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg);
  delayMicroseconds(75);
  int ret = SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS,HIGH);
  delayMicroseconds(1);
  return ret;

}

void mousecam_read_motion() {

  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(ADNS3080_MOTION_BURST);
  delayMicroseconds(75);
  motion =  SPI.transfer(0xff);
  dxreg =  SPI.transfer(0xff);
  dyreg =  SPI.transfer(0xff);
  squalreg =  SPI.transfer(0xff);
  shutter =  SPI.transfer(0xff)<<8;
  shutter |=  SPI.transfer(0xff);
  max_pix =  SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS,HIGH);
  delayMicroseconds(5);

}

//---------------------------------------
// Driving functions running on this core
//---------------------------------------

void driveStart(void * pvParameters) {  // set up drive subsystem

  pinMode(PIN_SS,OUTPUT);
  pinMode(PIN_MISO,INPUT);
  pinMode(PIN_MOSI,OUTPUT);
  pinMode(PIN_SCK,OUTPUT);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);

  mousecam_init();

  pinMode(AI1, OUTPUT);
  pinMode(AI2, OUTPUT);
  pinMode(BI1, OUTPUT);
  pinMode(BI2, OUTPUT);

  pinMode(SQUALPIN, OUTPUT);
  pinMode(STOPPIN, INPUT_PULLUP);
  pinMode(TIMEPIN, OUTPUT);

  ledcSetup(1,5000,12);
  ledcAttachPin(PWMA,1);
  ledcSetup(2,5000,12);
  ledcAttachPin(PWMB,2);

  int econfig = mousecam_read_reg(ADNS3080_EXTENDED_CONFIG);
  mousecam_write_reg(ADNS3080_EXTENDED_CONFIG, econfig | 0x01);

  driveSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(driveSemaphore);

  vTaskDelete(driveStartTask);

}

void measure() {  // take optical flow sensor measurements

    mousecam_read_motion();

    dr = convTwosComp(dyreg)*opticalFlowScale;
    dphi = convTwosComp(dxreg)*opticalFlowScale/distanceToCenter;

    dx = dr*std::cos(theta);
    dy = dr*std::sin(theta);

    x += dx;
    y += dy;
    theta += dphi;
    theta = principalAngle(theta);

    r += dr;
    phi += dphi;

    squal = squalreg*4;

}

void motorA(float v) {  // rotate rover left motor

  if (v < 0) {
    digitalWrite(AI1, HIGH);
    digitalWrite(AI2, LOW);
  }
  else {
    digitalWrite(AI1, LOW);
    digitalWrite(AI2, HIGH);
  }
  if (std::abs(v) > 1) {
    ledcWrite(1,4095);
  }
  else {
    ledcWrite(1,std::abs(v)*4095);
  }

}

void motorB(float v) {  // rotate rover right motor

  if (v < 0) {
    digitalWrite(BI1, LOW);
    digitalWrite(BI2, HIGH);
  }
  else {
    digitalWrite(BI1, HIGH);
    digitalWrite(BI2, LOW);
  }
  if (std::abs(v) > 1) {
    ledcWrite(2,4095);
  }
  else {
    ledcWrite(2,std::abs(v)*4095);
  }

}

void brake() {  // brake rover by stopping both motors

  digitalWrite(BI1, HIGH);
  digitalWrite(BI2, HIGH);
  digitalWrite(AI1, HIGH);
  digitalWrite(AI2, HIGH);

}

void translate(float v) { // continuously translate rover

  motorA(v+Pphi*phi);
  motorB(v-Pphi*phi);

}

void rotate(float omega) {  // continuously rotate rover

  motorA(-omega-Pr*r);
  motorB(omega-Pr*r);

  phi0 = phi;

}

void translateToTarget(float rtarget, float v) {  // translate to target distance

  motorA(sgn*v+Pphi*phi);
  motorB(sgn*v-Pphi*phi);

  if (sgn*(r+dr-rtarget) > 0) {

    brake();

    r = 0;
    phi = 0;

    xSemaphoreGive(driveSemaphore);
    state = idle;

  }

}

void rotateToTarget(float phitarget, float omega) { // rotate to target angle

  motorA(sgn*(-omega)-Pr*r);
  motorB(sgn*omega-Pr*r);

  if (sgn*(phi+dphi-phitarget) > 0) {

    brake();

    r = 0;
    phi = 0;

    xSemaphoreGive(driveSemaphore);
    state = idle;

  }

}

void moveToTarget(float rtarget, float phitarget, float v, float omega) {

  if (firstStep) {

    motorA(sgn*(-omega)-Pr*r);
    motorB(sgn*omega-Pr*r);

      if (sgn*(phi+dphi-phitarget) > 0) {

      brake();

      r = 0;
      phi = 0;

      firstStep = false;

      }

  }
  else {

    motorA(v+Pphi*phi);
    motorB(v-Pphi*phi);

    if (r+dr-rtarget > 0) {

      brake();

      r = 0;
      phi = 0;

      firstStep = true;

      xSemaphoreGive(driveSemaphore);
      state = idle;

    }

  }

}

void drive(void * pvParameters) {  // main loop for drive subsystem, ran at each sampling time

  TickType_t xLastWakeTime;
  const TickType_t xFrequency = Ts/portTICK_PERIOD_MS;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {

    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    digitalWrite(TIMEPIN, HIGH);

    measure();

    switch (state) {

      case idle:
      brake();
      break;

      case paused:
      brake();
      break;

      case translation:
      translate(speed);
      break;

      case rotation:
      rotate(speed);
      break;

      case translationToTarget:
      translateToTarget(target,speed);
      break;

      case rotationToTarget:
      rotateToTarget(target,speed);
      break;

      case rotationBack:
      rotateToTarget(target,speed);
      break;

      case movementToTarget:
      moveToTarget(target,target2,speed,speed2);
      break;

    }

    if (digitalRead(STOPPIN) == LOW) {

      brake();

      xSemaphoreGive(driveSemaphore);

      vTaskDelete(driveTask);

    }

    if (squal < 70) {

      digitalWrite(SQUALPIN, HIGH);

    }
    else {

      digitalWrite(SQUALPIN, LOW);

    }

    digitalWrite(TIMEPIN, LOW);

  }

}

//---------------------------------------------------------------
// Functions running on the other core to control drive subsystem
// --------------------------------------------------------------

void roverBegin() { // set up rover tasks on core

  xTaskCreatePinnedToCore(driveStart, "start", 10000, NULL, 2, &driveStartTask, 1); // setup task

  delay(50);  // wait for setup task to complete

  xTaskCreatePinnedToCore(drive, "drive", 10000, NULL, 1, &driveTask, 1); // loop task

}

void roverStop() {  // stops the current movement

  phi0 = phi; // remember angle to rotate back to if called

  r = 0;
  phi = 0;

  xSemaphoreGive(driveSemaphore); // give the semaphore back

  state = idle; // return to idle state

}

void roverWait() {  // wait until the current movement is complete

  while (true) {

    if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) { // wait until the semaphore is free

      xSemaphoreGive(driveSemaphore); // give back the semaphore immediately

      break;  // exit the waiting loop

    }

  }

}

void roverPause() { // pause the rover

  r0 = r;
  phi0 = phi;

  r = 0;
  phi = 0;

  pausestate = state; // remember the previous instruction

  xSemaphoreGive(driveSemaphore); // give back the semaphore

  state = paused; // enter the paused state

}

void roverResume() {  // resume the previous movements

  if (state == paused) {  // valid only if in paused state

    r = r0;
    phi = phi0;

    if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) {

      state = pausestate; // return to the previous instruction

    }

  }

}

void roverTranslate(float v) {  // continuously translate the rover

  if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) {

    speed = v;
    state = translation;

  }

}

void roverRotate(float omega) { // continuously rotate the rover

  if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) {
    
    speed = omega;
    state = rotation;

  }

}

void roverTranslateToTarget(float rtarget, float v) {  // translate to target distance

  if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) {

    if (rtarget < 0) {
      sgn = -1;
    }
    else {
      sgn = 1;
    }

    target = rtarget;
    speed = v;
    state = translationToTarget;

  }

}

void roverRotateToTarget(float phitarget, float omega) { // rotate to target angle

  if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) {

    if (phitarget < 0) {
      sgn = -1;
    }
    else {
      sgn = 1;
    }

    target = phitarget;
    speed = omega;
    state = rotationToTarget;

  }

}

void roverRotateBack(float omega) { // rotate back to previous angle

  if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) {

    if (phi0 > 0) {
      sgn = -1;
    }
    else {
      sgn = 1;
    }

    target = -phi0;
    speed = omega;
    state = rotationBack;

  }

}

void roverMoveToTarget(float xtarget, float ytarget, float v, float omega) {

  if (xSemaphoreTake(driveSemaphore, (TickType_t) 0) == pdTRUE) {

    target = std::hypot(xtarget-x, ytarget-y);
    target2 = principalAngle(std::atan2(ytarget-y, xtarget-x)-theta);
    speed = v;
    speed2 = omega;
    
    if (target2 < 0) {
      sgn = -1;
    }
    else {
      sgn = 1;
    }

    state = movementToTarget;

    Serial.println(target);
    Serial.println(target2);

  }

}

float getRoverX() { // get rover global x coordinate

  return x;

}

float getRoverY() { // get rover global y coordinate

  return y;
  
}

float getRoverTheta(bool degrees) { // get rover global theta angle

  if (degrees) return theta*180/PI;
  else return theta;
  
}

float getRoverR() { // get rover local r distance

  return r;

}

float getRoverPhi(bool degrees) { // get rover local phi angle

  if (degrees) return phi*180/PI;
  else return phi;

}

void roverResetGlobalCoords() {

  x = 0;
  y = 0;
  theta = 0;

}
