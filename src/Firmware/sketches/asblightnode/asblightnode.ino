#include "asb.h"

#define DEBUG                                   1
#define DEBUG_CANSNIFFER                        0

#define FIRMWARE_VERSION                        "1.0.0"

#define ASB_BRIDGE_NODE_ID                      0x0001
#define ASB_NODE_ID                             0x03E8

#define LED_TYPE                                RGB

/*
 * Define some optional offsets for color channels in the range 0..255
 * to trim some possible color inconsistency of the LED strip:
 */
#define LED_RED_OFFSET                          -20
#define LED_GREEN_OFFSET                        0
#define LED_BLUE_OFFSET                         0
#define LED_WHITE_OFFSET                        0

#define CROSSFADE_ENABLED                       true
#define CROSSFADE_DELAY_MICROSECONDS            1500
#define CROSSFADE_STEPCOUNT                     256

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define PIN_LED_RED                             5
#define PIN_LED_GREEN                           3
#define PIN_LED_BLUE                            6
#define PIN_LED_WHITE                           9

ASB asb0(ASB_NODE_ID);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);

bool stateOnOff;
bool transitionEffect;
uint8_t brightness; 

/*
 * These color values are the original state values:
 */

uint8_t originalRedValue = 0;
uint8_t originalGreenValue = 0;
uint8_t originalBlueValue = 0;
uint8_t originalWhiteValue = 0;

/*
 * These color values include offset and brightness:
 */

uint8_t currentRedValue = 0;
uint8_t currentGreenValue = 0; 
uint8_t currentBlueValue = 0;
uint8_t currentWhiteValue = 0;

uint8_t previousRedValue = currentRedValue;
uint8_t previousGreenValue = currentGreenValue;
uint8_t previousBlueValue = currentBlueValue;
uint8_t previousWhiteValue = currentWhiteValue;

enum LEDType {
    RGB,
    RGBW
};

void setup()
{
    Serial.begin(115200);

    char buffer[64] = {0};
    sprintf(buffer, "setup(): The node '0x%04X' was powered up.", ASB_NODE_ID);
    Serial.println(buffer);

    setupLEDs();
    setupCanBus();

    // After boot, publish current status
    sendCurrentStatePacket();
}

void setupLEDs()
{
    Serial.println("setupLEDs(): Setup LEDs...");

    // Set initial values for LED
    stateOnOff = true;
    transitionEffect = true;
    brightness = 255;

    // For RGBW LED type show only the native white LEDs
    if (LED_TYPE == RGBW)
    {
        originalRedValue = 0;
        originalGreenValue = 0;
        originalBlueValue = 0;
        originalWhiteValue = 255;
    }
    else
    {
        originalRedValue = 255;
        originalGreenValue = 255;
        originalBlueValue = 255;
        originalWhiteValue = 0;
    }

    #if DEBUG >= 2
        Serial.print("setupLEDs(): originalRedValue = ");
        Serial.print(originalRedValue);
        Serial.print(", originalGreenValue = ");
        Serial.print(originalGreenValue);
        Serial.print(", originalBlueValue = ");
        Serial.print(originalBlueValue);
        Serial.print(", originalWhiteValue = ");
        Serial.print(originalWhiteValue);
        Serial.println();
    #endif

    showGivenColor(originalRedValue, originalGreenValue, originalBlueValue, originalWhiteValue, transitionEffect);
}

void setupCanBus()
{
    Serial.println(F("setupCanBus(): Attaching CAN..."));

    const char busId = asb0.busAttach(&asbCan0);

    if (busId != -1)
    {
        Serial.println(F("setupCanBus(): Attaching CAN was successful."));

        setupCanBusLightChangedPacketReceived();
        setupCanBusRequestStatePacketReceived();
    }
    else
    {
        Serial.println(F("setupCanBus(): Attaching CAN failed!"));
    }
}

void setupCanBusLightChangedPacketReceived()
{
    Serial.println(F("setupCanBusLightChangedPacketReceived(): Attaching light state changed hook..."));

    const uint8_t hookOnPacketType = ASB_PKGTYPE_MULTICAST;
    const unsigned int hookOnTarget = ASB_NODE_ID;
    const uint8_t hookOnPort = 0xFF;
    const uint8_t hookOnFirstDataByte = ASB_CMD_S_LIGHT;

    if (asb0.hookAttach(hookOnPacketType, hookOnTarget, hookOnPort, hookOnFirstDataByte, onLightChangedPacketReceived))
    {
        Serial.println(F("setupCanBusLightChangedPacketReceived(): Attaching light state changed was successful."));
    }
    else
    {
        Serial.println(F("setupCanBusLightChangedPacketReceived(): Attaching light state changed failed!"));
    }
}

void onLightChangedPacketReceived(asbPacket &canPacket)
{
    stateOnOff = canPacket.data[1];
    brightness = constrain(canPacket.data[2], 0, 255);
    transitionEffect = canPacket.data[3];

    const uint8_t redValue = constrain(canPacket.data[4], 0, 255);
    const uint8_t greenValue = constrain(canPacket.data[5], 0, 255);
    const uint8_t blueValue = constrain(canPacket.data[6], 0, 255);
    const uint8_t whiteValue = constrain(canPacket.data[7], 0, 255);

    #if DEBUG >= 1
        Serial.print(F("onLightChangedPacketReceived(): The light was changed to: "));
        Serial.print(F("stateOnOff = "));
        Serial.print(stateOnOff);
        Serial.print(F(", brightness = "));
        Serial.print(brightness);
        Serial.print(F(", transitionEffect = "));
        Serial.print(transitionEffect);
        Serial.print(F(", redValue = "));
        Serial.print(redValue);
        Serial.print(F(", greenValue = "));
        Serial.print(greenValue);
        Serial.print(F(", blueValue = "));
        Serial.print(blueValue);
        Serial.print(F(", whiteValue = "));
        Serial.print(whiteValue);
        Serial.println();
    #endif

    originalRedValue = redValue;
    originalGreenValue = greenValue;
    originalBlueValue = blueValue;

    const uint8_t redValueWithOffset = constrain(originalRedValue + LED_RED_OFFSET, 0, 255);
    const uint8_t greenValueWithOffset = constrain(originalGreenValue + LED_GREEN_OFFSET, 0, 255);
    const uint8_t blueValueWithOffset = constrain(originalBlueValue + LED_BLUE_OFFSET, 0, 255);
    uint8_t whiteValueWithOffset;

    const uint8_t redValueWithBrightness = map(redValueWithOffset, 0, 255, 0, brightness);
    const uint8_t greenValueWithBrightness = map(greenValueWithOffset, 0, 255, 0, brightness);
    const uint8_t blueValueWithBrightness = map(blueValueWithOffset, 0, 255, 0, brightness);
    uint8_t whiteValueWithBrightness;

    if (LED_TYPE == RGBW)
    {
        originalWhiteValue = whiteValue;
        whiteValueWithOffset = constrain(originalWhiteValue + LED_WHITE_OFFSET, 0, 255);
        whiteValueWithBrightness = map(whiteValueWithOffset, 0, 255, 0, brightness);
    }
    else
    {
        originalWhiteValue = 0;
        whiteValueWithOffset = 0;
        whiteValueWithBrightness = 0;
    }

    if (stateOnOff == true)
    {
        showGivenColor(redValueWithBrightness, greenValueWithBrightness, blueValueWithBrightness, whiteValueWithBrightness, transitionEffect);
    }
    else
    {
        showGivenColor(0, 0, 0, 0, transitionEffect);
    }
}

void showGivenColor(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue, const bool transitionEffect)
{
    #if DEBUG >= 2
        Serial.print(F("showGivenColor(): redValue = "));
        Serial.print(redValue);
        Serial.print(F(", greenValue = "));
        Serial.print(greenValue);
        Serial.print(F(", blueValue = "));
        Serial.print(blueValue);
        Serial.print(F(", whiteValue = "));
        Serial.print(whiteValue);
        Serial.println();
    #endif

    if (CROSSFADE_ENABLED && transitionEffect)
    {
        showGivenColorWithTransition(redValue, greenValue, blueValue, whiteValue);
    }
    else
    {
        showGivenColorImmediately(redValue, greenValue, blueValue, whiteValue);
    }
}

void showGivenColorWithTransition(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{
    const float valueChangePerStepRed = calculateValueChangePerStep(previousRedValue, redValue);
    const float valueChangePerStepGreen = calculateValueChangePerStep(previousGreenValue, greenValue);
    const float valueChangePerStepBlue = calculateValueChangePerStep(previousBlueValue, blueValue);
    const float valueChangePerStepWhite = calculateValueChangePerStep(previousWhiteValue, whiteValue);

    #if DEBUG >= 2
        Serial.print("showGivenColorWithTransition(): valueChangePerStepRed = ");
        Serial.print(valueChangePerStepRed);
        Serial.print(", valueChangePerStepGreen = ");
        Serial.print(valueChangePerStepGreen);
        Serial.print(", valueChangePerStepBlue = ");
        Serial.print(valueChangePerStepBlue);
        Serial.print(", valueChangePerStepWhite = ");
        Serial.print(valueChangePerStepWhite);
        Serial.println();
    #endif

    float tempRedValue = currentRedValue;
    float tempGreenValue = currentGreenValue; 
    float tempBlueValue = currentBlueValue;
    float tempWhiteValue = currentWhiteValue;

    for (int i = 0; i < CROSSFADE_STEPCOUNT; i++)
    {
        tempRedValue = tempRedValue + valueChangePerStepRed;
        tempGreenValue = tempGreenValue + valueChangePerStepGreen;
        tempBlueValue = tempBlueValue + valueChangePerStepBlue;
        tempWhiteValue = tempWhiteValue + valueChangePerStepWhite;

        currentRedValue = round(tempRedValue);
        currentGreenValue = round(tempGreenValue);
        currentBlueValue = round(tempBlueValue);
        currentWhiteValue = round(tempWhiteValue);

        showGivenColorImmediately(currentRedValue, currentGreenValue, currentBlueValue, currentWhiteValue);
        delayMicroseconds(CROSSFADE_DELAY_MICROSECONDS);
    }

    previousRedValue = currentRedValue; 
    previousGreenValue = currentGreenValue; 
    previousBlueValue = currentBlueValue;
    previousWhiteValue = currentWhiteValue;
}

float calculateValueChangePerStep(const uint8_t startValue, const uint8_t endValue)
{
    const float valueChangePerStep = ((float) (endValue - startValue)) / ((float) CROSSFADE_STEPCOUNT);
    return valueChangePerStep;
}

void showGivenColorImmediately(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{
    #if DEBUG >= 2
        Serial.print(F("showGivenColorImmediately(): redValue = "));
        Serial.print(redValue);
        Serial.print(F(", greenValue = "));
        Serial.print(greenValue);
        Serial.print(F(", blueValue = "));
        Serial.print(blueValue);
        Serial.print(F(", whiteValue = "));
        Serial.print(whiteValue);
        Serial.println();
    #endif

    analogWrite(PIN_LED_RED, redValue);
    analogWrite(PIN_LED_GREEN, greenValue);
    analogWrite(PIN_LED_BLUE, blueValue);
    analogWrite(PIN_LED_WHITE, whiteValue);
}

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
    // Send current state if it was requested
    sendCurrentStatePacket();
}

bool sendCurrentStatePacket()
{
    const unsigned int targetAdress = ASB_BRIDGE_NODE_ID;
    byte lightStatePacketData[8] = {
        ASB_CMD_S_LIGHT,
        stateOnOff,
        brightness,
        transitionEffect,
        originalRedValue,
        originalGreenValue,
        originalBlueValue,
        originalWhiteValue
    };

    const byte lightStatePacketStats = asb0.asbSend(ASB_PKGTYPE_MULTICAST, targetAdress, sizeof(lightStatePacketData), lightStatePacketData);
    return (lightStatePacketStats == 0);
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
}
