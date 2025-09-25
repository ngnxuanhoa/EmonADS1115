#include "EmonADS1115.h"

// Khởi tạo con trỏ tĩnh
EmonADS1115* EmonADS1115::_instance = nullptr;

EmonADS1115::EmonADS1115() {
  // Constructor để trống
}

bool EmonADS1115::begin(uint8_t rdyPin, uint8_t numChannels, TwoWire &wire) {
  _rdyPin = rdyPin;
  _numChannels = numChannels;
  _instance = this; // Quan trọng: lưu lại instance cho ISR

  // Cấp phát bộ nhớ động cho các mảng dựa trên số kênh
  _ratios = new float[numChannels];
  _burdenResistors = new float[numChannels];
  _sumOfSquares = new double[numChannels];
  _sampleCounts = new long[numChannels];
  _rmsValues = new double[numChannels];

  // Khởi tạo các giá trị
  for (int i = 0; i < numChannels; i++) {
    _sumOfSquares[i] = 0;
    _sampleCounts[i] = 0;
    _rmsValues[i] = 0;
  }
  
  _isReady = false;
  _currentChannel = 0;
  _adcDataReady = false;
  _lastCalculationTime = 0;
  
  // Ước tính DC Offset (nên được hiệu chuẩn để chính xác hơn)
  // Giá trị ADC cho 1.65V với Gain 1 (+/-4.096V) là khoảng 13217
  _dcOffset = 13217; // Khởi tạo bằng 13217, sẽ được cập nhật sau bởi calibrate()

  if (!_ads.begin(ADS1115_ADDRESS, &wire)) {
    return false; // Trả về false nếu không tìm thấy ADS
  }

  _ads.setGain(GAIN_ONE);
  _ads.setDataRate(RATE_ADS1115_860SPS);

  // Kích hoạt chế độ RDY
  _ads.writeRegister(ADS1115_REG_POINTER_HITHRESH, 0x8000);
  _ads.writeRegister(ADS1115_REG_POINTER_LOTHRESH, 0x0000);

  // Cấu hình ngắt
  pinMode(_rdyPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(_rdyPin), isr_handler, FALLING);
  
  // Bắt đầu phép đo đầu tiên
  _ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0 + _currentChannel, false);

  return true;
}

void EmonADS1115::configureChannel(uint8_t channel, float ratio, float burdenResistor) {
  if (channel < _numChannels) {
    _ratios[channel] = ratio;
    _burdenResistors[channel] = burdenResistor;
  }
}

void IRAM_ATTR EmonADS1115::isr_handler() {
  if (_instance != nullptr) {
    _instance->_adcDataReady = true;
  }
}

void EmonADS1115::loop() {
  // Xử lý dữ liệu từ ngắt
  if (_adcDataReady) {
    _adcDataReady = false;

    int16_t adcValue = _ads.getLastConversionResults();
    double sample = adcValue - _dcOffset;
    _sumOfSquares[_currentChannel] += sample * sample;
    _sampleCounts[_currentChannel]++;

    _currentChannel = (_currentChannel + 1) % _numChannels;
    _ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0 + _currentChannel, false);
  }

  // Tính toán RMS định kỳ
  if (millis() - _lastCalculationTime >= CALCULATION_INTERVAL) {
    _lastCalculationTime = millis();

    for (int i = 0; i < _numChannels; i++) {
      if (_sampleCounts[i] > 0) {
        double meanSquare = _sumOfSquares[i] / _sampleCounts[i];
        double rmsADC = sqrt(meanSquare);

        float maxVoltage = 4.096;
        double voltageRMS = (rmsADC / 32767.0) * maxVoltage;
        double secondaryCurrentRMS = voltageRMS / _burdenResistors[i];
        _rmsValues[i] = secondaryCurrentRMS * _ratios[i];

        _sumOfSquares[i] = 0;
        _sampleCounts[i] = 0;
      } else {
        _rmsValues[i] = 0.0;
      }
    }
    _isReady = true; // Báo hiệu đã có kết quả tính toán đầu tiên
  }
}

double EmonADS1115::getRMS(uint8_t channel) {
  if (channel < _numChannels) {
    return _rmsValues[channel];
  }
  return 0.0; // Trả về 0 nếu kênh không hợp lệ
}

bool EmonADS1115::isReady() {
  return _isReady;
}
// **** etDCOffset ****
void EmonADS1115::setDCOffset(int offset) {
    _dcOffset = offset;
}
