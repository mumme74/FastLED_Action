/**
*  Copyright (c) 2019 Fredrik Johansson mumme74@github.com. All right reserved.
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
*  Lesser General Public License for more details.
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*
*  Actions.cpp
*
*  Created on: 16 apr 2019
*      Author: Fredrik Johansson
*/


#include "Actions.h"
#include "FastLED_Action.h"


uint32_t cRgbToUInt(CRGB &rgb){
  uint32_t col = rgb.r;
  col <<= 8;
  col |= rgb.g;
  col <<= 8;
  col |= rgb.b;
  return col;
}


ActionsContainer::ActionsContainer() :
  m_currentIdx(0)
{
}

ActionsContainer::~ActionsContainer()
{
}

size_t ActionsContainer::actionsSize() const
{
  return m_actions.length();
}

void ActionsContainer::addAction(ActionBase *action)
{
  m_actions.push(action);
  Serial.print("add action length:");Serial.println(m_actions.length());
}

void ActionsContainer::addAction(ActionBase &action)
{
  addAction(&action);
}

void ActionsContainer::removeAction(ActionBase *action)
{
  Serial.println("remove action");
  for(uint16_t i = 0; i < m_actions.length(); ++i) {
    if (m_actions[i] == action) {
      m_actions.remove(i);
      if (m_currentIdx == i) {
        if (m_actions.length() -1 <= i)
          setCurrentActionIdx(0);
      } else if (m_currentIdx > i)
        --m_currentIdx;
    }
  }
}

void ActionsContainer::removeAction(ActionBase &action)
{
  removeAction(&action);
}

void ActionsContainer::removeActionByIdx(size_t idx)
{
  if (m_actions.length() > idx)
    removeAction(m_actions[idx]);
}

void ActionsContainer::nextAction()
{
  //m_actions[m_currentIdx]->reset();
  ++m_currentIdx;
  if (m_actions.length() == m_currentIdx)
    m_currentIdx = 0;

  Serial.print("next action:");Serial.print(m_currentIdx);
  Serial.print(" actions.length:");Serial.println(m_actions.length());
}

uint16_t ActionsContainer::currentActionIdx()
{
  return m_currentIdx;
}

void ActionsContainer::setCurrentActionIdx(uint16_t idx)
{
  if (idx < m_actions.length()) {
    m_currentIdx = idx;
    m_actions[idx]->reset();
  }
}

ActionBase* ActionsContainer::currentAction()
{
  if (m_actions.length() > m_currentIdx)
    return m_actions[m_currentIdx];
  return nullptr; // when empty
}


// -------------------------------------------------------

ActionBase::ActionBase(uint32_t duration) :
  m_singleShot(false), m_endTime(0),
  m_nextIterTime(0),
  m_duration(duration), m_updateTime(DefaultTickMs),
  m_eventCB(nullptr)
{
}

// how many ms between each re-render 50 = 20Hz
const uint8_t ActionBase::DefaultTickMs = 50;

ActionBase::~ActionBase()
{
}

uint32_t ActionBase::noOfTicks() const
{
  return m_duration / m_updateTime;
}

uint16_t ActionBase::tickCount() const
{
  return (millis() - startTime()) / m_updateTime;
}

bool ActionBase::isRunning() const
{
  return m_endTime > 0;
}

bool ActionBase::isFinished() const
{
  // duration of 0 is forever
  return m_duration > 0 && m_endTime <= millis();
}

void ActionBase::reset()
{
  Serial.println("reset");
  m_endTime = m_nextIterTime = 0;
}

void ActionBase::loop(SegmentCommon *owner)
{
  if (m_endTime == 0) {
    m_endTime = millis() + m_duration;
    m_nextIterTime = millis() + m_updateTime;
    eventDelegate(owner, Start);
  } else if (m_nextIterTime <= millis()) {
    m_nextIterTime = millis() + m_updateTime;
    eventDelegate(owner, Tick);
  }


  if (isFinished()) {
    eventDelegate(owner, End);
    //Serial.print("loop:");Serial.print(m_duration);Serial.print(" ");
    //Serial.print(m_endTime);Serial.print(" ");Serial.println(millis());
    reset();

    if (m_singleShot) {
      Serial.println("remove");
      owner->removeAction(this); // caution, deletes this,
                                  // no code execution after this line
    } else
      owner->nextAction();
  }
}

void ActionBase::eventDelegate(SegmentCommon *owner, EvtType evtType)
{
  if(m_eventCB)
    (*m_eventCB)(this, owner, evtType);
  else
    onEvent(owner, evtType);
}

// --------------------------------------------------

ActionColor::ActionColor(CRGB color, uint32_t duration) :
    ActionBase(duration),
    m_color(color)
{
  m_eventCB = &ActionColor::eventCB;
}

ActionColor::~ActionColor()
{
}

// static
void ActionColor::eventCB(ActionBase *self, SegmentCommon *owner, EvtType evtType)
{
  reinterpret_cast<ActionColor*>(self)->onEvent(owner, evtType);
}

void ActionColor::onEvent(SegmentCommon *owner, EvtType evtType)
{
  if (evtType == Start) {
    for(uint16_t i = 0, end = owner->size(); i < end; ++i) {
      CRGB *rgb = (*owner)[i];
      *rgb = m_color;
    }
    owner->dirty();
  }
}

// -----------------------------------------------

ActionDark::ActionDark(uint32_t duration) :
    ActionColor(CRGB::Black, duration)
{
}

ActionDark::~ActionDark()
{
}

// -----------------------------------------------
ActionColorLadder::ActionColorLadder(CRGB leftColor, CRGB rightColor, uint32_t duration) :
    ActionBase(duration),
    m_leftColor(leftColor), m_rightColor(rightColor)
{
}

ActionColorLadder::~ActionColorLadder()
{
}

// static
void ActionColorLadder::eventCB(ActionBase *self, SegmentCommon *owner, EvtType evtType)
{
  reinterpret_cast<ActionColorLadder*>(self)->onEvent(owner, evtType);
}

void ActionColorLadder::onEvent(SegmentCommon *owner, EvtType evtType)
{
  if (evtType == Start) {
    int16_t diff[] = {
        m_leftColor.red   - m_rightColor.red,
        m_leftColor.green - m_rightColor.green,
        m_leftColor.blue  - m_rightColor.blue
    };

    uint16_t sz = owner->size();
    for (uint8_t c = 0; c < 3; ++c) {
      // iterate for each color
      float color = (float)diff[c] / (sz-1);

      for(uint16_t i = 0; i < sz; ++i) {
        float col = m_leftColor.raw[c] - (color * i);
        CRGB *rgb = (*owner)[i];
        rgb->raw[c] = round(col);
      }
    }
    owner->dirty();
  }
}


// -----------------------------------------------

ActionGotoColor::ActionGotoColor(CRGB fromColor, CRGB toColor, uint32_t duration) :
    ActionBase(duration),
    m_fromColor(fromColor), m_toColor(toColor)
{
}

ActionGotoColor::~ActionGotoColor()
{
}

// static
void ActionGotoColor::eventCB(ActionBase *self, SegmentCommon *owner, EvtType evtType)
{
  reinterpret_cast<ActionGotoColor*>(self)->onEvent(owner, evtType);
}

void ActionGotoColor::onEvent(SegmentCommon *owner, EvtType evtType)
{
  uint8_t r, g, b;
  switch(evtType) {
  case Start:
    r = m_fromColor.red;
    g = m_fromColor.green;
    b = m_fromColor.blue;
    break;
  case Tick: {
    // calculate how far away each color is and how much they should change each 70ms
    int16_t redDiff   = m_toColor.red - m_fromColor.red,
            greenDiff = m_toColor.green - m_fromColor.green,
            blueDiff  = m_toColor.blue  - m_fromColor.blue;
    uint16_t iterations = noOfTicks();

    uint8_t incR = (redDiff / iterations) || 1,
            incG = (greenDiff / iterations) || 1,
            incB = (blueDiff / iterations) || 1;
    uint16_t tickCnt = tickCount();
    r = m_toColor.red - incR * tickCnt;
    g = m_toColor.green - incG * tickCnt;
    b = m_toColor.blue - incB * tickCnt;
  }  break;
  case End:
    r = m_toColor.red;
    g = m_toColor.green;
    b = m_toColor.blue;
  }

  for(uint16_t i = 0, end = owner->size(); i < end; ++i) {
    CRGB *rgb = (*owner)[i];
    rgb->red = r;
    rgb->green = g;
    rgb->blue = b;
  }
  owner->dirty();
}

// ----------------------------------------------------------------

ActionFade::ActionFade(uint8_t toBrightness, uint32_t duration) :
    ActionBase(duration),
    m_toBrightness(toBrightness)
{
  if (m_toBrightness > 100)
    m_toBrightness = 100;
}

ActionFade::~ActionFade()
{
}

void ActionFade::eventCB(ActionBase *self, SegmentCommon *owner, EvtType evtType)
{
  reinterpret_cast<ActionFade*>(self)->onEvent(owner, evtType);
}

void ActionFade::onEvent(SegmentCommon *owner, EvtType evtType)
{
  uint8_t fadeFactor = 0;

  switch(evtType){
    case Start:
      fadeFactor = 0;
      break;
    case Tick: {
      if (tickCount() >= noOfTicks() -1)
        return;
      fadeFactor = 255 - m_toBrightness;
      fadeFactor = (float)fadeFactor / (noOfTicks() - tickCount());
    } break;
    case End:
      fadeFactor = 0; //255 - m_toBrightness;
      break;
    default:
      return; // do nothing
  }

  //Serial.println(fadeFactor);
  //bool printed = false;
  for(uint16_t i = 0, sz = owner->size(); i < sz; ++i) {
    CRGB *rgb = (*owner)[i];
    rgb->fadeLightBy(fadeFactor);


//    if (!printed) {
//      Serial.print("r:");Serial.print(rgb->r);
//      Serial.print(" g:");Serial.print(rgb->g);
//      Serial.print(" b:");Serial.println(rgb->b);
//      printed = true;
//    }

  }
}

// ----------------------------------------------------------------

ActionEaseInOut::ActionEaseInOut(CRGB toColor, int8_t easeTo, uint16_t duration) :
    ActionBase(duration),
    m_toColor(toColor), m_easeFactor(easeTo),
    m_rDiff(0), m_gDiff(0), m_bDiff(0)
{
}

ActionEaseInOut::~ActionEaseInOut()
{
}

void ActionEaseInOut::eventCB(ActionBase *self, SegmentCommon *owner, EvtType evtType)
{
  reinterpret_cast<ActionSnake*>(self)->onEvent(owner, evtType);
}

void ActionEaseInOut::onEvent(SegmentCommon *owner, EvtType evtType)
{
  switch(evtType) {
  case Start:{
    // get the diff of each color to desired color
    for (uint16_t i = 0, sz = owner->size(); i < sz; ++i) {
      CRGB *rgb = (*owner)[i];
      if (rgb->r > m_toColor.r)
        m_rDiff = max(rgb->r - m_toColor.r, m_rDiff);
      else
        m_rDiff = max(m_toColor.r - rgb->r, m_rDiff);

      if (rgb->g > m_toColor.g)
        m_gDiff = max(rgb->g - m_toColor.g, m_gDiff);
      else
        m_gDiff = max(m_toColor.g - rgb->g, m_gDiff);

      if (rgb->b > m_toColor.b)
        m_bDiff = max(rgb->b - m_toColor.b, m_bDiff);
      else
        m_bDiff = max(m_toColor.b - rgb->b, m_bDiff);
    }
    uint16_t ticks = noOfTicks();
    m_rDiff /= ticks;
    m_gDiff /= ticks;
    m_bDiff /= ticks;
  }  break;
  case Tick: {
    int8_t step = m_easeFactor / noOfTicks();
    int8_t curI = step * (tickCount() - noOfTicks());
    uint8_t factor = ease8InOutQuad((uint8_t)curI);
    bool printed = false;
    for (uint16_t i = 0, sz = owner->size(); i < sz; ++i) {
      CRGB *rgb = (*owner)[i];
      uint8_t r = rgb->r, g = rgb->g, b = rgb->b;
      rgb->r = factor * m_rDiff;
      rgb->g = factor * m_gDiff;
      rgb->b = factor * m_bDiff;
      if (!printed) {
        Serial.print("r:");Serial.print(rgb->r);
        Serial.print(" g:");Serial.print(rgb->g);
        Serial.print(" b:");Serial.println(rgb->b);
        printed = true;
      }
    }
  } break;
  case End: // fallthrough
  default: {
    for (uint16_t i = 0, sz = owner->size(); i < sz; ++i)
      *(*owner)[i] = m_toColor;
  }
  }
}

// ----------------------------------------------------------------

ActionSnake::ActionSnake(CRGB baseColor, CRGB snakeColor,
                         bool reversed, bool keepSnakeColor,
                         uint32_t duration):
    ActionBase(duration),
    m_baseColor(baseColor), m_snakeColor(snakeColor),
    m_keepSnakeColor(keepSnakeColor),
    m_reversed(reversed), m_snakeIdx(0)
{
}

ActionSnake::~ActionSnake()
{
}

void ActionSnake::eventCB(ActionBase *self, SegmentCommon *owner, EvtType evtType)
{
  reinterpret_cast<ActionSnake*>(self)->onEvent(owner, evtType);
}

void ActionSnake::onEvent(SegmentCommon *owner, EvtType evtType)
{
  switch(evtType) {
  case Tick:
    if (m_reversed)
      --m_snakeIdx;
    else
      ++m_snakeIdx;
    break;
  case Start:
    // we might have more leds than what is possible within duration
    if (owner->size() * m_updateTime <= m_duration) {
      m_updateTime = m_duration / (owner->size() * m_updateTime);
    }
    m_snakeIdx = m_reversed ? owner->size() -1 : 0;
    break;
  case End:
    m_snakeIdx = m_reversed ? 0 : owner->size() -1;
    break;
  default:
    m_snakeIdx = 0;
    break;
  }

  uint16_t sz = owner->size();
  uint16_t beginAt = m_keepSnakeColor ? m_snakeIdx :
                                (m_snakeIdx > 0 ? m_snakeIdx -1 : 0),
           endAt = sz;
  int8_t inc = 1;
  if (m_reversed) {
    beginAt = m_keepSnakeColor ? m_snakeIdx :
                                (m_snakeIdx +1 < sz ? m_snakeIdx+1: m_snakeIdx);
    endAt = 0;
    inc = -1;
  }
  for(uint16_t i = beginAt; i < endAt; i += inc) {
    CRGB *rgb = (*owner)[i];
    if (i == m_snakeIdx)
      *rgb = m_snakeColor;
    else
      *rgb = m_baseColor;
  }
}

