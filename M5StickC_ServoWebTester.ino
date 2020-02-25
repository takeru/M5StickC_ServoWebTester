#include <M5StickC.h>
#include <driver/i2s.h>
#include "esp_http_server.h"
#include <WiFi.h>

#include <ESP32Servo.h>
Servo servo1;
int32_t servo1_pin = 26;
const int32_t MIN_US = 500;
const int32_t MAX_US = 2500;

int pulse_width = 1500;

void setup() {
  M5.begin();

  servo1.setPeriodHertz(50);
  servo1.attach(servo1_pin, MIN_US, MAX_US);
  
  while(1){
    printf("Connecting...\n");
    bool ok = wifi_connect(10*1000);
    if(ok){
      IPAddress ipAddress = WiFi.localIP();
      printf("%s\n", ipAddress.toString().c_str());
      break;
    }
  }
  startHttpd();
}
 
void loop() {
  M5.update();
  if(M5.BtnA.wasPressed()){
    pulse_width = 1500;
  }else
  if(M5.BtnA.wasReleased()){
  }else
  if(M5.BtnB.wasPressed()){
  }else
  if(M5.BtnB.wasReleased()){
  }

  if(pulse_width != servo1.readMicroseconds()){
    servo1.writeMicroseconds(pulse_width);
  }

  unsigned long ms = millis();
  static unsigned long prevMs = 0;
  if(1000 < ms-prevMs){
//    static unsigned long prevTotalReadBytes = 0;
//    static unsigned long prevTotalReadCount = 0;
//    static unsigned long prevTotalSendBytes = 0;
//    if(prevMs){
//      printf("readBytes=%d(%d/s) readCount=%d(%d/s) sendBytes=%d(%d/s)\n",
//        totalReadBytes,
//        1000*(totalReadBytes-prevTotalReadBytes)/(ms-prevMs),
//        totalReadCount,
//        1000*(totalReadCount-prevTotalReadCount)/(ms-prevMs),
//        totalSendBytes,
//        1000*(totalSendBytes-prevTotalSendBytes)/(ms-prevMs)
//      );
//    }
//    prevTotalReadBytes = totalReadBytes;
//    prevTotalReadCount = totalReadCount;
//    prevTotalSendBytes = totalSendBytes;
    prevMs = ms;
    update_display();
  }
  delay(1);
}

#include "index_html.h"
httpd_handle_t httpd = NULL;

static esp_err_t index_handler(httpd_req_t *req){
  esp_err_t res = ESP_OK;
  res = httpd_resp_set_type(req, "text/html");
  if(res != ESP_OK){
    return res;
  }
  res = httpd_resp_send(req, _INDEX_HTML, strlen(_INDEX_HTML));
  return res;
}

#define _CTRL_RESP "{\"pulse_width\": %d}"
static esp_err_t ctrl_handler(httpd_req_t *req)
{
  esp_err_t res = ESP_OK;

  pulse_width = query_value_string(req, "pulse_width").toInt();
  printf("pulse_width=%d\n", pulse_width);
  #define BUF_SIZE 256
  char* buf[BUF_SIZE+1];
  size_t len = snprintf((char*)buf, BUF_SIZE, _CTRL_RESP, pulse_width);
  res = httpd_resp_send(req, (const char *)buf, len);
  return res;
}

static String query_value_string(httpd_req_t *req, const char* key)
{
  esp_err_t res = ESP_OK;
  String value = "";
  size_t query_len = httpd_req_get_url_query_len(req);
  if(0<query_len){
    query_len++;
    char* query_buf = (char*)malloc(query_len);
    res = httpd_req_get_url_query_str(req, query_buf, query_len);
    if(res == ESP_OK){
      char* val_buf = (char*)malloc(query_len);
      res = httpd_query_key_value(query_buf, key, val_buf, query_len);
      if(res == ESP_OK){
        value = String(val_buf);
      }
      free(val_buf);
    }
    free(query_buf);
  }
  return value;
}

void startHttpd(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t ctrl_uri = {
    .uri       = "/ctrl",
    .method    = HTTP_GET,
    .handler   = ctrl_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(httpd, &index_uri);
    httpd_register_uri_handler(httpd, &ctrl_uri);
  }
}

bool wifi_connect(int timeout_ms)
{
  if(WiFi.status() == WL_CONNECTED){
    return true;
  }
  uint8_t mac[6];
  WiFi.macAddress(mac);
  printf("mac: %s\n", mac_string(mac).c_str());

  WiFi.begin();
  unsigned long start_ms = millis();
  bool connected = false;
  while(1){
    connected = WiFi.status() == WL_CONNECTED;
    if(connected || (start_ms+timeout_ms)<millis()){
      break;
    }
    delay(1);
  }
  return connected;
}

void wifi_disconnect(){
  if(WiFi.status() != WL_DISCONNECTED){
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
  }
}

String mac_string(const uint8_t *mac)
{
  char macStr[18] = { 0 };
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void update_display()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(1, 1);
  M5.Lcd.printf("ServoWebTester\n");

  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;
  M5.Rtc.GetTime(&time);
  M5.Rtc.GetData(&date);
  M5.Lcd.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
    date.Year, date.Month, date.Date,
    time.Hours, time.Minutes, time.Seconds
  );

  IPAddress ipAddress = WiFi.localIP();
  M5.Lcd.printf("%s\n", ipAddress.toString().c_str());
  M5.Lcd.printf("pulse_width=%d\n", pulse_width);
}
