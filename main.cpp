//=====[Libraries]=============================================================
#include "mbed.h"
#include "arm_book_lib.h"

//=====[Defines]===============================================================


//=====[Declaration of public data types]======================================


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

    char str[100];
    int stringLength;

    sprintf(str, "Pot: %.2f \r\n", potReading);
    stringLength = strlen(str);
    uartUsb.write(str, stringLength);

    uartUsb.write("---\r\n", 5);

    if(potReading < 0.33) {
        // ON
        LeftLowBeam = ON;
        RightLowBeam = ON;
        
    } else if(potReading > 0.66) {
        // OFF
        LeftLowBeam = OFF;
        RightLowBeam = OFF;
    } else {
        // AUTO
    }

}