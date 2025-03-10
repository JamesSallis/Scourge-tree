/***************************************************************************
                debug.h  -  Global symbols for game debugging
                             -------------------
    begin                : Sat Oct 8 2005
    copyright            : (C) 2005 by Gabor Torok
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

#ifndef DEBUG_VALUES_H
#define DEBUG_VALUES_H
#pragma once

/**
 * This is where debug flags go.
 *
 * Include this file as needed but do not add to makefile.
 * Do not include this file in a .h file.
 * This way it will only recompile the affected .cpp files.
 */

#define SHOW_FPS true

// battle.cpp
#define DEBUG_BATTLE false

// calendar.cpp
#define CALENDAR_DEBUG 0

// creature.cpp
#define GOD_MODE 1
#define MONSTER_IMORTALITY 0

// partyeditor.cpp (if non-1, defaults are added)
#define STARTING_PARTY_LEVEL 1

// scourge.cpp
//#define CAVE_TEST 1
#define CAVE_TEST_LEVEL 4
#define BATTLES_ENABLED 1

// comment out to unset
#define DEBUG_KEYS 1
//#define BASE_DEBUG 1

// comment out to unset
//#define DEBUG_IDENTIFY_ITEM 1

// sqbinding.cpp
//#define DEBUG_SQUIRREL 1
#ifndef DEBUG_SQUIRREL
# define DEBUG_SQUIRREL 0
#endif

// show secret doors
//#define DEBUG_SECRET_DOORS 1

// show every creature's path
#define PATH_DEBUG 0

// #define DEBUG_SCREENSHOT 1

#define DEBUG_TRAPS 0

#define SMOOTH_DOORS 1

#define AMBIENT_PAUSE_MIN 1000
#define AMBIENT_ROLL 5

//#define DEBUG_HEIGHT_MAP 1

// Set to 0 or 1 to always rerun movies. Useful to debug movies that only occur once.
#define RERUN_MOVIES 0

// use squirrel calls in combar (special abilities, etc.)
#define SQUIRREL_ENABLED true

// torchlight
#define LIGHTS_ENABLED 1

// set to 1 to enable bounding box and ground grid drawing
#define DEBUG_MOUSE_POS 0

#define FOG_ENABLED 0

#endif

