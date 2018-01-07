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
#define LED_RED_OFFSET                          0
#define LED_GREEN_OFFSET                        0
#define LED_BLUE_OFFSET                         0
#define LED_WHITE_OFFSET                        0

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define PIN_LED_RED                             5
#define PIN_LED_GREEN                           3
#define PIN_LED_BLUE                            6
#define PIN_LED_WHITE                           9

ASB asb0(ASB_NODE_ID);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHz, PIN_CAN_INT);

int currentRedValue = 0;
int currentGreenValue = 0; 
int currentBlueValue = 0;
int currentWhiteValue = 0;

int previousRedValue = currentRedValue;
int previousGreenValue = currentGreenValue;
int previousBlueValue = currentBlueValue;
int previousWhiteValue = currentWhiteValue;

enum LEDType {
    RGB,
    RGBW
};

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "setup(): The node '0x%04X' was powered up.", ASB_NODE_ID);
    Serial.println(buffer);

    setupLEDs();
    setupCanBus();
}

void setupLEDs()
{
    Serial.println("setupLEDs(): Setup LEDs...");

    // For RGBW LED type show only the native white LEDs
    if (LED_TYPE == RGBW)
    {
        showGivenColor(0, 0, 0, 255);
    }
    else
    {
        showGivenColor(255, 255, 255, 0);
    }
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

void onLightChangedPacketReceived(const asbPacket &canPacket)
{
    const bool stateOnOff = canPacket.data[1];
    const uint8_t brightness = constrain(canPacket.data[2], 0, 255);

    const uint8_t redValue = constrain(canPacket.data[3], 0, 255);
    const uint8_t greenValue = constrain(canPacket.data[4], 0, 255);
    const uint8_t blueValue = constrain(canPacket.data[5], 0, 255);
    const uint8_t whiteValue = constrain(canPacket.data[6], 0, 255);

    #if DEBUG == 1
        Serial.println(F("onLightChangedPacketReceived(): The light was changed to:"));

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
    #endif

    const uint8_t redValueWithOffset = constrain(redValue + LED_RED_OFFSET, 0, 255);
    const uint8_t greenValueWithOffset = constrain(greenValue + LED_GREEN_OFFSET, 0, 255);
    const uint8_t blueValueWithOffset = constrain(blueValue + LED_BLUE_OFFSET, 0, 255);
    const uint8_t whiteValueWithOffset = constrain(whiteValue + LED_WHITE_OFFSET, 0, 255);

    const uint8_t redValueWithBrightness = map(redValueWithOffset, 0, 255, 0, brightness);
    const uint8_t greenValueWithBrightness = map(greenValueWithOffset, 0, 255, 0, brightness);
    const uint8_t blueValueWithBrightness = map(blueValueWithOffset, 0, 255, 0, brightness);
    const uint8_t whiteValueWithBrightness = map(whiteValueWithOffset, 0, 255, 0, brightness);

    if (stateOnOff == true)
    {
        showGivenColor(redValueWithBrightness, greenValueWithBrightness, blueValueWithBrightness, whiteValueWithBrightness);
    }
    else
    {
        showGivenColor(0, 0, 0, 0);
    }
}

void showGivenColor(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, uint8_t whiteValue)
{
    const bool fading = true;

    if (LED_TYPE != RGBW)
    {
        whiteValue = 0;
    }

    #if DEBUG == 1
        Serial.print(F("showGivenColor(): redValue = "));
        Serial.println(redValue);
        Serial.print(F("showGivenColor(): greenValue = "));
        Serial.println(greenValue);
        Serial.print(F("showGivenColor(): blueValue = "));
        Serial.println(blueValue);
        Serial.print(F("showGivenColor(): whiteValue = "));
        Serial.println(whiteValue);
    #endif

    if (fading)
    {
        showGivenColorWithFadeEffect(redValue, greenValue, blueValue, whiteValue);
    }
    else
    {
        showGivenColorImmediately(redValue, greenValue, blueValue, whiteValue);
    }
}

// TODO: move
#define CROSSFADE_DELAY                         2
#define CROSSFADE_STEPCOUNT                     255

void showGivenColorWithFadeEffect(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{
    // Calulate step count between old and new color value for each color part
    const int stepCountRed = calculateStepsBetweenColorValues(previousRedValue, redValue);
    const int stepCountGreen = calculateStepsBetweenColorValues(previousGreenValue, greenValue); 
    const int stepCountBlue = calculateStepsBetweenColorValues(previousBlueValue, blueValue);
    const int stepCountWhite = calculateStepsBetweenColorValues(previousWhiteValue, whiteValue);

    Serial.print(F("showGivenColorWithFadeEffect(): previousRedValue = "));
    Serial.println(previousRedValue);
    Serial.print(F("showGivenColorWithFadeEffect(): previousGreenValue = "));
    Serial.println(previousGreenValue);
    Serial.print(F("showGivenColorWithFadeEffect(): previousBlueValue = "));
    Serial.println(previousBlueValue);
    Serial.print(F("showGivenColorWithFadeEffect(): previousWhiteValue = "));
    Serial.println(previousWhiteValue);

    Serial.print(F("showGivenColorWithFadeEffect(): stepCountRed = "));
    Serial.println(stepCountRed);
    Serial.print(F("showGivenColorWithFadeEffect(): stepCountGreen = "));
    Serial.println(stepCountGreen);
    Serial.print(F("showGivenColorWithFadeEffect(): stepCountBlue = "));
    Serial.println(stepCountBlue);
    Serial.print(F("showGivenColorWithFadeEffect(): stepCountWhite = "));
    Serial.println(stepCountWhite);

    for (int i = 0; i <= CROSSFADE_STEPCOUNT; i++)
    {
        currentRedValue = calculateSteppedColorValue(stepCountRed, currentRedValue, i);
        currentGreenValue = calculateSteppedColorValue(stepCountGreen, currentGreenValue, i);
        currentBlueValue = calculateSteppedColorValue(stepCountBlue, currentBlueValue, i);
        currentWhiteValue = calculateSteppedColorValue(stepCountWhite, currentWhiteValue, i);

        showGivenColorImmediately(currentRedValue, currentGreenValue, currentBlueValue, currentWhiteValue);
        delay(CROSSFADE_DELAY);
    }

    previousRedValue = currentRedValue; 
    previousGreenValue = currentGreenValue; 
    previousBlueValue = currentBlueValue;
    previousWhiteValue = currentWhiteValue;
}

// TODO: smaller types
int calculateStepsBetweenColorValues(const int oldColorValue, const int newColorValue)
{
    int colorValueDifference = newColorValue - oldColorValue;

    if (colorValueDifference > 0)
    {
        colorValueDifference = CROSSFADE_STEPCOUNT / colorValueDifference;
    }

    return colorValueDifference;
}

// TODO: smaller types
int calculateSteppedColorValue(const int stepCount, const int currentColorValue, const int i) {

    int steppedColorValue = currentColorValue;

    if (stepCount && (i % stepCount) == 0)
    {
        if (stepCount > 0)
        {              
            steppedColorValue += 1;           
        } 
        else if (stepCount < 0)
        {
            steppedColorValue -= 1;
        } 
    }

    steppedColorValue = constrain(steppedColorValue, 0, 255);
    return steppedColorValue;
}

void showGivenColorImmediately(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{

    Serial.print(F("showGivenColorImmediately(): redValue = "));
    Serial.print(redValue);
    Serial.print(F(", greenValue = "));
    Serial.print(greenValue);
    Serial.print(F(", blueValue = "));
    Serial.print(blueValue);
    Serial.print(F(", whiteValue = "));
    Serial.print(whiteValue);
    Serial.println();


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

void onRequestStatePacketReceived(const asbPacket &canPacket)
{
    // Send current state if it was requested
    sendCurrentStatePacket();
}

bool sendCurrentStatePacket()
{
    // TODO: Send data
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
