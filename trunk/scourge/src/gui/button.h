/***************************************************************************
                          button.h  -  description
                             -------------------
    begin                : Thu Aug 28 2003
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

#ifndef BUTTON_H
#define BUTTON_H

#include "../constants.h"
#include "../sdlhandler.h"
#include "widget.h"
#include "label.h"

/**
  *@author Gabor Torok
  */

class SDLHandler;

class Button : public Widget {
 private:
  int x2, y2;
  Label *label;
  bool inside; // was the last event inside the button?
  float alpha, alphaInc;
  GLint lastTick;

 public: 
  Button(int x1, int y1, int x2, int y2, char *label);
  ~Button();
  inline Label *getLabel() { return label; }
  void handleEvent(SDLHandler *sdlHandler, SDL_Event *event, int x, int y);
  bool canHandle(SDLHandler *sdlHandler, SDL_Event *event, int x, int y);

  void drawWidget(Window *parent);
};

#endif

