/***************************************************************************
                     optionsmenu.h  -  The options menu
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

#ifndef OPTIONSMENU_H
#define OPTIONSMENU_H
#pragma once

#include <string.h>
#include "sdlhandler.h"
#include "sdleventhandler.h"
#include "sdlscreenview.h"
#include "scourge.h"
#include "userconfiguration.h"
#include "util.h"
#include "gui/window.h"
#include "gui/button.h"
#include "gui/scrollinglist.h"
#include "gui/cardcontainer.h"
#include "gui/multiplelabel.h"
#include "gui/checkbox.h"
#include "gui/slider.h"
#include "gui/eventhandler.h"

/**
  *@author Gabor Torok
  */

#define MAX_CONTROLS_LINE_SIZE 80

class Scourge;
class UserConfiguration;

/// The options menu.
class OptionsMenu : public EventHandler, RawEventHandler {
private:

	Scourge *scourge;
	UserConfiguration * uc;
	bool showDebug;
	bool controlsLoaded;
	bool videoLoaded;
	bool gameSettingsLoaded;
	bool waitingForNewKey;
	bool ignoreKeyUp;

	enum modeOptions {
		CONTROLS = 0, VIDEO, AUDIO, GAME_SETTINGS
	};
	int selectedMode;

	Window *mainWin;
	Button *controlsButton, *videoButton, *audioButton, *gameSettingsButton;
	Button *changeControlButton, *saveButton, *closeButton;
	Label * keyBindingsLabel;
	Label * waitingLabel;
	Label * changeTakeEffectLabel;

	MultipleLabel * gameSpeedML;
	Checkbox * alwaysCenterMapCheckbox;
	Checkbox * turnBasedBattle;
	Checkbox *outlineInteractiveItems;
	Checkbox *tooltipEnabled;
	Slider *tooltipInterval;
	MultipleLabel * logLevelML;
	MultipleLabel * pathFindingQualityML;

	MultipleLabel * videoResolutionML;
	Checkbox * fullscreenCheckbox;
	Checkbox * doublebufCheckbox;
	Checkbox * hwaccelCheckbox;
	Checkbox * stencilbufCheckbox;
	Checkbox * multitexturingCheckbox;
	Checkbox * anisofilterCheckbox;
	MultipleLabel * shadowsML;
	Checkbox * lights;

	CardContainer *cards;
	ScrollingList *controlBindingsList;

	Slider *musicVolume;
	Slider *effectsVolume;

	void setSelectedMode();
	void loadControls();
	void loadVideo();
	void loadGameSettings();

public:
	OptionsMenu( Scourge *scourge );
	~OptionsMenu();

	bool handleEvent( SDL_Event *event );
	bool handleEvent( Widget *widget, SDL_Event *event );
	void show();
	void hide();
	inline bool isVisible() {
		return mainWin->isVisible();
	}
};

#endif
