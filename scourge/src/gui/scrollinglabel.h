/***************************************************************************
         scrollinglabel.h  -  Scrollable multiline hypertext widget
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

#ifndef SCROLLING_LABEL_H
#define SCROLLING_LABEL_H
#pragma once

#include "gui.h"
#include "widget.h"
#include "button.h"
#include "window.h"
#include "draganddrop.h"
#include <vector>

/**
  *@author Gabor Torok
  */

/// Looks whether a hyperlink is in view or clicked on.
class WordClickedHandler {
public:
	WordClickedHandler() {
	}

	virtual ~WordClickedHandler() {
	}

	virtual void wordClicked( std::string const& word ) = 0;
	virtual void showingWord( char *word ) = 0;
};

/// A text box widget with clickable hyperlinks.
class ScrollingLabel : public Widget {
protected:
	static const int TEXT_SIZE = 3000;
	char text[ TEXT_SIZE ];
	int lineWidth;
	std::vector<std::string> lines;

	//  int count;
	//  const char **list;
	//  const Color *colors;
	//  const GLuint *icons;
	int value;
	int scrollerWidth, scrollerHeight;
	int listHeight;
	bool willSetScrollerHeight, willScrollToBottom;
	float alpha, alphaInc;
	GLint lastTick;
	bool inside;
	int scrollerY;
	bool dragging;
	int dragX, dragY;
	// never assigned a value: int selectedLine;
	//  DragAndDropHandler *dragAndDropHandler;
	bool innerDrag;
	int innerDragX, innerDragY;
	//  bool highlightBorders;
	// GLuint highlight;
	bool canGetFocusVar;

	std::map<char, Color> coloring;

	struct WordPos {
		int x, y, w, h;
		char word[255];
	};
	WordPos wordPos[1000];
	int wordPosCount;
	WordClickedHandler *handler;

	bool interactive;

public:

	bool debug;

	ScrollingLabel( int x, int y, int w, int h, char *text );
	virtual ~ScrollingLabel();

	inline void setInteractive( bool interactive ) {
		this->interactive = interactive;
	}

	inline void setWordClickedHandler( WordClickedHandler *handler ) {
		this->handler = handler;
	}

	inline void addColoring( char c, Color color ) {
		coloring[c] = color;
	}

	inline char *getText() {
		return text;
	}
	void setText( char const* s );

	/**
	 * Append text by scrolling off the top if it won't fit in the buffer.
	 */
	void appendText( const char *s );

	//  inline int getLineCount() { return count; }
	//  void setLines(int count, const char *s[], const Color *colors=NULL, const GLuint *icon=NULL);
	//  inline const char *getLine(int index) { return list[index]; }

	//  inline int getSelectedLine() { return selectedLine; }
	//  void setSelectedLine(int n);

	virtual void drawWidget( Window* parent );
	
	/**
	Return true, if the event activated this widget. (For example, button push, etc.)
	Another way to think about it is that if true, the widget fires an "activated" event
	to the outside world.
	 */
	virtual bool handleEvent( Window* parent, SDL_Event* event, int x, int y );
	
	virtual void removeEffects();

	// don't play sound when the value changes
	virtual inline bool hasSound() {
		return false;
	}

	inline bool canGetFocus() {
		return canGetFocusVar;
	}
	inline void setCanGetFocus( bool b ) {
		this->canGetFocusVar = b;
	}

private:
	char *printLine( Window* parent, int x, int y, char *s );
	int getWordPos( int x, int y );
	//  void selectLine(int x, int y);
	//  void drawIcon( int x, int y, GLuint icon );
	void moveSelectionUp();
	void moveSelectionDown();
};

#endif

