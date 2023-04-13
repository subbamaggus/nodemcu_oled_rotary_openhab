# nodemcu_oled_rotary_openhab


## setup
hardware used:
- NodeMCU V3
- Adafruit SSD1306 128x64 OLED
- Rotary Encoder
- Breadboards

## function

use NodeMCU to read "items" from openhab instance
with rotary encoder go through items
with button select item
all info shown on screen/display

## wiring

https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
Section: ESP8266 12-E NodeMCU Kit

### Display <> NodeMCU

| Display | Pin Label | Pin Description |
|---------|-----------|-----------------|
| VCC     | 3V        | 3.3V            |
| GND     | G         | GND             |
| SCL     | D1        | GPIO5           |
| SDA     | D2        | GPIO4           |

### Rotary <> NodeMCU

| Rotary | Pin Label | Pin Description |
|--------|-----------|-----------------|
| GND    | G         | GND             |
| +      | 3V        | 3.3V            |
| SW     | D7        | GPIO13          |
| DT     | D5        | GPIO14          |
| CLK    | D6        | GPIO12          |

## starting point
started with example from 
https://github.com/igorantolic/ai-esp32-rotary-encoder.git

## picture of the running setup

![Breadboard](https://github.com/subbamaggus/nodemcu_oled_rotary_openhab/blob/main/menu_all-items.jpeg?raw=true)
![Breadboard](https://github.com/subbamaggus/nodemcu_oled_rotary_openhab/blob/main/menu_single-item.jpeg?raw=true)
