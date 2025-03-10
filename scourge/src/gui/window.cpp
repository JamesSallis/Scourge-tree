/***************************************************************************
                        window.cpp  -  Window manager
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
#include "window.h"
#include "../render/texture.h"

using namespace std;

// ###### MS Visual C++ specific ###### 
#if defined(_MSC_VER) && defined(_DEBUG)
# define new DEBUG_NEW
# undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif 

//#define DEBUG_WINDOWS  
  
#define OPEN_STEPS 10

// should come from theme
#define INSET 8

// #define DEBUG_SCISSOR 1

const char Window::ROLL_OVER_SOUND[80] = "sound/ui/roll.wav";
const char Window::ACTION_SOUND[80] = "sound/ui/press.wav";
const char Window::DROP_SUCCESS[80] = "sound/equip/equip1.wav";
const char Window::DROP_FAILED[80] = "sound/equip/cant_equip.wav";

//#define DEBUG_WINDOWS

/**
  *@author Gabor Torok
  */

#define CLOSE_BUTTON_SIZE 10


Window::Window( ScourgeGui *scourgeGui, int x, int y, int w, int h, char const* title, bool hasCloseButton, int type, const char *themeName ) : Widget( x, y, w, h ) {
	theme = GuiTheme::getThemeByName( themeName );
	commonInit( scourgeGui, x, y, w, h, title, hasCloseButton, type );
}

Window::Window( ScourgeGui *scourgeGui, int x, int y, int w, int h, char const* title, Texture const& texture, bool hasCloseButton, int type, Texture const& texture2 ) : Widget( x, y, w, h ) {
	theme = GuiTheme::getThemeByName( GuiTheme::DEFAULT_THEME );
	/*
	this->texture = texture;
	this->texture2 = texture2;
	background.r = 1.0f;
	background.g = 0.85f;
	background.b = 0.5f;
	*/
	commonInit( scourgeGui, x, y, w, h, title, hasCloseButton, type );
}

void Window::commonInit( ScourgeGui *scourgeGui, int x, int y, int w, int h, char const* title, bool hasCloseButton, int type ) {
	this->escapeHandler = NULL;
	this->opening = false;
	this->animation = DEFAULT_ANIMATION;
	this->lastWidget = NULL;
	this->scourgeGui = scourgeGui;
	//  this->title = title;
	setTitle( title );
	this->visible = false;
	this->modal = false;
	this->widgetCount = 0;
	this->dragging = false;
	this->dragX = this->dragY = 0;
	this->gutter = 21 +
	               ( theme->getWindowBorderTexture() ?
	                 theme->getWindowBorderTexture()->width :
	                 0 );
	if ( hasCloseButton ) {
		if ( theme->getButtonHighlight() ) {
			this->closeButton = new Button( 0, 0, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE,
			                                theme->getButtonHighlight()->texture );
		} else {
			this->closeButton = new Button( 0, 0, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE, Texture::none() );
		}
	} else closeButton = NULL;
	openHeight = INSET * 2;
	this->type = type;
	this->locked = false;
	this->rawEventHandler = NULL;

	// FIXME: should come from gui.txt
	if ( type == SIMPLE_WINDOW ) {
		setBackgroundTileWidth( TILE_W );
		setBackgroundTileHeight( TILE_H );
	} else {
		setBackgroundTileWidth( 256 );
		setBackgroundTileHeight( 256 );
	}

	// make windows stay on screen
	this->move( x, y );
	currentY = y;
	scourgeGui->addWindow( this );
}

Window::~Window() {
	delete closeButton;
	// Delete all widgets, may cause problem if someday we use same widgets for
	// multiple windows. For now, no problem.
	for ( int i = 0; i < widgetCount ; i++ ) {
		scourgeGui->unregisterEventHandler( widget[i] );
		delete this->widget[i];
	}
	scourgeGui->removeWindow( this );
	
	/* This is now property of scourgeGui
	if (message_dialog != NULL) {
		Window *tmp = message_dialog; // to avoid recursion of ~Window
		message_dialog = NULL;
		delete tmp;
	}*/
}

// convenience method to register the same handler for all the window's widgets
void Window::registerEventHandler( EventHandler *eventHandler ) {
	for ( int i = 0; i < widgetCount ; i++ ) {
		scourgeGui->registerEventHandler( widget[i], eventHandler );
	}
	if( closeButton ) scourgeGui->registerEventHandler( closeButton, eventHandler );
}

void Window::unregisterEventHandler() {
	for ( int i = 0; i < widgetCount ; i++ ) {
		scourgeGui->unregisterEventHandler( widget[i] );
	}
	if( closeButton ) scourgeGui->unregisterEventHandler( closeButton );
}



Widget *Window::handleWindowEvent( SDL_Event *event, int x, int y ) {

	if ( dragging ) {
		handleEvent( NULL, event, x, y );
		return this;
	}

	// handle some special key strokes
	Widget *w = NULL;
	bool systemKeyPressed = false;
	if ( event->type == SDL_KEYUP ) {
		switch ( event->key.keysym.sym ) {
		case SDLK_ESCAPE:
			// select an open, non-locked window with a close button to close
			w = scourgeGui->selectCurrentEscapeHandler();
			systemKeyPressed = ( w == NULL );
			break;
		case SDLK_TAB:
			systemKeyPressed = true; break;
		default:
			break;
		}
	}
	
	if ( !systemKeyPressed ) {
		// handled by a component?
		bool insideWidget = false;
		if( !w ) {
			for ( int t = 0; t < widgetCount; t++ ) {
				if ( this->widget[t]->isVisible() && this->widget[t]->isEnabled() ) {
					if ( !insideWidget ) {
						insideWidget = this->widget[t]->isInside( x - getX(), y - getY() - gutter );
						if ( insideWidget
						   && ( event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEBUTTONDOWN ) 
						   && event->button.button == SDL_BUTTON_LEFT ) {
							scourgeGui->currentWin = this;
							setFocus( this->widget[t] );
						}
					}
					if ( this->widget[t]->handleEvent( this, event, x - getX(), y - getY() - gutter ) )
						w = this->widget[t];
				}
			}
		}
		
		// special handling
		// XXX: GUI: extract it from here
		if ( scourgeGui->message_button != NULL && w == scourgeGui->message_button && scourgeGui->message_dialog != NULL ) {
			scourgeGui->message_dialog->setVisible( false );
		}			
		
		if ( w ) {
			if ( w->hasSound() ) scourgeGui->playSound( Window::ACTION_SOUND, 127 );
			return w;
		}

		// handled by closebutton
		if ( closeButton ) {
			if ( !insideWidget ) {

				// w - 10 - ( closeButton->getWidth() ), topY + 8

				insideWidget =
				  closeButton->isInside( x - ( getX() + ( getWidth() - 10 - closeButton->getWidth() ) ),
				                         y - ( getY() + 8 ) );
			}
			if ( closeButton->handleEvent( this, event,
			                               x - ( getX() + ( getWidth() - 10 - closeButton->getWidth() ) ),
			                               y - ( getY() + 8 ) ) ) {
				scourgeGui->playSound( Window::ACTION_SOUND, 127 );
				return closeButton;
			}
		}

		if ( insideWidget &&
		        !( event->type == SDL_KEYUP ||
		           event->type == SDL_KEYDOWN ) ) {
			return this;
		}
	}
	
	// see if the window wants it
	if ( handleEvent( NULL, event, x, y ) ) {
		return this;
	}

	// swallow event if in a modal window
	return( isModal() ? this : NULL );
}

void Window::removeEffects() {
	for ( int t = 0; t < widgetCount; t++ ) {
		widget[t]->removeEffects();
	}
}

bool Window::isInside( int x, int y ) {
	return( dragging || Widget::isInside( x, y ) );
}

bool Window::handleEvent( Window*, SDL_Event* event, int x, int y ) {
	switch ( event->type ) {
	case SDL_KEYDOWN:
		return false;
	case SDL_KEYUP:
		if ( event->key.keysym.sym == SDLK_TAB ) {
			if ( SDL_GetModState() & KMOD_CTRL ) {
				if ( SDL_GetModState() & KMOD_SHIFT ) {
					scourgeGui->prevWindowToTop( this );
				} else {
					scourgeGui->nextWindowToTop( this );
				}
			} else if ( SDL_GetModState() & KMOD_SHIFT ) {
				prevFocus();
			} else {
				nextFocus();
			}
			return true;
		// XXX: GUI: extract it from here
		} else if ( event->key.keysym.sym == SDLK_ESCAPE && ( closeButton || scourgeGui->message_button ) && !isLocked() ) {
			scourgeGui->blockEvent();
			setVisible( false );
			// raise the next unlocked window
			scourgeGui->currentWin = NULL;
			scourgeGui->nextWindowToTop( NULL, false );
			return true;
		} else {
			return false;
		}
	case SDL_MOUSEMOTION:
		if ( dragging ) move( x - dragX, y - dragY );
		break;
	case SDL_MOUSEBUTTONUP:
		dragging = false;
		break;
	case SDL_MOUSEBUTTONDOWN:
		if ( event->button.button != SDL_BUTTON_LEFT ) return false;
		toTop();
		if ( !isLocked() ) {
			dragging = ( isInside( x, y ) );
			dragX = x - getX();
			dragY = y - getY();
		}
		break;
	}
	return isInside( x, y );
}

void Window::setFocus( Widget *w ) {
	bool focusSet = false;
	for ( int i = 0; i < widgetCount; i++ ) {
		bool b = ( w->canGetFocus() && widget[i] == w ) ||
		         ( !w->canGetFocus() && widget[i]->canGetFocus() && !focusSet );
		if ( !focusSet ) focusSet = b;
		widget[i]->setFocus( b );
	}
}

void Window::nextFocus() {
	bool setFocus = false;
	for ( int t = 0; t < 2; t++ ) {
		for ( int i = 0; i < widgetCount; i++ ) {
			if ( widget[i]->hasFocus() ) {
				widget[i]->setFocus( false );
				setFocus = true;
			} else if ( setFocus && widget[i]->canGetFocus() ) {
				widget[i]->setFocus( true );
				return;
			}
		}
	}
}

void Window::prevFocus() {
	bool setFocus = false;
	for ( int t = 0; t < 2; t++ ) {
		for ( int i = widgetCount - 1; i >= 0; i-- ) {
			if ( widget[i]->hasFocus() ) {
				widget[i]->setFocus( false );
				setFocus = true;
			} else if ( setFocus && widget[i]->canGetFocus() ) {
				widget[i]->setFocus( true );
				return;
			}
		}
	}
}

void Window::addWidget( Widget *widget ) {
	if ( widgetCount < MAX_WIDGET ) {
		this->widget[widgetCount++] = widget;

		// apply the window's color scheme
		if ( !theme ) {
			widget->setColor( getColor() );
			widget->setBackground( getBackgroundColor() );
			widget->setSelectionColor( getSelectionColor() );
			widget->setBorderColor( getBorderColor() );
		}
		setFocus( widget );
	} else {
		cerr << "Gui/Window.cpp : max widget limit reached!" << endl;
	}
}

void Window::removeWidget( Widget *widget ) {
	for ( int i = 0; i < widgetCount; i++ ) {
		if ( this->widget[i] == widget ) {
			for ( int t = i; t < widgetCount - 1; t++ ) {
				this->widget[t] = this->widget[t + 1];
			}
			widgetCount--;
			return;
		}
	}
}

void Window::drawWidget( Window* ) {
	GLint t = SDL_GetTicks();
	GLint topY;

	if ( animation == SLIDE_UP ) {

		if ( y > currentY ) {
			if ( t - lastTick > 10 ) {
				lastTick = t;
				y -= ( h / OPEN_STEPS );
				if ( y < currentY ) y = currentY;
			}
		}

		opening = ( y > currentY );
		topY = y - currentY;
		openHeight = h;

		scissorToWindow( false );

	} else {

		if ( openHeight < h ) {
			if ( t - lastTick > 10 ) {
				lastTick = t;
				openHeight += ( h / OPEN_STEPS ); // always open in the same number of steps
				if ( openHeight >= h ) openHeight = h;
			}
		}

		topY = ( h / 2 ) - ( openHeight / 2 );
		opening = ( openHeight < h );
	}

	glPushMatrix();

	glLoadIdentity( );

	if ( type != INVISIBLE_WINDOW ) {

		glsEnable( GLS_TEXTURE_2D );

		// tile the background
		if ( theme->getWindowTop() ) {
			glColor4f( theme->getWindowTop()->color.r, theme->getWindowTop()->color.g, theme->getWindowTop()->color.b, theme->getWindowTop()->color.a );
		} else {
			glColor3f( 1.0f, 0.6f, 0.3f );
		}

		glTranslatef( x, y, z );

		// HACK: blend window if top color's a < 1.0f
		if ( !isModal() ) {
			if ( theme->getWindowTop() && theme->getWindowTop()->color.a < 1.0f ) {
				glsEnable( GLS_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			}
		}

		drawBackground( topY, openHeight );

		glsDisable( GLS_TEXTURE_2D | GLS_BLEND );

		// draw drop-shadow
		if ( !isLocked() ) drawDropShadow( topY, openHeight );

		// top bar
		if ( title || ( closeButton && !isLocked() ) ) {
			glsEnable( GLS_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );

			glBegin( GL_TRIANGLE_STRIP );
			glVertex2i( 0, topY );
			glVertex2i( getWidth(), topY );
			glVertex2i( 0, topY + TITLE_HEIGHT );
			glVertex2i( getWidth(), topY + TITLE_HEIGHT );
			glEnd();

			glsDisable( GLS_BLEND );
		}

		if ( type == BASIC_WINDOW &&
		        theme->getWindowBorderTexture() ) {
			drawBorder( topY, openHeight );
		} else {
			drawLineBorder( topY, openHeight );
		}

		// print title
		if ( title ) drawTitle( topY, openHeight );

		// draw the close button
		if ( closeButton && !isLocked() ) drawCloseButton( topY, openHeight );

	}

	glsDisable( GLS_SCISSOR_TEST );

	// draw widgets
	bool tmp = isOpening();

	if ( tmp ) {
		scissorToWindow();
	}

	for ( int i = 0; i < widgetCount; i++ ) {
		if ( widget[i]->isVisible() ) {

			glPushMatrix();
			glLoadIdentity();

			// if this is modified, also change handleWindowEvent
			//glTranslatef(x, y + topY + TOP_HEIGHT, z + 5.0f);
			glTranslatef( x, y + topY + gutter, z + 5.0f );

			widget[i]->draw( this );

			glPopMatrix();
		}
	}

	if ( tmp ) {
		glsDisable( GLS_SCISSOR_TEST );
	}

	for ( int i = 0; i < widgetCount; i++ ) {
		if ( widget[i]->isVisible() ) {
			widget[i]->drawTooltip( this );
		}
	}

	glsEnable( GLS_TEXTURE_2D );

	glPopMatrix();
}

void Window::drawBackground( int topY, int openHeight ) {
	if ( theme->getWindowBackground() && !theme->getWindowBackground()->rep_h ) {
		theme->getWindowBackground()->texture.glBind();
		glBegin ( GL_TRIANGLE_STRIP );
		glTexCoord2f ( 0.0f, 0.0f );
		glVertex2i ( 0, topY );
		glTexCoord2f ( 1.0f, 0.0f );
		glVertex2i ( w, topY );
		glTexCoord2f ( 0.0f, 1.0f );
		glVertex2i ( 0, topY + openHeight );
		glTexCoord2f ( 1.0f, 1.0f );
		glVertex2i ( w, topY + openHeight );
		glEnd ();
	} else if ( type == SIMPLE_WINDOW ) {
		if ( theme->getWindowBackground() && theme->getWindowBackground()->texture.isSpecified() ) {
			theme->getWindowBackground()->texture.glBind();
		}
		glBegin ( GL_TRIANGLE_STRIP );
		glTexCoord2f ( 0.0f, 0.0f );
		glVertex2i ( 0, topY );
		glTexCoord2f ( 1.0f, 0.0f );
		glVertex2i ( w, topY );
		glTexCoord2f ( 0.0f, openHeight / static_cast<float>( tileHeight ) );
		glVertex2i ( 0, topY + openHeight );
		glTexCoord2f ( 1.0f, openHeight / static_cast<float>( tileHeight ) );
		glVertex2i ( w, topY + openHeight );
		glEnd ();
	} else if ( type == BASIC_WINDOW ) {
		if ( theme->getWindowBackground()
		        && theme->getWindowBackground()->texture.isSpecified() ) {
			theme->getWindowBackground()->texture.glBind();
		} else {
			glsDisable( GLS_TEXTURE_2D );
		}

		//applyBackgroundColor();
		if ( theme->getWindowBackground() ) {
			glColor4f( theme->getWindowBackground()->color.r, theme->getWindowBackground()->color.g,
			           theme->getWindowBackground()->color.b, theme->getWindowBackground()->color.a );
		}

		glBegin( GL_TRIANGLE_STRIP );
		glTexCoord2f( 0.0f, 0.0f );
		glVertex2i( 0, topY );
		glTexCoord2f( 1.0f, 0.0f );
		//glTexCoord2d( w/static_cast<float>(tileWidth), 0 );
		glVertex2i( w, topY );
		glTexCoord2f( 0.0f, ( openHeight ) / static_cast<float>( tileHeight ) );
		glVertex2i( 0, topY + openHeight );
		//glTexCoord2f( w/static_cast<float>(tileWidth), ( openHeight ) /static_cast<float>(tileHeight) );
		glTexCoord2f( 1.0f, ( openHeight ) / static_cast<float>( tileHeight ) );
		glVertex2i( w, topY + openHeight );
		glEnd();
	}
}

void Window::drawDropShadow( int topY, int openHeight ) {
	int n = 10;

	glsEnable( GLS_BLEND );
	glBlendFunc( GL_SRC_COLOR, GL_DST_COLOR );

	glColor4f( 0.15f, 0.15f, 0.15f, 0.25f );

	glBegin( GL_QUADS );
	glVertex2i ( n, topY + openHeight );
	glVertex2i ( n, topY + openHeight + n );
	glVertex2i ( w + n, topY + openHeight + n );
	glVertex2i ( w + n, topY + openHeight );

	glVertex2i ( w, topY + n );
	glVertex2i ( w, topY + openHeight );
	glVertex2i ( w + n, topY + openHeight );
	glVertex2i ( w + n, topY + n );
	glEnd();

	glsDisable( GLS_BLEND );
}

void Window::drawCloseButton( int topY, int openHeight ) {
	// apply the window's color scheme
	/*
	   closeButton->setColor( getColor() );
	   closeButton->setBackground( getBackgroundColor() );
	   closeButton->setSelectionColor( getSelectionColor() );
	   closeButton->setBorderColor( getBorderColor() );
	*/

	if ( theme->getButtonText() ) closeButton->setColor( theme->getButtonText() );
	if ( theme->getButtonBackground() ) closeButton->setBackground( &( theme->getButtonBackground()->color ) );
	if ( theme->getButtonHighlight() ) closeButton->setSelectionColor( &( theme->getButtonHighlight()->color ) );
	if ( theme->getButtonBorder() ) closeButton->setBorderColor( &( theme->getButtonBorder()->color ) );


	glPushMatrix();
	//glLoadIdentity();
	glTranslatef( w - 10.0f - ( closeButton->getWidth() ), topY + 8.0f, z + 5.0f );
	closeButton->draw( this );
	glPopMatrix();
}

void Window::drawTitle( int topY, int openHeight ) {
	glPushMatrix();
	glTranslatef( 0.0f, 0.0f, 5.0f );
	if ( theme->getWindowTitleText() ) {
		glColor4f( theme->getWindowTitleText()->r,
		           theme->getWindowTitleText()->g,
		           theme->getWindowTitleText()->b,
		           theme->getWindowTitleText()->a );
	} else {
		glColor3f( 1.0f, 1.0f, 1.0f );
	}
	scourgeGui->setFontType( Constants::SCOURGE_UI_FONT );
#ifdef DEBUG_WINDOWS
	scourgeGui->texPrint( 8, topY + TITLE_HEIGHT - 5, "%s (%d)", title, getZ() );
#else
	scourgeGui->texPrint( 8, topY + TITLE_HEIGHT - 5, "%s", title );
#endif
	scourgeGui->setFontType( Constants::SCOURGE_DEFAULT_FONT );
	glPopMatrix();
}

void Window::setTopWindowBorderColor() {
	if ( theme->getSelectedBorder() ) {
		glColor4f( theme->getSelectedBorder()->color.r,
		           theme->getSelectedBorder()->color.g,
		           theme->getSelectedBorder()->color.b,
		           theme->getSelectedBorder()->color.a );
	} else {
		applyHighlightedBorderColor();
	}
}

void Window::setWindowBorderColor() {
	//  } else if(isLocked()) {
	//    glColor3f(0.5f, 0.3f, 0.2f);
	//  } else
	if ( theme->getWindowBorder() ) {
		glColor4f( theme->getWindowBorder()->color.r,
		           theme->getWindowBorder()->color.g,
		           theme->getWindowBorder()->color.b,
		           theme->getWindowBorder()->color.a );
	} else {
		applyBorderColor();
	}
}

void Window::drawLineBorder( int topY, int openHeight ) {
	// add a border
	if ( scourgeGui->currentWin == this ) {
		setTopWindowBorderColor();
	} else {
		setWindowBorderColor();
	}

	if ( this == scourgeGui->currentWin || isLocked() || isModal() ) {
		glLineWidth( 3.0f );
	} else if ( theme->getWindowBorder() ) {
		glLineWidth( theme->getWindowBorder()->width );
	} else {
		glLineWidth( 2.0f );
	}
	glBegin( GL_LINES );
	glVertex2i( w, topY + openHeight );
	glVertex2i( 0, topY + openHeight );
	glVertex2i( 0, topY );
	glVertex2i( w, topY );
	glVertex2i( 0, topY );
	glVertex2i( 0, topY + openHeight );
	glVertex2i( w, topY );
	glVertex2i( w, topY + openHeight );
	glEnd();
	glLineWidth( 1.0f );
}

void Window::drawBorder( int topY, int openHeight ) {
	int n = 16; // FIXME: compute when loading textures

	glsEnable( GLS_TEXTURE_2D | GLS_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glColor4f( theme->getWindowBorderTexture()->color.r, theme->getWindowBorderTexture()->color.g, theme->getWindowBorderTexture()->color.b, theme->getWindowBorderTexture()->color.a );

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_nw.glBind();

	glTranslatef( theme->getWindowBorderTexture()->width, topY + theme->getWindowBorderTexture()->width, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( n, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, n );
	glTexCoord2i( 1, 1 );
	glVertex2i( n, n );
	glEnd();

	glPopMatrix();

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_ne.glBind();

	glTranslatef( getWidth() - n - theme->getWindowBorderTexture()->width, topY + theme->getWindowBorderTexture()->width, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( n, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, n );
	glTexCoord2i( 1, 1 );
	glVertex2i( n, n );
	glEnd();

	glPopMatrix();

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_se.glBind();

	glTranslatef( getWidth() - n - theme->getWindowBorderTexture()->width, topY + openHeight - n - theme->getWindowBorderTexture()->width, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( n, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, n );
	glTexCoord2i( 1, 1 );
	glVertex2i( n, n );
	glEnd();

	glPopMatrix();

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_sw.glBind();

	glTranslatef( theme->getWindowBorderTexture()->width, topY + openHeight - n - theme->getWindowBorderTexture()->width, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( n, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, n );
	glTexCoord2i( 1, 1 );
	glVertex2i( n, n );
	glEnd();
	glPopMatrix();

	int h = openHeight - 2 * ( n + theme->getWindowBorderTexture()->width );

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_west.glBind();

	glTranslatef( theme->getWindowBorderTexture()->width, topY + theme->getWindowBorderTexture()->width + n, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( n, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, h );
	glTexCoord2i( 1, 1 );
	glVertex2i( n, h );
	glEnd();

	glPopMatrix();

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_east.glBind();

	glTranslatef( getWidth() - n - theme->getWindowBorderTexture()->width, topY + theme->getWindowBorderTexture()->width + n, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( n, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, h );
	glTexCoord2i( 1, 1 );
	glVertex2i( n, h );
	glEnd();

	glPopMatrix();

	int w = getWidth() - 2 * ( n + theme->getWindowBorderTexture()->width );

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_north.glBind();

	glTranslatef( n + theme->getWindowBorderTexture()->width, topY + theme->getWindowBorderTexture()->width, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( w, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, n );
	glTexCoord2i( 1, 1 );
	glVertex2i( w, n );
	glEnd();

	glPopMatrix();

	glPushMatrix();

	theme->getWindowBorderTexture()->tex_south.glBind();

	glTranslatef( n + theme->getWindowBorderTexture()->width, topY + openHeight - n - theme->getWindowBorderTexture()->width, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( w, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, n );
	glTexCoord2i( 1, 1 );
	glVertex2i( w, n );
	glEnd();

	glPopMatrix();

	glsDisable( GLS_TEXTURE_2D );
	glsDisable( GLS_BLEND );
}


Button *Window::createButton( int x1, int y1, int x2, int y2, char const* label, bool toggle, Texture const& texture ) {
	if ( widgetCount < MAX_WIDGET ) {
		Button * theButton;
		theButton = new Button( x1, y1, x2, y2, scourgeGui->getHighlightTexture(), label, texture );
		theButton->setToggle( toggle );
		addWidget( ( Widget * )theButton );
		return theButton;
	} else {
		cerr << "Gui/Window.cpp : max widget limit reached!" << endl;
		return NULL;
	}
}

Label * Window::createLabel( int x1, int x2, char const* label, int color ) {
	if ( widgetCount < MAX_WIDGET ) {
		Label * theLabel;
		theLabel = new Label( x1, x2, label );

		// addwidget call must come before setColor...
		addWidget( ( Widget * )theLabel );

		// set new color or keep default color (black)
		if ( color == Constants::RED_COLOR ) {
			theLabel->setColor( 0.8f, 0.2f, 0.0f, 1.0f );
		} else if ( color == Constants::BLUE_COLOR ) {
			theLabel->setColor( 0.0f, 0.3f, 0.9f, 1.0f  );
		}
		return theLabel;
	} else {
		cerr << "Gui/Window.cpp : max widget limit reached!" << endl;
		return NULL;
	}
}

Checkbox * Window::createCheckbox( int x1, int y1, int x2, int y2, char *label ) {
	if ( widgetCount < MAX_WIDGET ) {
		Checkbox * theCheckbox;
		theCheckbox = new Checkbox( x1, y1, x2, y2, scourgeGui->getHighlightTexture(), label );
		addWidget( ( Widget * )theCheckbox );
		return theCheckbox;
	} else {
		cerr << "Gui/Window.cpp : max widget limit reached!" << endl;
		return NULL;
	}
}

TextField *Window::createTextField( int x, int y, int numChars ) {
	if ( widgetCount < MAX_WIDGET ) {
		TextField *tf;
		tf = new TextField( x, y, numChars );
		addWidget( ( Widget * )tf );
		return tf;
	} else {
		cerr << "Gui/Window.cpp : max widget limit reached!" << endl;
		return NULL;
	}
}

// scissor test: y screen coordinate is reversed, rectangle is
// specified by lower-left corner. sheesh!
void Window::scissorToWindow( bool insideOnly ) {
	GLint topY = ( h / 2 ) - ( openHeight / 2 );

	int n = 8;
	int sx, sy, sw, sh;

	if ( insideOnly ) {
		sx = x + n;
		sy = currentY + topY + openHeight - n;
		sw = w - n * 2;
		sh = openHeight - n * 2;
	} else {
		sx = x;
		sy = currentY + topY + openHeight;
		sw = w;
		sh = openHeight;
	}

#ifdef DEBUG_SCISSOR
	glPushMatrix();

	glTranslatef( -x, -y, 0.0f );

	glsDisable( GLS_TEXTURE_2D );

	if ( insideOnly ) {
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	} else {
		glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
	}

	glBegin( GL_LINE_LOOP );
	glVertex2i( sx, sy - sh );
	glVertex2i( sx + sw, sy - sh );
	glVertex2i( sx + sw, sy  );
	glVertex2i( sx, sy  );
	glEnd();

	glsEnable( GLS_TEXTURE_2D );

	glPopMatrix();
#endif

	glsEnable( GLS_SCISSOR_TEST );
	glScissor( sx, scourgeGui->getScreenHeight() - sy, sw, sh );
}

void Window::setVisible( bool b, bool animate ) {
	toTop();
	Widget::setVisible( b );
	if ( b ) {
		lastTick = 0;
		opening = ( !isLocked() );
		currentY = y;
		if ( animate ) {
			if ( animation == SLIDE_UP ) {
				openHeight = getHeight();
				y = getHeight();
			} else {
				openHeight = INSET * 2;
			}
		} else {
			openHeight = getHeight();
		}
		toTop();
	} else {
		for ( unsigned int i = 0; i < listeners.size(); i++ ) {
			listeners[i]->windowClosing();
		}
		y = currentY;
		scourgeGui->windowWasClosed = true;
		if( scourgeGui->currentWin == this ) scourgeGui->currentWin = NULL;
		scourgeGui->nextWindowToTop( NULL, false );

		// Any windows open?
		if ( !scourgeGui->anyFloatingWindowsOpen() ) scourgeGui->allWindowsClosed();
	}
}


// overridden so windows stay on screen and moving/rotating still works
void Window::move( int x, int y ) {
	this->x = x;
	if ( x < SCREEN_GUTTER ) this->x = SCREEN_GUTTER;
	if ( x >= scourgeGui->getScreenWidth() - ( w + SCREEN_GUTTER ) ) this->x = scourgeGui->getScreenWidth() - ( w + SCREEN_GUTTER + 1 );

	int newY = y;
	if ( y < SCREEN_GUTTER ) newY = SCREEN_GUTTER;
	if ( y >= scourgeGui->getScreenHeight() - ( h + SCREEN_GUTTER ) ) newY = scourgeGui->getScreenHeight() - ( h + SCREEN_GUTTER + 1 );

	int diffY = newY - this->currentY;
	this->currentY = newY;
	if ( animation == SLIDE_UP && this->y > this->currentY ) {
		this->y -= diffY;
	} else {
		this->y = newY;
	}
}

void Window::setLastWidget( Widget *w ) {
	if ( w != lastWidget ) {
		lastWidget = w;
		scourgeGui->playSound( Window::ROLL_OVER_SOUND, 127 );
	}
}

void Window::setMouseLock( Widget *widget ) {
	if ( widget ) {
		getScourgeGui()->lockMouse( this, widget );
	} else {
		getScourgeGui()->unlockMouse();
	}
}

void Window::resize( int w, int h ) {
  Widget::resize( w, h );
  // make sure it's not off the screen
  move( getX(), getY() );
}
