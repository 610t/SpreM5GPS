/*
  Function CoordinateToString() and the logic of caluclating GPGGA checksum is based on gnss_nmea.cpp.
  See license below.
 */

/*
 *  gnss_nmea.cpp - NMEA's GGA sentence
 *  Copyright 2017 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <SD.h>
#include <M5Unified.h>

#define STRING_BUFFER_SIZE 1024
#define LOGGER_MODE_MAX 2
bool logger_enable = false;
int logger_mode = 0;  // 0:CSV, 1:NEMA
bool sd_exist = false;

enum logger_mode_enum {
  logger_mode_csv,
  logger_mode_nmea,
};

bool data_update = false;  // Is data updated?

char gps_log_filename[STRING_BUFFER_SIZE] = "/GPS/gps_log.csv";

#define CORIDNATE_TYPE_LATITUDE 0  /**< Coordinate type latitude */
#define CORIDNATE_TYPE_LONGITUDE 1 /**< Coordinate type longitude */

static void CoordinateToString(char *pBuffer, int length, double Coordinate,
                               unsigned int cordinate_type) {
  double tmp;
  int Degree;
  int Minute;
  int Minute2;
  char direction;
  unsigned char fixeddig;

  const static struct {
    unsigned char fixeddigit;
    char dir[2];
  } CordInfo[] = {
    { .fixeddigit = 2, .dir = { 'N', 'S' } },
    { .fixeddigit = 3, .dir = { 'E', 'W' } },
  };

  if (cordinate_type > CORIDNATE_TYPE_LONGITUDE) {
    snprintf(pBuffer, length, ",,");
    return;
  }

  if (Coordinate >= 0.0) {
    tmp = Coordinate;
    direction = CordInfo[cordinate_type].dir[0];
  } else {
    tmp = -Coordinate;
    direction = CordInfo[cordinate_type].dir[1];
  }
  fixeddig = CordInfo[cordinate_type].fixeddigit;
  Degree = (int)tmp;
  tmp = (tmp - (double)Degree) * 60 + 0.00005;
  Minute = (int)tmp;
  tmp = (tmp - (double)Minute) * 10000;
  Minute2 = (int)tmp;

  snprintf(pBuffer, length, "%0*d%02d.%07d,%c", fixeddig, Degree, Minute, Minute2, direction);
}

void setup() {
  Serial.begin(115200);   // for debug output
  Serial2.begin(115200);  // get data from SPRESENSE

  M5.begin();
  M5.setLogDisplayIndex(0);
  M5.Display.setTextScroll(true);
  M5.Lcd.setTextSize(2);
  M5.Log.printf("SpreM5GPSense start!!\n");

#define MAX_SD_WAIT 5
  // Initialize SD
  //   If SD can't find MAX_SD_WAIT, run viewer only mode.
  int i;
  for (i = 0; i < MAX_SD_WAIT; i++) {
    if (SD.begin(GPIO_NUM_4, SPI, 15000000)) {
      break;
    }
    M5.Log.printf("SD Wait...\n");
    delay(500);
  }

  if (i != MAX_SD_WAIT) {
    M5.Log.printf("* Found SD\n");
    sd_exist = true;
    if (!SD.exists("/GPS")) {
      SD.mkdir("/GPS");
    }
  } else {
    M5.Log.printf("* Run viewer only mode\n");
    Serial2.read();  // Discard all incoming data.
    sd_exist = false;
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
  if (M5.BtnA.wasClicked() && logger_enable == false) {
    logger_enable = true;
    M5.Power.setLed(255);
    M5.Log.printf("* Logger mode on.\n");
  } else if (M5.BtnB.wasClicked() && logger_enable == true) {
    logger_enable = false;
    M5.Power.setLed(0);
    M5.Log.printf("* Logger mode off.\n");
  } else if (M5.BtnC.wasClicked()) {
    logger_mode++;
    if (logger_mode >= LOGGER_MODE_MAX) {
      logger_mode = 0;
    }
    switch (logger_mode) {
      case logger_mode_csv:
        snprintf(gps_log_filename, STRING_BUFFER_SIZE, "/GPS/gps_log.csv");
        break;
      case logger_mode_nmea:
        snprintf(gps_log_filename, STRING_BUFFER_SIZE, "/GPS/gps_log.nema");
        break;
      default:
        break;
    }
  }

  // GPS data
  String date, time;
  int numSat, numSatCalc;
  float lat, lon;
  float hdop;
  float alt;
  bool fix_state;

  int av = Serial2.available();
  if (av) {
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
  }
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
    } else if (line.startsWith("numSatCalc:")) {
      numSatCalc = getIntValue(line, "numSatCalc:");
    } else if (line.startsWith("Lat:")) {
      lat = getFloatValue(line, "Lat:");
    } else if (line.startsWith("Lon:")) {
      lon = getFloatValue(line, "Lon:");
    } else if (line.startsWith("alt:")) {
      alt = getFloatValue(line, "alt:");
    } else if (line.startsWith("HDOP:")) {
      hdop = getFloatValue(line, "HDOP:");
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
    data_update = false;
    if (logger_enable && sd_exist) {
      // Write GPS data to SD
      if (fix_state) {
        char StringBuffer[STRING_BUFFER_SIZE];
        String gps_str;


        switch (logger_mode) {
          case logger_mode_csv:
            {
              // Time
              int hh = (time.substring(0, 1)).toInt();
              int mm = (time.substring(2, 3)).toInt();
              int ss = (time.substring(4, 5)).toInt();
              int ms = (time.substring(7, 8)).toInt();

              snprintf(StringBuffer, STRING_BUFFER_SIZE, "%s,%02d:%02d:%02d.%02d,%f,n,%f,e,%f,%d,%f\n", date, hh, mm, ss, ms, lat, lon, alt, numSatCalc, hdop);
              gps_str = StringBuffer;
            }
            break;
          case logger_mode_nmea:
            {
              char latStr[STRING_BUFFER_SIZE];
              char lonStr[STRING_BUFFER_SIZE];

              CoordinateToString(latStr, STRING_BUFFER_SIZE, lat, CORIDNATE_TYPE_LATITUDE);
              CoordinateToString(lonStr, STRING_BUFFER_SIZE, lon, CORIDNATE_TYPE_LONGITUDE);

              snprintf(StringBuffer, STRING_BUFFER_SIZE, "$GPGGA,%s,%s,%s,1,%02d,%.1f,%.1f,M,34.05,M,1.0,512*", time.c_str(), latStr, lonStr, numSatCalc, hdop, alt);
              gps_str = StringBuffer;

              // Calculate checksum: based on gnss_nmea.cpp.
              unsigned short CheckSum = 0;
              {
                int cnt;
                const char *pStrDest = gps_str.c_str();

                /* Calculate checksum as xor of characters. */
                for (cnt = 1; pStrDest[cnt] != 0x00; cnt++) {
                  CheckSum = CheckSum ^ pStrDest[cnt];
                }
              }
              snprintf(StringBuffer, STRING_BUFFER_SIZE, "%02X\r\n", CheckSum);
              gps_str += StringBuffer;
            }
            break;
          default:
            break;
        }

        // Output to SD
        File GPSFile = SD.open(gps_log_filename, FILE_APPEND);
        GPSFile.printf(gps_str.c_str());
        GPSFile.close();

        // Display at M5Stack
        M5.Log.printf(gps_str.c_str());
        switch (logger_mode) {
          case logger_mode_csv:
            M5.Log.printf("Mode:CSV\n");
            break;
          case logger_mode_nmea:
            M5.Log.printf("Mode:NMEA\n");
            break;
          default:
            break;
        }
      }
    }
  }
}