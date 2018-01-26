#include "asb.h"

/*
 * Configuration
 */

#define DEBUG_LEVEL                             1
#define DEBUG_CANSNIFFER_ENABLED                0

#define FIRMWARE_VERSION                        "1.0.0"

#define ASB_BRIDGE_NODE_ID                      0x0001
#define ASB_NODE_ID                             0x07D0

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define PIN_BUTTON                              3
#define PIN_VIBRATION_MOTOR                     4
#define PIN_LED                                 5

/*
 * Initialisation
 */

ASB asb0(ASB_NODE_ID);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);

int oldButtonState = LOW;
bool internalButtonEnabledState = false;

/*
 * Setup
 */

void setup()
{
    Serial.begin(115200);

    char buffer[64] = {0};
    sprintf(buffer, "setup(): The node '0x%04X' was powered up.", ASB_NODE_ID);
    Serial.println(buffer);

    setupCanBus();
    setupVibrationMotor();
    setupLed();
    setupButton();
}

void setupCanBus()
{
    Serial.println(F("setupCanBus(): Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("setupCanBus(): Attaching CAN was successful."));

        setupCanBusSwitchChangedPacketReceived();
        setupCanBusRequestStatePacketReceived();
    }
    else
    {
        Serial.println(F("setupCanBus(): Attaching CAN failed!"));
    }
}

/*
 * Switch changed packet handling
 */

void setupCanBusSwitchChangedPacketReceived()
{
    Serial.println(F("setupCanBusSwitchChangedPacketReceived(): Attaching switch state changed hook..."));

    const uint8_t hookOnPacketType = ASB_PKGTYPE_MULTICAST;
    const unsigned int hookOnTarget = ASB_NODE_ID;
    const uint8_t hookOnPort = 0xFF;
    const uint8_t hookOnFirstDataByte = ASB_CMD_1B;

    if (asb0.hookAttach(hookOnPacketType, hookOnTarget, hookOnPort, hookOnFirstDataByte, onSwitchChangedPacketReceived))
    {
        Serial.println(F("setupCanBusSwitchChangedPacketReceived(): Attaching switch state changed was successful."));
    }
    else
    {
        Serial.println(F("setupCanBusSwitchChangedPacketReceived(): Attaching switch state changed failed!"));
    }
}

void onSwitchChangedPacketReceived(asbPacket &canPacket)
{
    // Check if packet is like expected
    if (canPacket.len == 2 && (canPacket.data[1] == 0x00 || canPacket.data[1] == 0x01))
    {
        internalButtonEnabledState = (canPacket.data[1] == 0x01);

        #if DEBUG_LEVEL >= 1
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

/*
 * Switch state request packet handling
 */

void setupCanBusRequestStatePacketReceived()
{
    Serial.println(F("setupCanBusRequestStatePacketReceived(): Attaching request state hook..."));

    const uint8_t hookOnPacketType = ASB_PKGTYPE_MULTICAST;
    const unsigned int hookOnTarget = ASB_NODE_ID;
    const uint8_t hookOnPort = 0xFF;
    const uint8_t hookOnFirstDataByte = ASB_CMD_REQ;

    if (asb0.hookAttach(hookOnPacketType, hookOnTarget, hookOnPort, hookOnFirstDataByte, onRequestStatePacketReceived))
    {
        Serial.println(F("setupCanBusRequestStatePacketReceived(): Attaching request state was successful."));
    }
    else
    {
        Serial.println(F("setupCanBusRequestStatePacketReceived(): Attaching request state failed!"));
    }
}

void onRequestStatePacketReceived(asbPacket &canPacket)
{
    sendCurrentStatePacket();
}

bool sendCurrentStatePacket()
{
    const unsigned int targetAdress = ASB_BRIDGE_NODE_ID;
    byte switchStatePacketData[2] = {ASB_CMD_1B, internalButtonEnabledState};
    const byte switchStatePacketStats = asb0.asbSend(ASB_PKGTYPE_MULTICAST, targetAdress, sizeof(switchStatePacketData), switchStatePacketData);

    return (switchStatePacketStats == 0);
}

void setupVibrationMotor()
{
    pinMode(PIN_VIBRATION_MOTOR, OUTPUT);
}

void setupLed()
{
    pinMode(PIN_LED, OUTPUT);

    // Self test of the LED without vibration
    pulseLedWithVibrationFeedback(200, 2, false);
}

void setupButton()
{
    // Because a capacitive switch module with internal pulldown is used, we need no pullup/down
    pinMode(PIN_BUTTON, INPUT);
}

void pulseLedWithVibrationFeedback(const int delayTimeInMilliseconds, const int repeatCount, const bool vibrateMotor)
{
    // The delay time is divided by 2 phases (fade in and fade out) and 256 steps
    const int stepDelayTimeInMicroseconds = (int) (((float) delayTimeInMilliseconds * 1000.0f) / 2.0f / 256.0f);

    // The delay between cycles (1000µs == 1ms)
    const int repeatDelayTimeInMicroseconds = delayTimeInMilliseconds * 1000;

    for (int t = 0; t < repeatCount; t++)
    {
        // Be sure, the LED is off before we start
        digitalWrite(PIN_LED, LOW);

        if (vibrateMotor)
        {
            digitalWrite(PIN_VIBRATION_MOTOR, HIGH);
        }

        #if DEBUG_LEVEL >= 2
            Serial.println(F("pulseLedWithVibrationFeedback(): Fade LED in"));
        #endif

        // Fade in the LED
        for (int i = 0; i < 256; i++)
        {
            analogWrite(PIN_LED, i);
            delayMicroseconds(stepDelayTimeInMicroseconds);
        }

        #if DEBUG_LEVEL >= 2
            Serial.println(F("pulseLedWithVibrationFeedback(): Fade LED in done"));
            Serial.println(F("pulseLedWithVibrationFeedback(): Fade LED out"));
        #endif

        // Fade out the LED
        for (int i = 256; i >= 0; i--)
        {
            analogWrite(PIN_LED, i);
            delayMicroseconds(stepDelayTimeInMicroseconds);
        }

        #if DEBUG_LEVEL >= 2
            Serial.println(F("pulseLedWithVibrationFeedback(): Fade LED in done"));
        #endif

        if (vibrateMotor)
        {
            digitalWrite(PIN_VIBRATION_MOTOR, LOW);
        }

        // A delay between cycles is only needed if the cycle is repeated more than once and also not after the last cycle
        if (repeatCount > 1 && t != repeatCount - 1)
        {
            #if DEBUG_LEVEL >= 2
                Serial.println(F("pulseLedWithVibrationFeedback(): Wait after one cycle"));
            #endif

            delayMicroseconds(repeatDelayTimeInMicroseconds);

            #if DEBUG_LEVEL >= 2
                Serial.println(F("pulseLedWithVibrationFeedback(): Wait after one cycle done"));
            #endif
        }
    }
}

/*
 * Loop
 */

void loop()
{
    #if DEBUG_CANSNIFFER_ENABLED == 1
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

        #if DEBUG_LEVEL >= 1
            Serial.print(F("loop(): internalButtonEnabledState = "));
            Serial.println(internalButtonEnabledState);
        #endif

        const bool statePacketSentSuccessfully = sendCurrentStatePacket();

        if (statePacketSentSuccessfully)
        {
            Serial.println(F("loop(): The CAN message was sent successfully."));
            pulseLedWithVibrationFeedback(200, 1, true);
        }
        else
        {
            Serial.println(F("loop(): The CAN message could not be sent successfully!"));
            pulseLedWithVibrationFeedback(100, 3, true);
        }
    }

    oldButtonState = newButtonState;
}
