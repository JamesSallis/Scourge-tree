/***************************************************************************
                          rpgitem.cpp  -  description
                             -------------------
    begin                : Sun Sep 28 2003
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
#ifndef RPG_ITEM_H
#define RPG_ITEM_H

#include "../constants.h"

class RpgItem {
 private:
  char *name, *desc, *shortDesc;
  int level;
  int type;
  int weight, price, quality;
  int action; // damage, defence, potion str.
  int speed; // 0-20, 0-fast, 20-slow, 10-avg
  int shape_index;
  int twohanded;

 public:
  enum itemNames {
    SHORT_SWORD=0,
    DAGGER,
    BASTARD_SWORD,
	LONG_SWORD,
	GREAT_SWORD,
	
	BATTLE_AXE,
	THROWING_AXE,

	CHEST,
	BOOKSHELF,
	CHEST2,
	BOOKSHELF2,
	
	// must be the last ones
	ITEM_COUNT
  };

  enum itemTypes {
	SWORD=0,
	AXE,
	BOW,
	CONTAINER,
	
	// must be last
	ITEM_TYPE_COUNT
  };

  enum twoHandedType {
	NOT_TWO_HANDED=0,
	ONLY_TWO_HANDED,
	OPTIONAL_TWO_HANDED
  };

  static RpgItem *items[];
  
  RpgItem(char *name, int level, int type, int weight, int price, int quality, 
		  int action, int speed, char *desc, char *shortDesc, int shape_index, int twohanded=NOT_TWO_HANDED);
  ~RpgItem();

  inline int getShapeIndex() { return shape_index; }
  inline char *getShortDesc() { return shortDesc; }  
};

#endif
