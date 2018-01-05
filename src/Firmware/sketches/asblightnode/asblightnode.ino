#include "asb.h"

#define FIRMWARE_VERSION                        "1.0.0"

#define ASB_BRIDGE_NODE_ID                      0x0001
#define ASB_NODE_ID                             0x03E8

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define LED_TYPE                                RGB

#define PIN_LED_RED                             5
#define PIN_LED_GREEN                           3
#define PIN_LED_BLUE                            6
#define PIN_LED_WHITE                           9

ASB asb0(ASB_NODE_ID);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);

enum LEDType {
    RGB,
    RGBW
};

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "The node '0x%04X' was powered up.", ASB_NODE_ID);
    Serial.println(buffer);

    setupLEDs();
    setupCanBus();
}

void setupLEDs()
{
    Serial.println("Setup LEDs...");

    // TODO: show last state instead
    showGivenColor(255, 255, 255, 255);
}

void setupCanBus()
{
    Serial.println(F("Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("Attaching CAN was successful."));

        Serial.println(F("Attaching light state changed hook..."));

        const uint8_t hookOnPacketType = ASB_PKGTYPE_MULTICAST;
        const uint8_t hookOnTarget = ASB_NODE_ID;
        const uint8_t hookOnPort = -1;
        const uint8_t hookOnFirstDataByte = ASB_CMD_S_LIGHT;

        if (asb0.hookAttach(hookOnPacketType, hookOnTarget, hookOnPort, hookOnFirstDataByte, onLightChangedPacketReceived))
        {
            Serial.println(F("Attaching light state changed was successful."));
        }
        else
        {
            Serial.println(F("Attaching light state changed was not successful!"));
        }
    }
    else
    {
        Serial.println(F("Attaching CAN was not successful!"));
    }
}

void onLightChangedPacketReceived(const asbPacket &canPacket)
{
    // Check if packet is like expected
    if (canPacket.len == 6)
    {
        const bool stateOnOff = canPacket.data[0];
        const uint8_t brightness = constrain(canPacket.data[1], 0, 255);

        const uint8_t redValue = constrain(canPacket.data[2], 0, 255);
        const uint8_t greenValue = constrain(canPacket.data[3], 0, 255);
        const uint8_t blueValue = constrain(canPacket.data[4], 0, 255);
        const uint8_t whiteValue = constrain(canPacket.data[5], 0, 255);

        const uint8_t mappedRedValue = map(redValue, 0, 255, 0, brightness);
        const uint8_t mappedGreenValue = map(greenValue, 0, 255, 0, brightness);
        const uint8_t mappedBlueValue = map(blueValue, 0, 255, 0, brightness);
        const uint8_t mappedWhiteValue = map(whiteValue, 0, 255, 0, brightness);

        // TODO: Pass other variables too?
        showGivenColor(mappedRedValue, mappedGreenValue, mappedBlueValue, mappedWhiteValue);

        Serial.print(F("onLightChangedPacketReceived(): The light was changed to "));

        Serial.print(F("onLightChangedPacketReceived(): stateOnOff = "));
        Serial.println(stateOnOff);

        Serial.print(F("onLightChangedPacketReceived(): brightness = "));
        Serial.println(brightness);

        Serial.print(F("onLightChangedPacketReceived(): redValue = "));
        Serial.println(redValue);
        Serial.print(F("onLightChangedPacketReceived(): greenValue = "));
        Serial.println(greenValue);
        Serial.print(F("onLightChangedPacketReceived(): blueValue = "));
        Serial.println(blueValue);
        Serial.print(F("onLightChangedPacketReceived(): whiteValue = "));
        Serial.println(whiteValue);
    }
    else
    {
        Serial.println(F("onLightChangedPacketReceived(): Invalid packet!"));
    }
}

void showGivenColor(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{
    analogWrite(PIN_LED_RED, redValue);
    analogWrite(PIN_LED_GREEN, greenValue);
    analogWrite(PIN_LED_BLUE, blueValue);

    if (LED_TYPE == RGBW)
    {
        analogWrite(PIN_LED_WHITE, whiteValue); 
    }
    else
    {
        analogWrite(PIN_LED_WHITE, 0);   
    }
}

void loop()
{
    //asb0.loop();

    const asbPacket canPacket = asb0.loop();

    if (canPacket.meta.busId != -1) {
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

        for(byte i=0; i<canPacket.len; i++) {
            Serial.print(F(" 0x"));
            Serial.print(canPacket.data[i], HEX);
        }

        Serial.println();
    }
}
