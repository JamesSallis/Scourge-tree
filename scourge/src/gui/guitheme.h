/***************************************************************************
                       guitheme.h  -  GUI theme loader
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

#ifndef GUI_THEME_H
#define GUI_THEME_H
#pragma once

#include "gui.h"
#include "widget.h"
#include <map>
#include "../render/texture.h"

/**
  *@author Gabor Torok
  */

/// A single element of a GUI theme.

class ThemeElement {
public:
	char textureFileName[40], north[40], south[40], east[40], west[40];
	char ne[40], nw[40], se[40], sw[40];
	Texture texture;
	Texture tex_north;
	Texture tex_south;
	Texture tex_east;
	Texture tex_west;
	Texture tex_nw;
	Texture tex_ne;
	Texture tex_sw;
	Texture tex_se;
	Color color;
	int width;
	int rep_h, rep_v;

	ThemeElement() {}
	~ThemeElement() {}

	void loadTextures( ScourgeGui *scourgeGui );
};

/// collection of all GUI themes.

class GuiTheme;
class GuiThemes {
public:
	GuiThemes( ScourgeGui *scourgeGui );
	~GuiThemes();
	
	GuiTheme* getByName( std::string const& s ) {
		if ( themes.find( s ) == themes.end() ) {
			std::cerr << "*** error: can't find theme: " << s << std::endl;
			return NULL;
		}
			
		return themes[ s ];
	}
private:
	std::map<std::string, GuiTheme*> themes;
	// GuiTheme is designed to know all of its kind 
	static GuiThemes* instance;
	friend class GuiTheme;
	// moved here, others don't use these 
	static ThemeElement* parseElement( const char* line );
	static Color* parseColor( const char* line );
	// undefine all unused implicits
	GuiThemes(); // undefined
	GuiThemes( GuiThemes const& that ); // undefined
	GuiThemes& operator=( GuiThemes const& that ); // undefined
};



/// A theme for the game's GUI.

class GuiTheme {
private:
	std::string nameX;
	ThemeElement *windowBack;
	ThemeElement *windowTop;
	ThemeElement *windowBorder;
	Color *windowTitleText;
	Color *windowText;
	ThemeElement *buttonBackground;
	ThemeElement *buttonSelectionBackground;
	ThemeElement *buttonHighlight;
	ThemeElement *buttonBorder;
	Color *buttonText;
	Color *buttonSelectionText;
	ThemeElement *listBackground;
	ThemeElement *inputBackground;
	Color *inputText;
	ThemeElement *selectionBackground;
	Color *selectionText;
	ThemeElement *selectedBorder;
	ThemeElement *selectedCharacterBorder;
	ThemeElement *windowBorderTexture;

	friend class GuiThemes;

public:
	GuiTheme( char const* name );
	virtual ~GuiTheme();

	static char DEFAULT_THEME[255];

	static GuiTheme* getThemeByName( const char *name ) {
		std::string s = name;
		if ( GuiThemes::instance == NULL ) { 
			std::cerr << "*** error: no GUI themes: " << s << std::endl;
			return NULL;
		}
		return GuiThemes::instance->getByName( s );
	}
 
	char const* getName() {
		return nameX.c_str();
	}
	inline ThemeElement *getWindowBackground() {
		return windowBack;
	}
	inline ThemeElement *getWindowTop() {
		return windowTop;
	}
	inline ThemeElement *getWindowBorder() {
		return windowBorder;
	}
	inline Color *getWindowTitleText() {
		return windowTitleText;
	}
	inline Color *getWindowText() {
		return windowText;
	}
	inline ThemeElement *getButtonBackground() {
		return buttonBackground;
	}
	inline ThemeElement *getButtonSelectionBackground() {
		return buttonSelectionBackground;
	}
	inline ThemeElement *getButtonHighlight() {
		return buttonHighlight;
	}
	inline ThemeElement *getButtonBorder() {
		return buttonBorder;
	}
	inline Color *getButtonText() {
		return buttonText;
	}
	inline Color *getButtonSelectionText() {
		return buttonSelectionText;
	}
	inline ThemeElement *getListBackground() {
		return listBackground;
	}
	inline ThemeElement *getInputBackground() {
		return inputBackground;
	}
	inline Color *getInputText() {
		return inputText;
	}
	inline ThemeElement *getSelectionBackground() {
		return selectionBackground;
	}
	inline Color *getSelectionText() {
		return selectionText;
	}
	inline ThemeElement *getSelectedBorder() {
		return selectedBorder;
	}
	inline ThemeElement *getSelectedCharacterBorder() {
		return selectedCharacterBorder;
	}
	inline ThemeElement *getWindowBorderTexture() {
		return windowBorderTexture;
	}

protected:
	void loadTextures( ScourgeGui *scourgeGui );

	inline void setWindowBackground( ThemeElement *element ) {
		this->windowBack = element;
	}
	inline void setWindowTop( ThemeElement *element ) {
		this->windowTop = element;
	}
	inline void setWindowBorder( ThemeElement *element ) {
		this->windowBorder = element;
	}
	inline void setWindowTitleText( Color *color ) {
		this->windowTitleText = color;
	}
	inline void setWindowText( Color *color ) {
		this->windowText = color;
	}
	inline void setButtonBackground( ThemeElement *element ) {
		this->buttonBackground = element;
	}
	inline void setButtonSelectionBackground( ThemeElement *element ) {
		this->buttonSelectionBackground = element;
	}
	inline void setButtonHighlight( ThemeElement *element ) {
		this->buttonHighlight = element;
	}
	inline void setButtonBorder( ThemeElement *element ) {
		this->buttonBorder = element;
	}
	inline void setButtonText( Color *color ) {
		this->buttonText = color;
	}
	inline void setButtonSelectionText( Color *color ) {
		this->buttonSelectionText = color;
	}
	inline void setListBackground( ThemeElement *element ) {
		this->listBackground = element;
	}
	inline void setInputBackground( ThemeElement *element ) {
		this->inputBackground = element;
	}
	inline void setInputText( Color *color ) {
		this->inputText = color;
	}
	inline void setSelectionBackground( ThemeElement *element ) {
		selectionBackground = element;
	}
	inline void setSelectionText( Color *color ) {
		this->selectionText = color;
	}
	inline void setSelectedBorder( ThemeElement *element ) {
		selectedBorder = element;
	}
	inline void setSelectedCharacterBorder( ThemeElement *element ) {
		selectedCharacterBorder = element;
	}
	inline void setWindowBorderTexture( ThemeElement *element ) {
		windowBorderTexture = element;
	}
};




#endif


