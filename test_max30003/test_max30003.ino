#include <SPI.h>
#include "protocentral_max30003.h"

// Use chip select pin D4 (examples default)
MAX30003 max30003(4);

void setup() {
  Serial.begin(115200);
  SPI.begin();

  if (!max30003.readDeviceID()) {
    while (1) { delay(1000); }
  }

  max30003.begin();         // start sensor, default sampling
  max30003.setSamplingRate(SR_128 ); // optional: change sample rate
}

void loop() {
  int32_t ecg = 0;
  if (max30003.readEcgSample(ecg)) {   // returns true when a new sample is available
    Serial.println(ecg/100);               // 24‑bit sign‑extended ADC value
  }

  // RTOR / heart rate (library keeps latest values)
  max30003.updateHeartRate();
  Serial.print("HR:"); Serial.print(max30003.heartRate());
  Serial.print(" RR(ms):"); Serial.println(max30003.rrInterval());

  delay(8);
}