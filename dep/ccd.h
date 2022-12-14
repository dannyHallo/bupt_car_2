#pragma once

#include "boardLed.h"
#include "math.h"
#include "oled.h"
#include "pinouts.h"

#define DEFAULT 0

// status definitions
const int STATUS_NORMAL   = 0;
const int STATUS_NO_TRACK = 1;
const int STATUS_PLATFORM = 2;

// Hardware related
const int cNumPixels  = 128;
const int cCountStart = 15;
const int cCountEnd   = 126;

// Line detection
const int cEffectiveLineWidthMin = 10;

// Explosure time
const int cDefaultExplosureTime = 10;
// const int cExplosureTimeStart       = 10;
// const int cExplosureTimeEnd         = 80;
const int cExplosureTimeStart       = 60;
const int cExplosureTimeEnd         = 140;
const int cExplosureTimePropagation = 10;

// Dark / light dynamic propagation
const float cThreholdSearchingPropagationInit = 0.01f;
const float cThreholdSearchingPropagation     = 0.1f;
const float cDarkRatioAbnormalMin             = 0.3f;
const float cDarkRatioAbnormalMax             = 0.8f;

// Blocking condition
const float cMinMaxRatioDeltaBlocked = 0.6f;

// Solid Black Line Detection
const float cBlockingConditionRatio = 0.5f;

// buffers allocated statically
int linearData[cNumPixels]{};
bool binaryData[cNumPixels]{};
bool binaryOnehotData[cNumPixels]{};

int avgMarkingVal = 0;

// the struct definition of the explosure record, contains is as below
struct explosureRecord {
  int minVal;
  int maxVal;
  int avgVal;
  int explosureTime;
  bool isValid;
  float contrast;
};

// initialization function
void initCCD() {
  pinMode(PINOUT_CCD_SI, OUTPUT);
  pinMode(PINOUT_CCD_CLK, OUTPUT);
  pinMode(PINOUT_CCD_AO, INPUT);

  digitalWrite(PINOUT_CCD_SI, LOW);  // IDLE state
  digitalWrite(PINOUT_CCD_CLK, LOW); // IDLE state
}

// the real function to capture the pixel values from the ccd hardware device
void captrueCCD(int explosureTimeMs) {
  digitalWrite(PINOUT_CCD_CLK, LOW);
  delayMicroseconds(1);
  digitalWrite(PINOUT_CCD_SI, HIGH);
  delayMicroseconds(1);

  digitalWrite(PINOUT_CCD_CLK, HIGH);
  delayMicroseconds(1);
  digitalWrite(PINOUT_CCD_SI, LOW);
  delayMicroseconds(1);

  digitalWrite(PINOUT_CCD_CLK, LOW);
  delayMicroseconds(2);

  /* and now read the real image */

  for (int i = 0; i < cNumPixels; i++) {
    digitalWrite(PINOUT_CCD_CLK, HIGH);

    delayMicroseconds(2);
    linearData[i] = analogRead(PINOUT_CCD_AO); // 8-bit is enough
    digitalWrite(PINOUT_CCD_CLK, LOW);
    delayMicroseconds(2);
  }

  digitalWrite(PINOUT_CCD_CLK, HIGH);
  delayMicroseconds(2);

  delay(explosureTimeMs);
}

// debug function: print the linear data to serial
void printCCDLinearData(int maxVal) {
  for (int i = 0; i < cNumPixels; i++) {
    int t = floor(float(linearData[i]) / float(maxVal) * 10.0f - 0.1f);
    Serial.print(char(48 + t));
  }
  Serial.println();
}

// debug function: print the binary data to serial
void printCCDBinaryRawData() {
  for (int i = 0; i < cNumPixels; i++) {
    char c = binaryData[i] ? 'x' : '-';
    Serial.print(c);
  }
  Serial.println();
}

// debug function: print the one ot data, indicating the center of the track, to serial
void printCCDOneHotData() {
  for (int i = 0; i < cNumPixels; i++) {
    char c = binaryOnehotData[i] ? '^' : ' ';
    Serial.print(c);
  }
  Serial.println();
}

// loop through all linear values, and get the maximum & minimum value from it
void parseLinearVals(int& minVal, int& maxVal, int& avgVal, bool debug = false) {
  maxVal = 0;
  minVal = 1e6;

  for (int i = cCountStart; i <= cCountEnd; i++) {
    int currentVal = linearData[i];

    if (maxVal < currentVal)
      maxVal = currentVal;
    if (minVal > currentVal)
      minVal = currentVal;
  }
  avgVal = customRound(float(minVal + maxVal) / 2.0f);

  if (debug) {
    Serial.print("avg: ");
    Serial.print(avgVal);
    Serial.print("    ");
    Serial.print("min: ");
    Serial.print(minVal);
    Serial.print("    ");
    Serial.print("max: ");
    Serial.println(maxVal);
  }
}

// get the black and white pixel num from the binary array
void parseBinaryVals(int& blackNum, int& whiteNum, int& totalNum, bool debug = false) {
  blackNum = whiteNum = 0;

  for (int i = cCountStart; i <= cCountEnd; i++) {
    bool currentVal = binaryData[i];

    if (currentVal) {
      blackNum++;
    } else {
      whiteNum++;
    }
  }

  totalNum = blackNum + whiteNum;
}

// the function to convert linear (raw) data to binary data
void linearToBinary(int minVal, int maxVal, int partingAvg = 0) {
  memset(binaryData, 0, sizeof(binaryData));

  partingAvg = (partingAvg == 0) ? (minVal + maxVal) / 2 : partingAvg;

  for (int i = cCountStart; i <= cCountEnd; i++) {
    binaryData[i] = (linearData[i] < partingAvg) ? true : false;
  }
}

// create one hot data from the point
void drawOneHot(int point) {
  for (int i = 0; i < cNumPixels; i++) {
    if (i == point) {
      binaryOnehotData[i] = true;
      continue;
    }
    binaryOnehotData[i] = false;
  }
}

// the core function of the ccd parsing logic:
int getTrackMidPixel() {
  int accumulatedDarkPixel = 0;

  int trackLeftPixel  = -1;
  int trackRightPixel = -1;
  int trackMidPixel   = -1;

  // parse binary data from left to right, and get the nearset black line from left, if the line
  // width meets the requirement, then record the mid point of this black line to be the track mid
  // pixel
  for (int i = cCountStart; i <= cCountEnd; i++) {
    bool currentPixel = binaryData[i];

    // Dark pixel
    if (currentPixel == true) {
      if (trackLeftPixel == -1) {
        trackLeftPixel = i;
      }
      accumulatedDarkPixel++;
    }

    // White pixel
    else {
      if (accumulatedDarkPixel >= customRound(cEffectiveLineWidthMin)) {
        trackRightPixel = i - 1;
        break;
      }

      accumulatedDarkPixel = 0;
      trackLeftPixel       = -1;
    }
  }

  // Reaches the end
  if (trackRightPixel == -1 && accumulatedDarkPixel >= customRound(cEffectiveLineWidthMin)) {
    trackRightPixel = cCountEnd;
  }

  // Nothing valid
  if (trackLeftPixel == -1 || trackRightPixel == -1)
    return -1;

  // Get mid point
  if ((trackRightPixel - trackLeftPixel) % 2 == 0) {
    trackMidPixel = customRound((trackRightPixel + trackLeftPixel) / 2.0f);
  } else {
    int trackMidPixelCandidate1 = customRound((trackRightPixel + trackLeftPixel - 1) / 2.0f);
    int trackMidPixelCandidate2 = trackMidPixelCandidate1 + 1;

    trackMidPixel = (linearData[trackMidPixelCandidate1] < linearData[trackMidPixelCandidate2])
                        ? trackMidPixelCandidate1
                        : trackMidPixelCandidate2;
  }

  return trackMidPixel;
}

// function excecuted once a power-on: get the best explosure time
// we get the best explosure time by testing every possible explosure time, and find the t with
// highest contrast (max / min)
void getBestExplosureTime(explosureRecord& bestRecord, bool& cameraIsBlocked, bool debug = false) {
  cameraIsBlocked    = false;
  bestRecord.isValid = false;

  int recordSize = (cExplosureTimeEnd - cExplosureTimeStart) / cExplosureTimePropagation + 1;
  float threholdsForEachExplosureTime[recordSize];
  float minMaxRatioResults[recordSize];
  explosureRecord records[recordSize];

  // Test for each explosuring time
  for (uint8_t i = 0; i < recordSize; i++) {
    explosureRecord& thisRecord = records[i];

    int& thisExplosureTime = thisRecord.explosureTime;
    int& thisMinVal        = thisRecord.minVal;
    int& thisMaxVal        = thisRecord.maxVal;
    int& thisAvgVal        = thisRecord.avgVal;
    bool& thisIsValid      = thisRecord.isValid;
    float& thisContrast    = thisRecord.contrast;

    thisIsValid       = false;
    thisExplosureTime = cExplosureTimeStart + cExplosureTimePropagation * i;

    if (debug) {
      Serial.print("explosure time: ");
      Serial.println(thisExplosureTime);
    }

    // Capture
    captrueCCD(thisExplosureTime);
    captrueCCD(0);

    parseLinearVals(thisMinVal, thisMaxVal, thisAvgVal, debug);
    thisContrast = (thisMaxVal == 0) ? 1 : float(thisMinVal) / float(thisMaxVal);

    linearToBinary(thisMinVal, thisMaxVal);
    int trackMidPixel = getTrackMidPixel();

    if (trackMidPixel != -1) {
      thisIsValid = true;

      if (debug) {
        drawOneHot(trackMidPixel);
        printCCDLinearData(thisMaxVal);
        printCCDBinaryRawData();
        printCCDOneHotData();
      }
    }

    if (!thisIsValid && debug)
      Serial.println("Failed to find threhold for this explosure time!");
  }

  // Loop through all records and select the one with best contrast (smallest)
  float minContrast = 1.0f;
  float maxContrast = 0.0f;

  for (uint8_t i = 0; i < recordSize; i++) {
    explosureRecord& thisRecord = records[i];

    int& thisExplosureTime = thisRecord.explosureTime;
    int& thisMinVal        = thisRecord.minVal;
    int& thisMaxVal        = thisRecord.maxVal;
    int& thisAvgVal        = thisRecord.avgVal;
    bool& thisIsValid      = thisRecord.isValid;
    float& thisContrast    = thisRecord.contrast;

    if (!thisIsValid)
      continue;

    Serial.print("explosure_time: ");
    Serial.print(thisExplosureTime);
    Serial.print("  contrast: ");
    Serial.println(thisContrast);

    if (thisContrast < minContrast) {
      minContrast = thisContrast;
      {
        bestRecord.explosureTime = thisExplosureTime;
        bestRecord.minVal        = thisMinVal;
        bestRecord.maxVal        = thisMaxVal;
        bestRecord.avgVal        = thisAvgVal;
        bestRecord.isValid       = true;
        bestRecord.contrast      = thisContrast;
      }
    }

    if (thisContrast > maxContrast)
      maxContrast = thisContrast;
  }

  // Output
  cameraIsBlocked = !bestRecord.isValid ||
                    (minContrast < 0.02 && maxContrast - minContrast > cMinMaxRatioDeltaBlocked);
}

int lastAvailableAverage = 0;

// the function to fetch track mid pixel, during normal tracking
void processCCD(int& trackMidPixel, int& tracingStatus, int explosureTime,
                bool resetAndExplosure = false, bool debug = false) {

  tracingStatus = STATUS_NORMAL;

  // Capture
  if (resetAndExplosure) {
    captrueCCD(explosureTime);
    captrueCCD(0);
  } else {
    captrueCCD(explosureTime);
  }

  // get min max avg values
  int minVal, maxVal, avgVal;
  parseLinearVals(minVal, maxVal, avgVal, debug);
  // use the values obtained above to convert the linear value to binary
  linearToBinary(minVal, maxVal, lastAvailableAverage);

  if (debug) {
    printCCDLinearData(maxVal);
    printCCDBinaryRawData();
  }

  // get the parsed black / white pixel num
  int blackNum, whiteNum, totalNum;
  parseBinaryVals(blackNum, whiteNum, totalNum);

  oledPrint("bl", blackNum, "wh", whiteNum, 2);

  // the discriminant condition whether the binary value indicate a solid black line, if so, the
  // tracing status is platform
  if (blackNum > int(totalNum * 0.7f)) {
    tracingStatus = STATUS_PLATFORM;
    return;
  }

  trackMidPixel = getTrackMidPixel();

  if (trackMidPixel != -1 && debug)
    drawOneHot(trackMidPixel);

  // fail to find the track mid pixel from the binary array, this means the track condition here is
  // ambiguous
  if (trackMidPixel == -1) {
    tracingStatus = STATUS_NO_TRACK;
    return;
  }

  // if (lastAvailableAverage == 0)
  lastAvailableAverage = avgVal;

  if (debug)
    printCCDOneHotData();

  // Pixel mapping
  trackMidPixel =
      customRound(map(float(trackMidPixel), float(cCountStart), float(cCountEnd), 0.0f, 128.0f));
}