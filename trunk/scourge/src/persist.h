/***************************************************************************
                          persist.h  -  description
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

#ifndef PERSIST_H
#define PERSIST_H

#include "common/constants.h"
#include "rpg/rpg.h"

class File;

#define PERSIST_VERSION 31

#define OLDEST_HANDLED_VERSION 15

typedef struct _DiceInfo {
  Uint32 version;
  Uint32 count, sides, mod;
} DiceInfo;

typedef struct _ItemInfo {
  Uint32 version;
  Uint32 level;
  Uint8 rpgItem_name[255];
  Uint8 shape_name[255];
  Uint32 blocking, currentCharges, weight;
  Uint32 quality;
  Uint32 price;
	Uint32 identifiedBits;
  Uint8 spell_name[255];
  Uint32 containedItemCount;
  struct _ItemInfo *containedItems[MAX_CONTAINED_ITEMS];

  Uint32 bonus, damageMultiplier, cursed, magicLevel;
  Uint8 monster_type[255];
  Uint8 magic_school_name[255];
  DiceInfo *magicDamage;
  Uint8 stateMod[Constants::STATE_MOD_COUNT];
  Uint8 skillBonus[Skill::SKILL_COUNT];

} ItemInfo;
/*
typedef struct _ContainedItemInfo {
  Uint32 version;
  Uint32 containedItemCount;
  ItemInfo containedItems[MAX_CONTAINED_ITEMS];
} ContainedItemInfo;
*/

typedef struct _CreatureInfo {
  Uint32 version;
  Uint8 name[255];
  Uint8 character_name[255];
  Uint32 character_model_info_index;
  Uint32 deityIndex;
  Uint8 monster_name[255];
  Uint32 hp, mp, exp, level, money, stateMod, protStateMod, x, y, z, dir;
  Uint32 speed, motion, armor, bonusArmor, thirst, hunger;
  Uint8 sex;
  Uint32 availableSkillPoints;
  Uint32 skills[Skill::SKILL_COUNT];
  Uint32 skillMod[Skill::SKILL_COUNT];
  Uint32 skillBonus[Skill::SKILL_COUNT];
	Uint32 skillsUsed[Skill::SKILL_COUNT];
  Uint32 portraitTextureIndex;

  // inventory
  Uint32 inventory_count;
  ItemInfo *inventory[MAX_INVENTORY_SIZE];
  //ContainedItemInfo containedItems[MAX_INVENTORY_SIZE];
  Uint32 equipped[Constants::INVENTORY_COUNT];

  // spells memorized ([school][spell]
  Uint32 spell_count;
  Uint8 spell_name[100][255];
  Uint8 quick_spell[12][255];
} CreatureInfo;

typedef struct _LocationInfo {
  Uint16 x, y, z;
  Uint8 floor_shape_name[255];
  Uint8 shape_name[255];
  Uint8 item_name[255];
	ItemInfo *item;
  Uint8 monster_name[255];
	CreatureInfo *creature;
	Uint8 item_pos_name[255];
	ItemInfo *item_pos;
	Uint8 magic_school_name[255]; // the deity at this location
} LocationInfo;

typedef struct _RugInfo {
	Uint32 texture;
	Uint8 isHorizontal;
	Uint32 angle;
	Uint16 cx, cy;
} RugInfo;

typedef struct _LockedInfo {
	Uint32 key;
	Uint8 value;
} LockedInfo;

typedef struct _DoorInfo {
	Uint32 key;
	Uint32 value;
} DoorInfo;

typedef struct _FogInfo {
	Uint8 fog[MAP_WIDTH][MAP_DEPTH];
	Uint8 players[MAP_WIDTH * MAP_DEPTH][4];
} FogInfo;

#define REF_TYPE_NAME 0
#define REF_TYPE_OBJECT 1

typedef struct _MapInfo {
  Uint32 version;
	Uint8 map_type;
  Uint16 start_x, start_y;
  Uint16 grid_x, grid_y;
  Uint32 pos_count;
  Uint8 theme_name[255];
	Uint8 reference_type;
  LocationInfo *pos[ MAP_WIDTH * MAP_DEPTH * MAP_VIEW_HEIGHT ];
	Uint32 rug_count;
	RugInfo *rugPos[ ( MAP_WIDTH / MAP_UNIT ) * ( MAP_DEPTH / MAP_UNIT ) ];
	Uint8 hasWater;
	Uint32 locked_count;
	LockedInfo *locked[ MAP_WIDTH * MAP_DEPTH * MAP_VIEW_HEIGHT ];
	Uint32 door_count;
	DoorInfo *door[ MAP_WIDTH * MAP_DEPTH * MAP_VIEW_HEIGHT ];
	Uint32 secret_count;
	LockedInfo *secret[ MAP_WIDTH * MAP_DEPTH ];
	FogInfo fog_info;
} MapInfo;

typedef struct _MissionInfo {
	Uint32 version;
  Uint8 level;
  Uint8 depth;
	Uint8 completed;
  Uint8 mapName[80];
	Uint8 templateName[80];
	Uint8 itemCount;
	Uint8 itemName[100][255];
	Uint8 itemDone[100];
	Uint8 monsterCount;
	Uint8 monsterName[100][255];
	Uint8 monsterDone[100];
} MissionInfo;

class Persist {
public:
  static LocationInfo *createLocationInfo( Uint16 x, Uint16 y, Uint16 z );
	static RugInfo *createRugInfo( Uint16 cx, Uint16 cy );
	static LockedInfo *createLockedInfo( Uint32 key, Uint8 value );
	static DoorInfo *createDoorInfo( Uint32 key, Uint32 value );
	static void saveMap( File *file, MapInfo *info );
  static MapInfo *loadMap( File *file );
  static void loadMapHeader( File *file, Uint16 *gridX, Uint16 *gridY );
  static void deleteMapInfo( MapInfo *info );

  static void saveCreature( File *file, CreatureInfo *info );
  static CreatureInfo *loadCreature( File *file );
  static void deleteCreatureInfo( CreatureInfo *info );

	static void saveMission( File *file, MissionInfo *info );
  static MissionInfo *loadMission( File *file );
  static void deleteMissionInfo( MissionInfo *info );

protected:
  static void saveItem( File *file, ItemInfo *item );
  static ItemInfo *loadItem( File *file );
  static void deleteItemInfo( ItemInfo *info );

  static void saveDice( File *file, DiceInfo *info );
  static DiceInfo *loadDice( File *file );
  static void deleteDiceInfo( DiceInfo *info );
};

#endif
