#pragma once

#include "SystemTypes.h"

class LdrArray {
 public:
  void begin();
  LdrReadings read();

 private:
  uint16_t readPin(uint8_t pin);
};
