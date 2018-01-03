#include "asb.h"

#define FIRMWARE_VERSION                        "1.0.0"

#define NODE_CAN_ADDRESS                        0x07D0

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define PIN_BUTTON                              3
#define PIN_VIBRATION_MOTOR                     4
#define PIN_LED                                 5

ASB asb0(NODE_CAN_ADDRESS);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);

int oldButtonState = LOW;

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "The node '0x%04X' was powered up.", NODE_CAN_ADDRESS);
    Serial.println(buffer);

    setupCanBus();
    setupButton();
    setupLed();
    setupVibrationMotor();
}

void setupCanBus()
{
    Serial.println(F("Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("Attaching CAN was successful."));

        const byte bootPacketData[1] = {ASB_CMD_BOOT};
        const byte bootPacketStats = asb0.asbSend(ASB_PKGTYPE_BROADCAST, 0xFFFF, sizeof(bootPacketData), bootPacketData);

        if (bootPacketStats != CAN_OK)
        {
            Serial.println(F("The boot complete message could not be sent successfully!"));
        }
    }
    else
    {
        Serial.println(F("Attaching CAN was not successful!"));
    }
}

void setupButton()
{
    // Because a capacitive switch module with internal pulldown is used, we need no pullup/down
    pinMode(PIN_BUTTON, INPUT);
}

void setupLed()
{
    pinMode(PIN_LED, OUTPUT);
}

void setupVibrationMotor()
{
    pinMode(PIN_VIBRATION_MOTOR, OUTPUT);
}

void pulseLedWithVibrationFeedback(const int delayTimeInMilliseconds, const int repeatCount)
{
    // Avoid devision by zero
    if (delayTimeInMilliseconds > 0)
    {
        const int stepDelayTimeInMicroseconds = (int) (((float) delayTimeInMilliseconds * 1000.0f) / 2.0f / 256.0f);

        for (int t = 0; t < repeatCount; t++)
        {
            // Be sure, the LED is off before we start
            digitalWrite(PIN_LED, LOW);

            digitalWrite(PIN_VIBRATION_MOTOR, HIGH);

            // Fade in the LED
            for (int i = 0; i < 256; i++)
            {
                analogWrite(PIN_LED, i);
                delayMicroseconds(stepDelayTimeInMicroseconds);
            }

            // Fade out the LED
            for (int i = 256; i >= 0; i--)
            {
                analogWrite(PIN_LED, i);
                delayMicroseconds(stepDelayTimeInMicroseconds);
            }

            digitalWrite(PIN_VIBRATION_MOTOR, LOW);

            // A delay between cycles is only needed if the cycle is repeated more than once and also not after the last cycle
            if (repeatCount > 1 && t != repeatCount - 1)
            {
                const int repeatDelayTimeInMicroseconds = (int) (((float) delayTimeInMilliseconds * 1000.0f) * 2.0f);
                delayMicroseconds(repeatDelayTimeInMicroseconds);
            }
        }
    }
}

void loop()
{
    asb0.loop();

    const bool newButtonState = digitalRead(PIN_BUTTON);

    if (oldButtonState != newButtonState && newButtonState == HIGH)
    {
        const unsigned int targetAdress = 0x0001;
        const byte switchPressedPacketData[2] = {ASB_CMD_1B, 1};
        const byte switchPressedPacketStats = asb0.asbSend(ASB_PKGTYPE_MULTICAST, targetAdress, sizeof(switchPressedPacketData), switchPressedPacketData);

        if (switchPressedPacketStats != CAN_OK)
        {
            Serial.println(F("Message could not be sent successfully!"));
            pulseLedWithVibrationFeedback(100, 3);
        }
        else
        {
            Serial.println(F("Message send OK!"));
            pulseLedWithVibrationFeedback(200, 1);
        }
    }

    oldButtonState = newButtonState;
}
