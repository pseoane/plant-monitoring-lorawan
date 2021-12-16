#include "./RGBLED.h"
#include "mbed.h"

RGBLED::RGBLED(DigitalOut redPin, DigitalOut greenPin, DigitalOut bluePin) : redPin(redPin), greenPin(greenPin), bluePin(bluePin) {};
RGBLED::~RGBLED() {
}
void RGBLED::setColor(uint8_t red, uint8_t green, uint8_t blue) {
	redPin = red;
	greenPin = green;
	bluePin = blue;
}

void RGBLED::setColor(char * colorName) {
	if (strcmp(colorName, "GREEN") == 0) {
		RGBLED::setColor(0, 1, 0);
		printf("Led set to green\n");
	} else if (strcmp(colorName, "BLUE") == 0) {
		RGBLED::setColor(0, 0, 1);
		printf("Led set to blue\n");
	} else if (strcmp(colorName, "RED") == 0) {
		RGBLED::setColor(1, 0, 0);
		printf("Led set to red\n");
	} else if (strcmp(colorName, "OFF") == 0) {
		RGBLED::setColor(0, 0, 0);
		printf("Led set to off\n");
	}
}

