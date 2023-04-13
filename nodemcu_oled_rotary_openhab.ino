#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// my local config
// git update-index --assume-unchanged config.h
#include "config.h"

// base version from:
// https://github.com/igorantolic/ai-esp32-rotary-encoder.git

#define ROTARY_ENCODER_A_PIN 12
#define ROTARY_ENCODER_B_PIN 14
#define ROTARY_ENCODER_BUTTON_PIN 13

#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */

//depending on your encoder - try 1,2 or 4 to get expected behaviour
#define ROTARY_ENCODER_STEPS 4

#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define Y_START_YELLOW 0
#define Y_START_BLUE 17

//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);

//const char *host = "http://192.168.178.69:8080/rest/items";
const char * host = "http://192.168.178.69:8080/rest/items/Shelly25_2_P";
uint16_t reload_url_time = 2000;
uint8_t menu_level = 0;

WiFiClient wifiClient;
static unsigned long lastTimeLooped = 0;

void connect_wifi() {
  IPAddress ip;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  ip = WiFi.localIP();
  Serial.println(ip);
}

void rotary_onButtonClick()
{
	static unsigned long lastTimePressed = 0;
	//ignore multiple press in that time milliseconds
	if (millis() - lastTimePressed < 500)
	{
		return;
	}
	lastTimePressed = millis();
	Serial.print("button pressed ");
	Serial.print(millis());
	Serial.println(" milliseconds after restart");

  if(0 == menu_level)
    menu_level = 1;
  else
    menu_level = 0;

  lastTimeLooped = 0;
}

void rotary_loop()
{
	//dont print anything unless value changed
	if (rotaryEncoder.encoderChanged())
	{
		Serial.print("Value: ");
		Serial.println(rotaryEncoder.readEncoder());
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

void get_items() 
{
  HTTPClient http;    //Declare object of class HTTPClient

  http.begin(wifiClient, host);     //Specify request destination
  
  int httpCode = http.GET();            //Send the request
  String payload = http.getString();    //Get the response payload from server

  Serial.print("Returned data from Server:");
  Serial.println(payload);    //Print request response payload

  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  const char* item = doc["name"];
  const char* state = doc["state"];
  Serial.print(item);
  Serial.print(":");
  Serial.println(state);

  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(5, 1);
  display.println("openhab");

  display.display();

  http.end();  //Close connection
}

void get_item_data(const char * host) 
{
  HTTPClient http;    //Declare object of class HTTPClient

  Serial.print("Request Link:");
  Serial.println(host);
  
  http.begin(wifiClient, host);     //Specify request destination
  
  int httpCode = http.GET();            //Send the request
  String payload = http.getString();    //Get the response payload from server

  Serial.print("Response Code:"); //200 is OK
  Serial.println(httpCode);   //Print HTTP return code

  Serial.print("Returned data from Server:");
  Serial.println(payload);    //Print request response payload

  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  const char* item = doc["name"];
  const char* state = doc["state"];
  Serial.print(item);
  Serial.print(":");
  Serial.println(state);

  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(5, 1);
  display.println("openhab > item");

  display.setTextSize(3);
  display.setCursor(15, 20);
  display.println(state);

  display.setTextSize(1);
  display.setCursor(2, 55);
  display.println(item);

  display.display();

  http.end();  //Close connection
}

void setup()
{
	Serial.begin(9600);

	//we must initialize rotary encoder
	rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	//set boundaries and if values should cycle or not
	//in this example we will set possible values between 0 and 1000;
	bool circleValues = false;
	rotaryEncoder.setBoundaries(0, 1000, circleValues); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)

	/*Rotary acceleration introduced 25.2.2021.
   * in case range to select is huge, for example - select a value between 0 and 1000 and we want 785
   * without accelerateion you need long time to get to that number
   * Using acceleration, faster you turn, faster will the value raise.
   * For fine tuning slow down.
   */
	//rotaryEncoder.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
	rotaryEncoder.setAcceleration(250); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration

  connect_wifi();
  
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  Serial.println("setup done");
}

void loop()
{
  int timeSinceLastLoop = millis() - lastTimeLooped;
	if (reload_url_time < timeSinceLastLoop)
	{
    lastTimeLooped = millis();
    if (1 == menu_level) {
		  get_item_data(host);
    } else {
      get_items();
    }
	}

	//in loop call your custom function which will process rotary encoder values
	rotary_loop();
	delay(50); //or do whatever you need to do...
}