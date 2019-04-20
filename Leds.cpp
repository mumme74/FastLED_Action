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

#include "Leds.h"

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

SegmentCommon::SegmentCommon() :
    ActionsContainer(),
    m_halted(true)
{
}

SegmentCommon::~SegmentCommon()
{
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

// --------------------------------------------------------------------

Segment::Segment() :
    SegmentCommon()
{
}

Segment::~Segment()
{
}

void Segment::addLedController(CLEDController *part)
{
  m_ledControllers.push(part);
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

// -----------------------------------------------------------

SegmentCompound::SegmentCompound() :
    SegmentCommon()
{
}

SegmentCompound::~SegmentCompound()
{
}

void SegmentCompound::addSegment(Segment *segment)
{
  m_segments.push(segment);
}

size_t SegmentCompound::segmentSize() const
{
  return m_segments.length();
}

void SegmentCompound::removeSegment(size_t idx)
{
  m_segments.remove(idx);
}

Segment* SegmentCompound::segmentAt(size_t idx)
{
  return m_segments[idx];
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

