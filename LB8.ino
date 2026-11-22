
#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>


// Данные Wi-Fi
const char* ssid = "ASOIU";
const char* password = "kaf.asoiu.48";

// Создание Web-сервера
WebServer server(80);

// Настройки NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 5 * 3600;
const int daylightOffset_sec = 0;

// Обработка главной страницы
void handleRoot() {
  server.send(200, "text/html",
    "<html><head><meta charset='UTF-8'>"
    "<meta http-equiv='refresh' content='1'>"
    "<title>M5StickS3 Clock</title></head>"
    "<body style='text-align:center;font-family:Arial'>"
    "<h1>M5StickS3 Clock</h1>"
    "<h2>Current time</h2>"
    "<h1 id='clock'>Loading...</h1>"
    "<script>"
    "async function updateTime(){"
    "let r=await fetch('/time');"
    "document.getElementById('clock').innerText=await r.text();"
    "}"
    "updateTime();setInterval(updateTime,1000);"
    "</script></body></html>");
}

// Передача текущего времени
void handleTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    server.send(503, "text/plain", "Time unavailable");
    return;
  }

  char timeString[20];

  strftime(timeString, sizeof(timeString),
           "%H:%M:%S", &timeinfo);

  server.send(200, "text/plain", timeString);
}

// Обработка неизвестных адресов
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void drawClock(struct tm timeinfo) {
  M5Canvas canvas(&M5.Display);

  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextDatum(MC_DATUM);

  int cx = M5.Display.width() / 2;
  int cy = 115;
  int radius = 48;

  canvas.fillSprite(TFT_BLACK);

  // Дата над часами
  char dateString[20];

  strftime(dateString, sizeof(dateString),
           "%d/%m/%Y", &timeinfo);

  canvas.setTextColor(TFT_GREEN);
  canvas.setTextSize(2);
  canvas.drawString(dateString, cx, 22);

  // Циферблат
  canvas.drawCircle(cx, cy, radius, TFT_WHITE);

  // Деления циферблата
  for (int i = 0; i < 12; i++) {
    float angle = (i * 30 - 90) * PI / 180.0;

    int x1 = cx + cos(angle) * 40;
    int y1 = cy + sin(angle) * 40;

    int x2 = cx + cos(angle) * 46;
    int y2 = cy + sin(angle) * 46;

    canvas.drawLine(x1, y1, x2, y2, TFT_WHITE);
  }

  // Расчёт углов стрелок
  float hourAngle =
    ((timeinfo.tm_hour % 12) * 30 +
     timeinfo.tm_min * 0.5 - 90) * PI / 180.0;

  float minuteAngle =
    (timeinfo.tm_min * 6 - 90) * PI / 180.0;

  float secondAngle =
    (timeinfo.tm_sec * 6 - 90) * PI / 180.0;

  // Часовая стрелка
  int hx = cx + cos(hourAngle) * 23;
  int hy = cy + sin(hourAngle) * 23;

  canvas.drawLine(cx, cy, hx, hy, TFT_YELLOW);

  // Минутная стрелка
  int mx = cx + cos(minuteAngle) * 34;
  int my = cy + sin(minuteAngle) * 34;

  canvas.drawLine(cx, cy, mx, my, TFT_WHITE);

  // Секундная стрелка
  int sx = cx + cos(secondAngle) * 42;
  int sy = cy + sin(secondAngle) * 42;

  canvas.drawLine(cx, cy, sx, sy, TFT_RED);

  // Центральная точка
  canvas.fillCircle(cx, cy, 3, TFT_RED);

  // Время под часами
  char timeString[20];

  strftime(timeString, sizeof(timeString),
           "%H:%M:%S", &timeinfo);

  canvas.setTextColor(TFT_WHITE);
  canvas.setTextSize(2);
  canvas.drawString(timeString, cx, 205);

  // Вывод готового изображения на экран
  canvas.pushSprite(0, 0);

  canvas.deleteSprite();
}

void setup() {
  // Инициализация устройства
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(0);
  M5.Display.setTextDatum(middle_center);
  M5.Display.fillScreen(TFT_BLACK);

  Serial.begin(115200);

  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);

  M5.Display.drawString("Connecting WiFi...",
                        M5.Display.width() / 2, 100);

  Serial.println("");

  // Ожидание подключения
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Синхронизация времени через NTP
  configTime(gmtOffset_sec, daylightOffset_sec,
             ntpServer);

  struct tm timeinfo;

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.drawString("Synchronizing NTP...",
                        M5.Display.width() / 2, 100);

  if (getLocalTime(&timeinfo, 15000)) {
    Serial.println("NTP synchronized!");
  } else {
    Serial.println("NTP synchronization failed!");
  }

  // Регистрация обработчиков Web-сервера
  server.on("/", handleRoot);
  server.on("/time", handleTime);
  server.onNotFound(handleNotFound);

  // Запуск Web-сервера
  server.begin();

  Serial.println("HTTP server started!");
}

void loop() {
  // Обработка запросов Web-сервера
  server.handleClient();

  // Получение и отображение времени
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    struct tm timeinfo;

    if (getLocalTime(&timeinfo)) {
      drawClock(timeinfo);
    }
  }

  delay(2);
}
