#include "asb.h"

#define FIRMWARE_VERSION                        "1.0.0"

#define NODE_CAN_ADDRESS                        0x0002

#define PIN_CAN_CS                              10
#define PIN_CAN_INT                             2

#define PIN_LED_RED                             14
#define PIN_LED_GREEN                           13
#define PIN_LED_BLUE                            12
#define PIN_LED_WHITE                           0

#define CROSSFADE_ENABLED                       true
#define CROSSFADE_DELAY                         2
#define CROSSFADE_STEPCOUNT                     255

ASB asb0(NODE_CAN_ADDRESS);
ASB_CAN asbCan0(PIN_CAN_CS, CAN_125KBPS, MCP_8MHZ, PIN_CAN_INT);

const uint32_t defaultOnColor = 0xFFFFFF;
const uint32_t defaultOffColor = 0x000000;

int currentRed = 0;
int currentGreen = 0; 
int currentBlue = 0;
int currentWhite = 0;

int oldRed = currentRed;
int oldGreen = currentGreen;
int oldBlue = currentBlue;
int oldWhite = currentWhite;

int oldButtonState = HIGH;

void setup()
{
    Serial.begin(115200);

    const char buffer[64];
    sprintf(buffer, "The node '0x%04X' was powered up.", NODE_CAN_ADDRESS);
    Serial.println(buffer);

    setupCanBus();
    setupToggleSwitch();
}

void setupCanBus()
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

    /**
     * Type: every type (0xFF)
     * Target: every target (0x0)
     * Port: every port (-1)
     * First data byte: ASB_CMD_1B
    //  */
    // Serial.println(F("Attaching light switched hook..."));

    // if (asb0.hookAttach(0xFF, 0x0, -1, ASB_CMD_1B, onLightSwitched) == false)
    // {
    //     Serial.println(F("Attaching light switched hook was not successful!"));
    // }

    // Serial.println(F("Attaching light color hook..."));

    // if (asb0.hookAttach(0xFF, 0x0, -1, ASB_CMD_S_RGB, onColorChanged) == false)
    // {
    //     Serial.println(F("Attaching light color hook was not successful!"));
    // }
}

void setupLEDStrip()
{
    Serial.println("Setup LED strip...");
    showGivenColor(defaultOnColor);
}

void onLightSwitched( asbPacket &canPacket)
{
    // TODO: add check if setup is done

    Serial.print(F("Light was switched "));

    if (canPacket.data[1] == 1)
    {
        Serial.println("on");
        showGivenColor(defaultOnColor);
    }
    else
    {
        Serial.println("off");
        showGivenColor(defaultOffColor);
    }
}

void onBrightnessChanged( asbPacket &canPacket)
{

}

void onColorChanged( asbPacket &canPacket)
{
    // TODO: add check if setup is done

    Serial.print(F("Color was switched "));

    const uint32_t rColor = canPacket.data[1];
    const uint32_t gColor = canPacket.data[2];
    const uint32_t bColor = canPacket.data[3];
}

void showGivenColor(const uint32_t color)
{
    if (CROSSFADE_ENABLED)
    {
        showGivenColorWithFadeEffect(color);
    }
    else
    {
        showGivenColorImmediately(color);
    }
}

void showGivenColorWithFadeEffect(const uint32_t color)
{
    // // Split new color into its color parts
    //  int newRed = (color >> 16) & 0xFF;
    //  int newGreen = (color >> 8) & 0xFF;
    //  int newBlue = (color >> 0) & 0xFF;

    // // Calulate step count between old and new color value for each color part
    //  int stepCountRed = calculateStepsBetweenColorValues(oldRed, newRed);
    //  int stepCountGreen = calculateStepsBetweenColorValues(oldGreen, newGreen); 
    //  int stepCountBlue = calculateStepsBetweenColorValues(oldBlue, newBlue);

    // for (int i = 0; i <= CROSSFADE_STEPCOUNT; i++)
    // {
    //     currentRed = calculateSteppedColorValue(stepCountRed, currentRed, i);
    //     currentGreen = calculateSteppedColorValue(stepCountGreen, currentGreen, i);
    //     currentBlue = calculateSteppedColorValue(stepCountBlue, currentBlue, i);

    //  int currentColor = (currentRed << 16) | (currentGreen << 8) | (currentBlue << 0);
    //     showGivenColorImmediately(currentColor);

    //     delay(CROSSFADE_DELAY);
    // }

    // oldRed = currentRed; 
    // oldGreen = currentGreen; 
    // oldBlue = currentBlue;
}

int calculateStepsBetweenColorValues(const int oldColorValue, const int newColorValue)
{
    int colorValueDifference = newColorValue - oldColorValue;

    if (colorValueDifference)
    {
        colorValueDifference = CROSSFADE_STEPCOUNT / colorValueDifference;
    }

    return colorValueDifference;
}

int calculateSteppedColorValue( int stepCount,  int currentColorValue,  int i)
{
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

void showGivenColorImmediately(const uint32_t color)
{
    const int rawRed = (color >> 16) & 0xFF;
    const int rawGreen = (color >> 8) & 0xFF;
    const int rawBlue = (color >> 0) & 0xFF;

    int mappedRed = map(rawRed, 0, 255, 0, 1024);
    mappedRed = constrain(mappedRed, 0, 1024);

    int mappedGreen = map(rawGreen, 0, 255, 0, 1024);
    mappedGreen = constrain(mappedGreen, 0, 1024);

    int mappedBlue = map(rawBlue, 0, 255, 0, 1024);
    mappedBlue = constrain(mappedBlue, 0, 1024);

    analogWrite(PIN_LED_RED, mappedRed);
    analogWrite(PIN_LED_GREEN, mappedGreen);
    analogWrite(PIN_LED_BLUE, mappedBlue);
}

void loop()
{
    asb0.loop();
}
