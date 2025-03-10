/***************************************************************************
                         label.cpp  -  Label widget
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
#include "../common/constants.h"
#include "label.h"
#include "window.h"

/**
  *@author Gabor Torok
  */

Label::Label( int x, int y, char const* text, int lineWidth, int fontType, int lineHeight ) : Widget( x, y, 0, 0 ) {
	this->lineWidth = lineWidth;
	this->fontType = fontType;
	this->lineHeight = lineHeight;
	this->specialColor = false;
	setText( text );
}

Label::~Label() {
}

void Label::drawWidget( Window* parent ) {
	if ( text ) {
		parent->getScourgeGui()->setFontType( fontType );
		GuiTheme *theme = parent->getTheme();
		if ( !specialColor && theme->getWindowText() ) {
			glColor4f( theme->getWindowText()->r, theme->getWindowText()->g, theme->getWindowText()->b, theme->getWindowText()->a );
		} else {
			applyColor();
		}
		if ( lines.empty() ) {
			// draw a single-line label
			parent->getScourgeGui()->texPrint( 0, 0, text );
		} else {
			int y = 0;
			for ( int i = 0; i < static_cast<int>( lines.size() ); i++ ) {
				parent->getScourgeGui()->texPrint( 0, y, lines[i].c_str() );
				y += lineHeight;
			}
		}
		parent->getScourgeGui()->setFontType( Constants::SCOURGE_DEFAULT_FONT );
	}
}

void Label::setText( char const* s ) {
	strncpy( text, ( s ? s : "" ), 3000 );
	text[2999] = '\0';
	lines.clear();
	if ( lineWidth > 1 && static_cast<int>( strlen( text ) ) >= lineWidth ) {
		breakText( text, lineWidth, &lines );
	}
}
