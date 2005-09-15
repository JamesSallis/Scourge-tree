/***************************************************************************
  traindialog.h  -  description
-------------------
    begin                : 9/9/2005
    copyright            : (C) 2005 by Gabor Torok
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

#ifndef TRAIN_DIALOG_H
#define TRAIN_DIALOG_H

#include "constants.h"
#include <map>

class Scourge;
class Creature;
class Window;
class Label;
class ScrollingLabel;
class Widget;
class Button;
class TextField;

using namespace std;

class TrainDialog {
private:
  Scourge *scourge;
  Creature *creature;
  Window *win;
  
  Label *creatureLabel, *pointsLabel;
  ScrollingLabel *result;
  Button *closeButton, *applyButton;

public:
  TrainDialog( Scourge *scourge );
  ~TrainDialog();
  void setCreature( Creature *creature );
  void updateUI();
  inline Window *getWindow() { return win; }
  void handleEvent( Widget *widget, SDL_Event *event );
  
protected:
  void train();
};

#endif

