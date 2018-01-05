#include "asb.h"

#define DEBUG                                   1
#define DEBUG_CANSNIFFER                        0

#define FIRMWARE_VERSION                        "1.0.0"

#define ASB_BRIDGE_NODE_ID                      0x0001
#define ASB_NODE_ID                             0x07D0

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define PIN_BUTTON                              3
#define PIN_VIBRATION_MOTOR                     4
#define PIN_LED                                 5

ASB asb0(ASB_NODE_ID);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);

int oldButtonState = LOW;
bool internalButtonEnabledState = false;

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "setup(): The node '0x%04X' was powered up.", ASB_NODE_ID);
    Serial.println(buffer);

    setupCanBus();
    setupLed();
    setupVibrationMotor();
    setupButton();
}

void setupCanBus()
{
    Serial.println(F("setupCanBus(): Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("setupCanBus(): Attaching CAN was successful."));

        Serial.println(F("setupCanBus(): Attaching switch state changed hook..."));

        const uint8_t hookOnPacketType = ASB_PKGTYPE_MULTICAST;
        const unsigned int hookOnTarget = ASB_NODE_ID;
        const uint8_t hookOnPort = 0xFF;
        const uint8_t hookOnFirstDataByte = ASB_CMD_1B;

        if (asb0.hookAttach(hookOnPacketType, hookOnTarget, hookOnPort, hookOnFirstDataByte, onSwitchChangedPacketReceived))
        {
            Serial.println(F("setupCanBus(): Attaching switch state changed was successful."));
        }
        else
        {
            Serial.println(F("setupCanBus(): Attaching switch state changed failed!"));
        }
    }
    else
    {
        Serial.println(F("setupCanBus(): Attaching CAN failed!"));
    }
}

void onSwitchChangedPacketReceived(const asbPacket &canPacket)
{
    // Check if packet is like expected
    if (canPacket.len == 2 && (canPacket.data[1] == 0x00 || canPacket.data[1] == 0x01))
    {
        internalButtonEnabledState = (canPacket.data[1] == 0x01);

        #if DEBUG == 1
            Serial.print(F("onSwitchChangedPacketReceived(): The switch was switched to "));
            Serial.println(internalButtonEnabledState);
        #endif

        pulseLedWithVibrationFeedback(200, 1, false);
    }
    else
    {
        Serial.println(F("onSwitchChangedPacketReceived(): Invalid packet!"));
    }
}

void setupLed()
{
    pinMode(PIN_LED, OUTPUT);
}

void setupVibrationMotor()
{
    pinMode(PIN_VIBRATION_MOTOR, OUTPUT);
}

void setupButton()
{
    // Because a capacitive switch module with internal pulldown is used, we need no pullup/down
    pinMode(PIN_BUTTON, INPUT);
}

void pulseLedWithVibrationFeedback(const int delayTimeInMilliseconds, const int repeatCount, const bool vibrateMotor)
{
    // Avoid devision by zero
    if (delayTimeInMilliseconds > 0)
    {
        const int stepDelayTimeInMicroseconds = (int) (((float) delayTimeInMilliseconds * 1000.0f) / 2.0f / 256.0f);

        for (int t = 0; t < repeatCount; t++)
        {
            // Be sure, the LED is off before we start
            digitalWrite(PIN_LED, LOW);

            if (vibrateMotor)
            {
                digitalWrite(PIN_VIBRATION_MOTOR, HIGH);
            }

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

            if (vibrateMotor)
            {
                digitalWrite(PIN_VIBRATION_MOTOR, LOW);
            }

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
    #if DEBUG_CANSNIFFER == 1
        const asbPacket canPacket = asb0.loop();

        if (canPacket.meta.busId != -1)
        {
            Serial.println(F("---"));
            Serial.print(F("Type: 0x"));
            Serial.println(canPacket.meta.type, HEX);
            Serial.print(F("Target: 0x"));
            Serial.println(canPacket.meta.target, HEX);
            Serial.print(F("Source: 0x"));
            Serial.println(canPacket.meta.source, HEX);
            Serial.print(F("Port: 0x"));
            Serial.println(canPacket.meta.port, HEX);
            Serial.print(F("Length: 0x"));
            Serial.println(canPacket.len, HEX);

            for (byte i = 0; i < canPacket.len; i++)
            {
                Serial.print(F(" 0x"));
                Serial.print(canPacket.data[i], HEX);
            }

            Serial.println();
        }
    #else
        asb0.loop();
    #endif

    const bool newButtonState = digitalRead(PIN_BUTTON);

    if (oldButtonState != newButtonState && newButtonState == HIGH)
    {
        internalButtonEnabledState = !internalButtonEnabledState;

        #if DEBUG == 1
            Serial.print(F("loop(): The switch was switched to "));
            Serial.println(internalButtonEnabledState);
        #endif

        const unsigned int targetAdress = ASB_BRIDGE_NODE_ID;
        const byte switchPressedPacketData[2] = {ASB_CMD_1B, internalButtonEnabledState ? 0x01 : 0x00};
        const byte switchPressedPacketStats = asb0.asbSend(ASB_PKGTYPE_MULTICAST, targetAdress, sizeof(switchPressedPacketData), switchPressedPacketData);

        if (switchPressedPacketStats != 0)
        {
            Serial.println(F("loop(): The CAN message could not be sent successfully!"));
            pulseLedWithVibrationFeedback(100, 3, true);
        }
        else
        {
            Serial.println(F("loop(): The CAN message was sent successfully."));
            pulseLedWithVibrationFeedback(200, 1, true);
        }
    }

    oldButtonState = newButtonState;
}
