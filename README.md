# FastLED_Action
A wrapper around FastLED library to make animations more object oriented and asyncronous.

It depends on *FastLED* (fastled.io) obviously...
But also on *DList* (github.com/mumme74/DList)

And for the test suite: *testmacros* (github.com/mumme74/testmacros)

# Description
This Lib is asyncronous and tries to abstract away different sequences and difficoult math's to objects called Actions.



# Object description
# FastLED_Action
`class FastLED_Action` is global object. It is constructed during boot.
It holds your program and handles interface to FastLED

`void FastLED_Action::loop()` 
Makes the system work, Lib must know when a new loop is occuring.
You should make a call to FastLED_Action in your loop function
```
void loop() {
  ...
  FastLED_Action::loop();
  ...
}
```
*Note!* due to asyncrounous nature of our lib, loop should be called as expected.
There are no blocking within delay functions in implementation so you shuld be able to use loop as usaual when your LED program executes in the background.

`void Fast::Action::program()` 
This is where you store your program
*Note!*   You must implement this function in your *.ino file
see example in bottom of this file

# Segments

## SegmentPart
Is the object that connets to a single ledController
`class SegmentPart(CLEDController *controller, uint8_t firstLed, uint8_t nLeds)`
*controller* is the FastLED controller for this led strip
*firstLed* is the first led that this part is working on
*nLeds* is the number of led this part handles



## Segment
Is a Object that can handle many parts, it must have at least 1 *SegmentPart*
It also abstract so we can handle leds from different LED strips on a single *Segment*

`class Segment` takes no parameters

`void addSegmentPart(SegmentPart *part)`
`void addSegmentPart(SegmentPart &part)` 
Add a *SegmentPart* to this segment
*Segment* can take many *SegmentParts*

`bool halted() const`
`void setHalted(bool halt)` 
Sets/Gets if *Segment* is halted
If halted, no action is done on this segment until it is un-Halted.

`size_t segmentPartSize() const`
 Returns how many segmentParts is stored.

`void removeSegmentPart(size_t idx)` 
Removes SegmentPart at idx.

`CRGB *operator[idx]`
Returns the CRGB corresponding to the led at *idx*.

`uint16_t size()` 
Returns how many leds this segment has.

`void dirty()` 
Call this if you have changed leds manually of this segment, else it wont render.

`uint32_t yieldUntilAction(uint16_t noOfActions = 1)`
Waits for action to finish
if actions duration is 0 (forever action) or if we are halted it returns immediately.
*noOfActions* is how many actions we wait until we return
returns time in ms that it has been of

`uint32_t yieldUntilAction(ActionBase &action)`
Same as above but waits until *action* has finished



## SegmentCompound
Is a container object that can take *Segment* or other *SegmentCompound*.
If you want to bind together several *Segments* to make a single Action work on all Segments or sub Compounds.

`class SegmentCompound`Takes no parameters

`void addSegment(Segment *segment)`
`void addSegment(Segment &segment)`

Add segment to this compound

`size_T segmentSize() const`
Returns haow many segment this compund has stored.
**NOTE!** SubContainers Segments is not counted here

`void removeSegmentByIdx(size_t idx)`
Removes segment at *idx* pos

`void removeSegment(Segment *segment)`
`void removeSegment(Segment &segment)`
Removes `the stored *Segment* pointed to by *segment* parameter

`Segment* segmentAt(size_t idx)`
Returns the segment at idx pos


`void addCompound(SegmentCompound *compound)`
`void addCompound(SegmentCompound &compound)`
Adds *compound* to the stored sub compounds

`void compoundSize()`
Returns how many sub compunds we have stored

`void removeCompoundByIdx(size_t idx)`
Removes the sub *compound* at idx pos

`void removeCompound(SegmentCompound *compound)`
`void removeCompound(SegmentCompound &compound)`
Removes *compound* from stored sub compounds.

`void compoundAt(size_t idx)`
Returns the compound at *idx* pos

`CRGB* operator[idx]`
Returns a pointer to the CRGB led at idx pos
Idx always start with local segments, then continues with sub *compound* leds. Regardless of the order that Segment/Compound was added to this Compound.
Is usefull when you want to manipulate the leds for this compound. If you do remeber to call *dirty()*, else it wont render

`uint16_t size()`
Ho many LEDs this compound has stored including sub compounds.

`void dirty()` 
Call this if you have changed leds manually of this segment, else it wont render.

`bool halted() const`
`void setHalted(bool halt)` 
Sets/Gets if this *compound* is halted
If halted, no action is done on this *compound* until it is un-Halted.

`uint32_t yieldUntilAction(uint16_t noOfActions = 1)`
Waits for action to finish
if actions duration is 0 (forever action) or if we are halted it returns immediately.
*noOfActions* is how many actions we wait until we return
returns time in ms that it has been of

`uint32_t yieldUntilAction(ActionBase &action)`
Same as above but waits until *action* has finished

`void addAction(ActionBase *action)`
`void addAction(ActionBase &action)`
Add *action* to the list of actions to run.

`size_t actionsSize() const`
Returns how many actions we have stored

`void removeAction(ActionBase *action)`
`void removeAction(ActionBase &action)`
Removes *action* from our actions.

`void removeActionByIdx(size_t idx)`
Removes *action* at idx from our list of actions to run.

`ùint16_t currentActionIdx()`
Returns the currently running actions idx in our store.

`void setCurrentActionIdx(uint16_t idx)`
Sets the currentAction it *idx*.

`ActionBase* currentAction()`
Returns a pointer to the currently running *action*



# Actions
Actions is the objects that does something on our *Segment* or *SegmentCompound* see example at buttom of this file.

Constructor might different at each different Action type
All Actions inherits *ActionBase* so all actions hae the following API

`uint32_t duration() const`
Returns how long this action should take
Duration is set when object is contructed, usually as the last parameter

`uint32_t startTime() const`
Returns what time it was when *action* started, time as in what was reported by *millis()*.

`uint32_t endTime() const`
Returns what time action will end, as in *millis()*.

`uint16_t noOfTicks() const`
Returns how many eventTicks this action has from start to finish.

`uint16_t tickCount() const`
Returns how many eventTicks this action has is at form start.

`bool isSingleShot() const`
`void setSingleShot(bool singleShot)`
Gets/Sets if this Action is a singleShot.
Singleshot means that action will remove itself from its owner after it completed

`bool isRunning() const`
Returns true if action is currently running

`bool isFinished() const`
Returns true if action has completed 

`void reset()`
Resets action so it can run again on next turn


## ActionColor
Is a action that simply turns all leds to a single color

`ActionColor(CRGB color, uint32_t duration = 1000)`
Constructor 
*color* the color that we want to set it to
*duration* how long action should last, defaults to 1000ms.


## ActionDark
A convinience action, equivialent to ActionColor(CRGB::Black)

`ActionDark(uint32_t duration = 1000)`
Constructor
*duration* how long action should last, defaults to 1000ms.

## ActionColorLadder
A action that turns leds from left to right to 2 colors
Sclaes leds in between so color mathces

`ActionColorLadder(CRGB leftColor, CRGB rightColor uint32_t duration)`
Constructor
*leftColor* the startcolor on the first led.
*rightColor* the end Color on the last led.
*duration* how long action should last, defaults to 1000ms.

## ActionGotoColor
A action that turns scales all leds formColor toColor with smooth transition

`ActionGotoColor(CRGB fromColor, CRGB toColor, uint32_t duration = 1000)`
Constructor
*formColor* start with this color
*toColor* ends with this color
*duration* how long action should last, defaults to 1000ms.


## ActionFade
A action that fades brightness toBrightness with smooth transition during duration

`ActionFade(uint8_t toBrightness, uint32_t duration = 1000)`
*toBrightness* where brightness shold fade to, 0-100 is available
*duration* how long action should last, defaults to 1000ms.


## ActionSnake
A action that sweeps around leds with a different color on a led during duration

`ActionSnake(CRGB baseColor, CRGB snakeColor, bool reversed = false, bool keepSnakeColor = false, uint32_t duration = 1000)`
*baseColor* the color that ia at beginning
*snakeColor* the color on the led that sweeps around
*reversed* indicates if swep should go backwards, default false
*keepSnakeColor* the leds that has been passed retians snakeColor, defaults to false
*duration* how long action should last, defaults to 1000ms.




# subclassing ActionBase
Note ! this is considered advanced usage.
You have to have knowledge of object inheritance in C++
If you want to create your own subclass you must implement 2 member funtions

`static void eventCB(ActionBase *self, SegmentCommon *owner, EvtType evtType)`
This function is used to upcast object to correct type.

`void onEvent(SegmentCommon *owner, EvtType evtType)`
Gets invoked on *Start*, each *Tick* and on *End*
A tick event is triggred each time m_updateTime has timed out
if duration is 1000ms and m_updateTime = 50ms it will be called 20 times




# Example

``` 
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
//  Leds strip led1 wraps around the O, L, and partly around Y. 
//  But we had to use another LEd strip to complete Y. 
//  These 2 strips are Controlled by CLEDController led1c and led2c
//  The led strip 2 continues around the underline so O and L are connected
//  to led1c and Y and underline to led2c
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
  
``` 



