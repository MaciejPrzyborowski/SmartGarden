#include <WiFi.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include "SPIFFS.h"
#include "time.h"

Preferences preferences;

const char* ssid = "najlepsza wifi na osce 2.4G";
const char* password = "studentpiwo";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Adafruit_BME280 hBME280;
bool BME280_Status = false;
float BME280_Temperature = 0.0;
float BME280_Humidity = 0.0;

Adafruit_BMP280 hBMP280;
bool BMP280_Status = false;
float BMP280_Temperature = 0.0;
float BMP280_Humidity = 0.0;

BH1750 hBH1750;
bool BH1750_Status = false;
float BH1750_Light = 0.0;
const unsigned int BH1750_LightDusk = 20;

const unsigned int HCSR501_PORT = 13;
bool HCSR501_Motion = false;

const unsigned int GREENHOUSE_PORT = 12;
const unsigned int GREENHOUSE_CH = 0;
const unsigned int GREENHOUSE_RES = 10;
const unsigned int GREENHOUSE_FREQ = 2000;
bool gh_auto_mode = false;
bool gh_manual_mode = false;
bool gh_manual_off = true;
bool gh_manual_on = false;
bool gh_temp_off_iS = false;
unsigned int gh_target_temp = 0;
unsigned int gh_temp_off = 0;

float PID_previous_error = 0.0;
float PID_previous_integral = 0.0;
float PID_Kp = 0.0453165;
float PID_Ki = 0.000163058;
float PID_Kd = 0.6988927;
float PID_dt = 0.1;

const unsigned int LED1_PORT = 26;
unsigned int LED1_State = HIGH;
bool l1_auto_mode = false;
bool l1_manual_mode = false;
bool l1_manual_off = true;
bool l1_manual_on = false;
bool l1_time_off_iS = false;
unsigned int l1_time_off_hour = 0;
unsigned int l1_time_off_min = 0;
unsigned int l1_time_on_hour = 0;
unsigned int l1_time_on_min = 0;

const unsigned int LED2_PORT = 27;
unsigned int LED2_State = HIGH;
bool l2_auto_mode = false;
bool l2_manual_mode = false;
bool l2_manual_off = true;
bool l2_manual_on = false;
bool l2_dusk = false;
bool l2_motion = false;
bool l2_dusk_motion = false;

const unsigned int WATERPUMP_PORT = 25;
bool wp_auto_mode = false;
bool wp_manual_mode = false;
bool wp_manual_on = false;
bool wp_manual_off = true;
bool wp_duration_iS = false;
unsigned int wp_duration = 0;
unsigned int wp_humidity = 0;
unsigned int wp_duration_sec = 0;
unsigned int wp_duration_day = 0;

unsigned long currentMillis = 0;
unsigned long previousMillis_peroid1 = 0;
unsigned long previousMillis_peroid2 = 0;
const unsigned long period1 = 100;
const unsigned long period2 = 1000;

void setup()
{
  Serial.begin(115200);
  Serial.println("--- INIT TEST ---");
  initPorts();
  initFS();
  initWiFi();
  initTime();
  initSensors();
  initSettingsData();
  initWebServerSocket();
  initRestoreData();
}

void loop()
{
  currentMillis = millis();
  if (currentMillis - previousMillis_peroid1 >= period1)
  {
    readSensorsValues();
    operateGreenHouse();
    if (currentMillis - previousMillis_peroid2 >= period2)
    {
      operateLed1();
      operateLed2();
      operateWaterPump();
      webSocketNotify(JSONInformationValues());
      previousMillis_peroid2 = millis();
    }
    previousMillis_peroid1 = millis();
  }
}

void initPorts()
{
  pinMode(LED1_PORT, OUTPUT);
  pinMode(LED2_PORT, OUTPUT);
  pinMode(WATERPUMP_PORT, OUTPUT);
  ledcSetup(GREENHOUSE_CH, GREENHOUSE_FREQ, GREENHOUSE_RES);
  ledcAttachPin(GREENHOUSE_PORT, GREENHOUSE_CH);
  ledcWrite(GREENHOUSE_CH, 0);
  digitalWrite(LED1_PORT, HIGH);
  digitalWrite(LED2_PORT, HIGH);
  digitalWrite(WATERPUMP_PORT, HIGH);
  Serial.println("Ports: OK");
}

void initFS()
{
  if (SPIFFS.begin())
  {
    Serial.println("SPIFFS: OK");
  }
  else
  {
    Serial.println("SPIFFS: FAILED");
  }
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
  }
  Serial.print("WiFi: ");
  Serial.println(WiFi.localIP());
}

void initTime()
{
  struct tm timeinfo;
  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = 3600;
  const int daylightOffset_sec = 3600;
  do
  {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(100);
  }
  while (!getLocalTime(&timeinfo));
  Serial.println(&timeinfo, "Data: %A, %B %d %Y %H:%M:%S");
}

void initSensors()
{
  BME280_Status = hBME280.begin(0x76);
  BMP280_Status = hBMP280.begin(0x77);
  BH1750_Status = hBH1750.begin();
  pinMode(HCSR501_PORT, INPUT);
  if (BME280_Status)
  {
    Serial.println("BME280: OK");
  }
  else
  {
    Serial.println("BME280: FAILED");
  }
  if (BMP280_Status)
  {
    Serial.println("BMP280: OK");
  }
  else {
    Serial.println("BMP280: FAILED");
  }
  if (BH1750_Status)
  {
    Serial.println("BH1750: OK");
  }
  else
  {
    Serial.println("BH1750: FAILED");
  }
  Serial.println("HC-SR501: OK");
}

void initSettingsData()
{
  preferences.begin("SmartGarden", false);
  // --- GreenHouse ---
  gh_auto_mode = preferences.getBool("gh_auto_mode", false);
  gh_manual_mode = preferences.getBool("gh_manual_mode", false);
  gh_manual_off = preferences.getBool("gh_manual_off", true);
  gh_manual_on = preferences.getBool("gh_manual_on", false);
  gh_temp_off_iS = preferences.getBool("gh_temp_off_iS", false);
  gh_target_temp = preferences.getUInt("gh_target_temp", 0);
  gh_temp_off = preferences.getUInt("gh_temp_off", 0);
  // --- LED 1 ---
  LED1_State = preferences.getUInt("LED1_State", HIGH);
  l1_auto_mode = preferences.getBool("l1_auto_mode", false);
  l1_manual_mode = preferences.getBool("l1_manual_mode", false);
  l1_manual_off = preferences.getBool("l1_manual_off", true);
  l1_manual_on = preferences.getBool("l1_manual_on", false);
  l1_time_off_iS = preferences.getBool("l1_time_off_iS", false);
  l1_time_off_hour = preferences.getUInt("l1_off_hour", 0);
  l1_time_off_min = preferences.getUInt("l1_off_min", 0);
  l1_time_on_hour = preferences.getUInt("l1_on_hour", 0);
  l1_time_on_min = preferences.getUInt("l1_on_min", 0);
  // --- LED 2 ---
  LED2_State = preferences.getUInt("LED2_State", HIGH);
  l2_auto_mode = preferences.getBool("l2_auto_mode", false);
  l2_manual_mode = preferences.getBool("l2_manual_mode", false);
  l2_manual_off = preferences.getBool("l2_manual_off", true);
  l2_manual_on = preferences.getBool("l2_manual_on", false);
  l2_dusk = preferences.getBool("l2_dusk", false);
  l2_motion = preferences.getBool("l2_motion", false);
  l2_dusk_motion = preferences.getBool("l2_dusk_motion", false);
  // --- WaterPump ---
  wp_auto_mode = preferences.getBool("wp_auto_mode", false);
  wp_manual_mode = preferences.getBool("wp_manual_mode", false);
  wp_manual_on = preferences.getBool("wp_manual_on", false);
  wp_manual_off = preferences.getBool("wp_manual_off", true);
  wp_duration_iS = preferences.getBool("wp_duration_iS", false);
  wp_duration = preferences.getUInt("wp_duration", 0);
  wp_humidity = preferences.getUInt("wp_humidity", 0);
  wp_duration_sec = preferences.getUInt("wp_duration_sec", 0);
  wp_duration_day = preferences.getUInt("wp_duration_day", 0);
  Serial.println("Settings: OK");
}

void initWebServerSocket()
{
  ws.onEvent(webSocketOnEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");
  server.begin();
  Serial.println("Server: OK");
}

void initRestoreData()
{
  previousMillis_peroid1 = millis();
  previousMillis_peroid2 = millis();
  digitalWrite(LED1_PORT, LED1_State);
  digitalWrite(LED2_PORT, LED2_State);
  Serial.println("Data: OK");
  Serial.println();
}

void webSocketOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_DATA)
  {
    webSocketOnMessage(arg, data, len);
  }
}

void webSocketOnMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char*)data, "getValues") == 0)
    {
      webSocketNotify(JSONGreenHouseValues());
      webSocketNotify(JSONLedsValues());
      webSocketNotify(JSONWaterPumpValues());
      webSocketNotify(JSONInformationValues());
    }
    else
    {
      StaticJsonDocument<400> jsonData;
      DeserializationError error = deserializeJson(jsonData, (char*)data);
      if (!error)
      {
        if (jsonData.containsKey("cfg"))
        {
          const char* cfgType = jsonData["cfg"];
          if (strcmp(cfgType, "greenhouse") == 0)
          {
            gh_auto_mode = jsonData['gh_auto_mode'];
            if (gh_auto_mode)
            {
              gh_target_temp = jsonData['gh_target_temp'];
              gh_temp_off_iS = jsonData['gh_temp_off_is_set'];
              if (gh_temp_off_iS)
              {
                gh_temp_off = jsonData['gh_temp_off'];
              }
              else
              {
                gh_temp_off = 0;
              }
              gh_manual_mode = false;
              gh_manual_off = false;
              gh_manual_on = false;
            }
            else
            {
              gh_manual_mode = jsonData['gh_manual_mode'];
              if (gh_manual_mode)
              {
                gh_manual_on = jsonData['gh_manual_on'];
                gh_manual_off = jsonData['gh_manual_off'];
              }
              else
              {
                gh_manual_on = false;
                gh_manual_off = false;
              }
              gh_temp_off_iS = false;
              gh_target_temp = 0;
              gh_temp_off = 0;
            }
            preferences.putBool("gh_auto_mode", gh_auto_mode);
            preferences.putBool("gh_manual_mode", gh_manual_mode);
            preferences.putBool("gh_manual_off", gh_manual_off);
            preferences.putBool("gh_manual_on", gh_manual_on);
            preferences.putBool("gh_temp_off_iS", gh_temp_off_iS);
            preferences.putUInt("gh_target_temp", gh_target_temp);
            preferences.putUInt("gh_temp_off", gh_temp_off);
          }
          else if (strcmp(cfgType, "led") == 0)
          {
            l1_auto_mode = jsonData['l1_auto_mode'];
            if (l1_auto_mode)
            {
              l1_time_on_hour = jsonData['l1_time_on_hour'];
              l1_time_on_min = jsonData['l1_time_on_min'];
              l1_time_off_iS = jsonData['l1_time_off_is_set'];
              if (l1_time_off_iS)
              {
                l1_time_off_hour = jsonData['l1_time_off_hour'];
                l1_time_off_min = jsonData['l1_time_off_min'];
              }
              else
              {
                l1_time_off_hour = 0;
                l1_time_off_min = 0;
              }
              l1_manual_mode = false;
              l1_manual_off = false;
              l1_manual_on = false;
            }
            else
            {
              l1_manual_mode = jsonData['l1_manual_mode'];
              if (l1_manual_mode)
              {
                l1_manual_on = jsonData['l1_manual_on'];
                l1_manual_off = jsonData['l1_manual_off'];
              }
              else
              {
                l1_manual_on = false;
                l1_manual_off = false;
              }
              l1_time_off_iS = false;
              l1_time_off_hour = 0;
              l1_time_off_min = 0;
              l1_time_on_hour = 0;
              l1_time_on_min = 0;
            }
            l2_auto_mode = jsonData['l2_auto_mode'];
            if (l2_auto_mode)
            {
              l2_dusk = jsonData['l2_dusk'];
              l2_motion = jsonData['l2_motion'];
              l2_dusk_motion = jsonData['l2_dusk_motion'];
              l2_manual_mode = false;
              l2_manual_off = false;
              l2_manual_on = false;
            }
            else
            {
              l2_manual_mode = jsonData['l2_manual_mode'];
              if (l2_manual_mode)
              {
                l2_manual_on = jsonData['l2_manual_on'];
                l2_manual_off = jsonData['l2_manual_off'];
              }
              else
              {
                l2_manual_on = false;
                l2_manual_off = false;
              }
              l2_dusk_motion = false;
              l2_motion = false;
              l2_dusk = false;
            }
            preferences.putBool("l1_auto_mode", l1_auto_mode);
            preferences.putBool("l1_manual_mode", l1_manual_mode);
            preferences.putBool("l1_manual_off", l1_manual_off);
            preferences.putBool("l1_manual_on", l1_manual_on);
            preferences.putBool("l1_time_off_iS", l1_time_off_iS);
            preferences.putUInt("l1_off_hour", l1_time_off_hour);
            preferences.putUInt("l1_off_min", l1_time_off_min);
            preferences.putUInt("l1_on_hour", l1_time_on_hour);
            preferences.putUInt("l1_on_min", l1_time_on_min);
            preferences.putBool("l2_auto_mode", l2_auto_mode);
            preferences.putBool("l2_manual_mode", l2_manual_mode);
            preferences.putBool("l2_manual_off", l2_manual_off);
            preferences.putBool("l2_manual_on", l2_manual_on);
            preferences.putBool("l2_dusk", l2_dusk);
            preferences.putBool("l2_motion", l2_motion);
            preferences.putBool("l2_dusk_motion", l2_dusk_motion);
          }
          else if (strcmp(cfgType, "water_pump") == 0)
          {
            wp_auto_mode = jsonData['wp_auto_mode'];
            if (wp_auto_mode)
            {
              wp_humidity = jsonData['wp_humidity_below'];
              wp_duration_iS = jsonData['wp_duration_time_is_set'];
              if (wp_duration_iS)
              {
                wp_duration = jsonData['wp_duration_time'];
              }
              else
              {
                wp_duration = 0;
              }
              wp_manual_mode = false;
              wp_manual_off = false;
              wp_manual_on = false;
            }
            else
            {
              wp_manual_mode = jsonData['wp_manual_mode'];
              if (wp_manual_mode)
              {
                wp_manual_on = jsonData['wp_manual_on'];
                wp_manual_off = jsonData['wp_manual_off'];
              }
              else
              {
                wp_manual_on = false;
                wp_manual_off = false;
              }
              wp_duration_iS = false;
              wp_duration = 0;
              wp_humidity = 0;
            }
            preferences.putBool("wp_auto_mode", wp_auto_mode);
            preferences.putBool("wp_manual_mode", wp_manual_mode);
            preferences.putBool("wp_manual_on", wp_manual_on);
            preferences.putBool("wp_manual_off", wp_manual_off);
            preferences.putBool("wp_duration_iS", wp_duration_iS);
            preferences.putUInt("wp_duration", wp_duration);
            preferences.putUInt("wp_humidity", wp_humidity);
          }
        }
      }
    }
  }
}

void webSocketNotify(String values)
{
  ws.textAll(values);
}

void readSensorsValues()
{
  if (BME280_Status)
  {
    BME280_Temperature = hBME280.readTemperature();
    BME280_Humidity = hBME280.readHumidity();
  }
  if (BMP280_Status)
  {
    BMP280_Temperature = hBMP280.readTemperature();
  }
  if (BH1750_Status)
  {
    BH1750_Light = hBH1750.readLightLevel();
  }
  if (digitalRead(HCSR501_PORT) == HIGH)
  {
    HCSR501_Motion = true;
  }
  else
  {
    HCSR501_Motion = false;
  }
}

void operateGreenHouse()
{
  if (gh_auto_mode)
  {
    if (gh_temp_off_iS && gh_temp_off <= BME280_Temperature)
    {
      if (ledcRead(GREENHOUSE_CH) != 0)
      {
        ledcWrite(GREENHOUSE_CH, 0);
      }
    }
    else
    {
      float PWM_Duty = 1023.0 * PIDTemperatureControl(gh_target_temp, BMP280_Temperature);
      if (PWM_Duty < 0.0)
      {
        PWM_Duty = 0.0;
      }
      else if (PWM_Duty > 1023.0)
      {
        PWM_Duty = 1023.0;
      }
      unsigned int iPWM_Duty = round(PWM_Duty);
      ledcWrite(GREENHOUSE_CH, iPWM_Duty);
    }
  }
  else if (gh_manual_mode && gh_manual_on)
  {
    if (ledcRead(GREENHOUSE_CH) != 1023)
    {
      ledcWrite(GREENHOUSE_CH, 1023);
    }
  }
  else if (ledcRead(GREENHOUSE_CH) != 0)
  {
    ledcWrite(GREENHOUSE_CH, 0);
  }
}

void operateLed1()
{
  if (l1_auto_mode)
  {
    struct tm timeinfo;
    if (l1_time_off_iS && timeinfo.tm_hour == l1_time_off_hour && timeinfo.tm_min == l1_time_off_min)
    {
      if (digitalRead(LED1_PORT) == LOW)
      {
        LED1_State = HIGH;
        digitalWrite(LED1_PORT, HIGH);
        preferences.putUInt("LED1_State", LED1_State);
      }
    }
    else if (timeinfo.tm_hour == l1_time_on_hour && timeinfo.tm_min == l1_time_on_min)
    {
      if (digitalRead(LED1_PORT) == HIGH)
      {
        LED1_State = LOW;
        digitalWrite(LED1_PORT, LOW);
        preferences.putUInt("LED1_State", LED1_State);
      }
    }
  }
  else if (l1_manual_mode && l1_manual_on)
  {
    if (digitalRead(LED1_PORT) == HIGH)
    {
      LED1_State = LOW;
      digitalWrite(LED1_PORT, LOW);
      preferences.putUInt("LED1_State", LED1_State);
    }
  }
  else if (digitalRead(LED1_PORT) == LOW)
  {
    LED1_State = HIGH;
    digitalWrite(LED1_PORT, HIGH);
    preferences.putUInt("LED1_State", LED1_State);
  }
}

void operateLed2()
{
  if (l2_auto_mode)
  {
    if (((l2_dusk || l2_dusk_motion) && BH1750_Light <= BH1750_LightDusk) || ((l2_motion || l2_dusk_motion) && HCSR501_Motion))
    {
      if (digitalRead(LED2_PORT) == HIGH)
      {
        LED2_State = LOW;
        digitalWrite(LED2_PORT, LOW);
        preferences.putUInt("LED2_State", LED2_State);
      }
    }
    else if (digitalRead(LED2_PORT) == LOW)
    {
      LED2_State = HIGH;
      digitalWrite(LED2_PORT, HIGH);
      preferences.putUInt("LED2_State", LED2_State);
    }
  }
  else if (l2_manual_mode && l2_manual_on)
  {
    if (digitalRead(LED2_PORT) == HIGH)
    {
      LED2_State = LOW;
      digitalWrite(LED2_PORT, LOW);
      preferences.putUInt("LED2_State", LED2_State);
    }
  }
  else if (digitalRead(LED2_PORT) == LOW)
  {
    LED2_State = HIGH;
    digitalWrite(LED2_PORT, HIGH);
    preferences.putUInt("LED2_State", LED2_State);
  }
}

void operateWaterPump()
{
  if (wp_auto_mode)
  {
    if (wp_duration_iS && digitalRead(WATERPUMP_PORT) == LOW)
    {
      struct tm timeinfo;
      if (timeinfo.tm_mday == wp_duration_day)
      {
        wp_duration_sec++;
      }
      else
      {
        wp_duration_sec = 0;
        wp_duration_day = timeinfo.tm_mday;
        preferences.putUInt("wp_duration_day", wp_duration_day);
      }
      preferences.putUInt("wp_duration_sec", wp_duration_sec);
    }
    if (wp_duration_iS && wp_duration * 60 <= wp_duration_sec)
    {
      if (digitalRead(WATERPUMP_PORT) == LOW)
      {
        digitalWrite(WATERPUMP_PORT, HIGH);
      }
    }
    else if (BME280_Humidity <= wp_humidity)
    {
      if (digitalRead(WATERPUMP_PORT) == HIGH)
      {
        digitalWrite(WATERPUMP_PORT, LOW);
      }
    }
  }
  else if (wp_manual_mode && wp_manual_on)
  {
    if (digitalRead(WATERPUMP_PORT) == HIGH)
    {
      digitalWrite(WATERPUMP_PORT, LOW);
    }
  }
  else if (digitalRead(WATERPUMP_PORT) == LOW)
  {
    digitalWrite(WATERPUMP_PORT, HIGH);
  }
}

float PIDTemperatureControl(float setPoint, float measured)
{
  float u, P, I, D, error, integral, derivative;

  error = setPoint - measured;
  integral = PID_previous_integral + (error + PID_previous_error);
  derivative = (error - PID_previous_error) / PID_dt;

  PID_previous_integral = integral;
  PID_previous_error = error;

  P = PID_Kp * error;
  I = PID_Ki * integral * (PID_dt / 2.0);
  D = PID_Kd * derivative;

  u = P + I + D;
  return u;
}

String JSONGreenHouseValues()
{
  String JSONString;
  StaticJsonDocument<300> GreenHouseValues;
  GreenHouseValues["gh_auto_mode"] = gh_auto_mode;
  GreenHouseValues["gh_manual_mode"] = gh_manual_mode;
  GreenHouseValues["gh_manual_on"] = gh_manual_on;
  GreenHouseValues["gh_manual_off"] = gh_manual_off;
  GreenHouseValues["gh_target_temp"] = gh_target_temp;
  GreenHouseValues["gh_temp_off_is_set"] = gh_temp_off_iS;
  GreenHouseValues["gh_temp_off"] = gh_temp_off;
  serializeJson(GreenHouseValues, JSONString);
  return JSONString;
}

String JSONLedsValues()
{
  String JSONString;
  StaticJsonDocument<400> LampsValues;
  LampsValues["l1_auto_mode"] = l1_auto_mode;
  LampsValues["l1_manual_mode"] = l1_manual_mode;
  LampsValues["l1_manual_on"] = l1_manual_on;
  LampsValues["l1_manual_off"] = l1_manual_off;
  LampsValues["l1_time_on_hour"] = l1_time_on_hour;
  LampsValues["l1_time_on_min"] = l1_time_on_min;
  LampsValues["l1_time_off_is_set"] = l1_time_off_iS;
  LampsValues["l1_time_off_hour"] = l1_time_off_hour;
  LampsValues["l1_time_off_min"] = l1_time_off_min;
  LampsValues["l2_auto_mode"] = l2_auto_mode;
  LampsValues["l2_manual_mode"] = l2_manual_mode;
  LampsValues["l2_manual_on"] = l2_manual_on;
  LampsValues["l2_manual_off"] = l2_manual_off;
  LampsValues["l2_dusk"] = l2_dusk;
  LampsValues["l2_motion"] = l2_motion;
  LampsValues["l2_dusk_motion"] = l2_dusk_motion;
  serializeJson(LampsValues, JSONString);
  return JSONString;
}

String JSONWaterPumpValues()
{
  String JSONString;
  StaticJsonDocument<300> WaterPumpValues;
  WaterPumpValues["wp_auto_mode"] = wp_auto_mode;
  WaterPumpValues["wp_manual_mode"] = wp_manual_mode;
  WaterPumpValues["wp_manual_on"] = wp_manual_on;
  WaterPumpValues["wp_manual_off"] = wp_manual_off;
  WaterPumpValues["wp_humidity_below"] = wp_humidity;
  WaterPumpValues["wp_duration_time_is_set"] = wp_duration_iS;
  WaterPumpValues["wp_duration_time"] = wp_duration;
  serializeJson(WaterPumpValues, JSONString);
  return JSONString;
}

String JSONInformationValues()
{
  char sBuffer[20];
  String JSONString;
  struct tm timeinfo;
  StaticJsonDocument<200> InformationValues;
  if (getLocalTime(&timeinfo))
  {
    strftime(sBuffer, sizeof(sBuffer), "%H:%M:%S", &timeinfo);
  }
  else
  {
    snprintf(sBuffer, sizeof(sBuffer), "xx:xx:xx");
  }
  InformationValues["current-time"] = sBuffer;
  snprintf(sBuffer, sizeof(sBuffer), "%0.2f °C", BMP280_Temperature);
  InformationValues["current-greenhouse-temp"] = sBuffer;
  snprintf(sBuffer, sizeof(sBuffer), "%d °C", gh_target_temp);
  InformationValues["target-greenhouse-temp"] = sBuffer;
  snprintf(sBuffer, sizeof(sBuffer), "%0.2f °C", BME280_Temperature);
  InformationValues["outside-temp"] = sBuffer;
  snprintf(sBuffer, sizeof(sBuffer), "%0.2f %%", BME280_Humidity);
  InformationValues["humidity"] = sBuffer;
  if (digitalRead(LED1_PORT) == HIGH)
  {
    snprintf(sBuffer, sizeof(sBuffer), "Wyłączona");
  }
  else
  {
    snprintf(sBuffer, sizeof(sBuffer), "Włączona");
  }
  InformationValues["lamp-1-state"] = sBuffer;
  if (digitalRead(LED2_PORT) == HIGH)
  {
    snprintf(sBuffer, sizeof(sBuffer), "Wyłączona");
  }
  else
  {
    snprintf(sBuffer, sizeof(sBuffer), "Włączona");
  }
  InformationValues["lamp-2-state"] = sBuffer;
  if (digitalRead(WATERPUMP_PORT) == HIGH)
  {
    snprintf(sBuffer, sizeof(sBuffer), "Wyłączona");
  }
  else
  {
    snprintf(sBuffer, sizeof(sBuffer), "Włączona");
  }
  InformationValues["water-pump-state"] = sBuffer;
  serializeJson(InformationValues, JSONString);
  return JSONString;
}
