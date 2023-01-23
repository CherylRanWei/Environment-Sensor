//#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_AS7341.h>
//#include <Adafruit_SSD1306.h>
#include <Adafruit_ILI9341.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include "TouchScreen.h"


#define SD_CS  9   //4 // SD card select pin
#define TFT_DC 14  //9
#define TFT_CS 15  //10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define YP A2 // must be an analog pin, use "An" notation! 
#define XM A5 // must be an analog pin, use "An" notation! 
#define YM A4 // can be a digital pin
#define XP A3 // can be a digital pin
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); 
#define X_MIN 750
#define X_MAX 325
#define Y_MIN 840
#define Y_MAX 240

//#define MAXBUF_REQUIREMENT 48
//
//#if (defined(I2C_BUFFER_LENGTH) &&                 \
//     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
//    (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
//#define USE_PRODUCT_INFO
//#endif

SensirionI2CSen5x sen5x;
Adafruit_AS7341 as7341;

RTC_PCF8523 rtc;
File myFile;


void setup() {
  Wire.begin();
  Serial.begin(9600);
  tft.begin();

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);

  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");


  if (! rtc.begin()) {
    Serial.flush();
    while (1) {
      Serial.println("Couldn't find RTC");
      delay(10);
    }
  }
  rtc.start();
  sen5x.begin(Wire);


  uint16_t error;
  char errorMessage[256];
  error = sen5x.deviceReset();
  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  float tempOffset = 0.0;
  error = sen5x.setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Temperature Offset set to ");
    Serial.print(tempOffset);
    Serial.println(" deg. Celsius");
  }

  // Start Measurement
  error = sen5x.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }


  if (!as7341.begin()) {
    Serial.println("Could not find AS7341");
    while (1) {
      delay(10);
    }
  }

  as7341.setATIME(100);
  as7341.setASTEP(100);
  as7341.setGain(AS7341_GAIN_256X);
}

void loop() {
  DateTime now = rtc.now();
  char buf2[] = "YYYY/MM/DD-hh:mm:ss";

  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;

  sen5x.readMeasuredValues(
    massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
    massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
    noxIndex);

  //    String data1[]={String(massConcentrationPm1p0), String(massConcentrationPm2p5), String(massConcentrationPm4p0),
  //                    String(massConcentrationPm10p0), String(ambientHumidity),
  //                    String(ambientTemperature), String(vocIndex), String(noxIndex)};


  //    Serial.println("%10f", &massConcentrationPm1p0);

  uint16_t readings[12];
  float counts[12];


  if (!as7341.readAllChannels(readings)) {
    Serial.println("Error reading all channels!");
    return;
  }

  //    for(uint8_t i = 0; i < 12; i++) {
  //      if(i == 4 || i == 5) continue;
  //      // we skip the first set of duplicate clear/NIR readings
  //      // (indices 4 and 5)
  //      counts[i] = as7341.toBasicCounts(readings[i]);
  //    }

  char output[200];
  snprintf(output, sizeof(output), "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
           now.toString(buf2),
           readings[0], readings[1], readings[2], readings[3], readings[6], readings[7], readings[8], readings[9], readings[10], readings[11]);

  Serial.print(output);

  String temp = String(ambientTemperature) + " " + (char)247 + "C";
  String hum = String(ambientHumidity) + " %";
  String PM25 = String(massConcentrationPm2p5);
  String voc = String (vocIndex);
  //    Serial.println(String(ambientTemperature));
  //    Serial.print("MassConcentrationPm1p0:");
  Serial.print("\t");
  Serial.print(massConcentrationPm1p0);
  Serial.print("\t");
  //    Serial.print("MassConcentrationPm2p5:");
  Serial.print(massConcentrationPm2p5);
  Serial.print("\t");
  //    Serial.print("MassConcentrationPm4p0:");
  Serial.print(massConcentrationPm4p0);
  Serial.print("\t");
  //    Serial.print("MassConcentrationPm10p0:");
  Serial.print(massConcentrationPm10p0);
  Serial.print("\t");
  //    Serial.print("AmbientHumidity:");
  Serial.print(ambientHumidity);
  Serial.print("\t");
  //    Serial.print("AmbientTemperature:");
  Serial.print(ambientTemperature);
  Serial.print("\t");
  //    Serial.print("VocIndex:");
  Serial.print(vocIndex);
  Serial.print("\t");
  //    Serial.print("NoxIndex:");
  if (isnan(noxIndex)) {
    Serial.println("n/a");
  } else {
    Serial.println(noxIndex);
  }


  tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.print(now.toString(buf2));

  tft.fillRect(0, 50, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 50);
  tft.println("Temperature: ");
  tft.fillRect(0, 80, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 80);
  tft.println(temp);

  tft.fillRect(0, 110, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 110);
  tft.println("Humidity: ");
  tft.fillRect(0, 140, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 140);
  tft.println(hum);

  tft.fillRect(0, 180, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 180);
  tft.println("PM 2.5: ");
  tft.fillRect(0, 210, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 210);
  tft.println(PM25);

  tft.fillRect(0, 250, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 250);
  tft.println("VOC: ");
  tft.fillRect(0, 280, 240, 15, ILI9341_BLACK);
  tft.setCursor(0, 280);
  tft.println(voc);


  myFile = SD.open("test.txt", FILE_WRITE);

  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.print(output);
    myFile.print("\t");
    myFile.print(massConcentrationPm1p0);
    myFile.print("\t");
    //    Serial.print("MassConcentrationPm2p5:");
    myFile.print(massConcentrationPm2p5);
    myFile.print("\t");
    //    Serial.print("MassConcentrationPm4p0:");
    myFile.print(massConcentrationPm4p0);
    myFile.print("\t");
    //    Serial.print("MassConcentrationPm10p0:");
    myFile.print(massConcentrationPm10p0);
    myFile.print("\t");
    //    Serial.print("AmbientHumidity:");
    myFile.print(ambientHumidity);
    myFile.print("\t");
    //    Serial.print("AmbientTemperature:");
    myFile.print(ambientTemperature);
    myFile.print("\t");
    //    Serial.print("VocIndex:");
    myFile.print(vocIndex);
    myFile.print("\t");
    //    Serial.print("NoxIndex:");
    if (isnan(noxIndex)) {
      myFile.println("n/a");
    } else {
      myFile.println(noxIndex);
    }

    // close the file:
    myFile.close();
  }
}
