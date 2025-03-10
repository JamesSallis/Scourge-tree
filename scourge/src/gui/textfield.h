
/***************************************************************************
                textfield.h  -  Single-line text input widget
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

#ifndef TEXTFIELD_H
#define TEXTFIELD_H
#pragma once

#include "gui.h"
#include "widget.h"
#include "window.h"
#include "label.h"

/**
  *@author Gabor Torok
  */

/// A single-line text input widget.
class TextField : public  Widget {
private:
	int numChars;
	bool inside; // was the last event inside the button?
	char *text;
	int pos, maxPos;
	int eventType;

public:

	enum {
		EVENT_KEYPRESS = 0,
		EVENT_ACTION
	};

	TextField( int x, int y, int numChars );
	~TextField();
	virtual bool handleEvent( Window* parent, SDL_Event* event, int x, int y );
	virtual void drawWidget( Window* parent );
	inline char *getText() {
		text[maxPos] = '\0'; return text;
	}
	void setText( const char *p );
	inline void setFocus( bool b ) {
		Widget::setFocus( b ); inside = b;
	}
	inline void clearText() {
		pos = maxPos = 0;
	}
	inline int getEventType() {
		return eventType;
	}
	// don't play sound when the value changes
	virtual inline bool hasSound() {
		return false;
	}
};

#endif

