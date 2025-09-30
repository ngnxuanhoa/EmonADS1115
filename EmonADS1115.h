#ifndef EmonADS1115_h
#define EmonADS1115_h

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

class EmonADS1115 {
public:
  EmonADS1115();

  // Chỉ còn một hàm begin() duy nhất
  bool begin(uint8_t numChannels = 1, uint8_t i2c_addr = 0x48, TwoWire &wire = Wire);

  void configureChannel(uint8_t channel, float ratio, float burdenResistor);
  void calibrate(uint16_t numSamples = 512);
  void setDCOffset(int offset);
  void loop();

  double getRMS(uint8_t channel);
  bool isReady();

private:
  void _calculateResults();

  Adafruit_ADS1X15 _ads;
  uint8_t _numChannels;

  float* _ratios;
  float* _burdenResistors;
  
  double* _sumOfSquares;
  long* _sampleCounts;
  double* _rmsValues;
  int _dcOffset;

  bool _isReadyFlag;
  unsigned long _lastCalculationTime;
  const unsigned int CALCULATION_INTERVAL = 1000;
  
  uint8_t _currentChannel;
};

#endif
