#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"

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

#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */

//depending on your encoder - try 1,2 or 4 to get expected behaviour
#define ROTARY_ENCODER_STEPS 4

//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

//const char *host = "http://192.168.178.69:8080/rest/items";
const char *host = "http://192.168.178.69:8080/rest/items/Local_Weather_and_Forecast_Cloudiness";

WiFiClient wifiClient;

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

void get_data() 
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
  const char* item0 = doc["name"];
  Serial.println(item0);

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
  
  get_data();

  Serial.println("setup done");
}

void loop()
{
	//in loop call your custom function which will process rotary encoder values
	rotary_loop();
	delay(50); //or do whatever you need to do...
}