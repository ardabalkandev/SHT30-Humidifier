# Smart Humidity Control System (SHT31 & Arduino)

Türkçe Versiyon için => README_tr.md

This project is a smart humidity control system that monitors ambient humidity and temperature using an Arduino board, automatically maintains specific humidity levels, and provides visual and audible warnings when thresholds are exceeded. The system uses a relay to control a humidifier (or dehumidifier) and features a user-friendly interface.

## Features

* **Real-time Humidity and Temperature Monitoring:** Reads instant humidity and temperature values using the SHT31 sensor.
* **Adjustable Target Humidity:** User-definable target humidity level (saved to EEPROM).
* **Automatic Humidification/Dehumidification Control:** Automatically turns on/off the connected humidifier (or dehumidifier) via a relay based on the target humidity level.
    * For low humidity: Runs the humidifier at specific intervals.
    * For high humidity: Turns off the humidifier (or runs the dehumidifier).
* **Adjustable Alarm Thresholds:** Separately adjustable low and high humidity alarm offsets for situations where humidity drops below or rises above the target (saved to EEPROM).
* **Visual and Audible Alarm:** Provides warnings with an LED and buzzer when set alarm thresholds are exceeded.
* **Temperature Unit Selection:** Ability to switch between Celsius (°C) or Fahrenheit (°F) (saved to EEPROM).
* **Manual Humidification:** Ability to manually trigger humidification with a single button press.
* **User Interface:** Easy setting and status monitoring via a 16x2 I2C LCD screen and 4 buttons.
* **Setting Persistence:** All settings (target humidity, temperature unit, alarm offsets) are saved to Arduino's internal EEPROM, preventing loss upon power interruption.
* **Hold-to-Adjust Function:** Quickly increase/decrease values by holding down the '+' and '-' buttons in setting modes.

## Hardware Used

* **Arduino Uno/Nano** (or a compatible Arduino board)
* **SHT31 Digital Humidity and Temperature Sensor** (I2C compatible)
* **16x2 I2C LCD Display Module** (I2C compatible)
* **1-Channel Relay Module** (5V)
* **Buzzer** (A passive buzzer was used)
* **LED** (For status indication or alarm, with a 220 Ohm resistor)
* **4 Push Buttons** (`SET`, `MINUS`, `PLUS`, `MANUAL`)
* **Jumper Wires**
* **Breadboard or Custom PCB** (For prototyping or permanent installation)
* **220 Ohm Resistor** (For the LED)
* **USB Cable** (For programming and powering the Arduino)
* **5V Power Supply** (For the system)
* **Humidifier / Dehumidifier** (Device to be controlled by the relay; in this system, I used two water atomizing nozzles controlled by a 220V solenoid valve [normally closed type])

## Wiring Diagram

The following connections are based on Arduino Uno/Nano pins. I2C pins (SDA/SCL) may vary on other Arduino models.

| Component          | Pin             | Arduino Pin          | Description                                    |
| :----------------- | :-------------- | :------------------- | :--------------------------------------------- |
| **Buttons** |                 |                      | (The other end of each button is connected to GND) |
| SET Button         |                 | Digital 2 (INPUT_PULLUP) | Cycles through setting modes                   |
| MINUS Button       |                 | Digital 3 (INPUT_PULLUP) | Decreases value / Changes unit                 |
| PLUS Button        |                 | Digital 4 (INPUT_PULLUP) | Increases value / Changes unit                 |
| MANUAL Button      |                 | Digital 5 (INPUT_PULLUP) | Triggers manual humidification                 |
| **Buzzer** | (+)             | Digital 6            | Audible alarm                                  |
| **Relay** | IN (Trigger)    | Digital 7            | Humidifier/dehumidifier control                |
| **LED** | (+) (with 220 Ohm resistor) | Digital 8            | Visual status/alarm indicator                  |
| **SHT31 Sensor** | SDA             | A4 (or dedicated SDA) | I2C Data Line                                  |
|                    | SCL             | A5 (or dedicated SCL) | I2C Clock Line                                 |
|                    | VCC             | 5V                   | Sensor power                                   |
|                    | GND             | GND                  | Sensor ground                                  |
| **16x2 I2C LCD** | SDA             | A4 (or dedicated SDA) | I2C Data Line (parallel with SHT31)            |
|                    | SCL             | A5 (or dedicated SCL) | I2C Clock Line (parallel with SHT31)           |
|                    | VCC             | 5V                   | LCD power                                      |
|                    | GND             | GND                  | LCD ground                                     |

**Note:** The relay module may have its own power connections (+ and -), which should be powered from the Arduino or an external 5V source. Remember to use a 220 Ohm series resistor when connecting the LED.

## Installation Instructions

1.  **Install Required Libraries:**
    * Open the Arduino IDE.
    * Go to `Sketch > Include Library > Manage Libraries...`.
    * Search for and install the following libraries:
        * `LiquidCrystal I2C` (by Frank de Brabander)
        * `Adafruit SHT31 Library` (by Adafruit)
2.  **Assemble the Circuit:** Connect all hardware components to the Arduino according to the wiring diagram above.
3.  **Upload the Code:**
    * Open this project file (e.g., `SmartHumidityControl.ino`) in the Arduino IDE.
    * Select your Arduino board from the `Tools > Board` menu (e.g., "Arduino Uno").
    * Select the serial port your Arduino is connected to from the `Tools > Port` menu.
    * Click the "Upload" button to upload the code to your Arduino board.
4.  **Test:**
    * Upon system startup, you will see a welcome message.
    * Cycle through setting modes using the `SET` button.
    * Adjust values using the `PLUS` and `MINUS` buttons. The hold-to-adjust feature allows for quick adjustments.
    * To save your settings, press the `SET` button again after cycling through all modes, or simply do not press any button for 5 seconds (timeout).
    * Test the alarm and humidification functions by changing the humidity level.
    * Test manual humidification using the `MANUAL` button.

## Setting Modes

Each press of the `SET` button cycles the system through the following setting modes:

1.  **Desired Humidity Setting:** Adjusts the target humidity level to be maintained in the environment (e.g., 50%).
    * Use `PLUS` / `MINUS` to change the value between 5 and 95. (Changes by 1 unit with a single press, by 5 units when held down.)
2.  **Temperature Unit Setting:** Sets the temperature display unit as Celsius (°C) or Fahrenheit (°F).
    * Use `PLUS` / `MINUS` to toggle the unit.
3.  **Low Humidity Alarm Threshold Setting:** Adjusts how many units below the target humidity the alarm will trigger (e.g., if target is 50%, "15 Units Below" means alarm at 35%).
    * Use `PLUS` / `MINUS` to change the offset value between 5 and 95.
4.  **High Humidity Alarm Threshold Setting:** Adjusts how many units above the target humidity the alarm will trigger (e.g., if target is 50%, "25 Units Above" means alarm at 75%).
    * Use `PLUS` / `MINUS` to change the offset value between 5 and 95.
5.  **Save and Exit:** Saves all settings to EEPROM and returns to normal mode. Also, if no button is pressed for 5 seconds in any setting mode, it automatically saves and returns to normal mode.

## Relay Control Logic

The system controls the humidifier when the humidity drops below the set target (considering the hysteresis value). If no alarm is active, the humidifier turns **ON for 3 seconds and OFF for 12 seconds.** This cycle continues until the humidity level rises.

**Relay Control in Alarm State:**

* **Low Humidity Alarm:** When humidity drops below the very low threshold, the humidifier runs for a longer duration, turning **ON for 5 seconds and OFF for 5 seconds.**
* **High Humidity Alarm:** When humidity rises above the very high threshold, the humidifier is turned off completely.

## Contribution

This project is open-source and open for development. Any feedback, bug fixes, or new feature suggestions are welcome. If you wish to contribute, please open an "issue" or submit a "pull request".

## License

This project is licensed under the MIT License. See the `LICENSE` file for more details.

---
