// #define BT_ON

#include "dep/bluetooth.h"
#include "dep/boardLed.h"
#include "dep/ccd.h"
#include "dep/color.h"
#include "dep/commandParser.h"
#include "dep/motor.h"
#include "dep/oled.h"
#include "dep/pid.h"
#include "dep/pinouts.h"
#include "dep/servo.h"

TaskHandle_t Task1Handle;
TaskHandle_t Task2Handle;

int command = -1;

pid angelPID(1, 0, 0.1);

void setup() {
  Serial.begin(115200);

  pinoutInitBoardLed();
  initColor();
  initCCD();
  initServo();
  initMotor();
  initBluetooth();
  initOled();

  assignTasks();

  pinMode(PINOUT_MOTOR_ON, INPUT_PULLDOWN);
}

void assignTasks() {
  xTaskCreatePinnedToCore(Task1,        // Task function
                          "Task1",      // Task name
                          2000,         // Stack size
                          NULL,         // Parameter
                          1,            // Priority
                          &Task1Handle, // Task handle to keep track of created task
                          0             // Core ID: 0:
  );

  xTaskCreatePinnedToCore(Task2,        // Task function
                          "Task2",      // Task name
                          2000,         // Stack size
                          NULL,         // Parameter
                          1,            // Priority
                          &Task2Handle, // Task handle to keep track of created task
                          1             // Core ID: 0:
  );
}

void autoTrack(int bestExplosureTime, float minThrehold) {
  bool motorEnable = false;
  if (digitalRead(PINOUT_MOTOR_ON)) {
    motorEnable = true;
  }

  int trackMidPixel   = 0;
  float usingThrehold = 0;
  int trackStatus     = 0;
  processCCD(trackMidPixel, usingThrehold, trackStatus, bestExplosureTime, minThrehold, true);

  oledPrint(usingThrehold, "Thre", 2);

  switch (trackStatus) {
  case STATUS_NORMAL:
    // Show status
    boardLedOff();
    oledPrint("tracking", 1);

    // Get val
    servoWritePixel(angelPID.update(trackMidPixel - 64) + 64);
    break;

  case STATUS_HIGH_DL:
  case STATUS_NO_TRACK:
    // Show status
    boardLedOn();
    if (trackStatus == STATUS_HIGH_DL)
      oledPrint("high ratio", 1);
    if (trackStatus == STATUS_HIGH_DL)
      oledPrint("no track", 1);

    servoWritePixel(127);
    break;
  }

  if (motorEnable) {
    int currentPower   = 0;
    float currentSpeed = 0;

    motorForward(0.6, currentPower, currentSpeed);
    Serial.print("power: ");
    Serial.print(currentPower);
    Serial.print(" ");
    Serial.print("speed: ");
    Serial.println(currentSpeed);

  } else {
    motorIdle();
  }
}

// This loop is automatically assigned to Core 1, so block it manually
void loop() { delay(1000); }

void Task1(void* pvParameters) {
  for (;;) {
    if (Serial.available()) {
      btSend(Serial.read());
    }

    command = btRecieve();
    vTaskDelay(20);
  }
}

void mainLoop1() {
  display.clearDisplay();

  int bestExplosureTime = 0;
  float minThrehold     = 0;
  bool cameraIsBlocked  = false;
  getBestExplosureTime(bestExplosureTime, minThrehold, cameraIsBlocked, false);

  Serial.println("----------------------------------------");
  if (cameraIsBlocked) {
    Serial.print("Best explosure time: ");
    Serial.print(bestExplosureTime);
    Serial.print(" with minimum ratio: ");
    Serial.println(minThrehold);
    Serial.println("Camera is blocked");
    Serial.println("Bluetooth mode activated");
  } else {
    Serial.print("Best explosure time: ");
    Serial.print(bestExplosureTime);
    Serial.print(" with minimum ratio: ");
    Serial.println(minThrehold);
    Serial.println("Tracking mode activated");
  }
  Serial.println("----------------------------------------");

  oledPrint(bestExplosureTime, "EPL(s)", 0);
  oledPrint(minThrehold, "RTO(%)", 1);
  oledFlush();
  delay(3000);
  display.clearDisplay();

  if (cameraIsBlocked) {
    oledPrint("CAM BLOCKED", 0);
    oledFlush();
    delay(1000);

    display.clearDisplay();
    oledPrint("BT MODE", 1);
    oledFlush();

    for (;;) {
      parseCommands(command);
    }
  } else {
    oledPrint("TRACK MODE", 1);
    oledFlush();
    delay(1000);

    for (;;) {
      display.clearDisplay();

      autoTrack(bestExplosureTime, minThrehold);
      oledFlush();
    }
  }
}

void testLoop() {
  getColor();
  delay(3000);
//   testColorLoop();
}

void Task2(void* pvParameters) {
  // Greeting from core 1
  display.clearDisplay();
  oledPrint("Hello!", 0);
  oledFlush();

  Serial.println("Hello!");

  for (;;) {
    testLoop();
  }
}