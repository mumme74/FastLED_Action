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

/*
SegmentPart::SegmentPart(CLEDController *controller,
                         uint8_t firstLed,
                         uint8_t noLeds) :
    m_firstIdx(firstLed), m_noLeds(noLeds),
    m_ledController(controller), m_hasChanges(false)
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
*/

// --------------------------------------------------------------------

FastLED_Action::FastLED_Action()
{
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
    action->loopDelegate(this);
  }
}

void SegmentCommon::render()
{
  // do upcast to correct type
  switch(m_type){
  case T_Segment: {
    Segment *seg = reinterpret_cast<Segment*>(this);
    seg->render();
  }  break;
  case T_Compound: {
    SegmentCompound *comp = reinterpret_cast<SegmentCompound*>(this);
    comp->render();
  }  break;
  default:
    break; // do nothing
  }
}

uint32_t SegmentCommon::yieldUntilNextAction()
{
  ActionBase *action = currentAction();
  if (m_halted || !action || action->duration() == 0)
    return 0;

  uint32_t time = millis();
  while(action->isRunning()) {
    yield();
    loop();
  }
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

void Segment::addLedController(CLEDController *part)
{
  m_ledControllers.push(part);
}

size_t Segment::ledControllerSize() const
{
  return m_ledControllers.length();
}

void Segment::removeLedController(size_t idx)
{
  m_ledControllers.remove(idx);
}

CLEDController* Segment::ledControllerAt(size_t idx)
{
  return m_ledControllers[idx];
}

Segment::ControllerList &Segment::controllerList()
{
  return m_ledControllers;
}

CRGB *Segment::operator[] (uint16_t idx)
{
  uint16_t led = 0;
  for (CLEDController *part = m_ledControllers.first();
      m_ledControllers.canMove(); part = m_ledControllers.next())
  {
    if (led + part->size() > idx) {
      return &(*part)[idx - led]; // found it!
    }
    led += part->size();
  }
  return nullptr;
}

uint16_t Segment::size()
{
  uint16_t sz = 0;
  for (CLEDController *part = m_ledControllers.first();
      m_ledControllers.canMove(); part = m_ledControllers.next())
  {
    sz += part->size();
  }
  return sz;
}

void Segment::render()
{
  for(auto cont = m_ledControllers.begin();
      m_ledControllers.canMove();
      cont = m_ledControllers.next())
  {
    cont->showLeds();
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

void SegmentCompound::removeSegment(size_t idx)
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
  return sz;
}

void SegmentCompound::addCompound(SegmentCompound *compound)
{
  FastLED_Action::registerItem(compound); // loop is controlled by this
                                          // compound from here on
  m_compounds.push(compound);
}

size_t SegmentCompound::compoundSize() const
{
  return m_compounds.length();
}

void SegmentCompound::removeCompound(size_t idx)
{
  FastLED_Action::registerItem(m_compounds[idx]);// re-register for loop control
  m_compounds.remove(idx);
}

SegmentCompound* SegmentCompound::compoundAt(size_t idx)
{
  return m_compounds[idx];
}


void SegmentCompound::render()
{
   for (Segment *segment = m_segments.first();
       m_segments.canMove(); segment = m_segments.next())
   {
     segment->render();
   }

   for(SegmentCompound *comp = m_compounds.first();
       m_compounds.canMove(); comp = m_compounds.next())
   {
     comp->render();
   }
}

SegmentCompound::CompoundList &SegmentCompound::compoundList()
{
  return m_compounds;
}
