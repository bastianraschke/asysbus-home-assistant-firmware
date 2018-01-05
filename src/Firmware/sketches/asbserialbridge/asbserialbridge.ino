#include "asb.h"

#define FIRMWARE_VERSION                        "1.0.0"

#define ASB_BRIDGE_NODE_ID                      0x0001
#define ASB_NODE_ID                             ASB_BRIDGE_NODE_ID

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

ASB asb0(ASB_NODE_ID);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);
ASB_UART asbUart0(Serial);

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "setup(): The node '0x%04X' was powered up.", ASB_NODE_ID);
    Serial.println(buffer);

    setupCanBus();
    setupUart();
}

void setupCanBus()
{
    Serial.println(F("setupCanBus(): Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("setupCanBus(): Attaching CAN was successful."));
    }
    else
    {
        Serial.println(F("setupCanBus(): Attaching CAN failed!"));
    }
}

void setupUart()
{
    Serial.println(F("setupUart(): Attaching UART..."));

    const char busId = asb0.busAttach(&asbUart0);

    if (busId != -1)
    {
        Serial.println(F("setupUart(): Attaching UART was successful."));
    }
    else
    {
        Serial.println(F("setupUart(): Attaching UART failed!"));
    }
}

void loop()
{
    asb0.loop();
}
