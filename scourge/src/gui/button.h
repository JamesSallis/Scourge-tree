/***************************************************************************
                         button.h  -  Button widget
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
#pragma once

#include "gui.h"
#include "widget.h"
#include "../render/texture.h"

/**
  *@author Gabor Torok
  */
class Texture;

/// A push button widget.
class Button : public Widget {
private:
	int x2, y2;
	char label[255];
	bool inside; // was the last event inside the button?
	GLint lastTick;
	int labelPos;
	bool toggle;
	bool selected;
	Texture highlight;
	bool glowing;
	bool inverse;
	int fontType;
	Texture texture;
	bool pressed;

public:

	enum {
		TOP = 0,
		CENTER,
		BOTTOM
	};

	Button( int x1, int y1, int x2, int y2, Texture const& highlight, char const* label = NULL, Texture const& texture = Texture::none() );
	~Button();

	inline void setInside( bool b ) {
		inside = b;
	}
	/**
	Set if this button is a toggle button.
	*/
	inline void setToggle( bool b ) {
		toggle = b;
	}
	inline bool isToggle() {
		return toggle;
	}

	inline void setTexture( Texture texture ) {
		this->texture = texture;
	}
	inline void setGlowing( bool b ) {
		glowing = b;
	}
	inline bool isGlowing() {
		return glowing;
	}
	/**
	For toggle buttons, this returns true if the button has been toggled.
	*/
	inline bool isSelected() {
		return selected;
	}
	inline void setSelected( bool b ) {
		selected = b;
	}
	inline void setLabelPosition( int p ) {
		labelPos = p;
	}
	inline int getLabelPosition() {
		return labelPos;
	}
	inline char *getLabel() {
		return label;
	}
	inline void setLabel( char const* s ) {
		strncpy( label, ( s ? s : "" ), 255 ); label[254] = '\0';
	}
	virtual bool handleEvent( Window* parent, SDL_Event* event, int x, int y );
	virtual void removeEffects();
	virtual void drawWidget( Window* parent );
	virtual inline void move( int x, int y ) {
		x2 += ( x - this->x );
		y2 += ( y - this->y );
		Widget::move( x, y );
	}

	inline int getFontType() {
		return fontType;
	}
	inline void setFontType( int n ) {
		fontType = n;
	}

};

#endif

