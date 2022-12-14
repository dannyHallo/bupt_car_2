#pragma once

// GPIO 34 - 39 can only be set as input mode and do not have software-enabled pullup or pulldown
// functions. ADC pins: 0, 2, 4, 12 - 15, 25 - 39

// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33

#define PINOUT_BOARD_LED_PIN 2

#define PINOUT_CCD_SI 14  // GPO
#define PINOUT_CCD_CLK 12 // GPO
#define PINOUT_CCD_AO 13  // ADC pin required

#define PINOUT_SERVO 18 // PWM pin required

#define PINOUT_LEFT_MOTOR_FRONT 27  // PWM pin required
#define PINOUT_LEFT_MOTOR_BACK 26   // PWM pin required
#define PINOUT_RIGHT_MOTOR_FRONT 33 // PWM pin required
#define PINOUT_RIGHT_MOTOR_BACK 25  // PWM pin required
#define PINOUT_LEFT_MOTOR_CODER_1
#define PINOUT_LEFT_MOTOR_CODER_2
#define PINOUT_RIGHT_MOTOR_CODER_1
#define PINOUT_RIGHT_MOTOR_CODER_2

#define PINOUT_SCL 22 // Oled & Color
#define PINOUT_SDA 21

#define PINOUT_E2A 19

#define PINOUT_MOTOR_ON 32 // Control