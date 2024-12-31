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

bool logger_mode = false;
bool sd_exist = false;

bool data_update = false;  // Is data updated?

const static char gps_log_filename[] = "/GPS/gps_log.nmea";

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
    if (logger_mode && sd_exist) {
      // Write out NMEA GGA data to SD
      if (fix_state) {
#define STRING_BUFFER_SIZE 1024
        char StringBuffer[STRING_BUFFER_SIZE];
        char latStr[STRING_BUFFER_SIZE];
        char lonStr[STRING_BUFFER_SIZE];

        CoordinateToString(latStr, STRING_BUFFER_SIZE, lat, CORIDNATE_TYPE_LATITUDE);
        CoordinateToString(lonStr, STRING_BUFFER_SIZE, lon, CORIDNATE_TYPE_LONGITUDE);

        snprintf(StringBuffer, STRING_BUFFER_SIZE, "$GPGGA,%s,%s,%s,1,%02d,%.1f,%.1f,M,34.05,M,1.0,512*", time.c_str(), latStr, lonStr, numSatCalc, hdop, alt);
        String gga = StringBuffer;

        // Calculate checksum: based on gnss_nmea.cpp.
        unsigned short CheckSum = 0;
        {
          int cnt;
          const char *pStrDest = gga.c_str();

          /* Calculate checksum as xor of characters. */
          for (cnt = 1; pStrDest[cnt] != 0x00; cnt++) {
            CheckSum = CheckSum ^ pStrDest[cnt];
          }
        }
        snprintf(StringBuffer, STRING_BUFFER_SIZE, "%02X\r\n", CheckSum);
        gga += StringBuffer;

        M5.Log.printf(gga.c_str());

        // Output to SD
        File GPSFile = SD.open(gps_log_filename, FILE_APPEND);
        GPSFile.printf(gga.c_str());
        GPSFile.close();
      }
    }
    data_update = false;
  }
}
