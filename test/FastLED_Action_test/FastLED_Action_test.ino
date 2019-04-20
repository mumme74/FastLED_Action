// test for FastLED wrapper

#include <testmacros.h>
#include <Arduino.h>
#include <FastLED_Action.h>

initTests();

const uint8_t OUTPIN_CH1 = 3,
              OUTPIN_CH2 = 4,
              OUTPIN_CH3 = 5;

const uint8_t NUMLEDS_CH1 = 150,
              NUMLEDS_CH2 = 50,
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
  Serial.print(" at ");Serial.println(line);
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

void testSingleChSegment(){
  setAllBlack();
  CLEDController
    *cont_all_ch1 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, NUMLEDS_CH1);
  Segment seg;
  seg.addLedController(cont_all_ch1);
  test(seg.controllerList().length(), 1);

  test(seg.halted(), false);
  seg.setHalted(true);
  test(seg.halted(), true);

  seg.ledControllerAt(0)->clearLedData();

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

  seg.yieldUntilNextAction();

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
  seg.yieldUntilNextAction();
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
  time = seg.yieldUntilNextAction();
  test(seg.actionsSize(), 1);
}

void testSegmentManyChannels(){
  setAllBlack();

  CLEDController
    *cont_ch1_part1 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, 0, 10),
    *cont_ch1_part2 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, 15, 15),
    *cont_ch1_part3 = &FastLED.addLeds<UCS1903, OUTPIN_CH1, BRG>(leds_ch1, 20, 35),
    *cont_ch2_part1 = &FastLED.addLeds<UCS1903, OUTPIN_CH2, BRG>(leds_ch2, 0, 25);
  Segment seg;
  seg.addLedController(cont_ch1_part1);
  seg.addLedController(cont_ch1_part2);
  seg.addLedController(cont_ch1_part3);
  seg.addLedController(cont_ch2_part1);
  test(seg.controllerList().length(), 4);
  test(cont_ch1_part1->size(), 10);
  test(cont_ch1_part2->size(), 15);
  test(cont_ch1_part3->size(), 20);
  test(cont_ch2_part1->size(), 25);

  ActionColor actColor(CRGB::Aqua);
  ActionDark actDark;

  seg.addAction(&actColor);
  seg.addAction(&actDark);

  test(seg.actionsSize(), 2);
  seg.setHalted(false);
  for(int i = 0; i < cont_ch1_part1->size(); ++i)
    testTypeHint(cRgbToUInt(leds_ch1[i]), CRGB::Black, uint32_t);
}


void runTests(){
  testBegin();
  testSingleChSegment();
  testSegmentManyChannels();


  testEnd();
}


void setup() {
  delay(100);
  Serial.begin(115000);
}

void loop(){
  testLoop();
  FastLED_Action::loop();
}
