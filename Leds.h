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
*  Leds.h
*
*  Created on: 15 apr 2019
*      Author: Fredrik Johansson
*/

#include <stdint.h>
#include <FastLED.h>
#include <DList.h>
#include "Actions.h"

#ifndef LEDS_H_
#define LEDS_H_

/**
 * @breif: a segment of leds within a single LED data out
 */
/*
class SegmentPart {
  uint8_t m_firstIdx,
           m_noLeds;
  CLEDController *m_ledController;
  bool m_hasChanges;

public:
  SegmentPart(CLEDController *controller, uint8_t firstLed, uint8_t noLeds);
  ~SegmentPart();

  void setLedController(CLEDController *controller);

  uint8_t firstLedIdx() const { return m_firstIdx; }
  uint8_t lastLedIdx() const { return m_firstIdx + m_noLeds; }
  uint16_t size() const { return m_noLeds; }
  CRGB *operator [] (uint8_t idx) const;

  bool hasChanges() const { return m_hasChanges; }
  void setHasChanges(bool hasChanges) { m_hasChanges = hasChanges; }
};
*/

//--------------------------------------------------------------

/**
 * @breif: Abstract base for all segments
 */
class SegmentCommon : public ActionsContainer {
protected:
  bool m_halted;
public:
  explicit SegmentCommon();
  virtual ~SegmentCommon();

  // halted status
  bool halted() const;
  void setHalted(bool halt);

  // tick, must be called from loop in root *.ino file
  void loop();

  // LEDs
  virtual CRGB* operator [] (uint16_t idx) = 0;
  virtual uint16_t size() = 0;

  void render();
};

// ---------------------------------------------------------

/**
 * @brief: a segment is built by 1 or more SegmentParts
 * combines 2 separate led strips with separate data out
 */
class Segment : public SegmentCommon {
public:
  typedef DListDynamic<CLEDController*> ControllerList;
  Segment();
  ~Segment();

  // add a subsegment to this segment, built up of several SegmentParts
  // a segment might be contained within several different data I/O pins
  // hence might have many controllers
  void addLedController(CLEDController *part);
  ControllerList &controllerList();

  // LEDs
  CRGB *operator [] (uint16_t idx);
  uint16_t size();
private:
  ControllerList m_ledControllers;
};

// -----------------------------------------------------------

/**
 * brief: make several segments work combined
 * ie like different segments of a logo
 */
class SegmentCompound : public SegmentCommon {
  DListDynamic<Segment*> m_segments;
public:
  SegmentCompound();
  ~SegmentCompound();

  /// add segment to to this compound
  void addSegment(Segment *segment);
  size_t segmentSize() const;
  void removeSegment(size_t idx);
  Segment* segmentAt(size_t idx);

  // LEDs
  CRGB *operator [] (uint16_t idx);
  uint16_t size();
};

#endif /* LEDS_H_ */
