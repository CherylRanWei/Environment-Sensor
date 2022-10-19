#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ILI9341.h>     // Hardware-specific library
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions

#include <SensirionI2CSen5x.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AS7341.h>

#include "TouchScreen.h"

#define YP A4 // must be an analog pin, use "An" notation! 
#define XM A7 // must be an analog pin, use "An" notation! 
#define YM A6 // can be a digital pin
#define XP A5 // can be a digital pin
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); 
#define X_MIN 750
#define X_MAX 325
#define Y_MIN 840
#define Y_MAX 240

//void SERCOM2_0_Handler() { Wire.onService(); }
//void SERCOM2_1_Handler() { Wire.onService(); }
//void SERCOM2_2_Handler() { Wire.onService(); }
//void SERCOM2_3_Handler() { Wire.onService(); }


#include "wiring_private.h"
#include "RTClib.h"

//#define Serial SERIAL_PORT_USBVIRTUAL
SensirionI2CSen5x sen5x;
Adafruit_AS7341 as7341;
RTC_PCF8523 myRTC;


// Comment out the next line to load from SPI/QSPI flash instead of SD card:
#define USE_SD_CARD

#define LIGHT_SENSOR   A2
#define TFT_D0         34 // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR         26 // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC         10 // Data/command pin
#define TFT_CS         11 // Chip-select pin
#define TFT_RST        24 // Reset pin
#define TFT_RD          9 // Read-strobe pin
#define TFT_BACKLIGHT  25
#define SD_CS          32

#if defined(USE_SD_CARD)
  SdFat                SD;         // SD card filesystem
  Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys
#else
  // SPI or QSPI flash filesystem (i.e. CIRCUITPY drive)
  #if defined(__SAMD51__) || defined(NRF52840_XXAA)
    Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS,
      PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
  #else
    #if (SPI_INTERFACES_COUNT == 1)
      Adafruit_FlashTransport_SPI flashTransport(SS, &SPI);
    #else
      Adafruit_FlashTransport_SPI flashTransport(SS1, &SPI1);
    #endif
  #endif
  Adafruit_SPIFlash    flash(&flashTransport);
  FatFileSystem        filesys;
  Adafruit_ImageReader reader(filesys); // Image-reader, pass in flash filesys
#endif


Adafruit_ILI9341       tft(tft8bitbus, TFT_D0, TFT_WR, TFT_DC, TFT_CS, TFT_RST, TFT_RD);
Adafruit_Image         img;             // An image loaded into RAM
int32_t                width  = 0,      // BMP image dimensions
                       height = 0;

File myFile;

//void requestHandler() {
//  Serial.println("Req: responding 4 bytes");
//  Wire.write("ping  ");
//}

void setup(void) {

//  ImageReturnCode stat; // Status from image-reading functions
  Wire.begin(0x08);
//  Serial.begin(9600);
//  while(!Serial);       // Wait for Serial Monitor before continuing
//  Wire.onRequest(requestHandler);
  // Turn on backlight (required on PyPortal)
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  tft.begin();          // Initialize screen
//  Serial.println("start");

  // Fill screen blue. Not a required step, this just shows that we're
  // successfully communicating with the screen.
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setTextWrap(true);


//  pinPeripheral(6, PIO_SERCOM_ALT);  // PAD[0]   //Assign SDA function to pin 0
//  pinPeripheral(5, PIO_SERCOM_ALT);  // PAD[1]   //Assign SCL function to pin 1


  /*************** SD CARD ***************/
//  Serial.print("SD Card...");
  if (!SD.begin(SD_CS)) {
      tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
      tft.setCursor(0, 0);
      tft.print("Card init. failed!");
      delay(100);
      tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
//    Serial.println("Card init. failed!");
//    Serial.println("FAILED");
    } 
  else {
      tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
      tft.setCursor(0, 0);
      tft.print("Card init OK!");
      delay(100);
      tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
//    Serial.println("OK!");
      
    }


  /*************** RTC ***************/
//  Serial.println("Finding RTC...");
  if (! myRTC.begin()) {
      tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
      tft.setCursor(0, 0);
      tft.print("Couldn't find RTC");
      delay(100);
      tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
//      Serial.println("Couldn't find RTC");
//      Serial.flush();
//      while (1){
//        Serial.println("Couldn't find RTC");
//        delay(10);
//      }
    }
    myRTC.start();

  /*************** SEN 5X ***************/
  sen5x.begin(Wire);
  uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();
    if (error) {
//        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
//        Serial.println(errorMessage);

        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.print(errorMessage);
        delay(200);
        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
    }  


  float tempOffset = 0.0;
    error = sen5x.setTemperatureOffsetSimple(tempOffset);
    if (error) {
//        Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
        errorToString(error, errorMessage, 256);
//        Serial.println(errorMessage);
        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.print(errorMessage);
        delay(200);
        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
  
    } else {
//        Serial.print("Temperature Offset set to ");
//        Serial.print(tempOffset);
//        Serial.println(" deg. Celsius");

          tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.print(tempOffset);
          delay(200);
          tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
    }


  error = sen5x.startMeasurement();
    if (error) {
//        Serial.print("Error trying to execute startMeasurement(): ");
        errorToString(error, errorMessage, 256);
//        Serial.println(errorMessage);

        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.print(errorMessage);
        delay(200);
        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
    }


  /*************** SPECTRUM ***************/    
    if (!as7341.begin()){
//      Serial.println("Could not find AS7341");
        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.print("Could not find AS7341");
        delay(200);
        tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
        
      while (1) { delay(10); }
    }
    
    as7341.setATIME(100);
    as7341.setASTEP(999);
    as7341.setGain(AS7341_GAIN_256X);

    
//  Serial.println("setup done");
  delay(2000); // Pause 2 seconds before moving on to loop()
}



int previousMillis = 0;

void loop() {
  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;
  uint16_t light = analogRead(LIGHT_SENSOR);
  TSPoint p = ts.getPoint();

  if(p.z > 800 || light > 300){
    digitalWrite(TFT_BACKLIGHT, HIGH);
    for(int i = 0; i < 30; i++){
      DateTime now = myRTC.now();
      char buf2[] = "YYYY/MM/DD-hh:mm:ss";

      sen5x.readMeasuredValues(
            massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
            massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
            noxIndex);
      uint16_t readings[12];
      float counts[12];

      if (!as7341.readAllChannels(readings)){
    //      Serial.println("Error reading all channels!");
          tft.fillRect(0, 0, 240, 15, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.print("Error reading all channels!");
          return;
        }

  
      for(uint8_t i = 0; i < 12; i++) {
          if(i == 4 || i == 5) continue;
          // we skip the first set of duplicate clear/NIR readings
          // (indices 4 and 5)
          counts[i] = as7341.toBasicCounts(readings[i]);
        }
  
      char output[200];
      snprintf(output, sizeof(output), "%s,%.04f,%.04f,%.04f,%.04f,%.04f,%.04f,%.04f,%.04f,%.04f,%.04f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", 
          now.toString(buf2),
          counts[0], counts[1], counts[2], counts[3], counts[6], counts[7], counts[8], counts[9], counts[10], counts[11],
          massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex);



      String temp = String(ambientTemperature) + " " + (char)247 + "C";
      String hum = String(ambientHumidity) + " %";
      String PM25 = String(massConcentrationPm2p5);
      String voc = String (vocIndex);
      /* printing that data to the Serial port in a meaningful format */ 
    //  Serial.println("************************************************************");
    //  Serial.println(output);

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
      // if the file opened okay, write to it:
      if (myFile) {
    //    Serial.print("Writing to test.txt...");
        myFile.println(output);
        // close the file:
        myFile.close();
    //    Serial.println("done.");
    delay(1);
      } 
    }
  }
  else{
      digitalWrite(TFT_BACKLIGHT, LOW);
  }



  
//  else {
    // if the file didn't open, print an error:
//    Serial.println("error opening test.txt");
//  }
}
