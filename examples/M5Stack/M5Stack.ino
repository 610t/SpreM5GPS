#include <M5Unified.h>

void setup() {
  Serial.begin(115200);   // for debug output
  Serial2.begin(115200);  // get data from SPRESENSE

  M5.begin();
  M5.setLogDisplayIndex(0);
  M5.Display.setTextScroll(true);
  M5.Lcd.setTextSize(2);
}

void loop() {
  int av = Serial2.available();
  if (av > 0) {
    char buf[1024] = { 0 };

    Serial2.readBytes(buf, av);

    Serial.printf("%s\n", buf);
    M5.Log.printf("%s\n", buf);
  }
}