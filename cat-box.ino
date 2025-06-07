#define PIR_PIN 26 // Cảm biến PIR nối chân D26
#define RELAY_PIN 15 // Relay nối chân D15

// Thông số để ổn định tín hiệu
const unsigned long CALIBRATION_TIME = 30000; // 30 giây hiệu chuẩn ban đầu
const unsigned long RELAY_ON_DURATION = 90000; // 1 phút 30 giây bật relay
const int REQUIRED_SAMPLES = 3;               // Số lần đọc liên tục để xác nhận chuyển động
unsigned long lastMotionTime = 0;             // Thời điểm cuối cùng phát hiện chuyển động
unsigned long relayOnTime = 0;                // Thời điểm bật relay
bool relayState = false;                      // Trạng thái hiện tại của relay
int motionCounter = 0;                        // Bộ đếm để xác nhận chuyển động
unsigned long startTime;                      // Thời gian bắt đầu chương trình

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Tắt relay lúc đầu
  
  // Thời gian hiệu chuẩn cảm biến PIR
  Serial.println("Đang hiệu chuẩn cảm biến PIR...");
  startTime = millis();
  
  // Vòng lặp đợi hiệu chuẩn với đồng hồ đếm ngược
  while (millis() - startTime < CALIBRATION_TIME) {
    unsigned long remaining = CALIBRATION_TIME - (millis() - startTime);
    Serial.print("Còn lại: ");
    Serial.print(remaining / 1000);
    Serial.println(" giây");
    delay(1000);
  }
  
  Serial.println("Cảm biến PIR đã được hiệu chuẩn, bắt đầu phát hiện chuyển động");
}

void loop() {
  unsigned long currentTime = millis();
  int motion = digitalRead(PIR_PIN);
  
  // In thông tin trạng thái mỗi giây để theo dõi
  static unsigned long lastPrintTime = 0;
  if (currentTime - lastPrintTime > 1000) {
    Serial.print("Trạng thái PIR: ");
    Serial.print(motion);
    Serial.print(" | Bộ đếm chuyển động: ");
    Serial.print(motionCounter);
    Serial.print(" | Trạng thái relay: ");
    Serial.print(relayState ? "BẬT" : "TẮT");
    if (relayState) {
      Serial.print(" | Thời gian còn lại: ");
      unsigned long timeLeft = (relayOnTime + RELAY_ON_DURATION > currentTime) ? 
                               (relayOnTime + RELAY_ON_DURATION - currentTime) / 1000 : 0;
      Serial.print(timeLeft);
      Serial.print(" giây");
    }
    Serial.println();
    lastPrintTime = currentTime;
  }
  
  // Kiểm tra nếu relay đã bật đủ 1 phút 30 giây thì tắt
  if (relayState && (currentTime - relayOnTime >= RELAY_ON_DURATION)) {
    Serial.println("*** Hết thời gian 1 phút 30 giây -> TẮT relay ***");
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
  }
  
  // Xử lý phát hiện chuyển động với bộ lọc
  if (motion == HIGH) {
    lastMotionTime = currentTime;
    motionCounter++;
    
    // Chỉ bật relay khi đủ số lần xác nhận liên tiếp và relay đang tắt
    if (motionCounter >= REQUIRED_SAMPLES && !relayState) {
      Serial.println("*** XÁC NHẬN chuyển động -> BẬT relay trong 1 phút 30 giây ***");
      digitalWrite(RELAY_PIN, HIGH);
      relayState = true;
      relayOnTime = currentTime;  // Lưu thời điểm bật relay
    }
  } 
  else {
    // Giảm dần bộ đếm khi không có chuyển động
    if (motionCounter > 0) {
      motionCounter--;
    }
  }

  
  delay(100);
}
