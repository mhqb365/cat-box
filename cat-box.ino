#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define PIR_PIN 26   // Cảm biến PIR nối chân D26
#define RELAY_PIN 15 // Relay nối chân D15

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

const char *ssid = "Mai Hoang Quoc Bao";
const char *password = "1234567890";

const unsigned long CALIBRATION_TIME = 30000;
const unsigned long NO_MOTION_DELAY = 180000;
const int REQUIRED_SAMPLES = 3;

unsigned long lastMotionTime = 0;
unsigned long relayStartTime = 0;
bool relayState = false;
int motionCounter = 0;
unsigned long startTime;

bool manualMode = false;
unsigned long manualStartTime = 0;

const char indexPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Bồn lựu đạn của 🐱 Noel</title>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
    <style>
      body {
        font-family: Arial;
        margin: 20px;
        background: #f0f0f0;
      }
      li {
        list-style: none;
      }
      .container {
        max-width: 400px;
        margin: auto;
        background: white;
        padding: 20px;
        border-radius: 10px;
        box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
      }
      .status {
        background: #e8f4fd;
        padding: 15px;
        border-radius: 5px;
        margin: 10px 0;
      }
      .button {
        display: inline-block;
        padding: 12px 24px;
        margin: 5px;
        text-decoration: none;
        border-radius: 5px;
        font-weight: bold;
        text-align: center;
        min-width: 100px;
        border: none;
        cursor: pointer;
      }
      .btn-on {
        background: #28a745;
        color: white;
      }
      .btn-off {
        background: #dc3545;
        color: white;
      }
      .btn-auto {
        background: #007bff;
        color: white;
      }
      .btn-on:hover {
        background: #218838;
      }
      .btn-off:hover {
        background: #c82333;
      }
      .btn-auto:hover {
        background: #0056b3;
      }
      h1 {
        color: #333;
        text-align: center;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Bồn lựu đạn của 🐱 Noel</h1>
      <div class="status">
        <li>WiFi: <strong id="wifi-status">%WIFI_STATE%</strong></li>
        <li>Chế độ: <strong id="mode-status">%MODE%</strong></li>
        <li>Quạt: <strong id="relay-status">%RELAY_STATE%</strong></li>
        <li>Cảm biến: <strong id="pir-status">%PIR_STATE%</strong></li>
      </div>
      <div style="text-align: center">
        <button id="btn-on" class="button btn-on">BẬT RELAY</button>
        <button id="btn-off" class="button btn-off">TẮT RELAY</button><br />
        <button id="btn-auto" class="button btn-auto">CHẾ ĐỘ TỰ ĐỘNG</button>
      </div>

      <script>
        $(document).ready(function () {
          $("#btn-on").click(function () {
            $.get("/relay/on", function (data) {
              updateStatus();
            });
          });

          $("#btn-off").click(function () {
            $.get("/relay/off", function (data) {
              updateStatus();
            });
          });

          $("#btn-auto").click(function () {
            $.get("/auto", function (data) {
              updateStatus();
            });
          });

          function updateStatus() {
            $.get("/status", function (data) {
              if (data.relay) $("#relay-status").text(data.relay);
              if (data.mode) $("#mode-status").text(data.mode);
              if (data.pir) $("#pir-status").text(data.pir);
              if (data.wifi) $("#wifi-status").text(data.wifi);
            }).fail(function () {
              console.log("Không thể cập nhật trạng thái");
            });
          }

          updateStatus();
          setInterval(updateStatus, 5000);
        });
      </script>
    </div>
  </body>
</html>
)rawliteral";

// Function declarations
void setupWebServer();
void handleRoot();
void handleRelayOn();
void handleRelayOff();
void handleAutoMode();
void handleStatus();

void setup()
{
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cat Box System");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  Serial.println("Đang kết nối WiFi...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20)
  {
    delay(500);
    Serial.print(".");
    lcd.setCursor(wifiAttempts % 16, 1);
    lcd.print(".");
    wifiAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println();
    Serial.println("WiFi đã kết nối thành công!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Tên mạng: ");
    Serial.println(WiFi.SSID());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    String ipString = WiFi.localIP().toString();
    if (ipString.length() > 16)
    {
      lcd.print(ipString.substring(ipString.length() - 16));
    }
    else
    {
      lcd.print(ipString);
    }
    delay(3000);

    setupWebServer();
    server.begin();
    Serial.println("Web server đã khởi động!");
    Serial.print("Truy cập: http://");
    Serial.println(WiFi.localIP());

    delay(3000);
  }
  else
  {
    Serial.println();
    Serial.println("Không thể kết nối WiFi! Tiếp tục hoạt động offline...");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Offline Mode");
    delay(2000);
  }

  Serial.println("Đang hiệu chuẩn cảm biến PIR...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating PIR");

  startTime = millis();

  while (millis() - startTime < CALIBRATION_TIME)
  {
    unsigned long remaining = CALIBRATION_TIME - (millis() - startTime);
    Serial.print("Còn lại: ");
    Serial.print(remaining / 1000);
    Serial.println(" giây");

    lcd.setCursor(0, 1);
    lcd.print("Wait: ");
    lcd.print(remaining / 1000);
    lcd.print("s    ");

    delay(1000);
  }

  Serial.println("Cảm biến PIR đã được hiệu chuẩn, bắt đầu phát hiện chuyển động");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  delay(3000);
  lcd.noBacklight();
}

void setupWebServer()
{
  // Enable CORS for all routes
  server.onNotFound([]()
                    {
    if (server.method() == HTTP_OPTIONS) {
      server.send(200);
    } else {
      server.send(404, "text/plain", "Not Found");
    } });

  server.on("/", handleRoot);
  server.on("/relay/on", handleRelayOn);
  server.on("/relay/off", handleRelayOff);
  server.on("/auto", handleAutoMode);
  server.on("/status", handleStatus);
}

void handleRoot()
{
  String page = String(indexPage);

  // Replace placeholders with actual values
  page.replace("%WIFI_STATE%", WiFi.status() == WL_CONNECTED ? "Đã kết nối" : "Không kết nối");
  page.replace("%MODE%", manualMode ? "Thủ công" : "Tự động");
  page.replace("%RELAY_STATE%", relayState ? "Bật" : "Tắt");
  page.replace("%PIR_STATE%", digitalRead(PIR_PIN) ? "Có chuyển động" : "Không chuyển động");

  server.send(200, "text/html", page);
}

void handleRelayOn()
{
  manualMode = true;
  manualStartTime = millis();
  digitalWrite(RELAY_PIN, HIGH);
  relayState = true;
  lcd.backlight();

  Serial.println("*** WEB: BẬT relay THỦ CÔNG ***");

  server.send(200, "application/json", "{\"status\":\"success\",\"relay\":\"on\"}");
}

void handleRelayOff()
{
  manualMode = true;
  manualStartTime = millis();
  digitalWrite(RELAY_PIN, LOW);
  relayState = false;
  lcd.noBacklight();

  Serial.println("*** WEB: TẮT relay THỦ CÔNG ***");

  server.send(200, "application/json", "{\"status\":\"success\",\"relay\":\"off\"}");
}

void handleAutoMode()
{
  manualMode = false;
  motionCounter = 0;

  if (relayState)
  {
    relayStartTime = millis();
  }

  Serial.println("*** WEB: Chuyển về CHẾ ĐỘ TỰ ĐỘNG ***");

  server.send(200, "application/json", "{\"status\":\"success\",\"mode\":\"auto\"}");
}

void handleStatus()
{
  String json = "{";
  json += "\"relay\":\"" + String(relayState ? "Bật" : "Tắt") + "\",";
  json += "\"mode\":\"" + String(manualMode ? "Thủ công" : "Tự động") + "\",";
  json += "\"pir\":\"" + String(digitalRead(PIR_PIN) ? "Có chuyển động" : "Không chuyển động") + "\",";
  json += "\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "Đã kết nối" : "Không kết nối") + "\",";
  json += "\"motionCounter\":" + String(motionCounter);
  json += "}";

  server.send(200, "application/json", json);
}

void loop()
{
  server.handleClient();

  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 30000)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi bị ngắt kết nối, đang thử kết nối lại...");
      WiFi.reconnect();
    }
    lastWiFiCheck = millis();
  }

  unsigned long currentTime = millis();
  int motion = digitalRead(PIR_PIN);

  static unsigned long lastLCDUpdate = 0;
  if (currentTime - lastLCDUpdate > 1000)
  {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("W:");
    lcd.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");
    lcd.print(" R:");
    lcd.print(relayState ? "ON " : "OFF");
    lcd.print(" ");
    lcd.print(manualMode ? "M" : "A");

    lcd.setCursor(0, 1);
    if (WiFi.status() == WL_CONNECTED)
    {
      String ipString = WiFi.localIP().toString();
      if (ipString.length() > 16)
      {
        lcd.print(ipString.substring(ipString.length() - 16));
      }
      else
      {
        lcd.print(ipString);
      }
    }
    else
    {
      lcd.print("Offline Mode");
    }

    lastLCDUpdate = currentTime;
  }

  static unsigned long lastPrintTime = 0;
  if (currentTime - lastPrintTime > 1000)
  {
    Serial.print("WiFi: ");
    Serial.print(WiFi.status() == WL_CONNECTED ? "Kết nối" : "Ngắt");
    Serial.print(" | PIR: ");
    Serial.print(motion);
    Serial.print(" | Bộ đếm: ");
    Serial.print(motionCounter);
    Serial.print(" | Relay: ");
    Serial.print(relayState ? "BẬT" : "TẮT");
    Serial.print(" | Chế độ: ");
    Serial.print(manualMode ? "THỦ CÔNG" : "TỰ ĐỘNG");

    if (lastMotionTime > 0)
    {
      unsigned long timeSinceLastMotion = (currentTime - lastMotionTime) / 1000;
      Serial.print(" | Lần cuối có chuyển động: ");
      Serial.print(timeSinceLastMotion);
      Serial.print(" giây trước");
    }
    Serial.println();
    lastPrintTime = currentTime;
  }

  if (!manualMode)
  {
    if (motion == HIGH)
    {
      lastMotionTime = currentTime;
      motionCounter++;

      lcd.backlight();

      if (motionCounter >= REQUIRED_SAMPLES && !relayState)
      {
        Serial.println("*** PHÁT HIỆN chuyển động -> BẬT relay trong 3 phút ***");
        digitalWrite(RELAY_PIN, HIGH);
        relayState = true;
        relayStartTime = currentTime;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MOTION DETECTED!");
        lcd.setCursor(0, 1);
        lcd.print("RELAY ON - 3min");
        delay(1000);
      }
    }
    else
    {
      if (motionCounter > 0)
      {
        motionCounter--;
      }
    }

    if (relayState && (currentTime - relayStartTime >= NO_MOTION_DELAY))
    {
      Serial.println("*** HẾT 3 phút -> TẮT relay ***");
      digitalWrite(RELAY_PIN, LOW);
      relayState = false;

      lcd.noBacklight();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("3 MINUTES UP");
      lcd.setCursor(0, 1);
      lcd.print("RELAY OFF");
      delay(1000);
    }
  }

  delay(100);
}