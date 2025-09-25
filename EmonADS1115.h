#ifndef EmonADS1115_h
#define EmonADS1115_h

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

class EmonADS1115 {
public:
  // Constructor
  EmonADS1115();

  // Hàm khởi tạo chính
  bool begin(uint8_t rdyPin, uint8_t numChannels = 1, TwoWire &wire = Wire);

  // numSamples: số lượng mẫu để lấy trung bình
  void calibrate(uint16_t numSamples = 512);

  // Cho phép người dùng tự đặt DC Offset nếu muốn
  void setDCOffset(int offset);
  
  // Hàm cấu hình cho từng kênh
  void configureChannel(uint8_t channel, float ratio, float burdenResistor);

  // Hàm này phải được gọi liên tục trong loop() chính của bạn
  void loop();

  // Hàm để lấy kết quả RMS
  double getRMS(uint8_t channel);
  
  // Hàm kiểm tra xem đã có kết quả tính toán đầu tiên chưa
  bool isReady();

private:
  // Hàm xử lý ngắt nội bộ (non-static)
  void handleInterrupt();

  // Con trỏ tĩnh đến instance của class để ISR có thể gọi
  static EmonADS1115* _instance;
  
  // Hàm ISR tĩnh
  static void IRAM_ATTR isr_handler();

  Adafruit_ADS1115 _ads;
  uint8_t _rdyPin;
  uint8_t _numChannels;

  // Cấu hình cho các kênh
  float* _ratios;
  float* _burdenResistors;

  // Biến cho máy trạng thái và ngắt
  volatile bool _adcDataReady;
  volatile uint8_t _currentChannel;

  // Biến để tính toán RMS
  double* _sumOfSquares;
  long* _sampleCounts;
  double* _rmsValues;
  bool _isReady;

  // Biến quản lý thời gian
  unsigned long _lastCalculationTime;
  const long CALCULATION_INTERVAL = 1000;
  int _dcOffset;
};

#endif