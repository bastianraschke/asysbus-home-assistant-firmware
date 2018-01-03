#include "asb.h"

#define FIRMWARE_VERSION                        "1.0.0"

#define NODE_CAN_ADDRESS                        0x0001

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

ASB asb0(NODE_CAN_ADDRESS);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);
ASB_UART asbUart0(Serial);

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "The node '0x%04X' was powered up.", NODE_CAN_ADDRESS);
    Serial.println(buffer);

    setupCanBus();
    setupUart();
}

void setupCanBus()
{
    Serial.println(F("Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("Attaching CAN was successful."));
    }
    else
    {
        Serial.println(F("Attaching CAN was not successful!"));
    }
}

void setupUart()
{
    Serial.println(F("Attaching UART..."));

    const char busId = asb0.busAttach(&asbUart0);

    if (busId != -1)
    {
        Serial.println(F("Attaching UART was successful."));
    }
    else
    {
        Serial.println(F("Attaching UART was not successful!"));
    }
}

void loop()
{
    asb0.loop();
}
