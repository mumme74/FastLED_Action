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
*  Actions.h
*
*  Created on: 16 apr 2019
*      Author: Fredrik Johansson
*/

#ifndef ACTIONS_H_
#define ACTIONS_H_
#include <DList.h>
#include <stdint.h>
#include <FastLED.h>

class SegmentCommon;
class ActionBase;


/**
 * @brief: base class for Segment and SegmentCompound
 *         implements actions logic
 */
class ActionsContainer {
protected:
  DListDynamic<ActionBase*> m_actions;
  uint16_t m_currentIdx;
public:
  explicit ActionsContainer();
  virtual ~ActionsContainer();

  // Actions
  size_t actionsSize() const;
  void actionsPush(ActionBase *action);
  void actionsRemove(ActionBase *action);
  void actionsRestart();
  ActionBase* actionsCurrent();
  void nextAction();
};

// ----------------------------------------------------------

/**
 * @brief: Base class for all actions
 */
class ActionBase {
protected:
  bool m_singleShot;
  unsigned long m_startTime;
  uint32_t m_duration;
  static const uint8_t m_updateTime;

public:
  /// action takes this long in milliseconds
  /// duration of 0 is a forever action
  explicit ActionBase(uint32_t duration = 1000);
  virtual ~ActionBase();

  /// action takes this long in milliseconds
  /// duration of 0 is a forever action
  uint32_t duration() const { return m_duration; }

  bool isSingleShot() const { return m_singleShot; }
  void setSingleShot(bool singleShot) { m_singleShot = singleShot; }

  virtual bool isRunning() const;
  virtual bool isFinished() const;
  virtual void reset();

  virtual void loop(SegmentCommon *owner); // subclass must implement functionality
};

// ----------------------------------------------------

/// set all leds to color
class ActionColor : public ActionBase {
  CRGB m_color;
public:
  explicit ActionColor(CRGB color, uint32_t duration = 1000);
  virtual ~ActionColor();

  virtual void loop(SegmentCommon *owner);
};

// ----------------------------------------------------

/// turn all leds of
class ActionDark : public ActionColor {
public:
  explicit ActionDark(uint32_t duration = 1000);
  ~ActionDark();
};

// ----------------------------------------------------

/// sets all leds to a increasing color from left to right
class ActionIncColor : public ActionBase {
  CRGB m_leftColor, m_rightColor;
public:
  explicit ActionIncColor(CRGB leftColor, CRGB rightColor, uint32_t duration = 1000);
  virtual ~ActionIncColor();

  virtual void loop(SegmentCommon *owner);
};

// ----------------------------------------------------

/// dims all leds simultaneously from -> to during duration time
class ActionDimAll : public ActionBase {
  CRGB m_fromColor, m_toColor;
  uint16_t m_nextIterTime, // m_startTime + m_nextIterTime is when we update
           m_iterations;
  int16_t m_incR, m_incG, m_incB;
public:
  explicit ActionDimAll(CRGB fromColor, CRGB toColor, uint32_t duration = 1000);
  virtual ~ActionDimAll();

  virtual void loop(SegmentCommon *owner);
  virtual void reset();
};

// -----------------------------------------------------



#endif /* ACTIONS_H_ */
