/***************************************************************************
                          storable.h  -  description
                             -------------------
    begin                : Sat Oct 22 2005
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

#ifndef STORABLE_H
#define STORABLE_H

#include "constants.h"

/**
 * @author Gabor Torok
 * 
 * Describes an object that can be referenced by the quick-spell buttons.
 * A spell or special capability.
 * 
 */
class Storable {

public:
  Storable() {
  }

  virtual ~Storable() {
  }

  enum {
    SPELL_STORABLE=0,
    SPECIAL_STORABLE
  };

  virtual const char *getName() = 0;
  virtual int getIconTileX() = 0;
  virtual int getIconTileY() = 0;
  virtual int getStorableType() = 0;
};

#endif

