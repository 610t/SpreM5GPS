#include <SD.h>
#include <M5Unified.h>

bool logger_mode = false;
bool sd_exist = false;

bool data_update = false;  // Is data updated?

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

String getStrValue(String data, String pattern) {
  data.replace(pattern.c_str(), "");
  return (data);
}

float getFloatValue(String data, String pattern) {
  data.replace(pattern.c_str(), "");
  return (data.toFloat());
}

int getIntValue(String data, String pattern) {
  data.replace(pattern.c_str(), "");
  return (data.toInt());
}

void loop() {
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

  // GPS data
  String date, time;
  int numSat;
  float lat, lon;
  bool fix_state;

  int av = Serial2.available();
  while (av > 0) {
    String line = Serial2.readStringUntil('\n');
    av = Serial2.available();

    M5.Log.printf("%s\n", line.c_str());

    // Convert to NMEA GGA format
    if (line.startsWith("Date:")) {
      date = getStrValue(line, "Date:");
    } else if (line.startsWith("Time:")) {
      time = getStrValue(line, "Time:");
    } else if (line.startsWith("numSat:")) {
      numSat = getIntValue(line, "numSat:");
    } else if (line.startsWith("Lat:")) {
      lat = getFloatValue(line, "Lat:");
    } else if (line.startsWith("Lon:")) {
      lon = getFloatValue(line, "Lon:");
    } else if (line.startsWith("Fix:")) {
      String fix = getStrValue(line, "Fix:");
      if (fix.startsWith("Fix")) {
        fix_state = true;
      } else {
        fix_state = false;
      }
    }
    data_update = true;
  }

  // Log to SD
  if (data_update) {
    if (logger_mode && sd_exist) {
      // Write out NMEA GGA data to SD
      if (fix_state) {
#define STRING_BUFFER_SIZE 1024
        char StringBuffer[STRING_BUFFER_SIZE];

        snprintf(StringBuffer, STRING_BUFFER_SIZE, "$GPGGA,%s,%f,N,%f,E,4,%d,0,0,M,1.0,0*76\n", time.c_str(), lat, lon, numSat);
        M5.Log.printf(StringBuffer);

        // Output to SD
        File GPSFile = SD.open(gps_log_filename, FILE_APPEND);
        GPSFile.printf(StringBuffer);
        GPSFile.close();
      }
    }
    M5.Log.printf("---\n");  // Print separater
    data_update = false;
  }
}
