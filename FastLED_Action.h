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

#ifndef FASTLED_ACTION_H_
#define FASTLED_ACTION_H_

#include <stdint.h>
#include <FastLED.h>
#include <DList.h>
#include "Actions.h"
#include <Arduino.h>


class SegmentCommon;
class Segment;
class SegmentCompound;

class FastLED_Action {
  DListDynamic<SegmentCommon*> m_items;
  static const uint8_t MAX_CHANNEL_COUNT = 10; // how many LED i/o port we can have
  CLEDController* m_dirtyControllers[MAX_CHANNEL_COUNT];
  static FastLED_Action s_instance;
  void _registerItem(SegmentCommon *item);
  void _unregisterItem(SegmentCommon *item);
  void _render();
  void _clearActions(SegmentCommon *item);
  void program(); // must implement in root *.ino file
public:
  FastLED_Action();
  ~FastLED_Action();

  /// register a new item that should be called each loop
  static void registerItem(SegmentCommon *item);
  /// unregister a item form beeing called on each loop
  static void unregisterItem(SegmentCommon *item);
  /// gets a ref to global singleton of this class
  static FastLED_Action &instance();
  /// should be called from loop() in root *.ino file
  static void loop();
  /// starts the program, repeatCount -1 repeats forever
  static void runProgram(int repeatCount = 0);

  static void clearAllActions();

  /// triggers a resend on each LED controller list
  void setLedControllerHasChanges(CLEDController *controller);
  bool ledControllerHasChanges(CLEDController *controller);
};

// ----------------------------------------------------------

/**
 * @breif: a segment of leds within a single LED data out
 */

class SegmentPart {
  uint8_t m_firstIdx,
           m_nLeds;
  CLEDController *m_ledController;

public:
  SegmentPart(CLEDController *controller, uint8_t firstLed, uint8_t nLeds);
  ~SegmentPart();

  void setLedController(CLEDController *controller);
  CLEDController *ledController() { return m_ledController; }

  uint8_t firstLedIdx() const { return m_firstIdx; }
  uint8_t lastLedIdx() const { return m_firstIdx + m_nLeds; }
  uint16_t size() const { return m_nLeds; }
  CRGB *operator [] (uint8_t idx) const;

  void dirty();
private:
  void _checkLedsWithinBounds();
};


//--------------------------------------------------------------

/**
 * @breif: Abstract base for all segments
 */
class SegmentCommon : public ActionsContainer {
public:
  enum typeEnum : uint8_t { T_InValid, T_Segment, T_Compound };
  explicit SegmentCommon(typeEnum type);
  virtual ~SegmentCommon();

  typeEnum type() const { return m_type; }

  // halted status
  bool halted() const;
  void setHalted(bool halt);

  // tick, must be called from loop in root *.ino file
  void loop();

  // LEDs
  virtual CRGB* operator [] (uint16_t idx) = 0;
  virtual uint16_t size() = 0;

  void dirty();


  /// waits for next action to occur
  /// if duration is 0 (forever action) or if we are halted
  /// it returns immediately
  /// noOfActions is how many actions we wait until we return
  /// returns time in ms that it has been of
  uint32_t yieldUntilAction(uint16_t noOfActions = 1);
  uint32_t yieldUntilAction(ActionBase &action);

protected:
  typeEnum m_type;
  bool m_halted;
};

// ---------------------------------------------------------

/**
 * @brief: a segment is built by 1 or more SegmentParts
 * combines 2 separate led strips with separate data out
 */
class Segment : public SegmentCommon {
public:
  typedef DListDynamic<SegmentPart*> PartsList;
  explicit Segment();
  ~Segment();

  // add a subsegment to this segment, built up of several SegmentParts
  // a segment might be contained within several different data I/O pins
  // hence might have many controllers
  void addSegmentPart(SegmentPart *part);
  void addSegmentPart(SegmentPart &part) { addSegmentPart(&part); }
  size_t segmentPartSize() const;
  void removeSegmentPart(size_t idx);
  SegmentPart* segmentPartAt(size_t idx);
  PartsList &segmentPartsList();

  // LEDs
  CRGB *operator [] (uint16_t idx);
  uint16_t size();

  void dirty();
private:
  PartsList m_segmentParts;
};

// -----------------------------------------------------------

/**
 * brief: make several segments work combined
 * ie like different segments of a logo
 */
class SegmentCompound : public SegmentCommon {
public:
  typedef DListDynamic<Segment*> SegmentList;
  typedef DListDynamic<SegmentCompound*> CompoundList;
  explicit SegmentCompound();
  ~SegmentCompound();

  /// add segment to to this compound
  void addSegment(Segment *segment);
  void addSegment(Segment &segment) { addSegment(&segment); }
  size_t segmentSize() const;
  void removeSegmentByIdx(size_t idx);
  void removeSegment(Segment *segment);
  void removeSegment(Segment &segment) { removeSegment(&segment); }
  Segment* segmentAt(size_t idx);
  SegmentList &segmentsList();

  /// add sub compound to this compound
  void addCompound(SegmentCompound *compound);
  void addCompound(SegmentCompound &compound) { addCompound(&compound); }
  size_t compoundSize() const;
  void removeCompoundByIdx(size_t idx);
  void removeCompound(SegmentCompound *compound);
  void removeCompound(SegmentCompound &compound) { removeCompound(&compound); }
  SegmentCompound* compoundAt(size_t idx);
  CompoundList &compoundList();

  // LEDs
  CRGB *operator [] (uint16_t idx);
  uint16_t size();

  void dirty();

private:
  SegmentList m_segments;
  CompoundList m_compounds;
};

#endif /* FASTLED_ACTION_H_ */
