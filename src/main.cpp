#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <SPI.h>
#include "Adafruit_LTR329_LTR303.h"
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <secrets.h>
#include <HTTPClient.h>
#include <Pushsafer.h>
#include <WiFiManager.h>

//pushsafer
bool daylight = true;

struct PushSaferInput input_temp;
struct PushSaferInput input_hum;
struct PushSaferInput input_light;

WiFiClient client;
Pushsafer pushsafer(PushsaferKey, client);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LTR329 ltr = Adafruit_LTR329();
uint8_t loopCnt = 0;
uint16_t v_p_i, inf;

//pushsafer ends


void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
      bool res;

    res = wm.autoConnect(); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }

  pinMode(16, OUTPUT);
  

  Serial.println("SHT31 test");
  if (!sht31.begin(0x44))
  { 
    Serial.println("Couldn't find SHT31 sensor!");
    while (1)
      delay(1);
  }
  Serial.println("Found SHT sensor!");


  ltr.setGain(LTR3XX_GAIN_1);
  
  ltr.setIntegrationTime(LTR3XX_INTEGTIME_50);

  ltr.setMeasurementRate(LTR3XX_MEASRATE_50);

  Serial.println("LTR test");
  if (!ltr.begin())
  {
    Serial.println("Couldn't find LTR sensor!");
    while (1)
      delay(10);
  }
  Serial.println("Found LTR sensor!");

 
  delay(100);


  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //pushsafer

  input_temp.sound = "8";
  input_temp.vibration = "1";
  input_temp.icon = "1";
  input_temp.iconcolor = "#FFCCCC";
  input_temp.priority = "1";
  input_temp.device = "62734";
  input_temp.url = "https://www.pushsafer.com";
  input_temp.urlTitle = "Open Pushsafer.com";

  input_hum = input_temp;
  input_light = input_temp;

}

void loop()
{
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(t))
  { // check if 'is not a number'
    Serial.print("Temp *C = ");
    Serial.print(t);
    Serial.print("\t\t");
  }
  else
  {
    Serial.println("Failed to read temperature");
  }

  if (!isnan(h))
  { // check if 'is not a number'
    Serial.print("Hum. % = ");
    Serial.println(h);
  }
  else
  {
    Serial.println("Failed to read humidity");
  }

  bool valid;
  uint16_t visible_plus_ir, infrared;

  if (ltr.newDataAvailable())
  {
    valid = ltr.readBothChannels(visible_plus_ir, infrared);
    if (valid)
    {
      Serial.print("CH0 Visible + IR: ");
      Serial.print(visible_plus_ir);
      Serial.print("\t\tCH1 Infrared: ");
      Serial.println(infrared);
    }
  }

//connect with API
  if (WiFi.status() == WL_CONNECTED)
  {

    StaticJsonDocument<96> doc;
    doc["device"] = DEVICE;
    doc["temp"] = t ;
    doc["hum"] = h ;
    doc["light"] = visible_plus_ir;
    String output;

    serializeJson(doc, output);
    Serial.println(output);


    HTTPClient http;

    http.begin(API_URI);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(output);
    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    }
    else
    {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  else

  {
    Serial.println("Error in WiFi connection");
  }

//Temperature warnings pushsafer

  if ( t >= 30.0 ){
    input_temp.message = "Temperature: " + String(t);
    input_temp.title = "too hot!";

  }else if ( t <= 25.0 ){
    input_temp.message = "Temperature: " + String(t);
    input_temp.title = "too cold!";
  }
  pushsafer.sendEvent(input_temp);
  Serial.println("Sent");

  //humidity warnings
  if ( h >= 60.0 ){
    input_hum.message = "Humidity: " + String(h);
    input_hum.title = "too moist!";
  }else if ( h <= 30.0 ){
    input_hum.message = "Humidity: " + String(h);
    input_hum.title = "too dry!";
  }
  pushsafer.sendEvent(input_hum);
  Serial.println("Sent");

  // light warnings
  if (visible_plus_ir <= 10.0){
    if (daylight) {
      daylight = false;
      input_light.message = "light= " + String(visible_plus_ir);
      input_light.title = "dark";
      pushsafer.sendEvent(input_light);
      Serial.println("Sent");
    }
  } else {
    if (!daylight){
      daylight= true;
      input_light.message = "light= " + String(visible_plus_ir);
      input_light.title = "bright";
      pushsafer.sendEvent(input_light);
      Serial.println("Sent");
    }
  }

  delay(10000);
}
