/***************************************************************************
                item.h  -  Class representing a specific item
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

#ifndef ITEM_H
#define ITEM_H
#pragma once

#include "persist.h"
#include "render/rendereditem.h"
#include "storable.h"
#include "rpg/rpg.h"
#include <vector>
#include <map>
#include "rpg/rpgitem.h"


class RpgItem;
class Color;
class GLShape;
class Spell;
class MagicSchool;
class Dice;
class Session;
class ShapePalette;
class ConfigLang;
class ConfigNode;
class Scourge;
class Texture;
class Creature;

/**
  *@author Gabor Torok
  */

/// This class defines attributes and appearance of an item.

/// This class is both the UI representation (shape) of the rpgItem and it's state (for example, wear).
/// All instances of the RpgItem point to the same RpgItem, but a new Item is created for each.

class Item : public RenderedItem, Storable {
public:

	#define GRID_SIZE 32

	enum {
		ID_BONUS = 0,
		ID_DAMAGE_MUL,
		ID_MAGIC_DAMAGE,
		ID_STATE_MOD,
		ID_PROT_STATE_MOD,
		ID_SKILL_BONUS,
		ID_CURSED,
		//number of bits that represents item identification
		ID_COUNT,
		ITEM_NAME_SIZE = 255
	};

	Item( Session *session, RpgItem *rpgItem, int level = 1, bool loading = false );
	~Item();
	
	inline void setInventoryOf( Creature *creature ) { inventoryOf = creature; }
	inline Creature *getInventoryOf() { return inventoryOf; }

	inline void setMissionObjectInfo( int missionId, int objectiveIndex ) {
		this->missionId = missionId;
		this->missionObjectiveIndex = objectiveIndex;
	}
	inline int getMissionId() {
		return missionId;
	}
	inline int getMissionObjectiveIndex() {
		return missionObjectiveIndex;
	}

	void renderIcon( Scourge *scourge, SDL_Rect *rect, int gridSize = GRID_SIZE, bool smallIcon = false );
	void renderIcon( Scourge *scourge, int x, int y, int w, int h, bool smallIcon = false );
	Texture getItemIconTexture( bool smallIcon = false );
	void getTooltip( char *tooltip );

	inline void setBackpackLocation( int x, int y ) {
		backpackX = x;
		backpackY = y;
	}
	/// x position in the backpack.
	inline int getBackpackX() {
		return backpackX;
	}
	/// y position in the backpack.
	inline int getBackpackY() {
		return backpackY;
	}
	int getBackpackWidth();
	int getBackpackHeight();

	inline void setIdentifiedBit( int bit, bool value ) {
		if ( value ) identifiedBits |= ( 1 << bit );
		else identifiedBits &= ( ( Uint32 )0xffff - ( Uint32 )( 1 << bit ) );
	}
	/// Checks whether a special item property has been identified.
	inline bool getIdentifiedBit( int bit ) {
		return( identifiedBits & ( 1 << bit ) ? true : false );
	}
	void identify( int infoDetailLevel );
	/// Returns true if all bits in identifiedBits are set to true, i.e. fully identified.
	inline bool isIdentified() {
		return( isMagicItem() && identifiedBits >= ( 1 << ID_COUNT ) - 1 );
	}
	bool isFullyIdentified();


	ItemInfo *save();
	//ContainedItemInfo saveContainedItems();
	static Item *load( Session *session, ItemInfo *info );

	/// Unused.
	inline Color *getColor() {
		return color;
	}
	/// Unused.
	inline void setColor( Color *c ) {
		color = c;
	}
	inline void setShape( GLShape *s ) {
		shape = s;
	}
	/// The item's 3D model.
	inline GLShape *getShape() {
		return shape;
	}
	/// This specific item's base item.
	inline RpgItem *getRpgItem() {
		return rpgItem;
	}
	inline bool isBlocking() {
		return blocking;
	}
	inline void setBlocking( bool b ) {
		blocking = b;
	}
	/// Number of remaining charges/uses.
	inline int getCurrentCharges() {
		return currentCharges;
	}
	void setCurrentCharges( int n );
	inline void setWeight( float f ) {
		if ( f < 0.0f )f = 0.1f; weight = f;
	}
	void setSpell( Spell *spell );
	inline Spell *getSpell() {
		return spell;
	}

	/// Sets whether the item's cursed status should be hidden or not.
	inline void setShowCursed( bool b ) {
		showCursed = b;
	}
	inline bool getShowCursed() {
		return showCursed;
	}

	void getDetailedDescription( std::string& s, bool precise = true );
	/// The item's localized name.
	inline char const* getItemName() {
		return itemName;
	}

	/// Number of other items this item contains.
	inline int getContainedItemCount() {
		return containedItemCount;
	}
	bool addContainedItem( Item *item, int itemX=0, int itemY=0, bool force=false );
	void removeContainedItem( Item *item );
	Item *getContainedItem( int index );
	void setContainedItem( int index, Item *item );
	bool isContainedItem( Item *item );
	/// Returns true if the item contains magical items.
	inline bool getContainsMagicItem() {
		return containsMagicItem;
	}

	bool decrementCharges();

	const std::string getRandomSound() {
		return rpgItem->getRandomSound();
	}

	void enchant( int level );

	char const* getType();


	// level-based attributes
	/// The item's level.
	inline int getLevel() {
		return level;
	}
	/// The item's weight.
	inline float getWeight() {
		return weight;
	}
	/// Price of the item without trade boni/mali.
	inline int getPrice() {
		return price;
	}
	/// Unused.
	inline int getQuality() {
		return quality;
	}

	inline bool isMagicItem() {
		return ( magicLevel > -1 );
	}
	bool isSpecial();
	inline std::map<int, int> *getSkillBonusMap() {
		return &skillBonus;
	}
	/// The bonus the item gives for the specified skill.
	inline int getSkillBonus( int skill ) {
		return ( skillBonus.find( skill ) == skillBonus.end() ? 0 : skillBonus[skill] );
	}
	/// The magic level (none, lesser, greater, champion, divine)
	inline int getMagicLevel() {
		return magicLevel;
	}
	inline int getBonus() {
		return bonus;
	}
	/// Returns the "x times damage against ..." bonus.
	inline int getDamageMultiplier() {
		return damageMultiplier;
	}
	/// The monster type the damage multiplier applies to.
	inline char const* getMonsterType() {
		return monsterType;
	}
	/// Magic school of the attached spell.
	inline MagicSchool *getSchool() {
		return school;
	}
	int rollMagicDamage();
	int getMagicResistance();
	char *describeMagicDamage();
	/// "Cursed" state of the item.
	inline bool isCursed() {
		return cursed;
	}
	inline void setCursed( bool b ) {
		this->cursed = b;
	}
	/// Does it set the specified state mod?
	inline bool isStateModSet( int mod ) {
		return( stateMod[mod] == 1 );
	}
	/// Does it protect against the specified state mod?
	inline bool isStateModProtected( int mod ) {
		return( stateMod[mod] == 2 );
	}
	int getRange();

	void debugMagic( char *s );

	// storable interface
	const char *getName();
	int getIconTileX();
	int getIconTileY();
	int getStorableType();
	const char *isStorable();
	
	Texture getContainerTexture();

private:
	Creature *inventoryOf;
	RpgItem *rpgItem;
	int shapeIndex;
	Color *color;
	GLShape *shape;
	bool blocking;
	Item *containedItems[MAX_CONTAINED_ITEMS];
	int containedItemCount;
	int currentCharges;
	Spell *spell;
	char itemName[ ITEM_NAME_SIZE ];
	bool containsMagicItem;
	bool showCursed;
	// unused: GLuint tex3d[1];
	// unused: unsigned char * textureInMemory;
	void trySetIDBit( int bit, float modifier, int infoDetailLevel );

	static const int PARTICLE_COUNT = 30;
	Uint32 iconEffectTimer;
	ParticleStruct *iconEffectParticle[PARTICLE_COUNT];
	Uint32 iconUnderEffectTimer;
	ParticleStruct *iconUnderEffectParticle[PARTICLE_COUNT];

	// Things that change with item level (override rpgitem values)
	int level;
	float weight;
	int price;
	int quality;

	// former magic attrib stuff
	int magicLevel;
	int bonus; // e.g.: sword +2
	int damageMultiplier; // 2=double damage, 3=triple, etc.
	char const* monsterType; // if not NULL, damageMultiplier only for this type of monster.
	MagicSchool *school; // magic damage by a school (or NULL if N/A)
	Dice *magicDamage;
	bool cursed;
	int stateMod[StateMod::STATE_MOD_COUNT]; // 0=nothing, 1=sets, 2=clears/protects against state mod when worn
	bool stateModSet;
	std::map<int, int> skillBonus;
	Session *session;
	Uint32 identifiedBits;
	int backpackX, backpackY;
	int missionId;
	int missionObjectiveIndex;
	// unused: std::map<RpgItem*, Texture> containerTextures;
	Texture containerTexture;

protected:
	bool findInventoryPosition( Item *item, int posX, int posY, bool useExistingLocationForSameItem );
	bool checkInventoryLocation( Item *item, bool useExistingLocationForSameItem, int posX, int posY );
	
	void commonInit( bool loading );
	void describeMagic( char const* displayName );

	DiceInfo *saveDice( Dice *dice );
	static DiceInfo *saveEmptyDice();
	static Dice *loadDice( Session *session, DiceInfo *info );

	void renderItemIcon( Scourge *scourge, int x, int y, int w, int h, bool smallIcon = false );
	void renderItemIconEffect( Scourge *scourge, int x, int y, int w, int h, int iw, int ih );
	void renderItemIconIdentificationEffect( Scourge *scourge, int x, int y, int w, int h );
	void renderUnderItemIconEffect( Scourge *scourge, int x, int y, int w, int h, int iw, int ih );
	// unused: void create3dTex( Scourge *scourge, float w, float h );
	void getItemIconInfo( Texture* texp, int *rwp, int *rhp, int *oxp, int *oyp, int *iw, int *ih, int w, int h, bool smallIcon = false );
};

#endif
