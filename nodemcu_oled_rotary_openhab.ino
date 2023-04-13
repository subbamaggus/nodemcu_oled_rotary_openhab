#include "Arduino.h"

#include "AiEsp32RotaryEncoder.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

// my local config
// git update-index --assume-unchanged config.h
#include "config.h"

// base version from:
// https://github.com/igorantolic/ai-esp32-rotary-encoder.git

#define ROTARY_ENCODER_A_PIN 12
#define ROTARY_ENCODER_B_PIN 14
#define ROTARY_ENCODER_BUTTON_PIN 13
#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 4

#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define Y_START_YELLOW 0
#define Y_START_BLUE 17

#define ITEMS_PER_PAGE 6

//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);

static unsigned long lastTimePressed = 0;

static unsigned long lastTimeLooped = 0;
bool circleValues = false;

WiFiClient wifiClient;
const char *url_host = "http://192.168.178.69:8080/rest/items";
String url_current_item = "";

uint16_t reload_url_time = 10000;
uint8_t menu_level = 0;

uint8_t selected_item = 0;
uint8_t selected_line = 0;
uint8_t list_start = 0;
uint8_t list_length = 0;

DynamicJsonDocument items(20480);
DynamicJsonDocument item(20480);


void connect_wifi()
{
  IPAddress ip;

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting WIFI");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    ip = WiFi.localIP();
    Serial.println(ip);
  }
}

void rotary_onButtonClick()
{
  int timeSinceLastPress = millis() - lastTimePressed;

  if (timeSinceLastPress < 500)
  {
    return;
  }
  lastTimePressed = millis();
  Serial.print("button pressed ");
  Serial.print(millis());
  Serial.println(" milliseconds after restart");

  if(0 == menu_level)
  {
    menu_level = 1;
  }
  else
  {
    menu_level = 0;
  }

  lastTimeLooped = 0;
}

void rotary_loop()
{
  if (rotaryEncoder.encoderChanged())
  {
    Serial.print("Value: ");
    Serial.println(rotaryEncoder.readEncoder());

    selected_item = rotaryEncoder.readEncoder();
    list_start = selected_item - 2;
    selected_line = 2;
    if (selected_item >= list_length - 3)
    {
      list_start = list_length - ITEMS_PER_PAGE;
      selected_line = selected_item - list_start;
    }
    if ((selected_item < 2) || (list_length <= ITEMS_PER_PAGE))
    {
      list_start = 0;
      selected_line = selected_item;
    }
  }
  if (rotaryEncoder.isEncoderButtonClicked())
  {
    rotary_onButtonClick();
  }
}

void IRAM_ATTR readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

void get_items(String url)
{
  HTTPClient http;

  Serial.print("Request Link: ");
  Serial.println(url);

  http.begin(wifiClient, url);

  int httpCode = 0;
  String payload = "";
  while((200 != httpCode) || (payload == ""))
  {
    httpCode = http.GET();

    Serial.print("http response code: ");
    Serial.println(httpCode);

    payload = http.getString();

    Serial.print("Returned data from Server:");
    Serial.println(payload);
  }

  DeserializationError error = deserializeJson(items, payload);
  list_length = items.size();

  for (int n = 0; n < list_length; n++)
  {
    const char* item = items[n]["label"];

    Serial.print(n);
    Serial.print(") ");
    Serial.println(item);
  }

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }
  
  http.end();
}

void get_item_data(String url)
{
  HTTPClient http;

  Serial.print("Request Link:");
  Serial.println(url);

  http.begin(wifiClient, url);

  int httpCode = 0;
  String payload = "";
  while((200 != httpCode) || (payload == ""))
  {
    httpCode = http.GET();

    Serial.print("http response code: ");
    Serial.println(httpCode);

    payload = http.getString();

    Serial.print("Returned data from Server:");
    Serial.println(payload);
  }

  DeserializationError error = deserializeJson(item, payload);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }

  http.end();
}

void display_items()
{
  rotaryEncoder.setBoundaries(0, list_length - 1, circleValues);

  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(5, 1);
  display.println("openhab");

  display.setCursor(0, Y_START_BLUE - 1);

  display.fillRect(0, Y_START_BLUE - 1 + selected_line * 8, 128, 8, WHITE);

  for (int n = list_start; n < list_start + ITEMS_PER_PAGE; n++)
  {
    const char* item = items[n]["label"];
    const char* item_link = items[n]["link"];

    if(selected_item == n)
    {
      display.setTextColor(BLACK);
      url_current_item = item_link;
    } else
    {
      display.setTextColor(WHITE);
    }

    display.print(" ");
    display.println(item);
  }

  display.display();
}

void display_item_data()
{
  const char* local_item = item["name"];
  const char* local_state = item["state"];

  Serial.print(local_item);
  Serial.print(":");
  Serial.println(local_state);

  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(5, 1);
  display.println("openhab > item");

  display.setTextSize(3);
  display.setCursor(15, 20);
  display.println(local_state);

  display.setTextSize(1);
  display.setCursor(2, 55);
  display.println(local_item);

  display.display();
}

void setup()
{
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextWrap(false);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(15, 20);
  display.println("starting");
  display.display();

  Serial.begin(9600);

  connect_wifi();

  get_items(url_host);

  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.setBoundaries(0, 1000, circleValues);
  rotaryEncoder.setAcceleration(250);

  Serial.println("setup done");
}

void loop()
{
  int timeSinceLastLoop = millis() - lastTimeLooped;
  if (reload_url_time < timeSinceLastLoop)
  {
    lastTimeLooped = millis();

    if (1 == menu_level)
    {
      connect_wifi();
      get_item_data(url_current_item);
      display_item_data();
    }
  }

  if (0 == menu_level)
  {
    display_items();
  }

  rotary_loop();
}