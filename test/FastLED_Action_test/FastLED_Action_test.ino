// test for FastLED wrapper

#include <testmacros.h>
#include <Arduino.h>
#include <FastLED_Action.h>

initTests();

const uint8_t OUTPIN_CH1 = 3,
              OUTPIN_CH2 = 4,
              OUTPIN_CH3 = 5;

const uint8_t NUMLEDS_CH1 = 150,
              NUMLEDS_CH2 = 55,
              NUMLEDS_CH3 = 200;

CRGB leds_ch1[NUMLEDS_CH1],
     leds_ch2[NUMLEDS_CH2],
     leds_ch3[NUMLEDS_CH3];

void checkTime(uint32_t time, uint32_t maxTime, int line){
  if (time > maxTime) {
    Serial.print("** Error took to much time:");Serial.print(time);
    Serial.print(" maxMs:");Serial.print(maxTime);
    ++_errCnt;
  } else {
    Serial.print("Time took:");Serial.print(time);
  }
  Serial.print(" at line: ");Serial.println(line);
  ++_testsCnt;
}

void setAllBlack(){
  // clear old stuff
  for(int i = 0; i < NUMLEDS_CH1; ++i)
    leds_ch1[i] = CRGB::Black;
  for(int i = 0; i < NUMLEDS_CH2; ++i)
    leds_ch2[i] = CRGB::Black;
  for(int i = 0; i < NUMLEDS_CH3; ++i)
    leds_ch3[i] = CRGB::Black;
}

void checkAllSegmentPartColors(SegmentPart &part, uint32_t color, int line){
  for(uint16_t i = 0; i < part.size(); ++i)
    testTypeHintLine(cRgbToUInt(*part[i]), color, uint32_t, line);
}

void testSingleChSegment(){
  setAllBlack();
  CLEDController
    *cont_all_ch1 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, NUMLEDS_CH1);
  Segment seg;
  SegmentPart segPart(cont_all_ch1, 0, NUMLEDS_CH1);
  seg.addSegmentPart(&segPart);
  test(seg.segmentPartsList().length(), 1);

  test(seg.halted(), false);
  seg.setHalted(true);
  test(seg.halted(), true);

  seg.segmentPartAt(0)->ledController()->clearLedData();

  // test that colors change on first loop, not from construction
  ActionColor actColor(CRGB::Aqua);
  seg.addAction(&actColor);
  for(int i = 0; i < NUMLEDS_CH1; ++i)
    testTypeHint(cRgbToUInt(leds_ch1[i]), CRGB::Black, uint32_t);

  seg.setHalted(false);
  test(seg.halted(), false);
  testTypeHint(cRgbToUInt(leds_ch1[0]), CRGB::Black, uint32_t);

  uint32_t time = millis();
  testDelay(10); // yields so loop can do its work

  // all leds should have color now
  for(int i = 0; i < NUMLEDS_CH1; ++i)
    testTypeHint(cRgbToUInt(leds_ch1[i]), CRGB::Aqua, uint32_t);

  // test ActionDark
  ActionDark actDark(350);
  seg.addAction(&actDark);
  test(seg.actionsSize(), 2);
  test(seg.currentActionIdx(), 0);

  seg.yieldUntilAction();

  test(actColor.isFinished(), true);
  time = millis() - time;
  checkTime(time, 1050, __LINE__);

  // we have switched action but not yet recolored
  testTypeHint(cRgbToUInt(leds_ch1[0]), CRGB::Aqua, uint32_t);
  testDelay(1); // let loop breathe
  time = millis();
  for(int i = 0; i < NUMLEDS_CH1; ++i)
    testTypeHint(cRgbToUInt(leds_ch1[i]), CRGB::Black, uint32_t);

  // test auto delete singleshot
  seg.setHalted(true);

  // test setCurrentAction
  seg.setCurrentActionIdx(1);
  test(seg.currentActionIdx(), 1);
  test((uint32_t)seg.currentAction(), (uint32_t)&actDark);
  seg.setCurrentActionIdx(0);
  test(seg.currentActionIdx(), 0);
  test((uint32_t)seg.currentAction(), (uint32_t)&actColor);
  seg.nextAction(); // we run actDark twice here
  actColor.reset();
  test((uint32_t)seg.currentAction(), (uint32_t)&actDark);

  actColor.setSingleShot(true);
  seg.setHalted(false);

  // check so we change color
  seg.yieldUntilAction();
  time = millis() - time;
  checkTime(time, 850, __LINE__); // we run actDark twice
  time = millis();
  test(actDark.isFinished(), true);
  // we shouldn't have any switch yet
  test((uint32_t)seg.currentAction(), (uint32_t)&actDark);
  while(seg.currentAction() != &actColor)
    testDelay(1);

  // now we should have colorAction
  test((uint32_t)seg.currentAction(), (uint32_t)&actColor);
  testDelay(1); // let it start
  test(actColor.isRunning(), true);

  for(int i = 0; i < NUMLEDS_CH1; ++i)
    testTypeHint(cRgbToUInt(leds_ch1[i]), CRGB::Aqua, uint32_t);

  // test so we don't autodelete to early
  testDelay(900 - (millis() - time));
  test(seg.actionsSize(), 2);
  time = seg.yieldUntilAction();
  test(seg.actionsSize(), 1);
}

void testSegmentManyChannels(){
  setAllBlack();

  CLEDController
    *cont_ch1 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, NUMLEDS_CH1),
    *cont_ch2 = &FastLED.addLeds<UCS1903, OUTPIN_CH2, BRG>(leds_ch2, NUMLEDS_CH2),
    *cont_ch3 = &FastLED.addLeds<UCS1903, OUTPIN_CH3, BRG>(leds_ch3, NUMLEDS_CH3);
  Segment seg;
  SegmentPart segPart1_ch1(cont_ch1, 10, 15),
              segPart2_ch1(cont_ch1, 30, 20),
              segPart3_ch1(cont_ch1, 55, 25),
              segPart1_ch2(cont_ch2, 5, 20),
              segPart2_ch2(cont_ch2, 30, 20),
              segPart1_ch3(cont_ch3, 10, 25);

  seg.addSegmentPart(&segPart1_ch1);
  seg.addSegmentPart(&segPart2_ch1);
  seg.addSegmentPart(&segPart3_ch1);
  seg.addSegmentPart(&segPart1_ch2);
  seg.addSegmentPart(&segPart2_ch2);
  seg.addSegmentPart(&segPart1_ch3);
  test(seg.segmentPartsList().length(), 6);

  ActionColor actColor(CRGB::Aqua);
  ActionDark actDark;

  seg.addAction(&actColor);
  seg.addAction(&actDark);

  test(seg.actionsSize(), 2);
  seg.setHalted(false);

  // check that we haven't changed before loop is called
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Black, __LINE__);
  FastLED_Action::loop();
  test((uint32_t)seg.currentAction(), (uint32_t)&actColor);

  // test for overwrite edges or underwrite (not writing within bounds)
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Aqua, __LINE__);
  testTypeHint(cRgbToUInt(leds_ch1[9]), CRGB::Black, uint32_t);
  testTypeHint(cRgbToUInt(leds_ch1[26]), CRGB::Black, uint32_t);
  testTypeHint(cRgbToUInt(leds_ch1[29]), CRGB::Black, uint32_t);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Aqua, __LINE__);
  testTypeHint(cRgbToUInt(leds_ch1[51]), CRGB::Black, uint32_t);
  testTypeHint(cRgbToUInt(leds_ch1[54]), CRGB::Black, uint32_t);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Aqua, __LINE__);
  testTypeHint(cRgbToUInt(leds_ch1[81]), CRGB::Black, uint32_t);
  testTypeHint(cRgbToUInt(leds_ch2[4]), CRGB::Black, uint32_t);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Aqua, __LINE__);
  testTypeHint(cRgbToUInt(leds_ch2[26]), CRGB::Black, uint32_t);
  testTypeHint(cRgbToUInt(leds_ch2[29]), CRGB::Black, uint32_t);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Aqua, __LINE__);
  testTypeHint(cRgbToUInt(leds_ch2[51]), CRGB::Black, uint32_t);
  testTypeHint(cRgbToUInt(leds_ch3[9]), CRGB::Black, uint32_t);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Aqua, __LINE__);
  testTypeHint(cRgbToUInt(leds_ch3[36]), CRGB::Black, uint32_t);

  // make sure it has cleared (send away any changes)
  test(FastLED_Action::instance().ledControllerHasChanges(cont_ch1), false)
  test(FastLED_Action::instance().ledControllerHasChanges(cont_ch2), false);
  test(FastLED_Action::instance().ledControllerHasChanges(cont_ch3), false);;

  seg.yieldUntilAction();
  test((uint32_t)seg.currentAction(), (uint32_t)&actDark);
  FastLED_Action::loop();

  checkAllSegmentPartColors(segPart1_ch1, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Black, __LINE__);

}

void testCompound(){
  setAllBlack();

  CLEDController
    *cont_ch1 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, NUMLEDS_CH1),
    *cont_ch2 = &FastLED.addLeds<UCS1903, OUTPIN_CH2, BRG>(leds_ch2, NUMLEDS_CH2),
    *cont_ch3 = &FastLED.addLeds<UCS1903, OUTPIN_CH3, BRG>(leds_ch3, NUMLEDS_CH3);
  Segment seg1, seg2, seg3;
  SegmentPart segPart1_ch1(cont_ch1, 10, 15),
              segPart2_ch1(cont_ch1, 30, 20),
              segPart3_ch1(cont_ch1, 55, 25),
              segPart1_ch2(cont_ch2, 5, 20),
              segPart2_ch2(cont_ch2, 30, 20),
              segPart1_ch3(cont_ch3, 10, 25);

  seg1.addSegmentPart(&segPart1_ch1);
  seg1.addSegmentPart(&segPart2_ch1);
  seg2.addSegmentPart(&segPart3_ch1);
  seg2.addSegmentPart(&segPart1_ch2);
  seg3.addSegmentPart(&segPart2_ch2);
  seg3.addSegmentPart(&segPart1_ch3);
  test(seg1.segmentPartsList().length(), 2);
  test(seg2.segmentPartsList().length(), 2);
  test(seg3.segmentPartsList().length(), 2);

  ActionColor actColor1(CRGB::Red),
              actColor2(CRGB::Green),
              actColor3(CRGB::Blue);

  seg1.addAction(&actColor1);
  seg2.addAction(&actColor2);
  seg3.addAction(&actColor3);


  // test that yield don't block if we are halted
  seg1.setHalted(true);
  uint32_t time = seg1.yieldUntilAction();
  test(time, 0);
  seg1.setHalted(false);
  FastLED_Action::loop();

  // make sure we have different colors on each segment
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Red, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Red, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Green, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Green, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Blue, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Blue, __LINE__);

  seg1.yieldUntilAction();

  // test so compound detaches segment from loop
  ActionDark actDark1, actDark2, actDark3;
  seg1.addAction(&actDark1);
  seg2.addAction(&actDark2);
  seg3.addAction(&actDark3);

  // test a single compound
  SegmentCompound compound;
  compound.addSegment(&seg1);
  compound.addSegment(&seg2);
  compound.addSegment(&seg3);
  testDelay(200); // breathe

  // make sure we still have different colors on each segment
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Red, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Red, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Green, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Green, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Blue, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Blue, __LINE__);

  ActionColor actComp1(CRGB::Aqua), actComp2(CRGB::White);
  compound.addAction(&actComp1);
  compound.addAction(&actComp2);

  FastLED_Action::loop();
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Aqua, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Aqua, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Aqua, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Aqua, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Aqua, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Aqua, __LINE__);

  compound.yieldUntilAction();
  FastLED_Action::loop();
  checkAllSegmentPartColors(segPart1_ch1, CRGB::White, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::White, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::White, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::White, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::White, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::White, __LINE__);

  // make sure compound re-attaches segment to loop control
  seg1.setCurrentActionIdx(0);
  seg1.currentAction()->reset();
  compound.removeSegment(&seg1);
  FastLED_Action::loop();

  // make sure it is turning black
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Red, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Red, __LINE__);
  seg1.yieldUntilAction();
  FastLED_Action::loop();
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Black, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Black, __LINE__);

  // compound storing compound
  SegmentCompound comp2;
  ActionColor actComp3(CRGB::Beige), actComp4(CRGB::Bisque);
  comp2.addAction(&actComp3);
  comp2.addAction(&actComp4);
  comp2.addCompound(&compound);
  comp2.addSegment(&seg1);

  FastLED_Action::loop();
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Beige, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Beige, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Beige, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Beige, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Beige, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Beige, __LINE__);

  comp2.yieldUntilAction();
  FastLED_Action::loop();

  // make sure we have detached first compound for loop control
  checkAllSegmentPartColors(segPart1_ch1, CRGB::Bisque, __LINE__);
  checkAllSegmentPartColors(segPart2_ch1, CRGB::Bisque, __LINE__);
  checkAllSegmentPartColors(segPart3_ch1, CRGB::Bisque, __LINE__);
  checkAllSegmentPartColors(segPart1_ch2, CRGB::Bisque, __LINE__);
  checkAllSegmentPartColors(segPart2_ch2, CRGB::Bisque, __LINE__);
  checkAllSegmentPartColors(segPart1_ch3, CRGB::Bisque, __LINE__);
}

void testActions(){
  setAllBlack();

    CLEDController
      *cont_ch1 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, NUMLEDS_CH1);
    Segment seg1;
    SegmentPart segPart1_ch1(cont_ch1, 10, 15),
                segPart2_ch1(cont_ch1, 30, 20),
                segPart3_ch1(cont_ch1, 55, 25);

    seg1.addSegmentPart(&segPart1_ch1);
    seg1.addSegmentPart(&segPart2_ch1);
    seg1.addSegmentPart(&segPart3_ch1);
    test(seg1.segmentPartsList().length(), 3);

    ActionColor actColor1(CRGB::Red),
                actColor2(CRGB::Green),
                actColor3(CRGB::Blue);

    seg1.addAction(&actColor1);
    seg1.addAction(&actColor2);
    seg1.addAction(&actColor3);

    actColor1.setSingleShot(true);
    actColor2.setSingleShot(true);
    actColor3.setSingleShot(true);

    // test singleshot
    test(seg1.actionsSize(), 3);
    FastLED_Action::loop();
    checkAllSegmentPartColors(segPart1_ch1, CRGB::Red, __LINE__);
    checkAllSegmentPartColors(segPart2_ch1, CRGB::Red, __LINE__);
    checkAllSegmentPartColors(segPart3_ch1, CRGB::Red, __LINE__);

    seg1.yieldUntilAction();
    FastLED_Action::loop();
    test(seg1.actionsSize(), 2);

    checkAllSegmentPartColors(segPart1_ch1, CRGB::Green, __LINE__);
    checkAllSegmentPartColors(segPart2_ch1, CRGB::Green, __LINE__);
    checkAllSegmentPartColors(segPart3_ch1, CRGB::Green, __LINE__);

    seg1.yieldUntilAction();
    FastLED_Action::loop();
    test(seg1.actionsSize(), 1);

    checkAllSegmentPartColors(segPart1_ch1, CRGB::Blue, __LINE__);
    checkAllSegmentPartColors(segPart2_ch1, CRGB::Blue, __LINE__);
    checkAllSegmentPartColors(segPart3_ch1, CRGB::Blue, __LINE__);

    seg1.yieldUntilAction();
    FastLED_Action::loop();
    test(seg1.actionsSize(), 0);

    // done testing singleshot/autoremove
    ActionGotoColor actGoColor1(CRGB::Aqua, CRGB::Orange),
                    actGoColor2(CRGB::Orange, CRGB::DarkGray, 2000),
                    actGoColor3(CRGB::DarkGray, CRGB::Red, 900);
    seg1.addAction(&actGoColor1);
    seg1.addAction(&actGoColor2);
    seg1.addAction(&actGoColor3);

    FastLED_Action::loop();
    uint32_t time = millis();
    checkAllSegmentPartColors(segPart1_ch1, CRGB::Aqua, __LINE__);
    seg1.yieldUntilAction();
    checkTime(millis() - time, 1050, __LINE__);
    FastLED_Action::loop();
    time = millis();
    checkAllSegmentPartColors(segPart1_ch1, CRGB::Orange, __LINE__);
    seg1.yieldUntilAction();
    checkTime(millis() - time, 2050, __LINE__);
    FastLED_Action::loop();
    time = millis();
    checkAllSegmentPartColors(segPart1_ch1, CRGB::DarkGray, __LINE__);
    seg1.yieldUntilAction();
    checkTime(millis() - time, 950, __LINE__);
    testTypeHint((uint32_t)seg1.currentAction(), (uint32_t)&actGoColor1, uint32_t);
    checkAllSegmentPartColors(segPart1_ch1, CRGB::Red, __LINE__);

    // remove all actions
    while(seg1.actionsSize() > 0)
      seg1.removeActionByIdx(0);

    test(seg1.actionsSize(), 0);

    // test Color Ladder
    ActionColorLadder actLadder1(CRGB::Black, CRGB::White);
    seg1.addAction(&actLadder1);

    FastLED_Action::loop();
    testTypeHint(cRgbToUInt(*seg1[0]), CRGB::Black, uint32_t);
    testTypeHint(cRgbToUInt(*seg1[(seg1.size() +1) / 2]), 0x828282, uint32_t);
    testTypeHint(cRgbToUInt(*seg1[seg1.size() -1]), 0xFFFFFF, uint32_t);
    seg1.yieldUntilAction();

    seg1.removeAction(&actLadder1);

    // begin test brightness
    for (uint16_t i = 0, sz = seg1.size(); i < sz; ++i)
      *seg1[i] = CRGB::White;

    ActionFade actBright1(10);
    seg1.addAction(actBright1);
    FastLED_Action::loop();
    testTypeHint(cRgbToUInt(*seg1[seg1.size() -1]), 0xFFFFFF, uint32_t);
    FastLED_Action::loop();
    seg1.yieldUntilAction();
    testTypeHint(cRgbToUInt(*seg1[seg1.size() -1]), 0x111111, uint32_t);

    while(seg1.actionsSize())
      seg1.removeActionByIdx(0);

    // begin test snake
    for (uint16_t i = 0, sz = seg1.size(); i < sz; ++i)
      *seg1[i] = CRGB::White;

    ActionSnake actSnake1(CRGB::Aqua, CRGB::White), actSnake2(CRGB::White, CRGB::Aqua, true, 500);
    seg1.addAction(actSnake1);
    seg1.addAction(actSnake2);

    FastLED_Action::loop();
    testTypeHint(cRgbToUInt(*seg1[0]), CRGB::White, uint32_t);
    testTypeHint(cRgbToUInt(*seg1[seg1.size() -1]), CRGB::Aqua, uint32_t);
    seg1.yieldUntilAction();

    testTypeHint(cRgbToUInt(*seg1[0]), CRGB::Aqua, uint32_t);
    testTypeHint(cRgbToUInt(*seg1[seg1.size() -1]), CRGB::White, uint32_t);

    FastLED_Action::loop();
    testTypeHint(cRgbToUInt(*seg1[0]), CRGB::White, uint32_t);
    testTypeHint(cRgbToUInt(*seg1[seg1.size() -1]), CRGB::Aqua, uint32_t);
    seg1.yieldUntilAction();

    testTypeHint(cRgbToUInt(*seg1[0]), CRGB::Aqua, uint32_t);
    testTypeHint(cRgbToUInt(*seg1[seg1.size() -1]), CRGB::White, uint32_t);
    while(seg1.actionsSize()> 0)
      seg1.removeActionByIdx(0);

    // test EaseInOut
    ActionEaseInOut actEaseOut(CRGB::Gray, -31), actEaseIn(CRGB::White, +31);
    seg1.addAction(actEaseOut);
    seg1.addAction(actEaseIn);
    for(uint16_t i = 0, sz = seg1.size(); i < sz; ++i)
      *seg1[i] = CRGB::White;

    FastLED_Action::loop();
    seg1.yieldUntilAction();
    testTypeHint(cRgbToUInt(*seg1[0]), 0x666666, uint32_t);

    FastLED_Action::loop();
    seg1.yieldUntilAction();
    testTypeHint(cRgbToUInt(*seg1[0]), 0xFFFFFF, uint32_t);
}

void runTests(){
  testBegin();

  testSingleChSegment();
  testSegmentManyChannels();
  testCompound();
  testActions();
  testEnd();
}

// normally our program ends up here
void FastLED_Action::program()
{
  testLoop();
}


void setup() {
  delay(100);
  Serial.begin(115200);
}

void loop(){
  FastLED_Action::runProgram();
  FastLED_Action::loop();
}
