#include <Wire.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "SinricProTemperaturesensor.h"

#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif 

// wifi config
#define WIFI_SSID         ""
#define WIFI_PASS         ""

// sinric config
#define APP_KEY           ""
#define APP_SECRET        ""
#define SWITCH_ID         ""
#define TEMP_SENSOR_ID    "" 

// pinout
#define RELAY_PIN 4
#define DHT_PIN 2

// loop step time in miliseconds
#define LOOP_STEP_TIME 100

// temperature after which the relay will be switched oof
#define TARGET_TEMPERATURE 28

// thermometer and display setup
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// state of the relay and the thermometer
bool relayState = false;
bool sensorState = false;

// timestamp of relay being on and time after it switches off
unsigned long relayOnMarker = 0;
unsigned long relayAutoOffTime = 120;

// timestamp of last temp/humidity report and time after new report is sent
unsigned long lastReportMarker = 0;
unsigned long reportTime = 10;

// setup is ran once, when the device boots up
void setup() {
  // setup serial monitor and relay output pin
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);

  // initialize exteernal components - relay, sensor, display
  toggleRelay(false);
  dht.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  
  // setup connections
  setupWiFi();
  setupSinricPro();
}

// setup function for WiFi connection - default
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

// setup function for SinricPro
void setupSinricPro() {
  // get device handlers
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];

  // set callbacks
  mySwitch.onPowerState(onPowerStateRelay);
  mySensor.onPowerState(onPowerStateSensor);

  // setup debug messages
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// when relay state toggles from Sinric
bool onPowerStateRelay(const String &deviceId, bool &state) {
  Serial.printf("Relay %s turned %s (via SinricPro) \r\n", deviceId.c_str(), state?"on":"off");
  updateRelayState(state);
  return true; 
}

// when sensor toggles - default behavior
bool onPowerStateSensor(const String &deviceId, bool &state) {
  Serial.printf("Temperaturesensor turned %s (via SinricPro) \r\n", state?"on":"off");
  sensorState = state;
  return true;
}

// loop is runs continuously when the device is on
void loop() {
  // get readings and report them it it's time
  float temperature = readTemperature();
  float humidity = readHumidity();
  maybeReportSensorData(temperature, humidity);
  
  // get time till auto relay turn off
  float time = getRemainingRelayTime();

  // turn off the relay if conditions are met 
  maybeTurnOffRelayBasedOnTime(time);
  maybeTurnOffRelayBasedOnTemperature(temperature);

  // update the display
  setDisplay(temperature, humidity, relayState, time);

  // handle Sinric and wait for next iteration
  SinricPro.handle();
  delay(LOOP_STEP_TIME);
}

// toggle relay locally
void toggleRelay(bool state){
  updateRelayState(state);
  // send state update to Sinric
  SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
  mySwitch.sendPowerStateEvent(relayState); 

  Serial.printf("Relay %s turned %s (programmaticaly)\r\n", mySwitch.getDeviceId().c_str(), relayState?"on":"off");
}

// change relay state logically, physically and update the timer
void updateRelayState(bool state){
  relayState = state;
  digitalWrite(RELAY_PIN, relayState?LOW:HIGH);
  if(relayState)
    relayOnMarker = millis();
  else
    relayOnMarker = 0;
}

// if enough time passed since last report, send a new one and reset the timer
void maybeReportSensorData(float temperature, float humidity){
  if(millis() - lastReportMarker >= reportTime * 1000){
    SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
    mySensor.sendTemperatureEvent(temperature, humidity);
    lastReportMarker = millis();
    Serial.println("Sent report - temperature: " + String(temperature) + "*C, humidity: " + String(humidity) + "%");
  }
}

// read temperature (with error handling)
float readTemperature(){
  float temperature = dht.readTemperature();
  if(isnan(temperature)){
    Serial.println("Failed to read temperature from DHT sensor!");
    return -1.0;
  }
  return temperature;
}

// read humidity (with error handling)
float readHumidity(){
  float humidity = dht.readHumidity();
  if(isnan(humidity)){
    Serial.println("Failed to read humidity from DHT sensor!");
    return -1.0;
  }
  return humidity;
}

// calculate remaining time until relay auto turn off
float getRemainingRelayTime(){
  if(relayOnMarker > 0)
    return (millis() - relayOnMarker) / 1000;
  return 0;
}

// turn off the relay if enough time has passed
void maybeTurnOffRelayBasedOnTime(float time){
  if(time >= relayAutoOffTime)
    toggleRelay(false);
}

// turn off the relay if target tmeperature is reached
void maybeTurnOffRelayBasedOnTemperature(float temperature){
  if(relayState && temperature >= TARGET_TEMPERATURE)
    toggleRelay(false);
}

// display sensor data, relay state and time if the relay is on
void setDisplay(float temperature, float humidity, bool relayState, float time){
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Temp: " + String(int(temperature)) + "*C\r\n"); 
  display.print("Humd: " + String(int(humidity)) + "%\r\n"); 
  display.print("Time: " + ((time>0)?String(int(relayAutoOffTime - time))+"s":"----") + "\r\n");
  display.print("Relay: " + String((relayState?"ON":"OFF")) + "\r\n");
  display.display();
}