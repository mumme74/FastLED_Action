// example.ino
//   make led strips around a logo, like so
//      ___        __         ___    ___
//    /     \     |  |        \  \  /  /
//   |       |    |  |         \  \/  /
//    \     /     |  |_____     \    /
//      ---       |________|     |__|
//   __________________________________
//  |                                  |
//   ----------------------------------
//  Leds strip led1 wraps around the O, L, and partly around Y. but we had to use another LEd strip to complete Y. These 2 strips are Controlled by CLEDController led1c and led2c
//  The led strip 2 continues around the underline so O and L are connected to led1c and Y and underline to led2c
//


// arduino MEGA 2560
#include <Arduino.h>
#include <FastLED_Action.h>

const uint8_t DATAOUT_CH1 = 3, // as in arduino i/o pin
              DATAOUT_CH2 = 4;

const uint8_t NUM_LEDS_CH1 = 150,
              NUM_LEDS_CH2 = 150;

CRGB leds_ch1[NUM_LEDS_CH1],
     leds_ch2[NUM_LEDS_CH2];

CLEDController
     *led1c = &FastLED.addLeds<UCS1903, DATAOUT_CH1, BRG>(leds_ch1, NUM_LEDS_CH1),
     *led2c = &FastLED.addLeds<UCS1903, DATAOUT_CH2, BRG>(leds_ch2, NUM_LEDS_CH2);

SegmentPart partO(led1c, 10, 50), // ledController, first led, how many leds
            partL(led1c, 60, 65),
            partYpart1(led1c, 130, 20),
            partYpart2(led2c, 0, 50), // note other ledController
            partUnderline(led2c, 60, 90);

Segment letterO,
        letterL,
        letterY,
        underline;

SegmentCompound letters,
                complete;

// our led program here
void FastLED_Action::program(){
    // init our segments for this logo
    letterO.addSegmentPart(partO);
    letterL.addSegmentPart(partL);
    letterY.addSegmentPart(partYpart1);
    letterY.addSegmentPart(partYpart2);
    underline.addSegmentPart(partUnderline);

    // create some color actions
    ActionColor actRed(CRGB::Red), // default duratioin 1000ms
                actGreen(CRGB::Green),
                actBlue(CRGB::Blue),
                actGray(CRGB::Gray);

    // ligth up one letter each second
    letterO.addAction(actRed);
    letterO.yieldUntilAction(actBlue); // wait for duration
    letterL.addAction(actGreen);
    letterL.yieldUntilAction(); // also wait for complete duration
    letterY.addAction(actBlue);
    letterY.yieldUntilAction(); // wait 1s
    underline.addAction(actGray);
    underline.yieldUntilAction();

    // store all segments in a container for one Time Access
    letters.addSegment(letterO);
    letters.addSegment(letterL);
    letters.addSegment(letterY);

    // add actions to all our letters
    letters.addAction(actRed);
    letters.addAction(actGreen);
    letters.addAction(actBlue);
    letters.yieldUntilAction(actBlue); // wait until all 3 color have changed all 3 letters

    // add to a complete container
    complete.addCompound(letters);
    complete.addSegment(underline);

    // create a ladder of coler from color to color
    ActionColorLadder actLadder(CRGB::Red, CRGB::Blue, 500);
    ActionSnake actSnakeR(CRGB::DarkGray, CRGB::WhiteSmoke),
                // runs around
                actSnakeL(CRGB::DarkGray, CRGB::WhiteSmoke, true, true);
                // runs around reversed and keeps color WhiteSmoke

    complete.addAction(actLadder);
    complete.addAction(actSnakeR);
    complete.addAction(actSnakeL);

    complete.yieldUntilAction(actSnakeL); // wait for all 3 colors

    // only work on underline, we need to remove underline from complete
    complete.removeSegment(underline);
    ActionGotoColor actGoColor1(CRGB::White, CRGB::Red),
                    actGoColor2(CRGB::Red, CRGB::White);
    underline.addAction(actGoColor1);
    underline.addAction(actGoColor2);
    underline.yieldUntilAction(2); // wait for these 2 to finish

    // now we return and our led animation program is finished
}

void setup(){
    Serial.begin(115200);
}

void loop() {
    FastLED_Action::loop(); // NOTE! must call loop for our lib

    // run our led animation if we have chars available on Serial
    if (Serial.available() > 0) {
        Serial.read(); // clear Serial
        FastLED_Action::runProgram(2); // runs our program 2 times,
                                       // if -1 it repeats forever
    }
}
