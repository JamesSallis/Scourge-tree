/***************************************************************************
                      persist.cpp  -  Savegame objects
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
#include "common/constants.h"
#include "persist.h"
#include "render/renderlib.h"
#include "io/file.h"

// ###### MS Visual C++ specific ###### 
#if defined(_MSC_VER) && defined(_DEBUG)
# define new DEBUG_NEW
# undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif 

using namespace std;

// since Persist is namespace now the former protected members of it
// got degraded to non-global details of implementation
namespace { //anonymous

void saveDice( File *file, DiceInfo *info ) {
	file->write( &( info->version ) );
	file->write( &( info->count ) );
	file->write( &( info->sides ) );
	file->write( &( info->mod ) );
}

void saveNpcInfoInfo( File *file, NpcInfoInfo *info ) {
	file->write( &( info->version ) );
	file->write( &( info->x ) );
	file->write( &( info->y ) );
	file->write( &( info->level ) );
	file->write( &( info->type ) );
	file->write( info->name, 255 );
	file->write( info->subtype, 255 );
}

NpcInfoInfo *loadNpcInfoInfo( File *file ) {
	NpcInfoInfo *info =  new NpcInfoInfo;
	file->read( &( info->version ) );
	file->read( &( info->x ) );
	file->read( &( info->y ) );
	file->read( &( info->level ) );
	file->read( &( info->type ) );
	file->read( info->name, 255 );
	file->read( info->subtype, 255 );
	return info;
}

void deleteNpcInfoInfo( NpcInfoInfo *info ) {
	delete info;
}

void saveItem( File *file, ItemInfo *info ) {
	file->write( &( info->version ) );
	file->write( &( info->level ) );
	file->write( info->rpgItem_name, 255 );
	file->write( info->shape_name, 255 );
	file->write( &( info->blocking ) );
	file->write( &( info->currentCharges ) );
	file->write( &( info->weight ) );
	file->write( &( info->quality ) );
	file->write( &( info->price ) );
	file->write( &( info->identifiedBits ) );
	file->write( info->spell_name, 255 );
	file->write( &( info->containedItemCount ) );
	for ( int i = 0; i < static_cast<int>( info->containedItemCount ); i++ ) {
		saveItem( file, info->containedItems[i] );
	}

	file->write( &( info->bonus ) );
	file->write( &( info->damageMultiplier ) );
	file->write( &( info->cursed ) );
	file->write( &( info->magicLevel ) );
	file->write( info->monster_type, 255 );
	file->write( info->magic_school_name, 255 );
	saveDice( file, info->magicDamage );
	for ( int i = 0; i < StateMod::STATE_MOD_COUNT; i++ ) {
		file->write( &( info->stateMod[i] ) );
	}
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		file->write( &( info->skillBonus[i] ) );
	}
	file->write( &( info->missionId ) );
	file->write( &( info->missionObjectiveIndex ) );
}

DiceInfo* loadDice( File* file ) {
	DiceInfo *info = new DiceInfo;
	file->read( &( info->version ) );
	file->read( &( info->count ) );
	file->read( &( info->sides ) );
	file->read( &( info->mod ) );
	return info;
}


ItemInfo* loadItem( File* file ) {
	ItemInfo *info = new ItemInfo;
	file->read( &( info->version ) );
	file->read( &( info->level ) );
	file->read( info->rpgItem_name, 255 );
	file->read( info->shape_name, 255 );
	file->read( &( info->blocking ) );
	file->read( &( info->currentCharges ) );
	file->read( &( info->weight ) );
	file->read( &( info->quality ) );
	file->read( &( info->price ) );
	if ( info->version >= 17 ) file->read( &( info->identifiedBits ) );
	else info->identifiedBits = 0;

	file->read( info->spell_name, 255 );
	file->read( &( info->containedItemCount ) );
	for ( int i = 0; i < static_cast<int>( info->containedItemCount ); i++ ) {
		info->containedItems[i] = loadItem( file );
	}

	file->read( &( info->bonus ) );
	file->read( &( info->damageMultiplier ) );
	file->read( &( info->cursed ) );
	file->read( &( info->magicLevel ) );
	file->read( info->monster_type, 255 );
	file->read( info->magic_school_name, 255 );
	info->magicDamage = loadDice( file );
	for ( int i = 0; i < StateMod::STATE_MOD_COUNT; i++ ) {
		file->read( &( info->stateMod[i] ) );
	}
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		file->read( &( info->skillBonus[i] ) );
	}
	if ( info->version == 36 || info->version == 37 ) {
		// just read and forget. this field is not used anymore.
		Uint8 mission;
		file->read( &( mission ) );
	}
	if ( info->version >= 38 ) {
		file->read( &( info->missionId ) );
		file->read( &( info->missionObjectiveIndex ) );
	} else {
		info->missionId = info->missionObjectiveIndex = 0;
	}
	return info;
}

void deleteItemInfo( ItemInfo *info ) {
	for ( int i = 0; i < static_cast<int>( info->containedItemCount ); i++ ) {
		deleteItemInfo( info->containedItems[i] );
	}
	delete info->magicDamage;
	delete info;
}

void saveTrap( File *file, TrapInfo *info ) {
	file->write( &( info->version ) );
	file->write( &( info->x ) );
	file->write( &( info->y ) );
	file->write( &( info->w ) );
	file->write( &( info->h ) );
	file->write( &( info->type ) );
	file->write( &( info->discovered ) );
	file->write( &( info->enabled ) );
}

TrapInfo *loadTrap( File *file ) {
	TrapInfo *info = new TrapInfo;
	file->read( &( info->version ) );
	file->read( &( info->x ) );
	file->read( &( info->y ) );
	file->read( &( info->w ) );
	file->read( &( info->h ) );
	file->read( &( info->type ) );
	file->read( &( info->discovered ) );
	file->read( &( info->enabled ) );
	return info;
}

void deleteTrapInfo( TrapInfo *info ) {
	delete info;
}

void saveGenerator( File *file, GeneratorInfo *info ) {
	file->write( &( info->version ) );
	file->write( &( info->rx ) );
	file->write( &( info->ry ) );
	file->write( &( info->x ) );
	file->write( &( info->y ) );
	file->write( &( info->count ) );
	file->write( info->monster, 255 );
}

GeneratorInfo *loadGenerator( File *file ) {
	GeneratorInfo *info = new GeneratorInfo;
	file->read( &( info->version ) );
	file->read( &( info->rx ) );
	file->read( &( info->ry ) );
	file->read( &( info->x ) );
	file->read( &( info->y ) );
	file->read( &( info->count ) );
	file->read( info->monster, 255 );
	return info;
}

void deleteGeneratorInfo( GeneratorInfo *info ) {
	delete info;
}

void deleteDiceInfo( DiceInfo *info ) {
	delete info;
}
} // anonymous namespace


LocationInfo *Persist::createLocationInfo( Uint16 x, Uint16 y, Uint16 z ) {
	LocationInfo *info = new LocationInfo;

	info->x = x;
	info->y = y;
	info->z = z;

	// preset strings to all 0 for better compressability.
	memset( info->item_pos_name, 0, sizeof( info->item_pos_name ) );
	memset( info->item_name, 0, sizeof( info->item_name ) );
	memset( info->monster_name, 0, sizeof( info->monster_name ) );
	memset( info->shape_name, 0, sizeof( info->shape_name ) );
	memset( info->floor_shape_name, 0, sizeof( info->floor_shape_name ) );
	memset( info->magic_school_name, 0, sizeof( info->magic_school_name ) );

	info->item_pos = NULL;
	info->item = NULL;
	info->creature = NULL;

	return info;
}

RugInfo *Persist::createRugInfo( Uint16 cx, Uint16 cy ) {
	RugInfo *info = new RugInfo;
	info->cx = cx;
	info->cy = cy;
	info->angle = 0;
	info->isHorizontal = 0;
	info->texture = 0;
	return info;
}

TrapInfo *Persist::createTrapInfo( int x, int y, int w, int h, int type, bool discovered, bool enabled ) {
	TrapInfo *info = new TrapInfo;
	info->version = PERSIST_VERSION;
	info->x = ( Uint16 )x;
	info->y = ( Uint16 )y;
	info->w = ( Uint16 )w;
	info->h = ( Uint16 )h;
	info->type = ( Uint8 )type;
	info->discovered = ( Uint8 )discovered;
	info->enabled = ( Uint8 )enabled;
	return info;
}

GeneratorInfo *Persist::createGeneratorInfo( int rx, int ry, int x, int y, int count, char *monster ) {
	GeneratorInfo *info = new GeneratorInfo;
	info->version = PERSIST_VERSION;
	info->rx = ( Uint16 )rx;
	info->ry = ( Uint16 )ry;
	info->x = ( Uint16 )x;
	info->y = ( Uint16 )y;
	info->count = ( Uint8 )count;
	strncpy( (char*)info->monster, monster, 254 );
	return info;
}

LockedInfo *Persist::createLockedInfo( Uint32 key, Uint8 value ) {
	LockedInfo *info = new LockedInfo;
	info->key = key;
	info->value = value;
	return info;
}

DoorInfo *Persist::createDoorInfo( Uint32 key, Uint32 value ) {
	DoorInfo *info = new DoorInfo;
	info->key = key;
	info->value = value;
	return info;
}

OutdoorTextureInfo *Persist::createOutdoorTextureInfo( Uint16 x, Uint16 y, Uint16 z ) {
	OutdoorTextureInfo *info = new OutdoorTextureInfo;
	info->x = x;
	info->y = y;
	info->z = z;
	info->offsetX = info->offsetY = 0;
	info->angle = 0;
	info->horizFlip = info->vertFlip = 0;
	memset( info->groundTextureName, 0, sizeof( info->groundTextureName ) );
	return info;
}

void Persist::saveMap( File *file, MapInfo *info ) {
	file->write( &( info->version ) );
	file->write( &( info->map_type ) );
	file->write( &( info->start_x ) );
	file->write( &( info->start_y ) );
	file->write( &( info->map_start_x ) );
	file->write( &( info->map_start_y ) );
	file->write( &( info->map_end_x ) );
	file->write( &( info->map_end_y ) );
	file->write( &( info->grid_x ) );
	file->write( &( info->grid_y ) );
	file->write( info->theme_name, 255 );
	file->write( &( info->hasWater ) );
	file->write( &( info->reference_type ) );
	file->write( &( info->edited ) );
	file->write( &( info->pos_count ) );
	for ( int i = 0; i < static_cast<int>( info->pos_count ); i++ ) {
		file->write( &( info->pos[i]->x ) );
		file->write( &( info->pos[i]->y ) );
		file->write( &( info->pos[i]->z ) );
		if ( strlen( ( char* )( info->pos[i]->floor_shape_name ) ) ) {
			file->write( info->pos[i]->floor_shape_name, 255 );
		} else {
			file->write( info->pos[i]->floor_shape_name );
		}

		Uint8 hasItemPos = ( strlen( ( char* )( info->pos[i]->item_pos_name ) ) || info->pos[i]->item_pos ? 1 : 0 );
		file->write( &( hasItemPos ) );
		if ( hasItemPos ) {
			if ( info->reference_type == REF_TYPE_NAME ) {
				file->write( info->pos[i]->item_pos_name, 255 );
			} else {
				saveItem( file, info->pos[i]->item_pos );
			}
		}

		if ( strlen( ( char* )( info->pos[i]->shape_name ) ) ) {
			file->write( info->pos[i]->shape_name, 255 );
		} else {
			file->write( info->pos[i]->shape_name );
		}

		Uint8 hasItem = ( strlen( ( char* )( info->pos[i]->item_name ) ) || info->pos[i]->item ? 1 : 0 );
		file->write( &( hasItem ) );
		if ( hasItem ) {
			if ( info->reference_type == REF_TYPE_NAME ) {
				file->write( info->pos[i]->item_name, 255 );
			} else {
				saveItem( file, info->pos[i]->item );
			}
		}

		Uint8 hasCreature = ( strlen( ( char* )( info->pos[i]->monster_name ) ) || info->pos[i]->creature ? 1 : 0 );
		file->write( &( hasCreature ) );
		if ( hasCreature ) {
			if ( info->reference_type == REF_TYPE_NAME ) {
				file->write( info->pos[i]->monster_name, 255 );
			} else {
				saveCreature( file, info->pos[i]->creature );
			}
		}

		Uint8 hasDeity = ( strlen( ( char* )( info->pos[i]->magic_school_name ) ) ? 1 : 0 );
		file->write( &( hasDeity ) );
		if ( hasDeity ) {
			file->write( info->pos[i]->magic_school_name, 255 );
		}

	}
	file->write( &( info->rug_count ) );
	for ( int i = 0; i < static_cast<int>( info->rug_count ); i++ ) {
		file->write( &( info->rugPos[i]->cx ) );
		file->write( &( info->rugPos[i]->cy ) );
		file->write( &( info->rugPos[i]->texture ) );
		file->write( &( info->rugPos[i]->isHorizontal ) );
		file->write( &( info->rugPos[i]->angle ) );
	}
	file->write( &( info->locked_count ) );
	for ( int i = 0; i < static_cast<int>( info->locked_count ); i++ ) {
		file->write( &( info->locked[i]->key ) );
		file->write( &( info->locked[i]->value ) );
	}
	file->write( &( info->door_count ) );
	for ( int i = 0; i < static_cast<int>( info->door_count ); i++ ) {
		file->write( &( info->door[i]->key ) );
		file->write( &( info->door[i]->value ) );
	}
	file->write( &( info->secret_count ) );
	for ( int i = 0; i < static_cast<int>( info->secret_count ); i++ ) {
		file->write( &( info->secret[i]->key ) );
		file->write( &( info->secret[i]->value ) );
	}
	for ( int x = 0; x < MAP_WIDTH; x++ ) {
		for ( int y = 0; y < MAP_DEPTH; y++ ) {
			file->write( &( info->fog_info.fog[x][y] ) );
			//for( int i = 0; i < 4; i++ ) {
			//file->write( &(info->fog_info.players[x + (y * MAP_WIDTH)][i] ) );
			//}
		}
	}
	file->write( &( info->heightMapEnabled ) );
	for ( int x = 0; x < MAP_TILES_X; x++ ) {
		for ( int y = 0; y < MAP_TILES_Y; y++ ) {
			file->write( &( info->ground[ x ][ y ] ) );
			file->write( &( info->climate[ x ][ y ] ) );
			file->write( &( info->vegetation[ x ][ y ] ) );
		}
	}
	file->write( &( info->trapCount ) );
	for ( int i = 0; i < info->trapCount; i++ ) {
		saveTrap( file, info->trap[ i ] );
	}
	file->write( &( info->generatorCount ) );
	for( int i = 0; i < info->generatorCount; i++ ) {
		saveGenerator( file, info->generator[ i ] );
	}
	file->write( &( info->outdoorTextureInfoCount ) );
	for ( int x = 0; x < static_cast<int>( info->outdoorTextureInfoCount ); x++ ) {
		OutdoorTextureInfo *oti = info->outdoorTexture[ x ];
		file->write( &( oti->x ) );
		file->write( &( oti->y ) );
		file->write( &( oti->z ) );
		file->write( &( oti->angle ) );
		file->write( &( oti->horizFlip ) );
		file->write( &( oti->vertFlip ) );
		file->write( &( oti->offsetX ) );
		file->write( &( oti->offsetY ) );
		file->write( oti->groundTextureName, 255 );
	}
}

// FIXME: reuse this in loadmap
void Persist::loadMapHeader( File *file, Uint16 *gridX, Uint16 *gridY, Uint32 *version ) {
	Uint8 i8;
	Uint16 i16;
	file->read( version );
	if ( *version >= 24 ) {
		file->read( &i8 );
	}
	file->read( &i16 );
	file->read( &i16 );
	file->read( gridX );
	file->read( gridY );
}

/// Loads a level map from disk.

/// The map can be an edited base map from the data directory
/// or a map that is stored together with a savegame.

MapInfo *Persist::loadMap( File *file ) {
	MapInfo *info = new  MapInfo;
	file->read( &( info->version ) );
	if ( info->version < PERSIST_VERSION ) {
		cerr << "*** Warning: loading older map file: v" << info->version <<
		" vs. v" << PERSIST_VERSION << endl;
	}
	if ( info->version >= 24 ) {
		file->read( &( info->map_type ) );
	} else {
		info->map_type = 1; // default to room-type: MapRenderHelper::ROOM_HELPER
	}
	file->read( &( info->start_x ) );
	file->read( &( info->start_y ) );
	if( info->version >= 42 ) {
		file->read( &( info->map_start_x ) );
		file->read( &( info->map_start_y ) );
		file->read( &( info->map_end_x ) );
		file->read( &( info->map_end_y ) );
	} else {
		info->map_start_x = info->map_start_y = 0;
		info->map_end_x = MAP_WIDTH;
		info->map_end_y = MAP_DEPTH;
	}
	file->read( &( info->grid_x ) );
	file->read( &( info->grid_y ) );
	file->read( info->theme_name, 255 );
	if ( info->version >= 21 ) {
		file->read( &( info->hasWater ) );
	} else {
		info->hasWater = 0;
	}
	if ( info->version >= 27 ) {
		file->read( &( info->reference_type ) );
	} else {
		info->reference_type = REF_TYPE_NAME;
	}
	if ( info->version >= 32 ) {
		file->read( &( info->edited ) );
	} else {
		info->edited = true;
	}
	file->read( &( info->pos_count ) );
	for ( int i = 0; i < static_cast<int>( info->pos_count ); i++ ) {
		info->pos[i] = new LocationInfo;
		file->read( &( info->pos[i]->x ) );
		file->read( &( info->pos[i]->y ) );
		file->read( &( info->pos[i]->z ) );

		file->read( info->pos[i]->floor_shape_name );
		if ( info->pos[i]->floor_shape_name[0] ) {
			file->read( info->pos[i]->floor_shape_name + 1, 254 );
		}

		strcpy( ( char* )( info->pos[i]->item_pos_name ), "" );
		info->pos[i]->item_pos = NULL;
		if ( info->version >= 19 ) {
			Uint8 hasItemPos;
			file->read( &( hasItemPos ) );
			if ( hasItemPos ) {
				if ( info->reference_type == REF_TYPE_NAME ) {
					file->read( info->pos[i]->item_pos_name, 255 );
				} else {
					info->pos[i]->item_pos = loadItem( file );
				}
			}
		}

		file->read( info->pos[i]->shape_name );
		if ( info->pos[i]->shape_name[0] ) {
			file->read( info->pos[i]->shape_name + 1, 254 );
		}

		strcpy( ( char* )( info->pos[i]->item_name ), "" );
		info->pos[i]->item = NULL;
		Uint8 hasItem;
		file->read( &( hasItem ) );
		if ( hasItem ) {
			if ( info->reference_type == REF_TYPE_NAME ) {
				file->read( info->pos[i]->item_name, 255 );
			} else {
				info->pos[i]->item = loadItem( file );
			}
		}

		strcpy( ( char* )( info->pos[i]->monster_name ), "" );
		info->pos[i]->creature = NULL;
		Uint8 hasCreature;
		file->read( &( hasCreature ) );
		if ( hasCreature ) {
			if ( info->reference_type == REF_TYPE_NAME ) {
				file->read( info->pos[i]->monster_name, 255 );
			} else {
				info->pos[i]->creature = loadCreature( file );
			}
		}

		if ( info->version >= 26 ) {
			Uint8 hasDeity;
			file->read( &( hasDeity ) );
			if ( hasDeity ) {
				file->read( info->pos[i]->magic_school_name, 255 );
			} else strcpy( ( char* )( info->pos[i]->magic_school_name ), "" );
		} else strcpy( ( char* )( info->pos[i]->magic_school_name ), "" );

		if ( info->version < 22 ) {
			// old door info (now unused)
			Uint8 locked;
			Uint16 key_x, key_y, key_z;

			file->read( &( locked ) );
			file->read( &( key_x ) );
			file->read( &( key_y ) );
			file->read( &( key_z ) );
		}
	}
	if ( info->version >= 20 ) {
		file->read( &( info->rug_count ) );
		for ( int i = 0; i < static_cast<int>( info->rug_count ); i++ ) {
			info->rugPos[i] = new RugInfo;
			file->read( &( info->rugPos[i]->cx ) );
			file->read( &( info->rugPos[i]->cy ) );
			file->read( &( info->rugPos[i]->texture ) );
			file->read( &( info->rugPos[i]->isHorizontal ) );
			file->read( &( info->rugPos[i]->angle ) );
		}
	} else {
		info->rug_count = 0;
	}
	if ( info->version >= 22 ) {
		file->read( &( info->locked_count ) );
		for ( int i = 0; i < static_cast<int>( info->locked_count ); i++ ) {
			info->locked[i] = new LockedInfo;
			file->read( &( info->locked[i]->key ) );
			file->read( &( info->locked[i]->value ) );
		}
		file->read( &( info->door_count ) );
		for ( int i = 0; i < static_cast<int>( info->door_count ); i++ ) {
			info->door[i] = new DoorInfo;
			file->read( &( info->door[i]->key ) );
			file->read( &( info->door[i]->value ) );
		}
	} else {
		info->locked_count = info->door_count = 0;
	}
	if ( info->version >= 23 ) {
		file->read( &( info->secret_count ) );
		for ( int i = 0; i < static_cast<int>( info->secret_count ); i++ ) {
			info->secret[i] = new LockedInfo;
			file->read( &( info->secret[i]->key ) );
			file->read( &( info->secret[i]->value ) );
		}
	} else {
		info->secret_count = 0;
	}
	if ( info->version >= 25 ) {
		for ( int x = 0; x < MAP_WIDTH; x++ ) {
			for ( int y = 0; y < MAP_DEPTH; y++ ) {
				file->read( &( info->fog_info.fog[x][y] ) );
				//for( int i = 0; i < 4; i++ ) {
				//file->read( &(info->fog_info.players[x + (y * MAP_WIDTH)][i] ) );
				//}
			}
		}
	} else {
		for ( int x = 0; x < MAP_WIDTH; x++ ) {
			for ( int y = 0; y < MAP_DEPTH; y++ ) {
				info->fog_info.fog[x][y] = 0; // FOG_UNVISITED
				for ( int i = 0; i < 4; i++ ) {
					info->fog_info.players[x + ( y * MAP_WIDTH )][i] = 5; // no player
				}
			}
		}
	}
	if ( info->version >= 34 ) {
		file->read( &( info->heightMapEnabled ) );
		for ( int x = 0; x < MAP_TILES_X; x++ ) {
			for ( int y = 0; y < MAP_TILES_Y; y++ ) {
				file->read( &( info->ground[ x ][ y ] ) );
				if( info->version >= 42 ) {
					file->read( &( info->climate[ x ][ y ] ) );
					file->read( &( info->vegetation[ x ][ y ] ) );
				} else {
					info->climate[ x ][ y ] = info->vegetation[ x ][ y ] = 0;
				}
			}
		}
	}
	if ( info->version >= 37 ) {
		file->read( &( info->trapCount ) );
		for ( int i = 0; i < info->trapCount; i++ ) {
			info->trap[ i ] = loadTrap( file );
		}
	} else {
		info->trapCount = 0;
	}
	if( info->version >= 49 ) {
		file->read( &( info->generatorCount ) );
		for( int i = 0; i < info->generatorCount; i++ ) {
			info->generator[i] = loadGenerator( file );
		}
	} else {
		info->generatorCount = 0;
	}
	if ( info->version >= 40 ) {
		file->read( &( info->outdoorTextureInfoCount ) );
		for ( int x = 0; x < static_cast<int>( info->outdoorTextureInfoCount ); x++ ) {
			OutdoorTextureInfo *oti = new OutdoorTextureInfo;
			file->read( &( oti->x ) );
			file->read( &( oti->y ) );
			file->read( &( oti->z ) );
			file->read( &( oti->angle ) );
			file->read( &( oti->horizFlip ) );
			file->read( &( oti->vertFlip ) );
			file->read( &( oti->offsetX ) );
			file->read( &( oti->offsetY ) );
			file->read( oti->groundTextureName, 255 );
			info->outdoorTexture[ x ] = oti;
		}
	} else {
		info->outdoorTextureInfoCount = 0;
	}
	return info;
}

void Persist::deleteMapInfo( MapInfo *info ) {
	for ( int i = 0; i < static_cast<int>( info->pos_count ); i++ ) {
		if ( info->pos[i]->item_pos ) deleteItemInfo( info->pos[i]->item_pos );
		if ( info->pos[i]->item ) deleteItemInfo( info->pos[i]->item );
		if ( info->pos[i]->creature ) deleteCreatureInfo( info->pos[i]->creature );
		delete info->pos[i];
	}
	for ( int i = 0; i < static_cast<int>( info->rug_count ); i++ ) {
		delete info->rugPos[i];
	}
	for ( int i = 0; i < static_cast<int>( info->locked_count ); i++ ) {
		delete info->locked[i];
	}
	for ( int i = 0; i < static_cast<int>( info->door_count ); i++ ) {
		delete info->door[i];
	}
	for ( int i = 0; i < static_cast<int>( info->secret_count ); i++ ) {
		delete info->secret[i];
	}
	for ( int i = 0; i < static_cast<int>( info->trapCount ); i++ ) {
		deleteTrapInfo( info->trap[i] );
	}
	for ( int i = 0; i < static_cast<int>( info->generatorCount ); i++ ) {
		deleteGeneratorInfo( info->generator[i] );
	}
	for ( int i = 0; i < static_cast<int>( info->outdoorTextureInfoCount ); i++ ) {
		delete info->outdoorTexture[i];
	}
	delete info;
}

void Persist::deleteCreatureInfo( CreatureInfo *info ) {
	for ( int i = 0; i < static_cast<int>( info->backpack_count ); i++ ) {
		deleteItemInfo( info->backpack[i] );
	}
	if ( info->npcInfo ) {
		deleteNpcInfoInfo( info->npcInfo );
	}
	delete info;
}



void Persist::deleteMissionInfo( MissionInfo *info ) {
	delete info;
}

void Persist::saveCreature( File *file, CreatureInfo *info ) {
	file->write( &( info->version ) );
	file->write( info->name, 255 );
	file->write( info->character_name, 255 );
	file->write( &info->character_model_info_index );
	file->write( &info->deityIndex );
	file->write( info->monster_name, 255 );
	file->write( &( info->hp ) );
	file->write( &( info->mp ) );
	file->write( &( info->exp ) );
	file->write( &( info->level ) );
	file->write( &( info->money ) );
	file->write( &( info->stateMod ) );
	file->write( &( info->protStateMod ) );
	file->write( &( info->x ) );
	file->write( &( info->y ) );
	file->write( &( info->z ) );
	file->write( &( info->dir ) );
	file->write( &( info->speed ) );
	file->write( &( info->motion ) );
	file->write( &( info->sex ) );
	file->write( &( info->armor ) );
	file->write( &( info->bonusArmor ) );
	file->write( &( info->thirst ) );
	file->write( &( info->hunger ) );
	file->write( &( info->availableSkillPoints ) );
	file->write( info->skills, Skill::SKILL_COUNT );
	file->write( info->skillMod, Skill::SKILL_COUNT );
	file->write( info->skillBonus, Skill::SKILL_COUNT );
	file->write( &info->portraitTextureIndex );
	file->write( &( info->backpack_count ) );
	for ( int i = 0; i < static_cast<int>( info->backpack_count ); i++ ) {
		saveItem( file, info->backpack[i] );
	}
  file->write( info->equipped, Constants::EQUIP_LOCATION_COUNT );
	file->write( &( info->spell_count ) );
	for ( int i = 0; i < static_cast<int>( info->spell_count ); i++ ) {
		file->write( info->spell_name[i], 255 );
	}
	for ( int i = 0; i < 12; i++ ) {
		file->write( info->quick_spell[i], 255 );
	}
	file->write( &( info->boss ) );
	file->write( &( info->mission ) );
	if ( info->npcInfo ) {
		Uint8 n = 1;
		file->write( &n );
		saveNpcInfoInfo( file, info->npcInfo );
	} else {
		Uint8 n = 0;
		file->write( &n );
	}
	file->write( &( info->missionId ) );
	file->write( &( info->missionObjectiveIndex ) );
	file->write( info->conversation, 255 );
}

CreatureInfo *Persist::loadCreature( File *file ) {
	CreatureInfo *info = new CreatureInfo;
	file->read( &( info->version ) );
	file->read( info->name, 255 );
	file->read( info->character_name, 255 );
	file->read( &info->character_model_info_index );
	file->read( &info->deityIndex );
	file->read( info->monster_name, 255 );
	file->read( &( info->hp ) );
	file->read( &( info->mp ) );
	file->read( &( info->exp ) );
	file->read( &( info->level ) );
	file->read( &( info->money ) );
	file->read( &( info->stateMod ) );
	file->read( &( info->protStateMod ) );
	file->read( &( info->x ) );
	file->read( &( info->y ) );
	file->read( &( info->z ) );
	file->read( &( info->dir ) );
	file->read( &( info->speed ) );
	file->read( &( info->motion ) );
	if ( info->version >= 16 ) {
		file->read( &( info->sex ) );
	} else {
		info->sex = Constants::SEX_MALE;
	}
	file->read( &( info->armor ) );
	file->read( &( info->bonusArmor ) );
	file->read( &( info->thirst ) );
	file->read( &( info->hunger ) );
	file->read( &( info->availableSkillPoints ) );
	file->read( info->skills, Skill::SKILL_COUNT );
	file->read( info->skillMod, Skill::SKILL_COUNT );
	file->read( info->skillBonus, Skill::SKILL_COUNT );
	file->read( &info->portraitTextureIndex );
	file->read( &( info->backpack_count ) );
	for ( int i = 0; i < static_cast<int>( info->backpack_count ); i++ ) {
		info->backpack[i] = loadItem( file );
	}
	file->read( info->equipped, Constants::EQUIP_LOCATION_COUNT );
	file->read( &( info->spell_count ) );
	for ( int i = 0; i < static_cast<int>( info->spell_count ); i++ ) {
		file->read( info->spell_name[i], 255 );
	}
	for ( int i = 0; i < 12; i++ ) {
		file->read( info->quick_spell[i], 255 );
	}
	if ( info->version >= 35 ) {
		file->read( &( info->boss ) );
	} else {
		info->boss = 0;
	}
	if ( info->version >= 36 ) {
		file->read( &( info->mission ) );
	} else {
		info->mission = 0;
	}
	if ( info->version >= 39 ) {
		Uint8 n;
		file->read( &n );
		info->npcInfo = ( n ? loadNpcInfoInfo( file ) : NULL );
	} else {
		info->npcInfo = NULL;
	}
	if( info->version >= 46 ) {
		file->read( &( info->missionId ) );
		file->read( &( info->missionObjectiveIndex ) );
	} else {
		info->missionId = info->missionObjectiveIndex = 0; 
	}
	if( info->version >= 50 ) {
		file->read( info->conversation, 255 );
	}
	return info;
}



void Persist::saveMission( File *file, MissionInfo *info ) {
	file->write( &( info->version ) );
	file->write( &( info->level ) );
	file->write( &( info->depth ) );
	file->write( &( info->completed ) );
	file->write( info->mapName, 80 );
	file->write( info->templateName, 80 );
	file->write( &( info->itemCount ) );
	for ( int i = 0; i < static_cast<int>( info->itemCount ); i++ ) {
		file->write( info->itemName[ i ], 255 );
		file->write( &( info->itemDone[ i ] ) );
	}
	file->write( &( info->monsterCount ) );
	for ( int i = 0; i < static_cast<int>( info->monsterCount ); i++ ) {
		file->write( info->monsterName[ i ], 255 );
		file->write( &( info->monsterDone[ i ] ) );
	}
	file->write( &( info->missionId ) );
}

MissionInfo *Persist::loadMission( File *file ) {
	MissionInfo *info = new MissionInfo;
	file->read( &( info->version ) );
	file->read( &( info->level ) );
	file->read( &( info->depth ) );
	if ( info->version >= 29 ) {
		file->read( &( info->completed ) );
	} else {
		info->completed = 0;
	}
	file->read( info->mapName, 80 );
	file->read( info->templateName, 80 );
	file->read( &( info->itemCount ) );
	for ( int i = 0; i < static_cast<int>( info->itemCount ); i++ ) {
		file->read( info->itemName[ i ], 255 );
		file->read( &( info->itemDone[ i ] ) );
	}
	file->read( &( info->monsterCount ) );
	for ( int i = 0; i < static_cast<int>( info->monsterCount ); i++ ) {
		file->read( info->monsterName[ i ], 255 );
		file->read( &( info->monsterDone[ i ] ) );
	}
	if ( info->version >= 38 ) {
		file->read( &( info->missionId ) );
	} else {
		info->missionId = Constants::getNextMissionId();
		cerr << "*** warning: mission file has no id. \
		Likely mission objects will not be found. \
		You should redo this mission." << endl;
	}
	return info;
}

