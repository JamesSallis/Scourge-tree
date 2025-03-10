/***************************************************************************
                        inven.h  -  Backpack widget
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

#ifndef INVEN_H
#define INVEN_H
#pragma once

#include <iostream>
#include <vector>
#include "gui/window.h"
#include "gui/button.h"
#include "gui/draganddrop.h"
#include "gui/widgetview.h"
#include "containerview.h"

/**
  *@author Gabor Torok
  */

class Creature;
class Scourge;
class Storable;
class ConfirmDialog;
class Item;
class PcUi;
class Storable;

/// Backpack widget and manager.

class Inven {
private:
	Creature *creature;
	Texture backgroundTexture;
	int currentHole;
	PcUi *pcUi;
	ContainerView *view;
	int x, y, w, h;
	Item *lastItem;
	Storable *storable;

public:
	Inven( PcUi *pcUi, int x, int y, int w, int h );
	~Inven();

	inline Storable *getStorable() {
		return storable;
	}
	inline void clearStorable() {
		storable = NULL;
	}

	inline Widget *getWidget() {
		return view;
	}
	bool handleEvent( SDL_Event *event );
	bool handleEvent( Widget *widget, SDL_Event *event );
	void setCreature( Creature *creature );

	//receive by other means
	bool receive( Item *item, bool atCursor );
	// drag+drop 
	void receive( Widget *widget );

protected:
	void storeItem( Item *item );
};

#endif

