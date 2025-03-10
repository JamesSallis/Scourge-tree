
/***************************************************************************
                   multiplayer.h  -  The multiplayer dialog
                             -------------------
    begin                : Tue Aug 12 2003
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

#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H
#pragma once

#include "scourge.h"
#include "gui/window.h"
#include "gui/label.h"
#include "gui/button.h"
#include "gui/textfield.h"
#include "gui/scrollinglist.h"
#include "party.h"

/**
  *@author Gabor Torok
  */

class Scourge;
class Party;
class Creature;

/// The multiplayer dialog that is reached from the main menu.
class MultiplayerDialog {
private:
	Scourge *scourge;
	Window *mainWin;
	Button *startServer;
	Button *joinServer;
	Button *okButton;
	TextField *serverName;
	TextField *serverPort;
	TextField *userName;
	ScrollingList *characterList;
	int value;
	// unused: char **charStr;
	Creature *pc[MAX_PARTY_SIZE];
	int pcCount;

public:

	static const int START_SERVER = 1;
	static const int JOIN_SERVER = 2;

	MultiplayerDialog( Scourge *scourge );
	~MultiplayerDialog();

	inline void show() {
		value = 0; mainWin->setVisible( true );
	}
	inline void hide() {
		mainWin->setVisible( false );
	}
	inline bool isVisible() {
		return mainWin->isVisible();
	}

	inline char *getServerName() {
		return serverName->getText();
	}
	inline char *getServerPort() {
		return serverPort->getText();
	}
	inline char *getUserName() {
		return userName->getText();
	}
	inline int getValue() {
		return value;
	}
	Creature *getCreature();

	bool handleEvent( SDL_Event *event );
	bool handleEvent( Widget *widget, SDL_Event *event );
};

#endif
