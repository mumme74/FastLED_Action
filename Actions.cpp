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

void ActionsContainer::removeActionByIdx(size_t idx)
{
  if (m_actions.length() > idx)
    removeAction(m_actions[idx]);
}

ActionBase* ActionsContainer::actionsCurrent()
{
  if (m_actions.length())
    return m_actions[m_currentIdx];
  return nullptr;
}

void ActionsContainer::nextAction()
{
  //m_actions[m_currentIdx]->reset();
  ++m_currentIdx;
  if (m_actions.length() == m_currentIdx)
    m_currentIdx = 0;

  Serial.print("next action:");Serial.println(m_currentIdx);
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
  m_duration(duration), m_loopCB(nullptr)
{
}

// how many ms between each re-render
const uint8_t ActionBase::m_updateTime = 70;

ActionBase::~ActionBase()
{
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
  m_endTime = 0;
}

void ActionBase::loopDelegate(SegmentCommon *owner)
{
  if(m_loopCB)
    (*m_loopCB)(this, owner);
  else
    loop(owner);
}

void ActionBase::loop(SegmentCommon *owner)
{
  if (m_endTime == 0)
    m_endTime = millis() + m_duration;

  if (isFinished()) {
    Serial.print("loop:");Serial.print(m_duration);Serial.print(" ");
    Serial.print(m_endTime);Serial.print(" ");Serial.println(millis());
    reset();
    if (m_singleShot) {
      Serial.println("remove");
      owner->removeAction(this); // caution, deletes this,
                                  // no code execution after this line
    } else
      owner->nextAction();
  }
}


// --------------------------------------------------

ActionColor::ActionColor(CRGB color, uint32_t duration) :
    ActionBase(duration),
    m_color(color)
{
  m_loopCB = &ActionColor::loopCB;
}

ActionColor::~ActionColor()
{
}

// static
void ActionColor::loopCB(ActionBase *self, SegmentCommon *owner)
{
  reinterpret_cast<ActionColor*>(self)->loop(owner);
}

void ActionColor::loop(SegmentCommon *owner)
{
  if (m_endTime == 0) {
    //Serial.print("start color loop:");Serial.println(cRgbToUInt(m_color), 16);
    //Serial.print("this:");Serial.println((int)this);
    for(uint16_t i = 0, end = owner->size(); i < end; ++i) {
      CRGB *rgb = (*owner)[i];
      *rgb = m_color;
    }
    owner->dirty();
  }

  ActionBase::loop(owner);
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
ActionIncColor::ActionIncColor(CRGB leftColor, CRGB rightColor, uint32_t duration) :
    ActionBase(duration),
    m_leftColor(leftColor), m_rightColor(rightColor)
{
}

ActionIncColor::~ActionIncColor()
{
}

// static
void ActionIncColor::loopCB(ActionBase *self, SegmentCommon *owner)
{
  reinterpret_cast<ActionIncColor*>(self)->loop(owner);
}

void ActionIncColor::loop(SegmentCommon *owner)
{
  if (m_endTime == 0) {
    int16_t diff[3] = {
        m_leftColor.red - m_rightColor.red,
        m_leftColor.green - m_rightColor.green,
        m_leftColor.blue  - m_rightColor.blue
    };

    for (uint8_t c = 0; c < 3; ++c) {
      // iterate for each color
      for(uint16_t i = 0, sz = owner->size(); i < sz; ++i) {
        float color = diff[c] / sz;
        color = m_leftColor.raw[c] + (color * i);
        CRGB *rgb = (*owner)[i];
        rgb->raw[c] = round(color);
      }
    }
    owner->dirty();
  }

  ActionBase::loop(owner);
}


// -----------------------------------------------

ActionDimAll::ActionDimAll(CRGB fromColor, CRGB toColor, uint32_t duration) :
    ActionBase(duration),
    m_fromColor(fromColor), m_toColor(toColor),
    m_nextIterTime(m_updateTime)
{
  // calculate how far away each color is and how much they should change each 70ms
  int16_t redDiff   = toColor.red - fromColor.red,
          greenDiff = toColor.green - fromColor.green,
          blueDiff  = toColor.blue  - fromColor.blue;
  int16_t maxDiff = max(max(redDiff, greenDiff), blueDiff),
          minDiff = min(min(redDiff, greenDiff), blueDiff);
  if ((-maxDiff) > minDiff)
    // darker color wins
    m_iterations = -minDiff / (duration || 1); // negate minDiff as we don't want
                                               // negative m_iterations
  else
    m_iterations = maxDiff / (duration || 1);

  m_incR = (redDiff / m_iterations) || 1;
  m_incG = (greenDiff / m_iterations) || 1;
  m_incB = (blueDiff / m_iterations) || 1;
}

ActionDimAll::~ActionDimAll()
{
}

// static
void ActionDimAll::loopCB(ActionBase *self, SegmentCommon *owner)
{
  reinterpret_cast<ActionDimAll*>(self)->loop(owner);
}

void ActionDimAll::loop(SegmentCommon *owner)
{
  uint8_t r, g, b;
  if (m_endTime == 0) {
    r = m_fromColor.red;
    g = m_fromColor.green;
    b = m_fromColor.blue;
  } else {
    r = m_toColor.red - (m_iterations * m_incR);
    g = m_toColor.green - (m_iterations * m_incG);
    b = m_toColor.blue - (m_iterations * m_incB);
  }

  uint32_t startTime = m_endTime - m_duration;
  if (m_endTime == 0 || startTime + m_nextIterTime <= millis()) {
    m_nextIterTime = millis() + m_updateTime;
    for(uint16_t i = 0, end = owner->size(); i < end; ++i) {
      CRGB *rgb = (*owner)[i];
      rgb->red = r;
      rgb->green = g;
      rgb->blue = b;
    }
    owner->dirty();
  }

  // call subclass
  ActionBase::loop(owner);
}

void ActionDimAll::reset()
{
  m_nextIterTime = m_updateTime;
  ActionBase::reset();
}




