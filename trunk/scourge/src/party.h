/***************************************************************************
                          party.h  -  description
                             -------------------
    begin                : Sat May 3 2003
    copyright            : (C) 2003 by Gabor Torok
    email                : cctorok@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PARTY_H
#define PARTY_H

#include <iostream>
#include <string>
#include "constants.h"
#include "scourge.h"
#include "map.h"
#include "calendar.h"
#include "rpg/character.h"
#include "rpg/monster.h"
#include "rpg/rpgitem.h"
#include "events/thirsthungerevent.h"
#include "gui/canvas.h"
#include "gui/widgetview.h"

using namespace std;

#define MAX_PARTY_SIZE 4

class Party : public WidgetView {
 private:
    
  Scourge *scourge;
  Creature *player;
  Creature *party[MAX_PARTY_SIZE];
  bool partyDead;
  bool player_only;
  int formation;
  Calendar * calendar;
  bool startRound;
  bool lastEffectOn;
  int oldX;
  char version[100], min_version[20];

  Window *mainWin;
  Button *inventoryButton;
  Button *optionsButton;
  Button *quitButton;
  Button *roundButton;
  Button *calendarButton;

  Button *diamondButton;
  Button *staggeredButton;
  Button *squareButton;
  Button *rowButton;
  Button *scoutButton;
  Button *crossButton;
  Button *player1Button;
  Button *player2Button;
  Button *player3Button;
  Button *player4Button;
  Button *groupButton;
  Button *minButton, *maxButton;
  CardContainer *cards;
  Canvas *minPartyInfo;
  Canvas *playerInfo[MAX_PARTY_SIZE];

  Button *layoutButton1, *layoutButton2, *layoutButton3, *layoutButton4;

  int partySize;

 public:
   // the hard coded party
   static Creature **pc;
   static int pcCount;

  Party(Scourge *scourge);
  virtual ~Party();

  void reset();
  void resetMultiplayer(Creature *c);

  void deleteParty();

  inline Calendar *getCalendar() { return calendar; } 

  inline Creature *getPlayer() { return player; }

  inline Window *getWindow() { return mainWin; }

  void drawView();

  bool handleEvent(Widget *widget, SDL_Event *event);

  void setPlayer(int n);
  inline void setPlayer(Creature *c) { player = c; }

  void setPartyMotion(int motion);
  
  void setFormation(int formation);

  inline int getFormation() { return formation; }

  inline Creature *getParty(int i) { return party[i]; } 

  // move the party
  void movePlayers();

  // returns false if the switch could not be made,
  // because the entire party is dead (the mission failed)
  bool switchToNextLivePartyMember();

  void togglePlayerOnly();

  void forceStopRound();
  void toggleRound(bool test);
  void toggleRound();
  inline bool isRealTimeMode() { return startRound; }

  void startPartyOnMission();

  void followTargets();

  void setTargetCreature(Creature *creature);
  inline bool isPartyDead() { return partyDead; }
  inline bool isPlayerOnly() { return player_only; }
  inline int getPartySize() { return partySize; }
  
  int getTotalLevel();

  /** 
	  Return the closest live player within the given radius or null if none can be found.
  */
  Creature *getClosestPlayer(int x, int y, int w, int h, int radius);

  void startEffect(int effect_type, int duration=Constants::DAMAGE_DURATION);

  void drawWidget(Widget *w);

  static Creature **createHardCodedParty(Scourge *scourge);

protected:
  void createUI();
  void resetPartyUI();
};

#endif
