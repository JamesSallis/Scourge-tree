/***************************************************************************
                 renderlib.h  -  Complete rendering interface
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

#ifndef RENDER_LIB_H
#define RENDER_LIB_H
#pragma once

/**
 * A way for external classes to this dir to get everything in one include file.
 */

//#include "../rpg/rpglib.h"
#include "map.h"
#include "mapsettings.h"
#include "maprender.h"
#include "mapadapter.h"
#include "effect.h"
#include "location.h"
#include "shape.h"
#include "glshape.h"
#include "gltorch.h"
#include "glcaveshape.h"
#include "glteleporter.h"
#include "gllocator.h"
#include "3dsshape.h"
#include "md2shape.h"
#include "animatedshape.h"
#include "Md2.h"
#include "rendereditem.h"
#include "renderedcreature.h"
#include "renderedprojectile.h"
#include "shapes.h"
#include "maprenderhelper.h"
#include "projectilerenderer.h"
#include "Md3.h"
#include "md3shape.h"
#include "modelwrapper.h"
#include "cutscene.h"
#include "weather.h"

#endif
