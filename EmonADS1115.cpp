#include "EmonADS1115.h"

EmonADS1115::EmonADS1115() {}

bool EmonADS1115::begin(uint8_t numChannels, uint8_t i2c_addr, TwoWire &wire) {
    _numChannels = numChannels;

    // Cấp phát bộ nhớ
    _ratios = new float[_numChannels];
    _burdenResistors = new float[_numChannels];
    _sumOfSquares = new double[_numChannels];
    _sampleCounts = new long[_numChannels];
    _rmsValues = new double[_numChannels];

    for (int i = 0; i < _numChannels; i++) {
        _sumOfSquares[i] = 0; _sampleCounts[i] = 0; _rmsValues[i] = 0;
    }
    
    _isReadyFlag = false;
    _currentChannel = 0;
    _lastCalculationTime = 0;
    _dcOffset = 0;

    if (!_ads.begin(i2c_addr, &wire)) {
      return false;
    }
    _ads.setGain(GAIN_ONE);
    _ads.setDataRate(RATE_ADS1115_860SPS);
    return true;
}

void EmonADS1115::configureChannel(uint8_t channel, float ratio, float burdenResistor) {
    if (channel < _numChannels) {
        _ratios[channel] = ratio;
        _burdenResistors[channel] = burdenResistor;
    }
}

void EmonADS1115::calibrate(uint16_t numSamples) {
    Serial.println("Bat dau hieu chuan DC Offset. Dam bao khong co tai!");
    long totalValue = 0;
    for (int i = 0; i < numSamples; i++) {
        totalValue += _ads.readADC_SingleEnded(0); // Calibrate based on channel 0
        delay(2);
    }
    _dcOffset = totalValue / numSamples;
    Serial.printf("Hieu chuan hoan tat. DC Offset = %d\n", _dcOffset);
}

void EmonADS1115::setDCOffset(int offset) {
    _dcOffset = offset;
}

void EmonADS1115::_calculateResults() {
    for (int i = 0; i < _numChannels; i++) {
        if (_sampleCounts[i] > 10) {
            double meanSquare = _sumOfSquares[i] / _sampleCounts[i];
            double rmsADC = sqrt(meanSquare);
            float maxVoltage = 4.096;
            double voltageRMS = (rmsADC / 32767.0) * maxVoltage;
            double secondaryCurrentRMS = voltageRMS / _burdenResistors[i];
            _rmsValues[i] = secondaryCurrentRMS * _ratios[i];
        } else {
            _rmsValues[i] = 0.0;
        }
        _sumOfSquares[i] = 0;
        _sampleCounts[i] = 0;
    }
    _isReadyFlag = true;
}

void EmonADS1115::loop() {
    // Chỉ cần lấy mẫu khi chưa đến lúc tính toán
    if (millis() - _lastCalculationTime < CALCULATION_INTERVAL) {
        // Đọc tuần tự từng kênh. Hàm này sẽ tự động chờ ~1.2ms.
        int16_t adcValue = _ads.readADC_SingleEnded(_currentChannel);
        // Sử dụng bộ lọc thông thấp IIR đơn giản để liên tục cập nhật offset
        const float alpha = 0.001;
        _dcOffset = (1.0 - alpha) * _dcOffset + alpha * adcValue;
        double sample = adcValue - _dcOffset;
        _sumOfSquares[_currentChannel] += sample * sample;
        _sampleCounts[_currentChannel]++;
        _currentChannel = (_currentChannel + 1) % _numChannels;
    } else {
        // Đã đến lúc tính toán và reset
        _lastCalculationTime = millis();
        _calculateResults();
    }
}

double EmonADS1115::getRMS(uint8_t channel) {
    return (channel < _numChannels) ? _rmsValues[channel] : 0.0;
}

bool EmonADS1115::isReady() {
    return _isReadyFlag;
}
