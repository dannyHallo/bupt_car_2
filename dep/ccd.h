#pragma once

#include "boardLed.h"
#include "math.h"
#include "oled.h"
#include "pinouts.h"

const int STATUS_NORMAL = 0;
const int STATUS_HIGH_DL = 1;
const int STATUS_NO_TRACK = 2;
const int STATUS_PLATFORM = 3;

// Hardware related
const int cNumPixels = 128;
const int cCountStart = 15;
const int cCountEnd = 126;

// Line detection
const int cEffectiveLineWidthMin = 10;

// Explosure time
const int cDefaultExplosureTime = 10;
const int cExplosureTimeStart = 10;
const int cExplosureTimeEnd = 80;
const int cExplosureTimePropagation = 5;

// Dark / light dynamic propagation
const float cThreholdSearchingPropagationInit = 0.01f;
const float cThreholdSearchingPropagation = 0.1f;
const float cDarkRatioAbnormalMin = 0.3f;
const float cDarkRatioAbnormalMax = 0.8f;

// Blocking condition
const float cMinMaxRatioDeltaBlocked = 0.6f;

// Solid Black Line Detection
const float cBlockingConditionRatio = 0.5f;

int linearData[cNumPixels]{};
bool binaryData[cNumPixels]{};
bool binaryOnehotData[cNumPixels]{};

int avgMarkingVal = 0;

void initCCD() {
  pinMode(PINOUT_CCD_SI,OUTPUT);
  pinMode(PINOUT_CCD_CLK,OUTPUT);
  pinMode(PINOUT_CCD_AO,INPUT);

  digitalWrite(PINOUT_CCD_SI,LOW);  // IDLE state
  digitalWrite(PINOUT_CCD_CLK,LOW); // IDLE state
}

void captrueCCD(int explosureTimeMs) {
  digitalWrite(PINOUT_CCD_CLK,LOW);
  delayMicroseconds(1);
  digitalWrite(PINOUT_CCD_SI,HIGH);
  delayMicroseconds(1);

  digitalWrite(PINOUT_CCD_CLK,HIGH);
  delayMicroseconds(1);
  digitalWrite(PINOUT_CCD_SI,LOW);
  delayMicroseconds(1);

  digitalWrite(PINOUT_CCD_CLK,LOW);
  delayMicroseconds(2);

  /* and now read the real image */

  for (int i = 0; i<cNumPixels; i++) {
    digitalWrite(PINOUT_CCD_CLK,HIGH);

    delayMicroseconds(2);
    linearData[i] = analogRead(PINOUT_CCD_AO); // 8-bit is enough
    digitalWrite(PINOUT_CCD_CLK,LOW);
    delayMicroseconds(2);
  }

  digitalWrite(PINOUT_CCD_CLK,HIGH);
  delayMicroseconds(2);

  for (int t = 0; t<explosureTimeMs*250; t++) {
    digitalWrite(PINOUT_CCD_CLK,LOW);
    delayMicroseconds(2);

    digitalWrite(PINOUT_CCD_CLK,HIGH);
    delayMicroseconds(2);
  }
}

void printCCDLinearData(int maxVal) {
  for (int i = 0; i<cNumPixels; i++) {
    int t = floor(float(linearData[i])/float(maxVal)*10.0f-0.1f);
    Serial.print(char(48+t));
  }
  Serial.println();
}

void printCCDBinaryRawData() {
  for (int i = 0; i<cNumPixels; i++) {
    char c = binaryData[i] ? 'x' : '-';
    Serial.print(c);
  }
  Serial.println();
}

void printCCDOneHotData() {
  for (int i = 0; i<cNumPixels; i++) {
    char c = binaryOnehotData[i] ? '^' : ' ';
    Serial.print(c);
  }
  Serial.println();
}

void processLinearVals(int& minVal,int& maxVal,int& avgVal,bool debug = false) {
  maxVal = 0;
  minVal = 1e6;

  for (int i = cCountStart; i<=cCountEnd; i++) {
    int currentVal = linearData[i];

    if (maxVal<currentVal)
      maxVal = currentVal;
    if (minVal>currentVal)
      minVal = currentVal;
  }
  avgVal = customRound(float(minVal+maxVal)/2.0f);

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

void linearToBinary(int minVal,int maxVal,float usingThrehold) {

  for (int i = 0; i<cNumPixels; i++) {
    binaryData[i] = false;
  }

  for (int i = cCountStart; i<=cCountEnd; i++) {
    binaryData[i] =
      (float(linearData[i])<(minVal+(maxVal-minVal)*usingThrehold)) ? true : false;
  }
}

void drawOneHot(int point) {
  for (int i = 0; i<cNumPixels; i++) {
    if (i==point) {
      binaryOnehotData[i] = true;
      continue;
    }
    binaryOnehotData[i] = false;
  }
}

int getTrackMidPixel() {
  int accumulatedDarkPixel = 0;

  int trackLeftPixel = -1;
  int trackRightPixel = -1;
  int trackMidPixel = -1;

  for (int i = cCountStart; i<=cCountEnd; i++) {
    bool currentPixel = binaryData[i];

    // Dark pixel
    if (currentPixel==true) {
      if (trackLeftPixel==-1) {
        trackLeftPixel = i;
      }
      accumulatedDarkPixel++;
    }

    // White pixel
    else {
      if (accumulatedDarkPixel>=customRound(cEffectiveLineWidthMin)) {
        trackRightPixel = i-1;
        break;
      }

      accumulatedDarkPixel = 0;
      trackLeftPixel = -1;
    }
  }

  // Reaches the end
  if (trackRightPixel==-1&&accumulatedDarkPixel>=customRound(cEffectiveLineWidthMin)) {
    trackRightPixel = cCountEnd;
  }

  // Nothing valid
  if (trackLeftPixel==-1||trackRightPixel==-1)
    return -1;

  // Get mid point
  if ((trackRightPixel-trackLeftPixel)%2==0) {
    trackMidPixel = customRound((trackRightPixel+trackLeftPixel)/2.0f);
  } else {
    int trackMidPixelCandidate1 = customRound((trackRightPixel+trackLeftPixel-1)/2.0f);
    int trackMidPixelCandidate2 = trackMidPixelCandidate1+1;

    trackMidPixel = (linearData[trackMidPixelCandidate1]<linearData[trackMidPixelCandidate2])
      ? trackMidPixelCandidate1
      : trackMidPixelCandidate2;
  }

  return trackMidPixel;
}

struct explosureRecord {
  int minVal;
  int maxVal;
  int avgVal;
  int explosureTime;
  float threhold;
  float contrast;
};

int bestAvg = 0;

void getBestExplosureTime(explosureRecord& bestRecord,bool& cameraIsBlocked,bool& bestAvailable,
  bool debug = false) {
  cameraIsBlocked = false;
  bestAvailable = false;

  // Clear previous val
  captrueCCD(cDefaultExplosureTime);

  int recordSize = (cExplosureTimeEnd-cExplosureTimeStart)/cExplosureTimePropagation+1;
  float threholdsForEachExplosureTime[recordSize];
  float minMaxRatioResults[recordSize];
  explosureRecord records[recordSize];

  // Test for each explosuring time
  for (uint8_t i = 0; i<recordSize; i++) {
    explosureRecord& thisRecord = records[i];

    int& thisExplosureTime = thisRecord.explosureTime;
    int& thisMinVal = thisRecord.minVal;
    int& thisMaxVal = thisRecord.maxVal;
    int& thisAvgVal = thisRecord.avgVal;
    float& thisThrehold = thisRecord.threhold;
    float& thisContrast = thisRecord.contrast;

    thisThrehold = -1;
    thisExplosureTime = cExplosureTimeStart+cExplosureTimePropagation*i;

    if (debug) {
      Serial.print("explosure time: ");
      Serial.println(thisExplosureTime);
    }

    // Capture
    captrueCCD(thisExplosureTime);
    captrueCCD(thisExplosureTime);

    processLinearVals(thisMinVal,thisMaxVal,thisAvgVal,debug);
    thisContrast = (thisMaxVal==0) ? 0 : float(thisMinVal)/float(thisMaxVal);

    // Find track dynamically
    for (float testThrehold = 0.0f; testThrehold<1.0f;
      testThrehold += cThreholdSearchingPropagationInit) {
      linearToBinary(thisMinVal,thisMaxVal,testThrehold);
      int trackMidPixel = getTrackMidPixel();

      if (trackMidPixel!=-1) {
        thisThrehold = testThrehold;

        if (debug) {
          drawOneHot(trackMidPixel);
          printCCDLinearData(thisMaxVal);
          printCCDBinaryRawData();
          printCCDOneHotData();
        }
        break;
      }
    }
    if (thisThrehold==-1&&debug)
      Serial.println("Failed to find threhold for this explosure time!");
  }

  // Find explosure time with minimum ratio
  float minThrehold = 1.0f;
  float minContrast = 1.0f;
  float maxContrast = 0.0f;

  for (uint8_t i = 0; i<recordSize; i++) {
    explosureRecord& thisRecord = records[i];

    int& thisExplosureTime = thisRecord.explosureTime;
    int& thisMinVal = thisRecord.minVal;
    int& thisMaxVal = thisRecord.maxVal;
    int& thisAvgVal = thisRecord.avgVal;
    float& thisThrehold = thisRecord.threhold;
    float& thisContrast = thisRecord.contrast;

    if (thisThrehold==-1)
      continue;

    Serial.print("explosure_time: ");
    Serial.print(thisExplosureTime);
    Serial.print("  threhold: ");
    Serial.print(thisThrehold);
    Serial.print("  contrast: ");
    Serial.println(thisContrast);

    if (thisThrehold<minThrehold) {
      minThrehold = thisThrehold;
      {
        bestRecord.explosureTime = thisExplosureTime;
        bestRecord.minVal = thisMinVal;
        bestRecord.maxVal = thisMaxVal;
        bestRecord.avgVal = thisAvgVal;
        bestRecord.threhold = thisThrehold;
        bestRecord.contrast = thisContrast;
      }
      bestAvailable = true;
    }

    if (thisContrast<minContrast)
      minContrast = thisContrast;

    if (thisContrast>maxContrast)
      maxContrast = thisContrast;
  }

  bestAvg = bestRecord.avgVal;
  Serial.print("bestAvg ");
  Serial.println(bestAvg);

  // Output
  cameraIsBlocked = !bestAvailable||
    (minContrast < 0.02&&maxContrast-minContrast > cMinMaxRatioDeltaBlocked);
}

void processCCD(int& trackMidPixel,float& usingThrehold,int& tracingStatus,int explosureTime,
  float minThrehold,bool debug = false) {

  int minVal = 0;
  int maxVal = 0;
  int avgVal = 0;

  tracingStatus = STATUS_NORMAL;

  // Capture
  captrueCCD(explosureTime);
  captrueCCD(explosureTime);

  // Parse linear data
  processLinearVals(minVal,maxVal,avgVal,debug);

  float solidRatio = (float)avgVal/(float)bestAvg;
  oledPrint(solidRatio,"solidRto",2);
  if (solidRatio<cBlockingConditionRatio) {
    boardLedOn();
    tracingStatus = STATUS_PLATFORM;
  } else {
    boardLedOff();
  }

  // Find track dynamically, starting from the best ratio
  for (float testThrehold = minThrehold; testThrehold<1.0f;
    testThrehold += cThreholdSearchingPropagation) {
    linearToBinary(minVal,maxVal,testThrehold);
    trackMidPixel = getTrackMidPixel();

    if (trackMidPixel!=-1) {
      // Compare ratio
      if (testThrehold>min(max(minThrehold*10,cDarkRatioAbnormalMin),cDarkRatioAbnormalMax)) {
        tracingStatus = STATUS_HIGH_DL;
      }

      if (debug)
        drawOneHot(trackMidPixel);

      usingThrehold = testThrehold;
      break;
    }
  }

  // Cannot find track
  if (trackMidPixel==-1) {
    tracingStatus = STATUS_NO_TRACK;
    usingThrehold = 0;
    return;
  }

  if (debug) {
    printCCDLinearData(maxVal);
    printCCDBinaryRawData();
    printCCDOneHotData();
  }

  // Map pixel
  trackMidPixel =
    customRound(map(float(trackMidPixel),float(cCountStart),float(cCountEnd),0.0f,128.0f));
}