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
const int BH1750_LightDusk = 20;

int HCSR501_PORT = 13;
bool HCSR501_Motion = false;

bool gh_time_on_isSet = false;
bool gh_time_off_isSet = false;
int gh_target_temperature = 30;
int gh_hour_on = 0;
int gh_minute_on = 0;
int gh_hour_off = 0;
int gh_minute_off = 0;

const int LED1_PORT = 26;
const int LED2_PORT = 27;
bool led_1_time_on_isSet = false;
bool led_1_time_off_isSet = false;
bool led_2_dusk = false;
bool led_2_motion = false;
int led_1_hour_on = 0;
int led_1_minute_on = 0;
int led_1_hour_off = 0;
int led_1_minute_off = 0;

const int WATERPUMP_PORT = 25;
bool wp_temperature_above_isSet = false;
bool wp_temperature_below_isSet = false;
bool wp_humidity_above_isSet = false;
bool wp_humidity_below_isSet = false;
int wp_temperature_above = 0;
int wp_temperature_below = 0;
int wp_humidity_above = 0;
int wp_humidity_below = 0;

const int freq = 2000;
const int ledChannel = 0;
const int resolution = 10;

int ilosc = 0;
unsigned long previousMillis;
unsigned long previousMillisWeb;
unsigned long currentMillis;
const unsigned long period = 100;
const unsigned long periodWeb = 1000;

void setup()
{
  ilosc = 0;
  Serial.begin(115200);
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(12, ledChannel);
  ledcWrite(ledChannel, 0);
  digitalWrite(LED1_PORT, HIGH);
  digitalWrite(LED2_PORT, HIGH);
  Serial.println("--- INIT TEST ---");
  initFS();
  initWiFi();
  initSettingsData();
  initWebServerSocket();
  initTime();
  initSensors();
  previousMillis = millis();
}

void loop()
{
  currentMillis = millis();
  if(currentMillis - previousMillis >= period)
  {
    readSensorsValues();
    float PWM_Duty = 1023.0 * PIDTemperatureControl(27.3, BMP280_Temperature);
    if(PWM_Duty < 0.0)
    {
      PWM_Duty = 0.0;
    }
    else if(PWM_Duty > 1023.0)
    {
      PWM_Duty = 1023.0;
    }
    int iPWM_Duty = round(PWM_Duty);
    Serial.print(iPWM_Duty);
    Serial.print(";");
    Serial.println(BMP280_Temperature);
    ledcWrite(ledChannel, iPWM_Duty);
    previousMillis = millis();
  }
  if(currentMillis - previousMillisWeb >= periodWeb)
  {
    webSocketNotify(JSONInformationValues());
    previousMillisWeb = millis();
  }
  //  operateLamps();
  //  ws.cleanupClients();
  //
  //  if(HCSR501_Motion)
  //  {
  //      Serial.println("[HC] Wykryto ruch");
  //  }
  //  if(++ilosc == 50)
  //  {
  //    ilosc = 0;
  //    if(digitalRead(WATERPUMP_PORT) == HIGH)
  //    {
  //      digitalWrite(WATERPUMP_PORT, LOW);
  //      Serial.println("[POMPA] NISKI");
  //    }
  //    else
  //    {
  //      digitalWrite(WATERPUMP_PORT, HIGH);
  //      Serial.println("[POMPA] WYSOKI");
  //    }
  //    if(digitalRead(LED1_PORT) == HIGH)
  //    {
  //      digitalWrite(LED1_PORT, LOW);
  //      digitalWrite(LED2_PORT, HIGH);
  //      Serial.println("[LED1] NISKI");
  //      Serial.println("[LED2] WYSOKI");
  //    }
  //    else
  //    {
  //      digitalWrite(LED1_PORT, HIGH);
  //      digitalWrite(LED2_PORT, LOW);
  //      Serial.println("[LED1] WYSOKI");
  //      Serial.println("[LED2] NISKI");
  //    }
  //  }
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
  Serial.println("WebSocket: OK");
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

void initSettingsData()
{
  pinMode(LED1_PORT, OUTPUT);
  pinMode(LED2_PORT, OUTPUT);
  pinMode(WATERPUMP_PORT, OUTPUT);
  preferences.begin("SmartGarden", false);
  // --- GreenHouse ---
  gh_time_on_isSet = preferences.getBool("gh_time_on_iS", false);
  gh_time_off_isSet = preferences.getBool("gh_time_off_iS", false);
  gh_target_temperature = preferences.getInt("gh_t_temp", 30);
  gh_hour_on = preferences.getInt("gh_hour_on", 0);
  gh_minute_on = preferences.getInt("gh_minute_on", 0);
  gh_hour_off = preferences.getInt("gh_hour_off", 0);
  gh_minute_off = preferences.getInt("gh_minute_off", 0);
  // --- LED ---
  led_1_time_on_isSet = preferences.getBool("l1_time_on_iS", false);
  led_1_time_off_isSet = preferences.getBool("l1_time_off_iS", false);
  led_2_dusk = preferences.getBool("l2_dusk", false);
  led_2_motion = preferences.getBool("l2_motion", false);
  led_1_hour_on = preferences.getInt("l1_hour_on", 0);
  led_1_minute_on = preferences.getInt("l1_minute_on", 0);
  led_1_hour_off = preferences.getInt("l1_hour_off", 0);
  led_1_minute_off = preferences.getInt("l1_minute_off", 0);
  // --- WaterPump ---
  wp_temperature_above_isSet = preferences.getBool("wp_temp_a_iS", false);
  wp_temperature_below_isSet = preferences.getBool("wp_temp_b_iS", false);
  wp_humidity_above_isSet = preferences.getBool("wp_hum_a_iS", false);
  wp_humidity_below_isSet = preferences.getBool("wp_hum_b_iS", false);
  wp_temperature_above = preferences.getInt("wp_temp_a", 0);
  wp_temperature_below = preferences.getInt("wp_temp_b", 0);
  wp_humidity_above = preferences.getInt("wp_hum_a", 0);
  wp_humidity_below = preferences.getInt("wp_hum_b", 0);
  Serial.println("Settings: OK");
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
  Serial.println();
}

void webSocketOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if(type == WS_EVT_DATA)
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
      webSocketNotify(JSONLampsValues());
      webSocketNotify(JSONWaterPumpValues());
      webSocketNotify(JSONInformationValues());
    }
    else
    {
      StaticJsonDocument<400> jsonData;
      DeserializationError error = deserializeJson(jsonData, (char*)data);
      if (!error)
      {
        if (jsonData.containsKey("config_type"))
        {
          const char* configType = jsonData["config_type"];
          if (strcmp(configType, "greenhouse") == 0)
          {
            gh_target_temperature = jsonData['target_temperature'];
            gh_time_on_isSet = jsonData['time_on_isSet'];
            if (gh_time_on_isSet)
            {
              gh_hour_on = jsonData['hour_on'];
              gh_minute_on = jsonData['minute_on'];
            }
            else
            {
              gh_hour_on = 0;
              gh_minute_on = 0;
            }
            gh_time_off_isSet = jsonData['time_off_isSet'];
            if (gh_time_off_isSet)
            {
              gh_hour_off = jsonData['hour_off'];
              gh_minute_off = jsonData['minute_off'];
            }
            else
            {
              gh_hour_off = 0;
              gh_minute_off = 0;
            }
            preferences.putBool("gh_time_on_iS", gh_time_on_isSet);
            preferences.putBool("gh_time_off_iS", gh_time_off_isSet);
            preferences.putInt("gh_t_temp", gh_target_temperature);
            preferences.putInt("gh_hour_on", gh_hour_on);
            preferences.putInt("gh_minute_on", gh_minute_on);
            preferences.putInt("gh_hour_off", gh_hour_off);
            preferences.putInt("gh_minute_off", gh_minute_off);
          }
          else if (strcmp(configType, "leds") == 0)
          {
            led_1_time_on_isSet = jsonData['led_1_time_on_isSet'];
            if (led_1_time_on_isSet)
            {
              led_1_hour_on = jsonData['led_1_hour_on'];
              led_1_minute_on = jsonData['led_1_minute_on'];
            }
            else
            {
              led_1_hour_on = 0;
              led_1_minute_on = 0;
            }
            led_1_time_off_isSet = jsonData['led_1_time_off_isSet'];
            if (led_1_time_off_isSet)
            {
              led_1_hour_off = jsonData['led_1_hour_off'];
              led_1_minute_off = jsonData['led_1_minute_off'];
            }
            else
            {
              led_1_hour_off = 0;
              led_1_minute_off = 0;
            }
            led_2_dusk = jsonData['led_2_dusk'];
            led_2_motion = jsonData['led_2_motion'];
            preferences.putBool("l1_time_on_iS", led_1_time_on_isSet);
            preferences.putBool("l1_time_off_iS", led_1_time_off_isSet);
            preferences.putBool("l2_dusk", led_2_dusk);
            preferences.putBool("l2_motion", led_2_motion);
            preferences.putInt("l1_hour_on", led_1_hour_on);
            preferences.putInt("l1_minute_on", led_1_minute_on);
            preferences.putInt("l1_hour_off", led_1_hour_off);
            preferences.putInt("l1_minute_off", led_1_minute_off);
          }
          else if (strcmp(configType, "water_pump") == 0)
          {
            wp_temperature_above_isSet = jsonData['temperature_above_isSet'];
            if (wp_temperature_above_isSet)
            {
              wp_temperature_above = jsonData['temperature_above'];
            }
            else
            {
              wp_temperature_above = 0;
            }
            wp_temperature_below_isSet = jsonData['temperature_below_isSet'];
            if (wp_temperature_below_isSet)
            {
              wp_temperature_below = jsonData['temperature_below'];
            }
            else
            {
              wp_temperature_below = 0;
            }
            wp_humidity_above_isSet = jsonData['humidity_above_isSet'];
            if (wp_humidity_above_isSet)
            {
              wp_humidity_above = jsonData['humidity_above'];
            }
            else
            {
              wp_humidity_above = 0;
            }
            wp_humidity_below_isSet = jsonData['humidity_below_isSet'];
            if (wp_humidity_below_isSet)
            {
              wp_humidity_below = jsonData['humidity_below'];
            }
            else
            {
              wp_humidity_below = 0;
            }
            preferences.putBool("wp_temp_a_iS", wp_temperature_above_isSet);
            preferences.putBool("wp_temp_b_iS", wp_temperature_below_isSet);
            preferences.putBool("wp_hum_a_iS", wp_humidity_above_isSet);
            preferences.putBool("wp_hum_b_iS", wp_humidity_below_isSet);
            preferences.putInt("wp_temp_a", wp_temperature_above);
            preferences.putInt("wp_temp_b", wp_temperature_below);
            preferences.putInt("wp_hum_a", wp_humidity_above);
            preferences.putInt("wp_hum_b", wp_humidity_below);
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

void operateLamps()
{
  struct tm timeinfo;
  if (led_1_time_on_isSet && timeinfo.tm_hour == led_1_hour_on && timeinfo.tm_min == led_1_minute_on)
  {
    if (digitalRead(LED1_PORT) == LOW)
    {
      digitalWrite(LED1_PORT, HIGH);
    }
  }
  else if (led_1_time_off_isSet && timeinfo.tm_hour == led_1_hour_off && timeinfo.tm_min == led_1_minute_off)
  {
    if (digitalRead(LED1_PORT) == HIGH)
    {
      digitalWrite(LED1_PORT, LOW);
    }
  }
  if ((led_2_dusk && BH1750_Light <= BH1750_LightDusk) || (led_2_motion && HCSR501_Motion))
  {
    if (digitalRead(LED2_PORT) == LOW)
    {
      digitalWrite(LED2_PORT, HIGH);
    }
  }
  else if (digitalRead(LED2_PORT) == HIGH)
  {
    digitalWrite(LED2_PORT, LOW);
  }
}

void operateWaterPump()
{
  if (wp_humidity_above_isSet && BME280_Humidity >= wp_humidity_above)
  {
    if (digitalRead(WATERPUMP_PORT) == HIGH)
    {
      digitalWrite(WATERPUMP_PORT, LOW);
    }
  }
  else if (wp_temperature_above_isSet && BME280_Temperature >= wp_temperature_above)
  {
    if (digitalRead(WATERPUMP_PORT) == HIGH)
    {
      digitalWrite(WATERPUMP_PORT, LOW);
    }
  }
  else if (wp_humidity_below_isSet && BME280_Humidity <= wp_humidity_below)
  {
    if (!wp_temperature_below_isSet || (wp_temperature_below_isSet && BME280_Temperature <= wp_temperature_below))
    {
      if (digitalRead(WATERPUMP_PORT) == LOW)
      {
        digitalWrite(WATERPUMP_PORT, HIGH);
      }
    }
  }
}

float PID_previous_error = 0.0;
float PID_previous_integral = 0.0;
float PID_Kp = 0.0453165;
float PID_Ki = 0.000163058;
float PID_Kd = 0.6988927;
float PID_dt = 0.1;

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
  StaticJsonDocument<200> GreenHouseValues;
  GreenHouseValues["target_temperature"] = gh_target_temperature;
  GreenHouseValues["time_on_isSet"] = gh_time_on_isSet;
  GreenHouseValues["hour_on"] = gh_hour_on;
  GreenHouseValues["minute_on"] = gh_minute_on;
  GreenHouseValues["time_off_isSet"] = gh_time_off_isSet;
  GreenHouseValues["hour_off"] = gh_hour_off;
  GreenHouseValues["minute_off"] = gh_minute_off;
  serializeJson(GreenHouseValues, JSONString);
  return JSONString;
}

String JSONLampsValues()
{
  String JSONString;
  StaticJsonDocument<200> LampsValues;
  LampsValues["led_1_time_on_isSet"] = led_1_time_on_isSet;
  LampsValues["led_1_hour_on"] = led_1_hour_on;
  LampsValues["led_1_minute_on"] = led_1_minute_on;
  LampsValues["led_1_time_off_isSet"] = led_1_time_off_isSet;
  LampsValues["led_1_hour_off"] = led_1_hour_off;
  LampsValues["led_1_minute_off"] = led_1_minute_off;
  LampsValues["led_2_dusk"] = led_2_dusk;
  LampsValues["led_2_motion"] = led_2_motion;
  serializeJson(LampsValues, JSONString);
  return JSONString;
}

String JSONWaterPumpValues()
{
  String JSONString;
  StaticJsonDocument<200> WaterPumpValues;
  WaterPumpValues["temperature_above_isSet"] = wp_temperature_above_isSet;
  WaterPumpValues["temperature_above"] = wp_temperature_above;
  WaterPumpValues["temperature_below_isSet"] = wp_temperature_below_isSet;
  WaterPumpValues["temperature_below"] = wp_temperature_below;
  WaterPumpValues["humidity_above_isSet"] = wp_humidity_above_isSet;
  WaterPumpValues["humidity_above"] = wp_humidity_above_isSet;
  WaterPumpValues["humidity_below_isSet"] = wp_humidity_below_isSet;
  WaterPumpValues["humidity_below"] = wp_humidity_below;
  serializeJson(WaterPumpValues, JSONString);
  return JSONString;
}

String JSONInformationValues()
{
  char sBuffer[20];
  String JSONString;
  struct tm timeinfo;
  StaticJsonDocument<200> InformationValues;
  if(getLocalTime(&timeinfo))
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
  snprintf(sBuffer, sizeof(sBuffer), "%d °C", gh_target_temperature);
  InformationValues["target-greenhouse-temp"] = sBuffer;
  snprintf(sBuffer, sizeof(sBuffer), "%0.2f °C", BME280_Temperature);
  InformationValues["outside-temp"] = sBuffer;
  snprintf(sBuffer, sizeof(sBuffer), "%0.2f %%", BME280_Humidity);
  InformationValues["humidity"] = sBuffer;
  if(digitalRead(LED1_PORT) == HIGH)
  {
    snprintf(sBuffer, sizeof(sBuffer), "Wyłączona");
  }
  else
  {
    snprintf(sBuffer, sizeof(sBuffer), "Włączona");
  }
  InformationValues["lamp-1-state"] = sBuffer;
  if(digitalRead(LED2_PORT) == HIGH)
  {
    snprintf(sBuffer, sizeof(sBuffer), "Wyłączona");
  }
  else
  {
    snprintf(sBuffer, sizeof(sBuffer), "Włączona");
  }
  InformationValues["lamp-2-state"] = sBuffer;
  if(digitalRead(WATERPUMP_PORT) == HIGH)
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
