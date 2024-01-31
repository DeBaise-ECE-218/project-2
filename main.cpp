//=====[Libraries]=============================================================
#include "mbed.h"
#include "arm_book_lib.h"

//=====[Defines]===============================================================

#define SELECTOR_ON_THRESHOLD 0.33
#define SELECTOR_OFF_THRESHOLD 0.66
#define DUSK_THRESHOLD 0.66
#define DAYLIGHT_THRESHOLD 0.82

#define DUSK_TO_DAYLIGHT_TIME 2000
#define DAYLIGHT_TO_DUSK_TIME 1000
#define DELAY_INCREMENT 20

//=====[Declaration of public data types]======================================

typedef enum {
    HEADLIGHT_INIT,
    HEADLIGHT_DUSK,
    HEADLIGHT_DUSK_DELAY,
    HEADLIGHT_DAYLIGHT_DELAY,
    HEADLIGHT_DAYLIGHT
} headlightState_t;

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

DigitalIn Dseat(D2);
DigitalIn Ignition(BUTTON1);

DigitalOut BlueLed(LED2);

AnalogIn potentiometer(A0);
AnalogIn lightsensor(A1);

DigitalOut LeftLowBeam(PE_10);
DigitalOut RightLowBeam(PE_12);

bool EngineOn = OFF;
bool timeOfDayState = OFF;

int accumulatedSwitchTime = 0;
headlightState_t headlightAutoState = HEADLIGHT_INIT;

void inputsInit();
void outputsInit();
void checkheadlights();
void checkignition();

// main() runs in its own thread in the OS
int main() {
    inputsInit();
    outputsInit();

    while (true) {
        checkignition();
        checkheadlights();
        delay(DELAY_INCREMENT);
    }

}

void inputsInit()
{
    Dseat.mode(PullDown);
}

void outputsInit() {
    BlueLed = OFF;
    LeftLowBeam = OFF;
    RightLowBeam = OFF;

}

void checkignition() {
    if(Dseat == ON && Ignition == ON && !EngineOn) {
        while(Ignition == ON) {} // delay until it goes off
        // Ignition is released
        EngineOn = ON;
    }

    if(Ignition == ON && EngineOn) {
        while(Ignition == ON) {}
        EngineOn = OFF;
    }

    BlueLed = EngineOn;
}

void checkheadlights() {
    float potReading = potentiometer.read();
    float lightReading = lightsensor.read();

    if(EngineOn) {
        if(potReading < SELECTOR_ON_THRESHOLD) {
            // ON
            headlightAutoState = HEADLIGHT_DUSK;
            LeftLowBeam = ON;
            RightLowBeam = ON;
            
        } else if(potReading > SELECTOR_OFF_THRESHOLD) {
            // OFF
            headlightAutoState = HEADLIGHT_DAYLIGHT;
            LeftLowBeam = OFF;
            RightLowBeam = OFF;
        } else {

            switch(headlightAutoState) {
                case HEADLIGHT_INIT:
                    LeftLowBeam = OFF;
                    RightLowBeam = OFF;
                    break;
                case HEADLIGHT_DAYLIGHT:
                    accumulatedSwitchTime = 0;
                    LeftLowBeam = OFF;
                    RightLowBeam = OFF;

                    if(lightReading < DUSK_THRESHOLD) {
                        headlightAutoState = HEADLIGHT_DUSK_DELAY;
                    }
                    break;
                case HEADLIGHT_DUSK:
                    accumulatedSwitchTime = 0;
                    LeftLowBeam = ON;
                    RightLowBeam = ON;
                    if(lightReading > DAYLIGHT_THRESHOLD) {
                        headlightAutoState = HEADLIGHT_DAYLIGHT_DELAY;
                    }
                    break;
                case HEADLIGHT_DUSK_DELAY:
                    if(lightReading < DUSK_THRESHOLD) {
                        if(accumulatedSwitchTime >= DAYLIGHT_TO_DUSK_TIME) {
                            headlightAutoState = HEADLIGHT_DUSK;
                        } else {
                            accumulatedSwitchTime += DELAY_INCREMENT;
                        }
                    } else {
                        headlightAutoState = HEADLIGHT_DAYLIGHT;
                    }
                    break;
                case HEADLIGHT_DAYLIGHT_DELAY:
                    if(lightReading > DAYLIGHT_THRESHOLD) {
                        if(accumulatedSwitchTime >= DUSK_TO_DAYLIGHT_TIME) {
                            headlightAutoState = HEADLIGHT_DAYLIGHT;
                        } else {
                            accumulatedSwitchTime += DELAY_INCREMENT;
                        }
                    } else {
                        headlightAutoState = HEADLIGHT_DUSK;
                    }
                    break;
                default:
                    break;
            }

        }
    }

}