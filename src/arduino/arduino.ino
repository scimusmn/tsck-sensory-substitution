#include "SerialController.hpp"

#define BAUDRATE 115200
SerialController serial;
unsigned long time, prevTime;
unsigned long totalTime;

void setup()
{
    serial.setup(BAUDRATE, onParse);
}

void loop()
{
    time = millis();
    unsigned int dt = time - prevTime;
    prevTime = time;

    totalTime += dt;
    if (totalTime > 1000) {
	totalTime = 0;
	serial.sendMessage("time", time);
    }
    
    serial.update();
}

void onParse(char* message, char* value)
{
    if (strcmp(message, "wake-arduino") == 0)
	serial.sendMessage("arduino-ready", "1");
    else
	serial.sendMessage("unknown-command", "1");
}
