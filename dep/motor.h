#pragma once

#include "../args.h"
#include "math.h"
#include "oled.h"
#include "pid.h"
#include "pinouts.h"
#include "speedControl.h"

#define PWM_CHANNEL_LEFT_MOTOR_FRONT 2
#define PWM_CHANNEL_LEFT_MOTOR_BACK 3
#define PWM_CHANNEL_RIGHT_MOTOR_FRONT 4
#define PWM_CHANNEL_RIGHT_MOTOR_BACK 5

const int cMotorResolution = 16; // Max: 16 bit
const int cDefaultPower    = 24000;
const int cMaximumPower    = 32000;

int currentPower;
float maxResolution = 0;
pid motorPID(speed_kp, speed_ki, speed_kd);

// init speed control, motor pins init, pwm init
void initMotor() {
  initSpeedControl();

  currentPower = cDefaultPower;

  ledcSetup(2, 1000, cMotorResolution); // Channel 1, 1kHz, 16 bit resolution
  ledcSetup(3, 1000, cMotorResolution); // Channel 2, 1kHz, 16 bit resolution
  ledcSetup(4, 1000, cMotorResolution); // Channel 3, 1kHz, 16 bit resolution
  ledcSetup(5, 1000, cMotorResolution); // Channel 4, 1kHz, 16 bit resolution

  ledcAttachPin(PINOUT_LEFT_MOTOR_FRONT, PWM_CHANNEL_LEFT_MOTOR_FRONT); // Channel 1: L motor front
  ledcAttachPin(PINOUT_LEFT_MOTOR_BACK, PWM_CHANNEL_LEFT_MOTOR_BACK);   // Channel 2: L motor back
  ledcAttachPin(PINOUT_RIGHT_MOTOR_FRONT,
                PWM_CHANNEL_RIGHT_MOTOR_FRONT);                         // Channel 3: R motor front
  ledcAttachPin(PINOUT_RIGHT_MOTOR_BACK, PWM_CHANNEL_RIGHT_MOTOR_BACK); // Channel 4: R motor back

  maxResolution = (1 << cMotorResolution) - 1;
}

// the basic function to control the motor
void motorControl(bool lFront, bool rFront, float lPower, float rPower) {
  clamp(lPower, 0.0f, float(maxResolution));
  clamp(rPower, 0.0f, float(maxResolution));

  if (lFront) {
    ledcWrite(PWM_CHANNEL_LEFT_MOTOR_FRONT, lPower);
    ledcWrite(PWM_CHANNEL_LEFT_MOTOR_BACK, 0);
  } else {
    ledcWrite(PWM_CHANNEL_LEFT_MOTOR_FRONT, 0);
    ledcWrite(PWM_CHANNEL_LEFT_MOTOR_BACK, lPower);
  }

  if (rFront) {
    ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_FRONT, rPower);
    ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_BACK, 0);
  } else {
    ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_FRONT, 0);
    ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_BACK, rPower);
  }
}

// Slow down slowly
void motorIdle() {
  ledcWrite(PWM_CHANNEL_LEFT_MOTOR_FRONT, 0);
  ledcWrite(PWM_CHANNEL_LEFT_MOTOR_BACK, 0);
  ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_FRONT, 0);
  ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_BACK, 0);
}

// Strong break
void motorBrake() {
  ledcWrite(PWM_CHANNEL_LEFT_MOTOR_FRONT, maxResolution);
  ledcWrite(PWM_CHANNEL_LEFT_MOTOR_BACK, maxResolution);
  ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_FRONT, maxResolution);
  ledcWrite(PWM_CHANNEL_RIGHT_MOTOR_BACK, maxResolution);
}

// this motor forward function uses fixed speed, instead of fixed power, to drive the car regardless
// the load
void motorForward(float aimSpeed) {
  if (aimSpeed == 0) {
    motorIdle();
    return;
  }

  float currentSpeed = getSpeed();
  float delta        = aimSpeed - currentSpeed;
  float p            = currentPower + motorPID.update(delta);

  p = min(p, cMaximumPower);

  clamp(currentPower, 0, cMaximumPower);
  motorControl(true, true, p, p);
}