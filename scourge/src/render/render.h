/***************************************************************************
                   render.h  -  Rendering interface class
                             -------------------
    begin                : Sat May 3 2003
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

#ifndef RENDER_H
#define RENDER_H
#pragma once

// definitely outside of dir
#include "../persist.h"
#include "../preferences.h"
#include "../util.h"

/**
 *@author Gabor Torok
 *
 * The class thru which this directory talks to the rest of the code.
 * Never include this file outside this dir. (use renderlib.h instead.)
 */
class Render {
public:
};

#endif
