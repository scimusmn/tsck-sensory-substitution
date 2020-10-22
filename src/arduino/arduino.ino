#include "SerialController.hpp"
#include "Button.h"

#define BAUDRATE 115200
#define BUTTON_PIN 7

SerialController serial;
Button button;

void setup()
{
    serial.setup(BAUDRATE, onParse);
    button.setup(BUTTON_PIN, [](int state) { serial.sendMessage("button", state); });
}

void loop()
{
    button.update();
    serial.update();
}

void onParse(char* message, char* value)
{
    if (strcmp(message, "wake-arduino") == 0)
	serial.sendMessage("arduino-ready", "1");
    else
	serial.sendMessage("unknown-command", "1");
}
