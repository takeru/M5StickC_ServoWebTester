#include <M5StickC.h>
#include <driver/i2s.h>
#include "esp_http_server.h"
#include <WiFi.h>

#include <Wire.h>
#include "Adafruit_PWMServoDriver.h"
Adafruit_PWMServoDriver servo = Adafruit_PWMServoDriver(0x40, Wire);

int8_t  _ctrl_sleeping     = -1;
int8_t  _ctrl_servo_number = -1;
int16_t _ctrl_pulse_width  = -1;
bool    sleeping;

void setup() {
  M5.begin();

  Wire.begin(0, 26); // SDA=0, SCL=26
  servo.begin();
  servo.setOscillatorFrequency(26000000); // Get it from oscillator.ino
  servo.setPWMFreq(50);
  sleep(true);

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
    _ctrl_sleeping = !sleeping;
  }else
  if(M5.BtnA.wasReleased()){
  }else
  if(M5.BtnB.wasPressed()){
  }else
  if(M5.BtnB.wasReleased()){
  }
  //servo.writeMicroseconds(0, 1500);

  if(0<=_ctrl_servo_number && 0<=_ctrl_pulse_width){
    servo.writeMicroseconds(_ctrl_servo_number, _ctrl_pulse_width);
    printf("servo_number=%d pulse_width=%d\n", _ctrl_servo_number, _ctrl_pulse_width);
    _ctrl_servo_number = -1;
    _ctrl_pulse_width  = -1;
  }
  if(0<=_ctrl_sleeping){
    sleep(_ctrl_sleeping==1);
    printf("sleep=%d\n", sleeping);
    _ctrl_sleeping = -1;
  }

  unsigned long ms = millis();
  static unsigned long prevMs = 0;
  if(1000 < ms-prevMs){
    prevMs = ms;
    update_display();
  }
  delay(1);
}

void sleep(bool s){
  sleeping = s;
  if(sleeping){
    servo.sleep();
  }else{
    servo.wakeup();
  }
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

static esp_err_t ctrl_handler(httpd_req_t *req)
{
  esp_err_t res = ESP_OK;

  String mode = query_value_string(req, "mode");
  if(mode == "servo"){
    _ctrl_servo_number = query_value_string(req, "servo_number").toInt();
    if(_ctrl_servo_number<0 || 15<_ctrl_servo_number){
      _ctrl_servo_number = -1;
      return ESP_FAIL;
    }
    _ctrl_pulse_width = query_value_string(req, "pulse_width").toInt();

    #define BUF_SIZE 256
    char* buf[BUF_SIZE+1];
    #define _CTRL_SERVO_RESP "{\"servo_number\": %d, \"pulse_width\": %d}"
    size_t len = snprintf((char*)buf, BUF_SIZE, _CTRL_SERVO_RESP, _ctrl_servo_number, _ctrl_pulse_width);
    res = httpd_resp_send(req, (const char *)buf, len);
  }

  if(mode == "sleep"){
    _ctrl_sleeping = query_value_string(req, "sleep").toInt()==1;

    #define BUF_SIZE 256
    char* buf[BUF_SIZE+1];
    #define _CTRL_SLEEP_RESP "{\"sleep\": %d}"
    size_t len = snprintf((char*)buf, BUF_SIZE, _CTRL_SLEEP_RESP, _ctrl_sleeping);
    res = httpd_resp_send(req, (const char *)buf, len);
  }

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
  //M5.Lcd.printf("ServoWebTester\n");

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
}
