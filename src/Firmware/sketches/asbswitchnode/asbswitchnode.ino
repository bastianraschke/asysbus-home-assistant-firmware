#include "asb.h"

#define FIRMWARE_VERSION                        "1.0.0"

#define NODE_CAN_ADDRESS                        0x000A

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define PIN_SWITCH                              3
#define PIN_VIBRATION_MOTOR                     4
#define PIN_LED                                 5

ASB asb0(NODE_CAN_ADDRESS);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);

int oldSwitchState = HIGH;

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "The node '0x%04X' was powered up.", NODE_CAN_ADDRESS);
    Serial.println(buffer);

    setupCan();
    setupSwitch();
    setupLed();
    setupVibrationMotor();
}

void setupCan()
{
    Serial.println(F("Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("Attaching CAN was successful."));

        const byte bootPacketData[1] = {ASB_CMD_BOOT};
        byte bootPackedStats = asb0.asbSend(ASB_PKGTYPE_BROADCAST, 0xFFFF, sizeof(bootPacketData), bootPacketData);
    }
    else
    {
        Serial.println(F("Attaching CAN was not successful!"));
    }
}

void setupSwitch()
{
    pinMode(PIN_SWITCH, INPUT_PULLUP);
    digitalWrite(PIN_SWITCH, HIGH);
}

void setupLed()
{
    pinMode(PIN_LED, OUTPUT);

    // LED test
    showLed(500);
}

void setupVibrationMotor()
{
    pinMode(PIN_VIBRATION_MOTOR, OUTPUT);
}

void showLed(const int delayTime)
{
    digitalWrite(PIN_LED, HIGH);
    delay(delayTime);
    digitalWrite(PIN_LED, LOW);
}

void vibrateMotor(const int delayTime)
{
    digitalWrite(PIN_VIBRATION_MOTOR, HIGH);
    delay(delayTime);
    digitalWrite(PIN_VIBRATION_MOTOR, LOW);
}

void loop()
{
    const asbPacket canPacket = asb0.loop();
    const bool newSwitchState = digitalRead(PIN_SWITCH);

    if (oldSwitchState != newSwitchState && newSwitchState == LOW)
    {
        const byte switchPressedPacketData[2] = {ASB_CMD_1B, 1};
        byte lightSwitchedPacketStats = asb0.asbSend(ASB_PKGTYPE_MULTICAST, 0x1234, sizeof(switchPressedPacketData), switchPressedPacketData);
        
        if (lightSwitchedPacketStats != CAN_OK)
        {
            Serial.println(F("Message could not be sent successfully!"));
        }
        else
        {
            Serial.println(F("Message send OK!"));
            showLed(100);
            vibrateMotor(100);
        }
    }

    oldSwitchState = newSwitchState;
}
