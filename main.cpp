//=====[Libraries]=============================================================
#include "mbed.h"
#include "arm_book_lib.h"

//=====[Declaration of private defines]========================================

#define SELECTOR_ON_THRESHOLD 0.33
#define SELECTOR_OFF_THRESHOLD 0.66
#define DUSK_THRESHOLD 0.66
#define DAYLIGHT_THRESHOLD 0.9

#define DUSK_TO_DAYLIGHT_TIME 2000
#define DAYLIGHT_TO_DUSK_TIME 1000
#define DELAY_INCREMENT 20

//=====[Declaration of private data types]=====================================

typedef enum {
    HEADLIGHT_INIT,
    HEADLIGHT_DUSK,
    HEADLIGHT_DUSK_DELAY,
    HEADLIGHT_DAYLIGHT_DELAY,
    HEADLIGHT_DAYLIGHT
} headlightState_t;

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

DigitalIn driverSeatButton(D2);
DigitalIn ignitionButton(BUTTON1);

DigitalOut BlueLed(LED2);

AnalogIn potentiometer(A0);
AnalogIn lightSensor(A1);

DigitalOut LeftLowBeam(PE_10);
DigitalOut RightLowBeam(PE_12);

//=====[Declaration and initialization of private global variables]============

bool engineOn = OFF;
bool timeOfDayState = OFF;

int accumulatedSwitchTime = 0;

headlightState_t headlightAutoState = HEADLIGHT_INIT;

//=====[Declarations (prototypes) of private functions]========================

void inputsInit();
void outputsInit();
void checkHeadlights();
void checkIgnition();

// Run logic for program: Run checks for ignition and headlight subsystems
int main() {
    inputsInit();
    outputsInit();

    while (true) {
        checkIgnition();
        checkHeadlights();
        delay(DELAY_INCREMENT);
    }

}

//=====[Implementations of public functions]===================================

/* Initialize all inputs */
void inputsInit()
{
    driverSeatButton.mode(PullDown);
}

/* Initialize all outputs */
void outputsInit() {
    BlueLed = OFF;
    LeftLowBeam = OFF;
    RightLowBeam = OFF;
}

/***
    Run logic for checking ignition subsystem
    - logic for turning engine on/off
    - logic for turning blue indicator on/off
***/
void checkIgnition() {
    if(driverSeatButton == ON && ignitionButton == ON && !engineOn) {
        while(ignitionButton == ON) {} // delay until it goes off
        // Ignition is released
        engineOn = ON;
    }

    if(ignitionButton == ON && engineOn) {
        while(ignitionButton == ON) {}
        engineOn = OFF;
    }

    BlueLed = engineOn;
}

/***
    Run logic for checking headlight subsystem
    - Read in potentiometer for switching modes
    - Read in light sensor reading
    - Turn on/off headlights if in manual mode
    - Perform state machine (for delaying headlights on/off) if in automatic mode
*/
void checkHeadlights() {
    float potReading = potentiometer.read();
    float lightReading = lightSensor.read();

    if(engineOn) {
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

            // perform state machine for automatic mode
            switch(headlightAutoState) {
                //initial state
                case HEADLIGHT_INIT:
                    LeftLowBeam = OFF;
                    RightLowBeam = OFF;
                    break;
                // DAYLIGHT state => turn off headlights, check for light level threshold
                case HEADLIGHT_DAYLIGHT:
                    accumulatedSwitchTime = 0;
                    LeftLowBeam = OFF;
                    RightLowBeam = OFF;

                    if(lightReading < DUSK_THRESHOLD) {
                        headlightAutoState = HEADLIGHT_DUSK_DELAY;
                    }
                    break;
                // DUSK state => turn on headlights, check for light level threshold
                case HEADLIGHT_DUSK:
                    accumulatedSwitchTime = 0;
                    LeftLowBeam = ON;
                    RightLowBeam = ON;
                    if(lightReading > DAYLIGHT_THRESHOLD) {
                        headlightAutoState = HEADLIGHT_DAYLIGHT_DELAY;
                    }
                    break;
                // DUSK DELAY state => ensure light level stays low: 
                // if so, check how much time has passed and go to corresponding state
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
                // DAYLIGHT DELAY state => ensure light level stays high: 
                // if so, check how much time has passed and go to corresponding state
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