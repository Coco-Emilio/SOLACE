#pragma once

#include <Arduino.h>

class MotionController {
 public:
  void begin();
  void track(float azimuthError, float elevationError);
  void moveToward(float azimuthTarget, float elevationTarget);
  float azimuth() const { return azimuthDeg_; }
  float elevation() const { return elevationDeg_; }

 private:
  void attachPwm();
  void writeServo(uint8_t pin, uint8_t legacyChannel, float angleDeg,
                  uint16_t minimumUs, uint16_t maximumUs, bool reversed);
  void writeAngles();
  float azimuthDeg_ = 90;
  float elevationDeg_ = 90;
};
