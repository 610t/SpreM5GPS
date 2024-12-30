#include <M5Unified.h>

bool logger_mode = false;
bool sd_exist = false;

void setup() {
  Serial.begin(115200);   // for debug output
  Serial2.begin(115200);  // get data from SPRESENSE

  M5.begin();
  M5.setLogDisplayIndex(0);
  M5.Display.setTextScroll(true);
  M5.Lcd.setTextSize(2);

  // Initialize SD here
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
  }

  // Log to SD
  if (logger_mode && sd_exist) {
    // Convert to NEMA format
  }
}