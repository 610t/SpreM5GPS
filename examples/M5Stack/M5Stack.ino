#include <SD.h>
#include <M5Unified.h>

bool logger_mode = false;
bool sd_exist = false;

const static char gps_log_filename[] = "/GPS/gps_log.txt";

void setup() {
  Serial.begin(115200);   // for debug output
  Serial2.begin(115200);  // get data from SPRESENSE

  M5.begin();
  M5.setLogDisplayIndex(0);
  M5.Display.setTextScroll(true);
  M5.Lcd.setTextSize(2);
  M5.Log.printf("SpreM5GPSense start!!\n");

  // Initialize SD
  while (!SD.begin(GPIO_NUM_4, SPI, 15000000)) {
    M5.Log.printf("SD Wait...");
    delay(500);
  }
  sd_exist = true;
  if (!SD.exists("/GPS")) {
    SD.mkdir("/GPS");
  }
}

void loop() {
  char buf[1024] = { 0 };

  M5.update();

  // logger mode on/off
  if (M5.BtnA.wasClicked() && logger_mode == false) {
    logger_mode = true;
    M5.Power.setLed(255);
    M5.Log.printf("* Logger mode on.\n");
  } else if (M5.BtnB.wasClicked() && logger_mode == true) {
    logger_mode = false;
    M5.Power.setLed(0);
    M5.Log.printf("* Logger mode off.\n");
  }

  int av = Serial2.available();
  if (av > 0) {
    Serial2.readBytes(buf, av);

    Serial.printf("%s\n", buf);
    M5.Log.printf("%s\n", buf);

    // Log to SD
    if (logger_mode && sd_exist) {
      //// Get data value from buffer? ////

      // Write out NMEA GGA data to SD
      File GPSFile = SD.open(gps_log_filename, FILE_APPEND);
      GPSFile.printf("$GPGGA,....\n");
      GPSFile.close();
    }
  }
}