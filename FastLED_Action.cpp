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
*  Leds.cpp
*
*  Created on: 15 apr 2019
*      Author: Fredrik Johansson
*/

#include "FastLED_Action.h"


FastLED_Action::FastLED_Action()
{
  for(int i = 0; i < MAX_CHANNEL_COUNT; ++i)
    m_dirtyControllers[i] = nullptr;
}

FastLED_Action::~FastLED_Action()
{
}

// static
FastLED_Action FastLED_Action::s_instance;

void FastLED_Action::_registerItem(SegmentCommon *item)
{
  m_items.push(item);
}

void FastLED_Action::_unregisterItem(SegmentCommon *item)
{
  size_t idx = 0;
  for(SegmentCommon *itm = m_items.first();
      m_items.canMove(); itm = m_items.next())
  {
    if (itm == item)
      m_items.remove(idx);
    ++idx;
  }
}

// static
void FastLED_Action::registerItem(SegmentCommon *item)
{
  FastLED_Action::s_instance._registerItem(item);
}

// static
void FastLED_Action::unregisterItem(SegmentCommon *item)
{
  FastLED_Action::s_instance._unregisterItem(item);
}

// static
FastLED_Action &FastLED_Action::instance()
{
  return FastLED_Action::s_instance;
}

// static
void FastLED_Action::loop()
{
  DListDynamic<SegmentCommon*> &items = FastLED_Action::s_instance.m_items;
  for(auto itm = items.first(); items.canMove(); itm = items.next())
  {
    itm->loop();
  }
  s_instance._render();
}

// static
void FastLED_Action::runProgram(int repeatCount)
{
  // this member function makes it possible to run asyncronously
  static bool running = false; // row only run first invocation
  if (!running) {
    running  = true;
    do {
      s_instance.program();
      s_instance.clearAllActions(); // safety cleanup
      if (repeatCount < -1)
        ++repeatCount;
    } while(repeatCount--);
    running = false;
  }
}

// static
void FastLED_Action::clearAllActions()
{
  s_instance._clearActions(nullptr);
}

void FastLED_Action::_render()
{
  // render changes
  for(int i = MAX_CHANNEL_COUNT -1; i >= 0; --i) {
    CLEDController *cont = m_dirtyControllers[i];
    if (cont != nullptr) {
      cont->showLeds();
      m_dirtyControllers[i] = nullptr;
    }
  }
}

void FastLED_Action::_clearActions(SegmentCommon *item)
{
  if (!item) {
    for(auto itm = m_items.first(); m_items.canMove(); itm = m_items.next())
      _clearActions(itm);
    return;
  }

  if (item->type() == Segment::T_Compound) {
    SegmentCompound *comp = reinterpret_cast<SegmentCompound*>(item);
    for(uint16_t i = 0, sz = comp->compoundSize(); i < sz; ++i)
      _clearActions(comp->compoundAt(i));
  }

  while(item->actionsSize() > 0)
    item->removeActionByIdx(0);
}

void FastLED_Action::setLedControllerHasChanges(CLEDController *controller)
{
  for(int i = 0; i < MAX_CHANNEL_COUNT; ++i) {
    CLEDController *cont = m_dirtyControllers[i];
    if (cont == controller)
      return; // already dirty
    if (cont == nullptr) {
      m_dirtyControllers[i] = controller;
      return;
    }
  }
}

bool FastLED_Action::ledControllerHasChanges(CLEDController *controller)
{
  for(int i = 0; i < MAX_CHANNEL_COUNT; ++i) {
    CLEDController *cont = m_dirtyControllers[i];
    if (cont == controller)
      return true; // already dirty
  }
  return false;
}

// --------------------------------------------------------------------

SegmentPart::SegmentPart(CLEDController *controller,
                         uint8_t firstLed,
                         uint8_t nLeds) :
    m_firstIdx(firstLed), m_noLeds(nLeds),
    m_ledController(controller)
{
}

SegmentPart::~SegmentPart()
{
}

void SegmentPart::setLedController(CLEDController *controller)
{
  m_ledController = controller;
}

CRGB *SegmentPart::operator [] (uint8_t idx) const
{
  uint16_t i = m_firstIdx + idx;
  if (m_ledController->size() <= (int)i)
    return nullptr;
  return &m_ledController->leds()[i];
}

void SegmentPart::dirty()
{
  FastLED_Action::instance().setLedControllerHasChanges(m_ledController);
}

// --------------------------------------------------------------------

SegmentCommon::SegmentCommon(typeEnum type) :
    ActionsContainer(),
    m_type(type), m_halted(false)
{
  FastLED_Action::registerItem(this);
}

SegmentCommon::~SegmentCommon()
{
  FastLED_Action::unregisterItem(this);
}

bool SegmentCommon::halted() const
{
  return m_halted;
}

void SegmentCommon::setHalted(bool halt)
{
  m_halted = halt;
}

void SegmentCommon::loop()
{
  if (!m_halted && m_actions.length()) {
    ActionBase *action = m_actions[m_currentIdx];
    action->loop(this);
  }
}

void SegmentCommon::dirty()
{
  // do upcast to correct type
  switch(m_type){
  case T_Segment: {
    Segment *seg = reinterpret_cast<Segment*>(this);
    seg->dirty();
  }  break;
  case T_Compound: {
    SegmentCompound *comp = reinterpret_cast<SegmentCompound*>(this);
    comp->dirty();
  }  break;
  default:
    break; // do nothing
  }
}

uint32_t SegmentCommon::yieldUntilAction(uint16_t noOfActions)
{
  ActionBase *action = currentAction();
  if (m_halted || !action || noOfActions == 0)
  {
    return 0;
  }

  uint32_t time = millis();
  do {
    action = currentAction();
    if (action && !action->isRunning())
      FastLED_Action::loop();
    while(action && action->isRunning()) {
      yield();
      loop();
    }
  } while(--noOfActions > 0);
  return millis() - time;
}

uint32_t SegmentCommon::yieldUntilAction(ActionBase &action)
{
  ActionBase *curAction = currentAction();
  if (m_halted || !curAction)
  {
    return 0;
  }

  uint32_t time = millis();
  do {
    curAction = currentAction();
    if (!curAction->isRunning())
      FastLED_Action::loop();
    while(curAction->isRunning()) {
      yield();
      loop();
    }
  } while(curAction != &action);
  return millis() - time;
}

// --------------------------------------------------------------------

Segment::Segment() :
    SegmentCommon(T_Segment)
{
}

Segment::~Segment()
{
}

void Segment::addSegmentPart(SegmentPart *part)
{
  m_segmentParts.push(part);
}

size_t Segment::segmentPartSize() const
{
  return m_segmentParts.length();
}

void Segment::removeSegmentPart(size_t idx)
{
  m_segmentParts.remove(idx);
}

SegmentPart* Segment::segmentPartAt(size_t idx)
{
  return m_segmentParts[idx];
}

Segment::PartsList &Segment::segmentPartsList()
{
  return m_segmentParts;
}

CRGB *Segment::operator[] (uint16_t idx)
{
  uint16_t led = 0;
  for (SegmentPart *part = m_segmentParts.first();
      m_segmentParts.canMove(); part = m_segmentParts.next())
  {
    if (led + part->size() > idx) {
      return (*part)[idx - led]; // found it!
    }
    led += part->size();
  }
  return nullptr;
}

uint16_t Segment::size()
{
  uint16_t sz = 0;
  for (SegmentPart *part = m_segmentParts.first();
      m_segmentParts.canMove(); part = m_segmentParts.next())
  {
    sz += part->size();
  }
  return sz;
}

void Segment::dirty()
{
  for(auto part = m_segmentParts.begin();
      m_segmentParts.canMove();
      part = m_segmentParts.next())
  {
    part->dirty();
  }
}

// -----------------------------------------------------------

SegmentCompound::SegmentCompound() :
    SegmentCommon(T_Compound)
{
}

SegmentCompound::~SegmentCompound()
{
}

void SegmentCompound::addSegment(Segment *segment)
{
  FastLED_Action::unregisterItem(segment); // unregister loop control,
                                           // controlled by this Compound
  m_segments.push(segment);
}

size_t SegmentCompound::segmentSize() const
{
  return m_segments.length();
}

void SegmentCompound::removeSegment(Segment *segment)
{
  for(uint16_t i = 0; i < m_segments.length(); ++i) {
    if (m_segments[i] == segment)
      removeSegmentByIdx(i);
  }
}

void SegmentCompound::removeSegmentByIdx(size_t idx)
{
  FastLED_Action::registerItem(m_segments[idx]); // re-register for loop control
  m_segments.remove(idx);
}

Segment* SegmentCompound::segmentAt(size_t idx)
{
  return m_segments[idx];
}

SegmentCompound::SegmentList &SegmentCompound::segmentsList()
{
  return m_segments;
}

CRGB* SegmentCompound::operator [](uint16_t idx)
{
  uint16_t led = 0;
  for (Segment *segment = m_segments.first();
      m_segments.canMove(); segment = m_segments.next())
  {
    if (led + segment->size() > idx) {
      return (*segment)[idx - led]; // found it!
    }
    led += segment->size();
  }
  // look in sub-compounds
  for (SegmentCompound *compound = m_compounds.first();
      m_compounds.canMove(); compound = m_compounds.next())
  {
    if (led + compound->size() > idx) {
      return (*compound)[idx - led]; // found it!
    }
    led += compound->size();
  }
  return nullptr;
}

uint16_t SegmentCompound::size()
{
  uint16_t sz = 0;
  for (Segment *segment = m_segments.first();
      m_segments.canMove(); segment = m_segments.next())
  {
    sz += segment->size();
  }

  for (SegmentCompound *compound = m_compounds.first();
      m_compounds.canMove(); compound = m_compounds.next())
  {
    sz += compound->size();
  }
  return sz;
}

void SegmentCompound::addCompound(SegmentCompound *compound)
{
  FastLED_Action::unregisterItem(compound); // loop is controlled by this
                                          // compound from here on
  m_compounds.push(compound);
}

size_t SegmentCompound::compoundSize() const
{
  return m_compounds.length();
}

void SegmentCompound::removeCompound(SegmentCompound *compound)
{
  for(uint16_t i = 0; i < m_compounds.length(); ++i) {
    if (m_compounds[i] == compound)
      removeCompoundByIdx(i);
  }
}

void SegmentCompound::removeCompoundByIdx(size_t idx)
{
  FastLED_Action::registerItem(m_compounds[idx]);// re-register for loop control
  m_compounds.remove(idx);
}

SegmentCompound* SegmentCompound::compoundAt(size_t idx)
{
  return m_compounds[idx];
}


void SegmentCompound::dirty()
{
   for (Segment *segment = m_segments.first();
       m_segments.canMove(); segment = m_segments.next())
   {
     segment->dirty();
   }

   for(SegmentCompound *comp = m_compounds.first();
       m_compounds.canMove(); comp = m_compounds.next())
   {
     comp->dirty();
   }
}

SegmentCompound::CompoundList &SegmentCompound::compoundList()
{
  return m_compounds;
}
