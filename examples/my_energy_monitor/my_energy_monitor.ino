#include <EmonADS1115.h>

// --- CẤU HÌNH ---
#define ADC_RDY_PIN 25   // Chân GPIO nối với ALERT/RDY
#define NUM_CHANNELS 3   // Chúng ta đo 3 kênh

// Tạo một instance của thư viện
EmonADS1115 emon;

void setup() {
  Serial.begin(115200);

  // Khởi tạo thư viện
  if (!emon.begin(ADC_RDY_PIN, NUM_CHANNELS)) {
    Serial.println("Lỗi khởi tạo EmonADS1115!");
    while(1);
  }

 // Đợi một chút để hệ thống ổn định trước khi hiệu chuẩn
  delay(500);
  // Thực hiện hiệu chuẩn tự động
  emon.calibrate(); 

  // (Tùy chọn) Nếu bạn đã biết DC Offset chính xác từ trước, 
  // bạn có thể bỏ qua calibrate() và dùng:
  // emon.setDCOffset(13175);

  // Cấu hình từng kênh
  // emon.configureChannel(channel, CT_RATIO, BURDEN_RESISTOR);
  emon.configureChannel(0, 2000.0, 22.0); // Kênh A0 cho Pha 1
  emon.configureChannel(1, 2000.0, 22.0); // Kênh A1 cho Pha 2
  emon.configureChannel(2, 2000.0, 22.0); // Kênh A2 cho Pha 3
  
  Serial.println("Đang chờ kết quả đo đầu tiên...");
}

unsigned long lastPrintTime = 0;

void loop() {
  // Hàm này PHẢI được gọi liên tục. Nó sẽ tự quản lý mọi thứ.
  emon.loop(); 

  // Chỉ in kết quả mỗi 2 giây để không làm tràn Serial Monitor
  if (millis() - lastPrintTime >= 2000) {
    lastPrintTime = millis();
    
    // Chỉ in khi đã có kết quả tính toán đầu tiên
    if (emon.isReady()) {
      Serial.println("---------------------------------");
      for (int i = 0; i < NUM_CHANNELS; i++) {
        double current = emon.getRMS(i);
        Serial.print("Dòng điện Pha ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(current, 3);
        Serial.println(" A");
      }
    }
  }
}
