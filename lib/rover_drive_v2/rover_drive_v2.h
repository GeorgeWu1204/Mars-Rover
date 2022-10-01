#ifndef ROVER_DRIVE_H
#define ROVER_DRIVE_H

void roverBegin();

void roverStop();

void roverWait();

void roverPause();

void roverResume();

void roverTranslate(float v);

void roverRotate(float omega);

void roverTranslateToTarget(float rtarget, float v);

void roverRotateToTarget(float phitarget, float omega);

void roverRotateBack(float omega);

void roverMoveToTarget(float xtarget, float ytarget, float v, float omega);

float getRoverX();

float getRoverY();

float getRoverTheta(bool degrees);

float getRoverR();

float getRoverPhi(bool degrees);

void roverResetGlobalCoords();

#endif
