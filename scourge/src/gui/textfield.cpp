/***************************************************************************
              textfield.cpp  -  Single-line text input widget
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
#include "textfield.h"

/**
  *@author Gabor Torok
  */

#define AVG_CHAR_WIDTH 6
#define OFFSET 5

TextField::TextField( int x, int y, int numChars ):
		Widget( x, y, x + numChars * AVG_CHAR_WIDTH, 16 ) {
	this->inside = false;
	this->numChars = numChars;
	this->text = new char [numChars + 1];
	this->pos = 0;
	this->maxPos = 0;
	this->eventType = EVENT_KEYPRESS;
}

TextField::~TextField() {
	delete [] text;
}

bool TextField::handleEvent( Window* parent, SDL_Event* event, int x, int y ) {
	inside = hasFocus();
	eventType = EVENT_KEYPRESS;
	// handle it
	if ( inside ) {
		switch ( event->type ) {
		case SDL_KEYUP:
			if ( event->key.keysym.sym == SDLK_RETURN ) {
				eventType = EVENT_ACTION;
			} else {
				parent->getScourgeGui()->blockEvent();
			}
			return true;
		case SDL_KEYDOWN:
			if ( event->key.keysym.sym == SDLK_RETURN ) {
				eventType = EVENT_ACTION;
			} else {
				parent->getScourgeGui()->blockEvent();
				if ( ( event->key.keysym.sym == SDLK_BACKSPACE && pos > 0 ) ||
				        ( event->key.keysym.sym == SDLK_DELETE && pos < maxPos ) ) {
					for ( int i = ( event->key.keysym.sym == SDLK_BACKSPACE ? pos - 1 : pos ); i < maxPos - 1; i++ ) {
						text[i] = text[i + 1];
					}
					if ( event->key.keysym.sym == SDLK_BACKSPACE )
						pos--;
					maxPos--;
				} else if ( event->key.keysym.sym == SDLK_LEFT && pos > 0 ) {
					pos--;
				} else if ( event->key.keysym.sym == SDLK_RIGHT && pos < maxPos ) {
					pos++;
				} else if ( event->key.keysym.sym == SDLK_HOME ) {
					pos = 0;
				} else if ( event->key.keysym.sym == SDLK_END ) {
					pos = maxPos;
				} else if ( maxPos < numChars && event->key.keysym.sym > SDLK_ESCAPE && event->key.keysym.sym < SDLK_UP && event->key.keysym.sym != SDLK_DELETE ) {
					for ( int i = maxPos; i > pos; i-- ) {
						text[i] = text[i - 1];
					}
					if ( SDL_GetModState() & KMOD_SHIFT ) {
						if ( event->key.keysym.sym >= SDLK_a && event->key.keysym.sym <= SDLK_z ) {
							text[pos] = ( 'A' - 'a' ) + event->key.keysym.sym;
						} else {
							// FIXME: US-keyboard definitions of keys
							switch ( event->key.keysym.sym ) {
							case SDLK_LEFTBRACKET : text[pos] = '{'; break;
							case SDLK_RIGHTBRACKET : text[pos] = '}'; break;
							case SDLK_SEMICOLON : text[pos] = ':'; break;
							case SDLK_QUOTE : text[pos] = '"'; break;
							case SDLK_COMMA : text[pos] = '<'; break;
							case SDLK_PERIOD : text[pos] = '>'; break;
							case SDLK_SLASH : text[pos] = '?'; break;
							case SDLK_BACKSLASH : text[pos] = '|'; break;
							case SDLK_BACKQUOTE : text[pos] = '~'; break;
							case SDLK_1 : text[pos] = '!'; break;
							case SDLK_2 : text[pos] = '@'; break;
							case SDLK_3 : text[pos] = '#'; break;
							case SDLK_4 : text[pos] = '$'; break;
							case SDLK_5 : text[pos] = '%'; break;
							case SDLK_6 : text[pos] = '^'; break;
							case SDLK_7 : text[pos] = '&'; break;
							case SDLK_8 : text[pos] = '*'; break;
							case SDLK_9 : text[pos] = '('; break;
							case SDLK_0 : text[pos] = ')'; break;
							case SDLK_MINUS : text[pos] = '_'; break;
							case SDLK_EQUALS : text[pos] = '+'; break;
							default: text[pos] = event->key.keysym.sym;
							}
						}
					} else {
						text[pos] = event->key.keysym.sym;
					}
					pos++;
					maxPos++;
				} else {
					return true;
				}
			}
		default:
			break;
		}
	}
	return false;
}

void TextField::drawWidget( Window* parent ) {
	GuiTheme *theme = parent->getTheme();

	//glColor3f( 1.0f, 1.0f, 1.0f );
	if ( theme->getInputBackground() ) {
		glColor4f( theme->getInputBackground()->color.r,
		           theme->getInputBackground()->color.g,
		           theme->getInputBackground()->color.b,
		           theme->getInputBackground()->color.a );
	} else {
		applyHighlightedBorderColor();
	}
	glPushMatrix();
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2i( 0, 0 );
	glVertex2i( getWidth(), 0 );
	glVertex2i( 0, getHeight() );
	glVertex2i( getWidth(), getHeight() );
	glEnd();
	glPopMatrix();

	if ( theme->getInputText() ) {
		glColor4f( theme->getInputText()->r,
		           theme->getInputText()->g,
		           theme->getInputText()->b,
		           theme->getInputText()->a );
	} else {
		applyColor();
	}
//  parent->getSDLHandler()->texPrintMono(OFFSET, 12, getText());

	char letter[2];
	for ( int i = 0; i < maxPos; i++ ) {
		glPushMatrix();
		glTranslatef( OFFSET + i * AVG_CHAR_WIDTH, 0.0f, 0.0f );
		letter[0] = text[i];
		letter[1] = '\0';
		parent->getScourgeGui()->setFontType( Constants::SCOURGE_MONO_FONT );
		parent->getScourgeGui()->texPrint( 0, 12, letter );
		parent->getScourgeGui()->setFontType( Constants::SCOURGE_DEFAULT_FONT );
		glPopMatrix();
	}

	// border
	if ( theme->getButtonBorder() ) {
		glColor4f( theme->getButtonBorder()->color.r,
		           theme->getButtonBorder()->color.g,
		           theme->getButtonBorder()->color.b,
		           theme->getButtonBorder()->color.a );
	} else {
		applyBorderColor();
	}
	glPushMatrix();
	glBegin( GL_LINE_LOOP );
	glVertex2i( 0, 0 );
	glVertex2i( 0, getHeight() );
	glVertex2i( getWidth(), getHeight() );
	glVertex2i( getWidth(), 0 );
	glEnd();
	glPopMatrix();
	if ( inside ) {
		glLineWidth( 3.0f );
		// cursor
		glLineWidth( 2.0f );
		glPushMatrix();
		glTranslatef( OFFSET + pos * AVG_CHAR_WIDTH + 1, 0.0f, 0.0f );
		glBegin( GL_LINES );
		glVertex2i( 0, 0 );
		glVertex2i( 0, getHeight() );
		glEnd();
		glPopMatrix();
		glLineWidth( 1.0f );
	}
}

void TextField::setText( const char *s ) {
	strncpy( text, ( s ? s : "" ), numChars );
	text[numChars - 1] = '\0';
	maxPos = strlen( text );
}

