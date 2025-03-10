/***************************************************************************
              creature.cpp  -  A class describing any creature
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
#include "creature.h"
#include "item.h"
#include "pathmanager.h"
#include "render/renderlib.h"
#include "session.h"
#include "shapepalette.h"
#include "events/event.h"
#include "events/potionexpirationevent.h"
#include "events/statemodexpirationevent.h"
#include "events/thirsthungerevent.h"
#include "sqbinding/sqbinding.h"
#include "debug.h"
#include "sound.h"
#include "conversation.h"

using namespace std;

//#define DEBUG_CREATURE_AI 1

//#define DEBUG_INVENTORY 1

// The creature AI
#define AI_DECISION_INTERVAL 1000

#define PERCEPTION_DELTA 2000

bool loading = false;

//#define DEBUG_CAPABILITIES

#define MOVE_DELAY 7

// at this fps, the players step 1 square
#define FPS_ONE 10.0f

// how fast to turn
#define TURN_STEP_COUNT 5

// how far to move away when in the player's way
#define AWAY_DISTANCE 8

// how close to stay to the player
#define CLOSE_DISTANCE 8

/// Describes monster toughness in general.
struct MonsterToughness {
	float minSkillBase, maxSkillBase;
	float minHpMpBase, maxHpMpBase;
	float armorMisfuction;
};

// goes from not very tough to tough
MonsterToughness monsterToughness[] = {
	{  .5f,  0.8f,     .7f,      1,  .33f },
	{ .75f,  0.9f,    .75f,      1,  .15f },
	{  .8f,     1,    .75f,  1.25f,   .5f }
};


Creature::Creature( Session *session, Character *character, char const* name, int sex, int character_model_info_index ) : RenderedCreature( session->getPreferences(), session->getShapePalette(), session->getMap() ) {
	this->session = session;
	this->character = character;
	this->monster = NULL;
	setName( name );
	this->character_model_info_index = character_model_info_index;
	this->model_name = session->getShapePalette()->getCharacterModelInfo( sex, character_model_info_index )->model_name;
	this->skin_name = session->getShapePalette()->getCharacterModelInfo( sex, character_model_info_index )->skin_name;
	this->originalSpeed = this->speed = 5; // start neutral speed
	this->motion = Constants::MOTION_MOVE_TOWARDS;
	this->armor = 0;
	this->armorChanged = true;
	this->bonusArmor = 0;
	this->thirst = 10;
	this->hunger = 10;
	this->shape = session->getShapePalette()->getCreatureShape( model_name.c_str(), skin_name.c_str(), session->getShapePalette()->getCharacterModelInfo( sex, character_model_info_index )->scale );
	this->sex = sex;
	commonInit();
}

Creature::Creature( Session *session, Monster *monster, GLShape *shape, bool initMonster ) : RenderedCreature( session->getPreferences(), session->getShapePalette(), session->getMap() ) {
	this->session = session;
	this->character = NULL;
	this->monster = monster;
	setName( monster->getDisplayName() );
	this->model_name = monster->getModelName();
	this->skin_name = monster->getSkinName();
	this->originalSpeed = this->speed = monster->getSpeed();
	this->motion = Constants::MOTION_LOITER;
	this->armor = monster->getBaseArmor();
	this->armorChanged = true;
	this->bonusArmor = 0;
	this->shape = shape;
	this->sex = Constants::SEX_MALE;
	commonInit();
	this->level = monster->getLevel();
	if ( initMonster ) monsterInit();
}

void Creature::commonInit() {
	this->summoner = NULL;
	this->backpack = new Item( session, RpgItem::getItemByName( "Backpack" ), 1 );
	this->backpack->setInventoryOf( this );
	this->lastDecision = 0;
	this->portrait.clear();

	this->scripted = false;
	this->scriptedAnim = MD2_STAND;
	this->lastPerceptionCheck = 0;
	this->boss = false;
	this->savedMissionObjective = false;

	this->backpackSorted = false;

	this->causeOfDeath[0] = 0;
	( ( AnimatedShape* )shape )->setCreatureSpeed( speed );

	for ( int i = 0; i < RpgItem::DAMAGE_TYPE_COUNT; i++ ) {
		lastArmor[i] = lastArmorSkill[i] = lastDodgePenalty[i] = 0;
	}
	for ( int i = 0; i < 12; i++ ) quickSpell[ i ] = NULL;
	this->lastMove = 0;
	this->moveCount = 0;
	this->x = this->y = this->z = 0;
	this->dir = Constants::MOVE_UP;
	this->formation = DIAMOND_FORMATION;
	this->tx = this->ty = -1;
	this->selX = this->selY = -1;
	this->cantMoveCounter = 0;
	this->pathManager = new PathManager( this );

	this->preferredWeapon = -1;
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		equipped[i] = MAX_BACKPACK_SIZE;
	}
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		skillBonus[i] = skillsUsed[i] = skillMod[i] = 0;
	}
	//this->stateMod = ( 1 << StateMod::dead ) - 1;
	//this->protStateMod = ( 1 << StateMod::dead ) - 1;
	this->stateMod = 0;
	this->protStateMod = 0;
	this->level = 1;
	this->experience = 0;
	this->hp = 0;
	this->mp = 0;
	this->startingHp = 0;
	this->startingMp = 0;
	this->ac = 0;
	this->targetCreature = NULL;
	this->targetX = this->targetY = this->targetZ = 0;
	this->targetItem = NULL;
	this->lastTick = 0;
	this->lastTurn = 0;
	this->facingDirection = Constants::MOVE_UP; // good init ?
	this->failedToMoveWithinRangeAttemptCount = 0;
	this->action = Constants::ACTION_NO_ACTION;
	this->actionItem = NULL;
	this->actionSpell = NULL;
	this->actionSkill = NULL;
	this->preActionTargetCreature = NULL;
	this->angle = this->wantedAngle = this->angleStep = 0;
	this->portraitTextureIndex = 0;
	this->deityIndex = -1;
	this->availableSkillMod = 0;
	this->hasAvailableSkillPoints = false;

	// Yes, monsters have backpack weight issues too
	backpackWeight =  0.0f;
	for ( int i = 0; i < backpack->getContainedItemCount(); i++ ) {
		backpackWeight += backpack->getContainedItem( i )->getWeight();
	}
	this->money = this->level * Util::dice( 10 );
	calculateExpOfNextLevel();
	this->battle = new Battle( session, this );

	lastEnchantDate.setDate( -1, -1, -1, -1, -1, -1 );

	this->npcInfo = NULL;
	this->mapChanged = false;
	this->moving = false;

	evalSpecialSkills();

	// ref the default conversation
	this->conversation = Conversation::ref( this, "general", session->getGameAdapter() );
}

Creature::~Creature() {
	closestEnemy = closestFriend = NULL;
	closestFriends.clear();
	closestEnemies.clear();
	portrait.clear();

	// cancel this creature's events
	Party* party = session->getParty();
	if ( party != NULL ) party->getCalendar()->cancelEventsForCreature( this );

	// now delete the creature
	session->getGameAdapter()->removeBattle( battle );
	delete battle;
	delete pathManager;
	// delete the md2/3 shape
	ShapePalette* shapepal = session->getShapePalette();
	if ( shapepal != NULL ) {
		shapepal->decrementSkinRefCountAndDeleteShape( model_name.c_str(),
		                                               skin_name.c_str(),
		                                               shape,
		                                               monster );
	}
	// delete the backpack infos
	for ( map<Item*, BackpackInfo*>::iterator e = invInfos.begin(); e != invInfos.end(); ++e ) {
		BackpackInfo *info = e->second;
		delete info;
	}
	delete backpack;
	
	if( conversation ) {
		Conversation::unref( this, conversation );
		conversation = NULL;
	}
}

/// Changes a character-type creature's profession, and applies the effects.

void Creature::changeProfession( Character *c ) {
	enum { MSG_SIZE = 120 };
	char message[ MSG_SIZE ];

	// boost skills
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		int maxValue = c->getSkill( i );
		if ( maxValue > 0 ) {
			int oldValue = character->getSkill( i );
			int newValue = getSkill( i ) + ( oldValue > 0 ? maxValue - oldValue : maxValue );
			setSkill( i, newValue );

			snprintf( message, MSG_SIZE, _( "%1$s's skill in %2$s has increased." ), getName(), Skill::skills[ i ]->getDisplayName() );
			session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_STATS );
		}
	}

	// remove forbidden items
	for( int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++ ) {
		if ( equipped[i] < MAX_BACKPACK_SIZE ) {
			Item *item = backpack->getContainedItem( equipped[i] );
			if ( !c->canEquip( item->getRpgItem() ) ) {
				doff( equipped[i] );
				snprintf( message, MSG_SIZE, _( "%1$s is not allowed to equip %2$s." ), getName(), item->getName() );
				session->getGameAdapter()->writeLogMessage( message );
			}
		}
	}

	// add capabilities?


	this->character = c;
	setHp();
	setMp();
}

CreatureInfo *Creature::save() {
	CreatureInfo *info =  new CreatureInfo;
	info->version = PERSIST_VERSION;
	strncpy( ( char* )info->name, getName(), 254 );
	info->name[254] = 0;
	if ( isMonster() || isNpc() ) {
		strcpy( ( char* )info->character_name, "" );
		strcpy( ( char* )info->monster_name, monster->getType() );
		info->character_model_info_index = 0;
		info->npcInfo = ( isNpc() && getNpcInfo() ? getNpcInfo()->save() : NULL );
	} else {
		strcpy( ( char* )info->character_name, character->getName() );
		strcpy( ( char* )info->monster_name, "" );
		info->character_model_info_index = character_model_info_index;
		info->npcInfo = NULL;
	}
	info->deityIndex = deityIndex;
	info->hp = hp;
	info->mp = mp;
	info->exp = experience;
	info->level = level;
	info->money = money;
	info->stateMod = stateMod;
	info->protStateMod = protStateMod;
	info->x = toint( x );
	info->y = toint( y );
	info->z = toint( z );
	info->dir = dir;
	info->speed = originalSpeed;
	info->motion = motion;
	info->sex = sex;
	info->armor = 0;
	info->bonusArmor = bonusArmor;
	//info->bonusArmor = 0;
	info->thirst = thirst;
	info->hunger = hunger;
	info->availableSkillPoints = availableSkillMod;
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		info->skills[i] = skills[i];
		info->skillBonus[i] = skillBonus[i];
		info->skillsUsed[i] = skillsUsed[i];
		info->skillMod[i] = skillMod[i];
	}
	info->portraitTextureIndex = portraitTextureIndex;

	// backpack
	info->backpack_count = backpack->getContainedItemCount();
	for ( int i = 0; i < backpack->getContainedItemCount(); i++ ) {
		info->backpack[i] = backpack->getContainedItem( i )->save();
	}
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		info->equipped[i] = equipped[i];
	}

	// spells
	int count = 0;
	for ( int i = 0; i < MagicSchool::getMagicSchoolCount(); i++ ) {
		MagicSchool *school = MagicSchool::getMagicSchool( i );
		for ( int t = 0; t < school->getSpellCount(); t++ ) {
			Spell *spell = school->getSpell( t );
			if ( isSpellMemorized( spell ) ) {
				strcpy( ( char* )info->spell_name[count++], spell->getName() );
			}
		}
	}
	info->spell_count = count;

	for ( int i = 0; i < 12; i++ ) {
		strcpy( ( char* )info->quick_spell[ i ],
		        ( getQuickSpell( i ) ? getQuickSpell( i )->getName() : "" ) );
	}

	info->boss = ( Uint8 )boss;
	//info->mission = ( Uint8 )( session->getCurrentMission() && session->getCurrentMission()->isMissionCreature( this ) ? 1 : 0 );
	
	info->missionId = getMissionId();
	info->missionObjectiveIndex = getMissionObjectiveIndex();

	strcpy( (char*)info->conversation, getConversation() ? getConversation()->getFilename().c_str() : "" );
	return info;
}

Creature *Creature::load( Session *session, CreatureInfo *info ) {
	Creature *creature = NULL;

	if ( !strlen( ( char* )info->character_name ) ) {

		Monster *monster = Monster::getMonsterByName( ( char* )info->monster_name );
		if ( !monster ) {
			cerr << "Error: can't find monster: " << ( char* )info->monster_name << endl;
			return NULL;
		}
		GLShape *shape = session->getShapePalette()->
		                 getCreatureShape( monster->getModelName(),
		                                   monster->getSkinName(),
		                                   monster->getScale(),
		                                   monster );
		creature = session->newCreature( monster, shape, true );
		creature->setName( ( char* )info->name ); // important for npc-s
		if ( info->npcInfo ) {
			NpcInfo *npcInfo = NpcInfo::load( info->npcInfo );
			if ( npcInfo ) creature->setNpcInfo( npcInfo );
		}

		// fixme: throw away this code when saving stats_mods and calendar events is implemented
		// for now, set a monsters stat mods as declared in creatures.txt
		creature->stateMod = monster->getStartingStateMod();
	} else {
		creature = new Creature( session,
		                         Characters::getByName( ( char* )info->character_name ),
		                         ( char* )info->name,
		                         info->sex,
		                         info->character_model_info_index );
	}

	// don't recalculate skills
	// NOTE: don't call return until loading=false.
	loading = true;

//  cerr << "*** LOAD: creature=" << info->name << endl;
	creature->setDeityIndex( info->deityIndex );
	creature->setHp( info->hp );
	creature->setMp( info->mp );
	creature->setExp( info->exp );
	creature->setLevel( info->level );
	creature->setMoney( info->money );
	creature->moveTo( info->x, info->y, info->z );
	creature->setDir( info->dir );
	//creature->setSpeed( info->speed );
	//creature->setMotion( info->motion );
	//creature->setArmor( info->armor );

	// info->bonusArmor: can't be used until calendar is also persisted
	//creature->setBonusArmor( info->bonusArmor );

	creature->setThirst( info->thirst );
	creature->setHunger( info->hunger );
	creature->setAvailableSkillMod( info->availableSkillPoints );

	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		creature->skills[i] = info->skills[i];
		// Don't set skillBonus: it's reconstructed via the backpack.
		//creature->skillBonus[i] = info->skillBonus[i];
		// Don't set skillUsed: it's not used.
		//creature->skillsUsed[i] = info->skillsUsed[i];
		creature->skillMod[i] = info->skillMod[i];
	}

	// stateMod and protStateMod not useful until calendar is also persisted
	//creature->stateMod = info->stateMod;
	//creature->protStateMod = info->protStateMod;
	// these two don't req. events:
	if ( info->stateMod & ( 1 << StateMod::dead ) ) creature->setStateMod( StateMod::dead, true );
	//if(info->stateMod & (1 << Constants::leveled)) creature->setStateMod(Constants::leveled, true);

	// backpack
	//creature->backpack_count = info->backpack_count;
	for ( int i = 0; i < static_cast<int>( info->backpack_count ); i++ ) {
		Item *item = Item::load( session, info->backpack[i] );
		if ( item ) {
			// force add to backpack
			creature->addToBackpack( item, 0, 0, true );
		}
	}
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		if ( info->equipped[i] < MAX_BACKPACK_SIZE ) {
			creature->equipFromBackpack( info->equipped[i], i );
		} else {
			creature->equipped[i] = info->equipped[i];
		}
	}

	creature->portraitTextureIndex = info->portraitTextureIndex;
	if ( creature->portraitTextureIndex >= session->getShapePalette()->getPortraitCount( creature->getSex() ) )
		creature->portraitTextureIndex = session->getShapePalette()->getPortraitCount( creature->getSex() ) - 1;

	// spells
	for ( int i = 0; i < static_cast<int>( info->spell_count ); i++ ) {
		Spell *spell = Spell::getSpellByName( ( char* )info->spell_name[i] );
		creature->addSpell( spell );
	}

	for ( int i = 0; i < 12; i++ ) {
		if ( strlen( ( char* )info->quick_spell[ i ] ) ) {
			Spell *spell = Spell::getSpellByName( ( char* )info->quick_spell[ i ] );
			if ( spell ) creature->setQuickSpell( i, spell );
			else {
				SpecialSkill *special = SpecialSkill::findByName( ( char* )info->quick_spell[ i ], false );
				if ( special ) creature->setQuickSpell( i, special );
				else {
					// it's an item. Find it
					for ( int t = 0; t < creature->getBackpackContentsCount(); t++ ) {
						Item *item = creature->getBackpackItem( t );
						if ( !strcmp( item->getName(), ( char* )info->quick_spell[ i ] ) ) {
							creature->setQuickSpell( i, ( Storable* )item );
							break;
						}
					}
				}
			}
		}
	}

	creature->setBoss( info->boss != 0 );
//	creature->setSavedMissionObjective( info->mission != 0 );
//	if ( creature->isSavedMissionObjective() ) {
//		cerr << "*********************************" << endl;
//		cerr << "Loaded mission creature:" << creature->getName() << endl;
//		cerr << "*********************************" << endl;
//	}
	creature->setMissionObjectInfo( static_cast<int>( info->missionId ), static_cast<int>( info->missionObjectiveIndex ) );

	creature->calculateExpOfNextLevel();

	creature->evalSpecialSkills();
	
	if( strlen( (char*)info->conversation ) > 0 ) {
		string fn = (char*)info->conversation;
		creature->setConversation( fn );
	}

	// recalculate skills from now on
	loading = false;

	return creature;
}

/// Returns the UNLOCALIZED name of a character/monster/NPC.

char const* Creature::getType() {
	return( monster ? monster->getType() : character->getName() );
}

/// Gets the number of experience points required to level up.

void Creature::calculateExpOfNextLevel() {
	if ( !( isPartyMember() || isWanderingHero() ) ) return;
	expOfNextLevel = 0;
	for ( int i = 0; i < level; i++ ) {
		expOfNextLevel += ( ( i + 1 ) * character->getLevelProgression() );
	}
}

/// Selects (with a 1/10 chance) a random new movement direction.

void Creature::switchDirection( bool force ) {
	int n = Util::dice( 10 );
	if ( n == 0 || force ) {
		int dir = Util::dice( 4 );
		switch ( dir ) {
		case 0: setDir( Constants::MOVE_UP ); break;
		case 1: setDir( Constants::MOVE_DOWN ); break;
		case 2: setDir( Constants::MOVE_LEFT ); break;
		case 3: setDir( Constants::MOVE_RIGHT ); break;
		}
	}
}

/// Moves the creature 1 step into the specified direction.

bool Creature::move( Uint16 dir ) {
	//if(character) return false;

	// is monster (or npc) doing something else?
	if ( ( ( AnimatedShape* )getShape() )->getCurrentAnimation() != MD2_RUN ) return false;

	switchDirection( false );

	// a hack for runaway creatures
	if ( !( x > 10 && x < MAP_WIDTH - 10 &&
	        y > 10 && y < MAP_DEPTH - 10 ) ) {
		if ( monster ) cerr << "hack for " << getName() << endl;
		switchDirection( true );
		return false;
	}

	GLfloat nx = x;
	GLfloat ny = y;
	GLfloat nz = z;
	GLfloat step = getStep();
	switch ( dir ) {
	case Constants::MOVE_UP:
		ny = y - step;
		break;
	case Constants::MOVE_DOWN:
		ny = y + step;
		break;
	case Constants::MOVE_LEFT:
		nx = x - step;
		break;
	case Constants::MOVE_RIGHT:
		nx = x + step;
		break;
	}
	setFacingDirection( dir );

	if ( !session->getMap()->moveCreature( toint( x ), toint( y ), toint( z ),
	        toint( nx ), toint( ny ), toint( nz ), this ) ) {
		( ( AnimatedShape* )shape )->setDir( dir );
		if ( session->getMap()->getHasWater() &&
		        !( toint( x ) == toint( nx ) &&
		           toint( y ) == toint( ny ) ) ) {
			session->getMap()->startEffect( toint( getX() + getShape()->getWidth() / 2 ),
			    toint( getY() - getShape()->getDepth() / 2 ), 0,
			    Constants::EFFECT_RIPPLE, ( Constants::DAMAGE_DURATION * 4 ),
			    getShape()->getWidth(), getShape()->getDepth() );
		}
		moveTo( nx, ny, nz );
		setDir( dir );
		return true;
	} else {
		switchDirection( true );
		return false;
	}
}

/// Searches for a creature target within the specified range.

void Creature::setTargetCreature( Creature *c, bool findPath, float range ) {
	targetCreature = c;
	if ( findPath ) {
		if ( !setSelCreature( c , range, false ) ) {
			// FIXME: should mark target somehow. Path alg. cannot reach it; blocked by something.
			// Keep the target creature anyway.
			if ( session->getPreferences()->isBattleTurnBased() ) {

				session->getGameAdapter()->writeLogMessage( _( "Can't find path to target. Sorry!" ), Constants::MSGTYPE_SYSTEM );
				session->getGameAdapter()->setCursorMode( Constants::CURSOR_FORBIDDEN );
			}
		}
	}
}

/// Makes the creature follow another creature. Returns true for success.

bool Creature::follow( Creature *leader ) {
	float dist = getDistance( leader );
	if ( dist < CLOSE_DISTANCE ) {
		if ( cantMoveCounter > 5 ) {
			cantMoveCounter = 0;
		} else {
			return true;
		}
	}
	//speed = FAST_SPEED;
	// Creature *player = session->getParty()->getPlayer();
	//bool found = pathManager->findPathToCreature( leader, player, session->getMap());
	return setSelCreature( leader, 1 );
}

/// Sets where to move the creature. Returns true if the move is possible, false otherwise.

bool Creature::setSelXY( int x, int y, bool cancelIfNotPossible ) {
	
	if( !session->getParty() || !session->getParty()->getPlayer() ) return false;
	
	bool ignoreParty = session->getParty()->getPlayer() == this && !session->getGameAdapter()->inTurnBasedCombat();
	int oldSelX = selX;
	int oldSelY = selY;
	int oldtx = tx;
	int oldty = ty;

	selX = x;
	selY = y;
	if ( x < 0 || y < 0 ) return true; //this is often used to deselect

	setMotion( Constants::MOTION_MOVE_TOWARDS );

	// find the path
	tx = selX;
	ty = selY;
	// Does the path lead to the destination?
	bool ret = pathManager->findPath( selX, selY,
	           session->getParty()->getPlayer(),
	           session->getMap(),
	           ignoreParty );

	/**
	 * For pc-s cancel the move.
	 */
	if ( !ret && character && cancelIfNotPossible ) {
		pathManager->clearPath();
		selX = oldSelX;
		selY = oldSelY;
		tx = oldtx;
		ty = oldty;
	} else {
		//make the selected location equal the end of our path
		Location last = pathManager->getEndOfPath();
		selX = last.x;
		selY = last.y;
	}

	// FIXME: when to play sound?
	if ( ret && session->getParty()->getPlayer() == this ) {
		// play command sound
		if ( 0 == Util::dice(  session->getPreferences()->getSoundFreq() ) &&
		        !getStateMod( StateMod::dead ) ) {
			//session->playSound(getCharacter()->getRandomSound(Constants::SOUND_TYPE_COMMAND));
			int panning = session->getMap()->getPanningFromMapXY( this->x, this->y );
			playCharacterSound( GameAdapter::COMMAND_SOUND, panning );
		}
	}

	return ret;
}

/// Use this instead of setSelXY when targetting creatures so that it will check all locations occupied by large creatures.

bool Creature::setSelCreature( Creature* creature, float range, bool cancelIfNotPossible ) {
	bool ignoreParty = session->getParty()->getPlayer() == this && !session->getGameAdapter()->inTurnBasedCombat();
	int oldSelX = selX;
	int oldSelY = selY;
	int oldtx = tx;
	int oldty = ty;

	selX = toint( creature->getX() + creature->getShape()->getWidth() / 2.0f );
	selY = toint( creature->getY() + creature->getShape()->getDepth() / 2.0f );
	Creature * oldTarget = targetCreature;

	targetCreature = creature;

	setMotion( Constants::MOTION_MOVE_TOWARDS );
	tx = ty = -1;

	// find the path
	tx = selX;
	ty = selY;
	// Does the path lead close enough to the destination?
//	bool ret = pathManager->findPathToCreature( creature, session->getParty()->getPlayer(), session->getMap(), range, ignoreParty );
	bool ret = pathManager->findPathToCreature( creature, this, session->getMap(), range, ignoreParty );

	/**
	 * For pc-s cancel the move.
	 */
	if ( !ret && session->getParty()->isPartyMember(this) && cancelIfNotPossible ) {
		pathManager->clearPath();
		selX = oldSelX;
		selY = oldSelY;
		tx = oldtx;
		ty = oldty;
		targetCreature = oldTarget;
	}

	// FIXME: when to play sound?
	if ( ret && session->getParty()->getPlayer() == this ) {
		// play command sound
		if ( creature->getX() > -1 &&
		        0 == Util::dice(  session->getPreferences()->getSoundFreq() ) &&
		        !getStateMod( StateMod::dead ) ) {
			//session->playSound(getCharacter()->getRandomSound(Constants::SOUND_TYPE_COMMAND));
			int panning = session->getMap()->getPanningFromMapXY( this->x, this->y );
			playCharacterSound( GameAdapter::COMMAND_SOUND, panning );
		}
	}
	return ret;
}

Location *Creature::moveToLocator() {
	Location *pos = NULL;

	//we either have a target we want to reach, or we are wandering around
	if ( selX > -1 || getMotion() == Constants::MOTION_LOITER ) {
		// take a step
		pos = takeAStepOnPath();

		// did we step on a trap?
		evalTrap();

		// if we've no more steps
		if ( pathManager->atEndOfPath() ) {
			stopMoving();
			setMotion( Constants::MOTION_STAND );
		} else if ( pos ) {
			cantMoveCounter++;
			if ( isPartyMember() ) {
				pathManager->moveNPCsOffPath( this, session->getMap() );
			}
			if ( cantMoveCounter > 5 ) {
				stopMoving();
				setMotion( Constants::MOTION_STAND );
				cantMoveCounter = 0;
			}
		} else if ( !pos ) {
			cantMoveCounter = 0;
			setMoving( true );
		}

	} else {

		if ( !( getMotion() == Constants::MOTION_LOITER || getMotion() == Constants::MOTION_STAND ) ) {
#if PATH_DEBUG
			cerr << "Creature stuck: " << getName() << endl;
#endif
			stopMoving();
			setMotion( Constants::MOTION_STAND );
		}

	}

	return pos;
}

/// Move the creature one step along its path. Handles blocking objects.

/// Returns the blocking shape or NULL
/// if move is possible.

Location *Creature::takeAStepOnPath() {
	Location *position = NULL;
	int a = ( ( AnimatedShape* )getShape() )->getCurrentAnimation();

	if ( !pathManager->atEndOfPath() && a == MD2_RUN ) { //a != MD2_TAUNT ) {

		// take a step on the bestPath
		Location location = pathManager->getNextStepOnPath();

		GLfloat newX = getX();
		GLfloat newY = getY();

		int cx = toint( newX );
		int cy = toint( newY ); //current x,y

		GLfloat step = getStep();
		float targetX = static_cast<float>( location.x );
		float targetY = static_cast<float>( location.y );

		//get the direction to the target location
		float diffX = targetX - newX; //distance between creature's (continuous) location and the target (discrete) location
		float diffY = targetY - newY;
		//get the x and y values for a step-length vector in the direction of diffX,diffY
		//float dist = sqrt(diffX*diffX + diffY*diffY); //distance to location
		float dist = Constants::distance( newX, newY, 0, 0, targetX, targetY, 0, 0 ); //distance to location
		//if(dist < step) step = dist; //if the step is too great, we slow ourselves to avoid overstepping
		if ( dist != 0.0f ) { // thee shall not divide with zero
			float stepX = ( diffX * step ) / dist;
			float stepY = ( diffY * step ) / dist;

			newY += stepY;
			newX += stepX;

			int nx = toint( newX );
			int ny = toint( newY );

			position = session->getMap()->
			           moveCreature( cx, cy, toint( getZ() ),
			                         nx, ny, toint( getZ() ),
			                         this );

			if ( position && cx != location.x && cy != location.y && ( ( cx != nx && cy == ny ) || ( cx == nx && cy != ny ) ) ) {
#if PATH_DEBUG
				cerr << "Popping: " << this->getName() << endl;
#endif
				//we are blocked at our next step, are moving diagonally, and did not complete the diagonal move
				newX = targetX;
				newY = targetY; //we just "pop" to the target location
				nx = toint( newX );
				ny = toint( newY );
				position = session->getMap()->
				           moveCreature( cx, cy, toint( getZ() ),
				                         nx, ny, toint( getZ() ),
				                         this );
			}
		}

		if ( !position ) {
			computeAngle( newX, newY );
			showWaterEffect( newX, newY );
			moveTo( newX, newY, getZ() );
			if ( toint( newX ) == location.x && toint( newY ) == location.y ) {
				pathManager->incrementPositionOnPath();
			}
		} else {
#if PATH_DEBUG
			cerr << "Blocked, stopping: " << this->getName() << endl;
#endif
		}

	}

	return position;
}

/// Computes the angle the creature should assume to reach newX, nexY.

void Creature::computeAngle( GLfloat newX, GLfloat newY ) {
	GLfloat a = Util::getAngle( newX, newY, 1, 1,
	                            getX(), getY(), 1, 1 );

	if ( pathManager->atStartOfPath() || a != wantedAngle ) {
		wantedAngle = a;
		GLfloat diff = Util::diffAngle( a, angle );
		angleStep = diff / static_cast<float>( TURN_STEP_COUNT );
	}

	if ( fabs( angle - wantedAngle ) > 2.0f ) {
		GLfloat diff = Util::diffAngle( wantedAngle, angle );
		if ( fabs( diff ) < angleStep ) {
			angle = wantedAngle;
		} else {
			angle += angleStep;
		}
		if ( angle < 0.0f ) angle = 360.0f + angle;
		if ( angle >= 360.0f ) angle -= 360.0f;
	} else {
		angle = wantedAngle;
	}

	( ( AnimatedShape* )shape )->setAngle( angle + 180.0f );
}

void Creature::showWaterEffect( GLfloat newX, GLfloat newY ) {
	if ( session->getMap()->getHasWater() &&
	        !( toint( getX() ) == toint( newX ) &&
	           toint( getY() ) == toint( newY ) ) ) {
		session->getMap()->startEffect( toint( getX() + getShape()->getWidth() / 2 ),
		    toint( getY() - getShape()->getDepth() / 2 ), 0,
		    Constants::EFFECT_RIPPLE,
		    ( Constants::DAMAGE_DURATION * 4 ),
		    getShape()->getWidth(), getShape()->getDepth() );
	}
}

/// Stops movement.

void Creature::stopMoving() {
	cantMoveCounter = 0;
	pathManager->clearPath();
	selX = selY = -1;
	speed = originalSpeed;
	getShape()->setCurrentAnimation( MD2_STAND );
	if ( session->getParty()->getPlayer() == this ) session->getSound()->stopFootsteps();
}

/// Plays the footstep sound.

Uint32 lastFootstepTime = 0;
void Creature::playFootstep() {
	Uint32 now = SDL_GetTicks();
	if ( now - lastFootstepTime > ( Uint32 ) ( session->getPreferences()->getGameSpeedTicks() * 4 ) && getMotion() != Constants::MOTION_STAND ) {
		int panning = session->getMap()->getPanningFromMapXY( this->x, this->y );
		session->getSound()->startFootsteps( session->getAmbientSoundName(), session->getGameAdapter()->getCurrentDepth(), panning );
		lastFootstepTime = now;
	}
}

/// Checks whether the creature has any moves left on its path.

bool Creature::anyMovesLeft() {
	return( selX > -1 && !pathManager->atEndOfPath() );
}


/// Returns the weight the creature can carry before being overloaded.

float Creature::getMaxBackpackWeight() {
	return static_cast<float>( getSkill( Skill::POWER ) ) * 2.5f;
}

void Creature::pickUpOnMap( RenderedItem *item ) {
	addToBackpack( ( Item* )item );
}

/// Adds an item to the creature's backpack.

bool Creature::addToBackpack( Item *item, int itemX, int itemY, bool force ) {
	BackpackInfo *info = getBackpackInfo( item, true );
	if ( backpack->addContainedItem( item, itemX, itemY, force ) ) {

		info->equipIndex = -1;
		info->backpackIndex = backpack->getContainedItemCount() - 1;

		backpackWeight += item->getWeight();

		if ( backpackWeight > getMaxBackpackWeight() ) {
			if ( isPartyMember() ) {
				char msg[80];
				snprintf( msg, 80, _( "%s is overloaded." ), getName() );
				session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_STATS );
			}
			setStateMod( StateMod::overloaded, true );
		}

		// check if the mission is over
		if ( isPartyMember() &&
		        session->getCurrentMission() &&
		        session->getCurrentMission()->itemFound( item ) ) {
			session->getGameAdapter()->missionCompleted();
		}
		
#ifdef DEBUG_INVENTORY
	if( this == session->getParty()->getPlayer() ) {
		cerr << "Creature::addToBackpack: " << item->getName() << endl;
		debugBackpack();
	}
#endif		

		return true;
	} else {
		cerr << "*** error: unable to add to inventory of creature=" << getName() << " item=" << item->getName() << endl;
		return false;
	}
}

/// Returns the backpack index and equip index of an item.

BackpackInfo *Creature::getBackpackInfo( Item *item, bool createIfMissing ) {
	if ( invInfos.find( item ) == invInfos.end() ) {
		if ( createIfMissing ) {
			BackpackInfo *info = new BackpackInfo();
			invInfos[ item ] = info;
			return info;
		} else {
			return NULL;
		}
	} else {
		return invInfos[ item ];
	}
}

/// Returns the backpack index of an item.

int Creature::findInBackpack( Item *item ) {
#ifdef DEBUG_INVENTORY
	if( this == session->getParty()->getPlayer() ) {
		cerr << "findInBackpack: item=" << item->getName() << " address=" << item << endl;
		debugBackpack();
	}
#endif	
	BackpackInfo *info = getBackpackInfo( item );
	return( info ? info->backpackIndex : -1 );
	/*
	 for(int i = 0; i < backpack_count; i++) {
	   Item *invItem = backpack[i];
	   if(item == invItem) return i;
	 }
	 return -1;
	*/
}

void Creature::debugBackpack() {
	cerr << "**************************************" << endl;
	cerr << "backpack size=" << backpack->getContainedItemCount() << endl;
	for( int i = 0; i < backpack->getContainedItemCount(); i++ ) {
		cerr << "\titem: "  << backpack->getContainedItem( i )->getName() << " address=" << backpack->getContainedItem( i ) << endl;
	}
	cerr << "InvInfos size=" << invInfos.size() << endl;
	for( map<Item*, BackpackInfo*>::iterator e = invInfos.begin(); e != invInfos.end(); ++e ) {
		Item *item = e->first;
		BackpackInfo *bpi = e->second;
		cerr << "\titem: " << item->getName() << " address=" << item << " backpack info: " << bpi->backpackIndex << "," << bpi->equipIndex << endl;
	}
	cerr << "Equipped: " << endl;
	for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		if(equipped[i] < MAX_BACKPACK_SIZE) {
			cerr << "\tat: " << i << " location: " << (1 << i) << " index: " << equipped[i] << " item: " << getBackpackItem(equipped[i])->getName() << " address: " << getBackpackItem(equipped[i]) << endl;
		}	
	}	
	cerr << "**************************************" << endl;	
}

/// Removes an item from the backpack at index.

Item *Creature::removeFromBackpack( int backpackIndex ) {
#ifdef DEBUG_INVENTORY
	if( this == session->getParty()->getPlayer() ) {
		cerr << "Creature::removeFromBackpack: " << backpackIndex << endl;
		if( this == session->getParty()->getPlayer() ) debugBackpack();
	}
#endif
	
	Item *item = NULL;
	if ( backpackIndex < backpack->getContainedItemCount() ) {
		// drop item if carrying it
		doff( backpackIndex );
		
		// drop from backpack
		item = backpack->getContainedItem( backpackIndex );		

		BackpackInfo *info = getBackpackInfo( item );
		invInfos.erase( item );
		delete info;

		// remove it from quickspell slot
		for ( int i = 0; i < 12; i++ ) {
			Storable *storable = getQuickSpell( i );
			if ( storable ) {
				if ( storable->getStorableType() == Storable::ITEM_STORABLE ) {
					if ( ( Item* )storable == item ) {
						setQuickSpell( i, NULL );
					}
				}
			}
		}

		backpackWeight -= item->getWeight();
		if ( getStateMod( StateMod::overloaded ) && backpackWeight < getMaxBackpackWeight() ) {
			if ( isPartyMember() ) {
				char msg[80];
				snprintf( msg, 80, _( "%s is not overloaded anymore." ), getName() );
				session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_STATS );
			}
			setStateMod( StateMod::overloaded, false );
		}
		
		// remove it from the backpack
		backpack->removeContainedItem( item );

		// update the backpack infos of items higher than the removed item... (HACK!)
		for ( int i = backpackIndex; i < backpack->getContainedItemCount(); i++ ) {
			BackpackInfo *info = getBackpackInfo( backpack->getContainedItem( i ) );
			info->backpackIndex--;
		}
		// adjust equipped indexes too
    for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
			if ( equipped[i] > backpackIndex && equipped[i] < MAX_BACKPACK_SIZE ) {
				equipped[i]--;
			}
		}
		recalcAggregateValues();
	}
	return item;
}

/// returns true if ate/drank item completely and false else

bool Creature::eatDrink( int backpackIndex ) {
	return eatDrink( getBackpackItem( backpackIndex ) );
}

bool Creature::eatDrink( Item *item ) {
	enum {MSG_SIZE = 500 };
	char msg[MSG_SIZE];
	char buff[200];
	RpgItem * rpgItem = item->getRpgItem();

	int type = rpgItem->getType();
	//weight = item->getWeight();
	int level = item->getLevel();
	if ( type == RpgItem::FOOD ) {
		if ( getHunger() == 10 ) {
			snprintf( msg, MSG_SIZE, _( "%s is not hungry at the moment." ), getName() );
			session->getGameAdapter()->writeLogMessage( msg );
			return false;
		}

		// TODO : the quality member of rpgItem should indicate if the
		// food is totally healthy or roten or partially roten etc...
		// We eat the item and it gives us "level" hunger points back
		setHunger( getHunger() + level );
		strcpy( buff, rpgItem->getShortDesc() );
		buff[0] = tolower( buff[0] );
		snprintf( msg, MSG_SIZE, _( "%1$s eats %2$s." ), getName(), buff );
		session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_PLAYERITEM );
		bool b = item->decrementCharges();
		if ( b ) {
			snprintf( msg, MSG_SIZE, _( "%s is used up." ), item->getItemName() );
			session->getGameAdapter()->writeLogMessage( msg );
		}
		return b;
	} else if ( type == RpgItem::DRINK ) {
		if ( getThirst() == 10 ) {
			snprintf( msg, MSG_SIZE, _( "%s is not thirsty at the moment." ), getName() );
			session->getGameAdapter()->writeLogMessage( msg );
			return false;
		}
		setThirst( getThirst() + level );
		strcpy( buff, rpgItem->getShortDesc() );
		buff[0] = tolower( buff[0] );
		snprintf( msg, MSG_SIZE, _( "%1$s drinks %2$s." ), getName(), buff );
		session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_PLAYERITEM );
		// TODO : according to the alcool rate set drunk state or not
		bool b = item->decrementCharges();
		if ( b ) {
			snprintf( msg, MSG_SIZE, _( "%s is used up." ), item->getItemName() );
			session->getGameAdapter()->writeLogMessage( msg );
		}
		return b;
	} else if ( type == RpgItem::POTION ) {
		// It's a potion
		// Even if not thirsty, character will always drink a potion
		strcpy( buff, rpgItem->getShortDesc() );
		buff[0] = tolower( buff[0] );
		setThirst( getThirst() + level );
		snprintf( msg, MSG_SIZE, _( "%1$s drinks from %2$s." ), getName(), buff );
		session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_PLAYERITEM );
		usePotion( item );
		bool b = item->decrementCharges();
		if ( b ) {
			snprintf( msg, MSG_SIZE, _( "%s is used up." ), item->getItemName() );
			session->getGameAdapter()->writeLogMessage( msg );
		}
		return b;
	} else {
		session->getGameAdapter()->writeLogMessage( _( "You cannot eat or drink that!" ) );
		return false;
	}
}

/// Uses a potion.

void Creature::usePotion( Item *item ) {
	// nothing to do?
	if ( item->getRpgItem()->getPotionSkill() == -1 ) return;

	int n;
	enum {MSG_SIZE = 255 };
	char msg[ MSG_SIZE ];

	int skill = item->getRpgItem()->getPotionSkill();
	if ( skill < 0 ) {
		switch ( -skill - 2 ) {
		case Constants::HP:
			n = item->getRpgItem()->getPotionPower() + item->getLevel();
			if ( n + getHp() > getMaxHp() )
				n = getMaxHp() - getHp();
			setHp( getHp() + n );
			snprintf( msg, MSG_SIZE, _( "%s heals %d points." ), getName(), n );
			session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_STATS );
			startEffect( Constants::EFFECT_SWIRL, ( Constants::DAMAGE_DURATION * 4 ) );
			return;
		case Constants::MP:
			n = item->getRpgItem()->getPotionPower() + item->getLevel();
			if ( n + getMp() > getMaxMp() )
				n = getMaxMp() - getMp();
			setMp( getMp() + n );
			snprintf( msg, MSG_SIZE, _( "%s receives %d magic points." ), getName(), n );
			session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_STATS );
			startEffect( Constants::EFFECT_SWIRL, ( Constants::DAMAGE_DURATION * 4 ) );
			return;
		case Constants::AC: {
				bonusArmor += item->getRpgItem()->getPotionPower();
				recalcAggregateValues();
				snprintf( msg, MSG_SIZE, _( "%s feels impervious to damage!" ), getName() );
				session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_STATS );
				startEffect( Constants::EFFECT_SWIRL, ( Constants::DAMAGE_DURATION * 4 ) );

				// add calendar event to remove armor bonus
				// (format : sec, min, hours, days, months, years)
				Date d( 0, item->getRpgItem()->getPotionTime() + item->getLevel(), 0, 0, 0, 0 );
				Event *e =
				  new PotionExpirationEvent( session->getParty()->getCalendar()->getCurrentDate(),
				                             d, this, item, session, 1 );
				session->getParty()->getCalendar()->scheduleEvent( ( Event* )e );   // It's important to cast!!
			}
			return;
		default:
			cerr << "Implement me! (other potion skill boost)" << endl;
			return;
		}
	} else {
		skillBonus[skill] += item->getRpgItem()->getPotionPower();
		// recalcAggregateValues();
		snprintf( msg, MSG_SIZE, _( "%s feels at peace." ), getName() );
		session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_STATS );
		startEffect( Constants::EFFECT_SWIRL, ( Constants::DAMAGE_DURATION * 4 ) );

		// add calendar event to remove armor bonus
		// (format : sec, min, hours, days, months, years)
		Date d( 0, item->getRpgItem()->getPotionTime() + item->getLevel(), 0, 0, 0, 0 );
		Event *e = new PotionExpirationEvent( session->getParty()->getCalendar()->getCurrentDate(), d, this, item, session, 1 );
		session->getParty()->getCalendar()->scheduleEvent( ( Event* )e );   // It's important to cast!!
	}
}

/// Set the creature's next action (carried out at begin of next battle turn).

void Creature::setAction( int action, Item *item, Spell *spell, SpecialSkill *skill ) {
	this->action = action;
	this->actionItem = item;
	this->actionSpell = spell;
	this->actionSkill = skill;
	preActionTargetCreature = getTargetCreature();
	// zero the clock
	setLastTurn( 0 );

	enum {MSG_SIZE = 80 };
	char msg[ MSG_SIZE ];
	switch ( action ) {
	case Constants::ACTION_EAT_DRINK:
		this->battle->invalidate();
		snprintf( msg, MSG_SIZE, _( "%1$s will consume %2$s." ), getName(), item->getItemName() );
		break;
	case Constants::ACTION_CAST_SPELL:
		this->battle->invalidate();
		snprintf( msg, MSG_SIZE, _( "%1$s will cast %2$s." ), getName(), spell->getDisplayName() );
		break;
	case Constants::ACTION_SPECIAL:
		this->battle->invalidate();
		snprintf( msg, MSG_SIZE, _( "%1$s will use capability %2$s." ), getName(), skill->getDisplayName() );
		break;
	case Constants::ACTION_NO_ACTION:
		// no-op
		preActionTargetCreature = NULL;
		strcpy( msg, "" );
		break;
	default:
		cerr << "*** Error: unknown action " << action << endl;
		return;
	}

	if ( strlen( msg ) ) {
		if ( session->getParty()->isPartyMember(this) ) {
			session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_PLAYERBATTLE );
		} else {
			session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_NPCBATTLE );
		}
	}
}

/// equip or doff if already equipped

bool Creature::equipFromBackpack( int backpackIndex, int equipIndexHint ) {
	this->battle->invalidate();
	// doff
	if ( doff( backpackIndex ) ) return true;
	// don
	// FIXME: take into account: two-handed weapons, min skill req-s., etc.
	Item *item = getBackpackItem( backpackIndex );
	if( !item ) return false;
#ifdef DEBUG_INVENTORY
	cerr << "item at pos " << backpackIndex << " item=" << item << endl;
	debugBackpack();
#endif

	int place = -1;
	vector<int> places;
	for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		// if the slot is empty and the item can be worn here
		if ( item->getRpgItem()->getEquip() & ( 1 << i ) &&
		        equipped[i] == MAX_BACKPACK_SIZE ) {
			if ( i == equipIndexHint ) {
				place = i;
				break;
			}
			places.push_back( i );
		}
	}
	if ( place == -1 && !places.empty() ) {
		place = places[ Util::dice(  places.size() ) ];
	}
	if ( place > -1 ) {

		BackpackInfo *info = getBackpackInfo( item );
		info->equipIndex = place;

		equipped[ place ] = backpackIndex;

		// once worn, show if it's cursed
		item->setShowCursed( true );

		// handle magic attrib settings
		if ( item->isMagicItem() ) {

			// increase skill bonuses
			map<int, int> *m = item->getSkillBonusMap();
			for ( map<int, int>::iterator e = m->begin(); e != m->end(); ++e ) {
				int skill = e->first;
				int bonus = e->second;
				setSkillBonus( skill, getSkillBonus( skill ) + bonus );
			}
			// if armor, enhance magic resistance
			if ( !item->getRpgItem()->isWeapon() && item->getSchool() ) {
				int skill = item->getSchool()->getResistSkill();
				setSkillBonus( skill, getSkillBonus( skill ) + item->getMagicResistance() );
			}

		}

		// recalc current weapon, and the state mods
		recalcAggregateValues();

		// call script
		if ( isPartyMember() ) session->getSquirrel()->callItemEvent( this, item, "equipItem" );

		return true;
	} else {
		return false;
	}
}

/// Unequips an item.

int Creature::doff( int backpackIndex ) {
	// doff
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		if ( equipped[i] == backpackIndex ) {
			Item *item = getBackpackItem( backpackIndex );
			equipped[i] = MAX_BACKPACK_SIZE;
			BackpackInfo *info = getBackpackInfo( item );
			info->equipIndex = -1;

			// handle magic attrib settings
			if ( item->isMagicItem() ) {

				// unset the good attributes
				for ( int k = 0; k < StateMod::STATE_MOD_COUNT; k++ ) {
					if ( item->isStateModSet( k ) ) {
						this->setStateMod( k, false );
					}
				}
				// unset the protected attributes
				for ( int k = 0; k < StateMod::STATE_MOD_COUNT; k++ ) {
					if ( item->isStateModProtected( k ) ) {
						this->setProtectedStateMod( k, false );
					}
				}
				// decrease skill bonus
				map<int, int> *m = item->getSkillBonusMap();
				for ( map<int, int>::iterator e = m->begin(); e != m->end(); ++e ) {
					int skill = e->first;
					int bonus = e->second;
					setSkillBonus( skill, getSkillBonus( skill ) - bonus );
				}
				// if armor, reduce magic resistance
				if ( !item->getRpgItem()->isWeapon() && item->getSchool() ) {
					int skill = item->getSchool()->getResistSkill();
					setSkillBonus( skill, getSkillBonus( skill ) - item->getMagicResistance() );
				}

			}

			// recalc current weapon, and the state mods
			recalcAggregateValues();

			// call script
			if ( isPartyMember() ) session->getSquirrel()->callItemEvent( this, item, "doffItem" );

			return 1;
		}
	}
	return 0;
}


/// Get the item at an equip index. (What is at equipped location?)
/// The parameter is an int from 0 - EQUIP_LOCATION_COUNT
Item *Creature::getEquippedItemByIndex( int equipIndex ) {
	int n = equipped[equipIndex];
	if ( n < MAX_BACKPACK_SIZE ) {
		return getBackpackItem( n );
	}
	return NULL;
}	


/// Get the item at an equip index. (What is at equipped location?)
/// The parameter is a power of 2 (see constants for EQUIP_LOCATION values
Item *Creature::getEquippedItem( int equipLocation ) {
	
	// find out which power of 2 it is
	int equipIndex = Constants::getLocationIndex( equipLocation );
	
	return getEquippedItemByIndex( equipIndex );
}

/// Returns whether the item at an equip index is a weapon.
/// The parameter is a power of 2 (see constants for EQUIP_LOCATION values

bool Creature::isEquippedWeapon( int equipLocation ) {
	Item *item = getEquippedItem( equipLocation );
	return( item && item->getRpgItem()->isWeapon() );
}


/// Returns whether the creature has an item currently equipped.

bool Creature::isEquipped( Item *item ) {
	BackpackInfo *info = getBackpackInfo( item );
	return( info && info->equipIndex > -1 );
	/*
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
	   if(equipped[i] < MAX_BACKPACK_SIZE &&
	      backpack[ equipped[i] ] == item ) return true;
	 }
	 return false;
	*/
}

/// Returns whether an item at an backpack location is currently equipped somewhere.

bool Creature::isEquipped( int backpackIndex ) {
	if ( backpackIndex < 0 || backpackIndex >= backpack->getContainedItemCount() ) return false;
	return isEquipped( backpack->getContainedItem( backpackIndex ) );
	/*
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
	   if( equipped[i] == backpackIndex ) return true;
	 }
	 return false;
	*/
}

/// Unequips cursed items.

bool Creature::removeCursedItems() {
	bool found = false;
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		if ( equipped[i] < MAX_BACKPACK_SIZE && backpack->getContainedItem( equipped[i] )->isCursed() ) {
			found = true;
			// not the most efficient way to do this, but it works...
			doff( equipped[i] );
		}
	}
	return found;
}

/// Gets the equip index of the item stored at an backpack index. (Where is the item worn?)

int Creature::getEquippedIndex( int backpackIndex ) {
	if ( backpackIndex < 0 || backpackIndex >= backpack->getContainedItemCount() ) return -1;
	BackpackInfo *info = getBackpackInfo( backpack->getContainedItem( backpackIndex ) );
	return info->equipIndex;
	/*
  for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
	   if(equipped[i] == index) return i;
	 }
	 return -1;
	*/
}

/// Returns whether an item is worn in the backpack (also recurses containers).

bool Creature::isItemInBackpack( Item *item ) {
	// -=K=-: reverting that back; carried container contents get deleted in Session::cleanUpAfterMission otherwise
	// return( getBackpackInfo( item ) ? true : false );

	for ( int i = 0; i < backpack->getContainedItemCount(); i++ ) {
		if ( backpack->getContainedItem( i ) == item || ( backpack->getContainedItem( i )->getRpgItem()->getType() == RpgItem::CONTAINER &&
				backpack->getContainedItem( i )->isContainedItem( item ) ) )
			return true;
	}
	return false;

}

/// Calculates the aggregate values based on equipped items.

void Creature::recalcAggregateValues() {
	armorChanged = true;

	// try to select a new preferred weapon if needed.
	if ( preferredWeapon == -1 || !isEquippedWeapon( preferredWeapon ) ) {
		int values[] = {
      Constants::EQUIP_LOCATION_LEFT_HAND, 
      Constants::EQUIP_LOCATION_RIGHT_HAND, 
      Constants::EQUIP_LOCATION_WEAPON_RANGED,
			-1
		};
		preferredWeapon = -1;
		for ( int i = 0; values[ i ] > -1; i++ ) {
			if ( isEquippedWeapon( values[ i ] ) ) {
				preferredWeapon = values[ i ];
				break;
			}
		}
	}

	for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
		Item *item = getEquippedItemByIndex( i );
		// handle magic attrib settings
		if ( item != NULL && item->isMagicItem() ) {

			// set the good attributes
			for ( int k = 0; k < StateMod::STATE_MOD_COUNT; k++ ) {
				if ( item->isStateModSet( k ) ) {
					this->setStateMod( k, true );
				}
			}
			// set the protected attributes
			for ( int k = 0; k < StateMod::STATE_MOD_COUNT; k++ ) {
				if ( item->isStateModProtected( k ) ) {
					this->setProtectedStateMod( k, true );
				}
			}
			// refresh map for invisibility, etc.
			session->getMap()->refresh();
		}
	}
}

/// Selects the next equipped weapon as the active weapon.

bool Creature::nextPreferredWeapon() {
#ifdef DEBUG_INVENTORY
	cerr << "nextPreferredWeapon" << endl;
	debugBackpack();
#endif	
	int pos = preferredWeapon;
	for ( int i = 0; i < 4; i++ ) {
		switch ( pos ) {
    case Constants::EQUIP_LOCATION_LEFT_HAND: pos = Constants::EQUIP_LOCATION_RIGHT_HAND; break;
    case Constants::EQUIP_LOCATION_RIGHT_HAND: pos = Constants::EQUIP_LOCATION_WEAPON_RANGED; break;
    case Constants::EQUIP_LOCATION_WEAPON_RANGED: pos = -1; break;
    case -1: pos = Constants::EQUIP_LOCATION_LEFT_HAND; break;
		}
#ifdef DEBUG_INVENTORY
		if( this == session->getParty()->getPlayer() ) {
			if( pos != -1 ) {
				cerr << "\tchecking pos=" << pos << " equipped=" << getEquippedItem( pos ) << " weapon: " << isEquippedWeapon( pos ) << endl;
			}
		}
#endif
		if ( pos == -1 || isEquippedWeapon( pos ) ) {
			preferredWeapon = pos;
			return true;
		}
	}
	preferredWeapon = -1;
	return false;
}

/// Returns an equipped weapon with an action radius >= dist, or NULL otherwise.

Item *Creature::getBestWeapon( float dist, bool callScript ) {

	Item *ret = NULL;

	// for TB combat for players, respect the current weapon
	if ( session->getPreferences()->isBattleTurnBased() && isPartyMember() ) {
		ret = ( preferredWeapon > -1 ? getEquippedItem( preferredWeapon ) : NULL );
	} else {
		int location[] = {
      Constants::EQUIP_LOCATION_RIGHT_HAND,
      Constants::EQUIP_LOCATION_LEFT_HAND,
      Constants::EQUIP_LOCATION_WEAPON_RANGED,
			-1
		};
		for ( int i = 0; location[i] > -1; i++ ) {
			Item *item = getEquippedItem( location[i] );
			if ( item &&
			        item->getRpgItem()->isWeapon() &&
			        item->getRange() >= dist ) {
				ret = item;
				break;
			}
		}
	}
	if ( isPartyMember() && ret && callScript && SQUIRREL_ENABLED ) {
		session->getSquirrel()->callItemEvent( this, ret, "startBattleWithItem" );
	}

	return ret;
}

/// Returns the initiative for a battle round, the higher, the faster the attack.

int Creature::getInitiative( int *max ) {
	float n = ( getSkill( Skill::SPEED ) + ( getSkill( Skill::LUCK ) / 5.0f ) );
	if ( max ) *max = toint( n );
	return toint( Util::roll( 0.0f, n ) );
}

/// Returns the number of projectiles that can be launched simultaneously.

/// it is a function of speed, level, and weapon skill
/// this method returns a number from 1-10

int Creature::getMaxProjectileCount( Item *item ) {
	int n = static_cast<int>( static_cast<double>( getSkill( Skill::SPEED ) + ( getLevel() * 10 ) +
	                          getSkill( item->getRpgItem()->getDamageSkill() ) ) / 30.0f );
	if ( n <= 0 )
		n = 1;
	return n;
}

/// Returns the projectiles that have been fired by the creature.

vector<RenderedProjectile*> *Creature::getProjectiles() {
	map<RenderedCreature*, vector<RenderedProjectile*>*> *m = RenderedProjectile::getProjectileMap();
	return( m->find( this ) == m->end() ? NULL : ( *m )[ ( RenderedCreature* )this ] );
}

/// Take some damage and show a nice damage effect. Return true if the creature is killed.

bool Creature::takeDamage( float damage,
                           int effect_type,
                           GLuint delay ) {

	int intDamage = toint( damage );
	addRecentDamage( intDamage );

	hp -= intDamage;
	// if creature dies start effect at its location
	if ( hp > 0 ) {
		startEffect( effect_type );
		int pain = Util::dice(  3 );
		getShape()->setCurrentAnimation( pain == 0 ? static_cast<int>( MD2_PAIN1 ) : ( pain == 1 ? static_cast<int>( MD2_PAIN2 ) : static_cast<int>( MD2_PAIN3 ) ) );
	} else if ( effect_type != Constants::EFFECT_GLOW ) {
		session->getMap()->startEffect( toint( getX() ), toint( getY() - this->getShape()->getDepth() + 1 ), toint( getZ() ),
		    effect_type, ( Constants::DAMAGE_DURATION * 4 ),
		    getShape()->getWidth(), getShape()->getDepth(), delay );
	}

	// creature death here so it can be used from script
	if ( hp <= 0 ) {
		if ( !( ( isMonster() && MONSTER_IMORTALITY ) || ( isPartyMember() && GOD_MODE ) ) ) {
			session->creatureDeath( this );
		}
		return true;
	} else {
		return false;
	}
}

/// Raises the creature from the dead.

void Creature::resurrect( int rx, int ry ) {
	// remove all state mod effects
	for ( int i = 0; i < StateMod::STATE_MOD_COUNT; i++ ) {
		setStateMod( i, false );
	}
	if ( getThirst() < 5 ) setThirst( 5 );
	if ( getHunger() < 5 ) setHunger( 5 );

	setHp( Util::pickOne(  1, 3 ) );

	findPlace( rx, ry );

	startEffect( Constants::EFFECT_TELEPORT, ( Constants::DAMAGE_DURATION * 4 ) );

	char msg[120];
	snprintf( msg, 120, _( "%s is raised from the dead!" ), getName() );
	session->getGameAdapter()->writeLogMessage( msg, Constants::MSGTYPE_STATS );
}

/// Gives experience points for a creature kill.

/// add exp after killing a creature
/// only called for characters

int Creature::addExperience( Creature *creature_killed ) {
	int n = ( creature_killed->level + 1 ) * 25;
	// extra for killing higher level creatures
	int bonus = ( creature_killed->level - getLevel() );
	if ( bonus > 0 ) n += bonus * 10;
	return addExperienceWithMessage( n );
}

/// Gives experience points.

/// Add n exp points. Only called for characters
/// Note that n can be a negative number. (eg.: failure to steal)

int Creature::addExperience( int delta ) {
	int n = delta;
	experience += n;
	if ( experience < 0 ) {
		n = experience;
		experience = 0;
	}

	// level up?
	if ( experience >= expOfNextLevel ) {
		level++;
		hp += getStartingHp();
		mp += getStartingMp();
		calculateExpOfNextLevel();
		setAvailableSkillMod( getAvailableSkillMod() + character->getSkillBonus() );
		char message[255];
		snprintf( message, 255, _( "  %s levels up!" ), getName() );
		session->getGameAdapter()->startTextEffect( message );
		session->getGameAdapter()->refreshBackpackUI();
	}

	evalSpecialSkills();

	return n;
}

/// Gives experience points and adds an appropriate message to the log scroller.

/// Add experience and show message in map window. Also shows
/// message if creature leveled up. Use generally for party
/// characters only.

int Creature::addExperienceWithMessage( int exp ) {
	int n = 0;
	if ( !getStateMod( StateMod::dead ) ) {
		enum { MSG_SIZE = 120 };
		char message[ MSG_SIZE ];
		int oldLevel = level;
		n = addExperience( exp );
		if ( n > 0 ) {
			snprintf( message, MSG_SIZE, _( "%s gains %d experience points." ), getName(), n );
			session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_STATS );
			if ( oldLevel != level ) {
				snprintf( message, MSG_SIZE, _( "%s gains a level!" ), getName() );
				session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_MISSION );
			}
		} else if ( n < 0 ) {
			snprintf( message, MSG_SIZE, _( "%s looses %d experience points!" ), getName(), -n );
			session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_STATS );
		}
	}
	return n;
}

/// Adds money after a creature kill

int Creature::addMoney( Creature *creature_killed ) {
	int n = creature_killed->level - getLevel();
	if ( n < 1 ) n = 1;
	// fixme: use creature_killed->getMonster()->getMoney() instead of 100.0f
	long delta = ( long )n * Util::dice( 50 );
	money += delta;
	return delta;
}


/// Returns the angle between the creature and its target.

float Creature::getTargetAngle() {
	//if(!targetCreature) return -1.0f;
	if ( !targetCreature ) return angle;
	return Util::getAngle( getX(), getY(), getShape()->getWidth(), getShape()->getDepth(),
	                       getTargetCreature()->getX(), getTargetCreature()->getY(),
	                       getTargetCreature()->getShape()->getWidth(),
	                       getTargetCreature()->getShape()->getHeight() );
}

/// Returns whether the creature knows a specific spell.

// FIXME: O(n) but there aren't that many spells...
bool Creature::isSpellMemorized( Spell *spell ) {
	for ( int i = 0; i < static_cast<int>( spells.size() ); i++ ) {
		if ( spells[i] == spell ) return true;
	}
	return false;
}

/// Adds a spell to the creature.

/// FIXME: O(n) but there aren't that many spells...
/// return true if spell was added, false if creature already had this spell

bool Creature::addSpell( Spell *spell ) {
	for ( vector<Spell*>::iterator e = spells.begin(); e != spells.end(); ++e ) {
		Spell *thisSpell = *e;
		if ( thisSpell == spell ) return false;
	}
	spells.push_back( spell );
	return true;
}

// this assumes that hasTarget() was called first.
bool Creature::isTargetValid() {
	// is it a non-creature target? (item or location)
	if ( !getTargetCreature() ) return true;
	if ( getTargetCreature()->getStateMod( StateMod::dead ) ) return false;
	// when attacking, attack the opposite kind (unless possessed)
	// however, you can cast spells on anyone
	if ( getAction() == Constants::ACTION_NO_ACTION &&
	        !canAttack( getTargetCreature() ) ) return false;
	return true;
}

/// Returns whether the creature can attack the other creature, and the mouse cursor the game should display.

bool Creature::canAttack( RenderedCreature *creature, int *cursor ) {
	// when attacking, attack the opposite kind (unless possessed)
	bool ret;
	if ( isMonster() ) {
		if ( !getStateMod( StateMod::possessed ) ) {
			ret = ( ( !creature->isMonster() && !creature->getStateMod( StateMod::possessed ) ) ||
			        ( creature->isMonster() && creature->getStateMod( StateMod::possessed ) ) );
		} else {
			ret = ( ( !creature->isMonster() && creature->getStateMod( StateMod::possessed ) ) ||
			        ( creature->isMonster() && !creature->getStateMod( StateMod::possessed ) ) );
		}
	} else {
		if ( !getStateMod( StateMod::possessed ) ) {
			ret = ( ( creature->isMonster() && !creature->getStateMod( StateMod::possessed ) ) ||
			        ( !creature->isMonster() && creature->getStateMod( StateMod::possessed ) ) );
		} else {
			ret = ( ( !creature->isMonster() && !creature->getStateMod( StateMod::possessed ) ) ||
			        ( creature->isMonster() && creature->getStateMod( StateMod::possessed ) ) );
		}
	}
	if ( ret && cursor ) {
		float dist = getDistanceToTarget( creature );
		Item *item = getBestWeapon( dist );
		if ( dist <= getBattle()->calculateRange( item ) ) {
			*cursor = ( !item ? Constants::CURSOR_NORMAL :
			            ( item->getRpgItem()->isRangedWeapon() ?
			              Constants::CURSOR_RANGED :
			              Constants::CURSOR_ATTACK ) );
		} else {
			*cursor = Constants::CURSOR_MOVE;
		}
	}
	return ret;
}

/// Cancels the creature's target selection.

void Creature::cancelTarget() {
	setTargetCreature( NULL );
	setTargetItem( 0, 0, 0, NULL );
	if ( preActionTargetCreature ) setTargetCreature( preActionTargetCreature );
	preActionTargetCreature = NULL;
	setAction( Constants::ACTION_NO_ACTION );
	if ( !isPartyMember() && !scripted ) {
		setMotion( Constants::MOTION_LOITER );
		pathManager->findWanderingPath( CREATURE_LOITERING_RADIUS, session->getParty()->getPlayer(), session->getMap() );
	}
}

/// Does the item's prerequisite apply to this creature?

bool Creature::isWithPrereq ( Item *item ) {
  // If the item is non-magical or used up, get out.
  if ( !item->isMagicItem() || ( item->getCurrentCharges() < 1 ) ) return false;

  // Is it a potion?
  if ( item->getRpgItem()->getPotionSkill() < 0 ) {
    int potionSkill = ( ( item->getRpgItem()->getPotionSkill() < 0 ) ? ( -item->getRpgItem()->getPotionSkill() - 2 ) : 0 ) ;
    float remainingHP = getHp() / ( getMaxHp() ? getMaxHp() : 1 );
    float remainingMP = getMp() / ( getMaxMp() ? getMaxMp() : 1 );
    if ( ( potionSkill == Constants::HP ) && ( remainingHP <= LOW_HP ) ) return true;
    if ( ( potionSkill == Constants::MP ) && ( ( remainingMP <= LOW_MP ) && getMaxMp() ) ) return true;
  }

  // Is it an item holding a spell?
  if ( item->getSpell() ) {
    if ( isWithPrereq( item->getSpell() ) ) return true;
  }

return false;
}

/// Does the spell's prerequisite apply to this creature?

bool Creature::isWithPrereq( Spell *spell ) {
	if ( spell->isStateModPrereqAPotionSkill() ) {
		switch ( spell->getStateModPrereq() ) {
		case Constants::HP:
			//cerr << "\tisWithPrereq: " << getName() << " max hp=" << getMaxHp() << " hp=" << getHp() << endl;
			return( getHp() <= static_cast<int>( static_cast<float>( getMaxHp() ) * LOW_HP ) );
		case Constants::MP:
			return( getMp() <= static_cast<int>( static_cast<float>( getMaxMp() ) * LOW_MP ) );
		case Constants::AC:
			/*
			FIXME: Even if needed only cast it 1 out of 4 times.
			Really need some AI here to remember if the spell helped or not. (or some way
			to predict if casting a spell will help.) Otherwise the monster keeps casting
			Body of Stone to no effect.
			Also: HIGH_AC should not be hard-coded...
			*/
			float armor, dodgePenalty;
			getArmor( &armor, &dodgePenalty, 0 );
			return( armor >= HIGH_AC ? false
			        : ( Util::dice( 4 ) == 0 ? true
			            : false ) );
		default: return false;
		}
	} else {
		return getStateMod( spell->getStateModPrereq() );
	}
}

/// Finds the closest possible target creature for a spell.

Creature *Creature::findClosestTargetWithPrereq( Spell *spell ) {

	// is it self?
	if ( isWithPrereq( spell ) ) return this;

	// who are the possible targets?
	vector<Creature*> possibleTargets;
	for( set<Creature*>::iterator i = closestFriends.begin(); i != closestFriends.end(); ++i ) {
		Creature *c = *i;
		if( c->isWithPrereq( spell ) ) {
			possibleTargets.push_back( c );				
		}
	}	

	// find the closest one that is closer than 20 spaces away.
	Creature *closest = NULL;
	float closestDist = 0;
	for ( int i = 0; i < static_cast<int>( possibleTargets.size() ); i++ ) {
		Creature *p = possibleTargets[ i ];
		float dist =
		  Constants::distance( getX(),  getY(),
		                       getShape()->getWidth(), getShape()->getDepth(),
		                       p->getX(), p->getY(),
		                       p->getShape()->getWidth(),
		                       p->getShape()->getDepth() );
		if ( !closest || dist < closestDist ) {
			closest = p;
			closestDist = dist;
		}
	}
	return( closest && closestDist < (float)CREATURE_SIGHT_RADIUS ? closest : NULL );
}


void Creature::getClosestCreatures( int radius ) {
	closestFriends.clear();
	closestEnemies.clear();
	int sx = toint( getX() ) + getShape()->getWidth() / 2 - radius;
	int sy = toint( getY() ) - getShape()->getDepth() / 2 - radius;
	int ex = toint( getX() ) + getShape()->getWidth() / 2 + radius;
	int ey = toint( getY() ) - getShape()->getDepth() / 2 + radius;
	float closestDistFriend = -1;
	float closestDistEnemy = -1;
	closestFriend = closestEnemy = NULL;
	float dist = -1;
	for( int xx = sx; xx < ex; xx++ ) {
		for( int yy = sy; yy < ey; yy++ ) {
			if( session->getMap()->isValidPosition( xx, yy, 0 ) ) {
				Location *pos = session->getMap()->getLocation( xx, yy, 0 );
				if( pos && pos->creature ) {
					Creature *c = (Creature*)pos->creature;
					dist = getDistance( c );
					if ( !c->getStateMod( StateMod::dead ) && 
							session->getMap()->isLocationVisible( toint( c->getX() ), toint( c->getY() ) ) && 
							session->getMap()->isLocationInLight( toint( c->getX() ), toint( c->getY() ), c->getShape() ) ) {
						if( c->isEvil() == isEvil() ) {
							closestFriends.insert( c );
							if( closestFriend == NULL || dist < closestDistFriend ) {
								closestFriend = c;
								closestDistFriend = dist;								
							}
						} else {
							closestEnemies.insert( c );
							if( closestEnemy == NULL || dist < closestDistEnemy ) {
								closestEnemy = c;
								closestDistEnemy = dist;								
							}
						}
					}
				}
			}
		}
	}
}


/// Basic NPC/monster AI.

void Creature::decideAction() {
  Uint32 now = SDL_GetTicks();
  if( now - lastDecision < AI_DECISION_INTERVAL ) return;
  //cerr << "decideAction for " << getName() << endl;
  lastDecision = now;

  if ( scripted ) {
    getShape()->setCurrentAnimation( getScriptedAnimation() );
    return;
  }
  
  // find the closest friends and enemies (do this only once; it's expensive)
  getClosestCreatures( CREATURE_SIGHT_RADIUS );
                                

  // todo: This code may slow down rendering. Instead make this call per event, for example:
  // creatureNearDeath, creatureWillAttack, etc. The current function (in map.nut) is Karzul's combat
  // behavior.
  // override from squirrel
  if( SQUIRREL_ENABLED ) {
    HSQOBJECT *cref = session->getSquirrel()->getCreatureRef( this );
    if ( cref ) {
      bool result;
      session->getSquirrel()->callBoolMethod( "decideAction", cref, &result );
      if ( result ) return;
    }
  }

  // This is the AI's decision matrix. On every decision cycle, it is walked
  // top to bottom and collects the max weights between all rows (states) that
  // currently apply to the situation. The result is an array of
  // (AI_ACTION_COUNT) weights which are then normalized. A dice is thrown
  // against random indices until the roll is <= the weight saved there. The
  // associated action is then executed.

  // For the sake of consistency, the sum of the weights in a row should
  // always be 1.

  // Actions (from left to right):
  // AtkClosest, Cast, AreaCast, Heal, ACCast, AtkRandom, StartLoiter, StopMoving, GoOn
  float decisionMatrix[ AI_STATE_COUNT ][ AI_ACTION_COUNT ] = {
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.25f, 0.0f, 0.75f }, // Hanging around, no enemies
    { 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f }, // Hanging around, enemies near
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.8f }, // Loitering, no enemies
    { 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f }, // Loitering, enemies near
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.8f, 0.0f }, // Loitering, end of path
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.8f }, // Moving towards target
    { 0.0f, 0.3f, 0.1f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.4f }, // Engaging target
    { 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f }, // Low HP
    { 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.65f }, // Target has low HP
    { 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.1f, 0.0f, 0.0f, 0.1f }, // Low MP
    { 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.65f }, // Target has low MP
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f, 0.0f, 0.75f }, // No nice AC boost
    { 0.0f, 0.1f, 0.5f, 0.0f, 0.2f, 0.2f, 0.0f, 0.0f, 0.0f }, // Surrounded (min. 3 attackers)
    { 0.0f, 0.4f, 0.25f, 0.0f, 0.2f, 0.15f, 0.0f, 0.0f, 0.0f }, // Friendlies outnumbered by enemy
    { 0.0f, 0.2f, 0.15f, 0.0f, 0.1f, 0.2f, 0.0f, 0.0f, 0.35f } // Friendlies outnumbering enemy
  };

  float decisionWeights[ AI_ACTION_COUNT ];
  for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
    decisionWeights[ i ] = 0.0f;
  }

  // STEP 1: Collect the weights of the active states.

  // Collect the standing and loitering states.

  if ( ( getMotion() == Constants::MOTION_STAND ) && !closestEnemy ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_STANDING_NO_ENEMY ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_STANDING_NO_ENEMY ][ i ];
    }
  } else if ( ( getMotion() == Constants::MOTION_STAND ) && closestEnemy && !hasTarget() ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_STANDING_ENEMY_AROUND ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_STANDING_ENEMY_AROUND ][ i ];
    }
  } else if ( ( getMotion() == Constants::MOTION_LOITER ) && !closestEnemy && !getPathManager()->atEndOfPath() ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_LOITERING_NO_ENEMY ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_LOITERING_NO_ENEMY ][ i ];
    }
  } else if ( ( getMotion() == Constants::MOTION_LOITER ) && closestEnemy && !hasTarget() ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_LOITERING_ENEMY_AROUND ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_LOITERING_ENEMY_AROUND ][ i ];
    }
  } else if ( ( getMotion() == Constants::MOTION_LOITER ) && getPathManager()->atEndOfPath() ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_LOITERING_END_OF_PATH ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_LOITERING_END_OF_PATH ][ i ];
    }
  } else if ( ( getMotion() == Constants::MOTION_MOVE_TOWARDS ) && hasTarget() ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_MOVING_TOWARDS_ENEMY ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_MOVING_TOWARDS_ENEMY ][ i ];
    }
  } else if ( ( getMotion() == Constants::MOTION_STAND ) && hasTarget() ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_ENGAGING_ENEMY ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_ENGAGING_ENEMY ][ i ];
    }
  }

  // Collect the HP/MP states.

  float remainingHP = getHp() / ( getMaxHp() ? getMaxHp() : 1 );

  if ( remainingHP <= LOW_HP ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_LOW_HP ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_LOW_HP ][ i ];
    }
  }

  if ( getTargetCreature() ) {
   remainingHP = getTargetCreature()->getHp() / ( getTargetCreature()->getMaxHp() ? getTargetCreature()->getMaxHp() : 1 );

    if ( remainingHP <= LOW_HP ) {
      for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_ENEMY_LOW_HP ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_ENEMY_LOW_HP ][ i ];
      }
    }
  }

  float remainingMP = getMp() / ( getMaxMp() ? getMaxMp() : 1 );

  if ( ( remainingMP <= LOW_MP ) && getMaxMp() ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_LOW_MP ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_LOW_MP ][ i ];
    }
  }

  if ( getTargetCreature() ) {
   remainingMP = getTargetCreature()->getMp() / ( getTargetCreature()->getMaxMp() ? getTargetCreature()->getMaxMp() : 1 );

    if ( ( remainingMP <= LOW_MP ) && getTargetCreature()->getMaxMp() ) {
      for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_ENEMY_LOW_MP ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_ENEMY_LOW_MP ][ i ];
      }
    }
  }

  // Collect the "I love to pimp my armor class" state.

  float armor, dodgePenalty;
  getArmor( &armor, &dodgePenalty, 0 );

  if ( armor < HIGH_AC ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_AC_NEEDS_PIMPING ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_AC_NEEDS_PIMPING ][ i ];
    }
  }

  // Get information for the last 3 states.

  int numAttackers = 0;
  int numFriendlies = closestFriends.size();
  int numFoes = closestEnemies.size();
  for( set<Creature*>::iterator i = closestEnemies.begin(); i != closestEnemies.end(); ++i ) {
  	Creature *c = *i;
  	if( c->getTargetCreature() == this ) numAttackers++;
  }

  // Collect the "surrounded" state.

  if ( numAttackers > 2 ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_SURROUNDED ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_SURROUNDED ][ i ];
    }
  }

  // Collect the "outnumbered" states.

  if ( numFoes > ( numFriendlies * 2 ) ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_OUTNUMBERED ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_OUTNUMBERED ][ i ];
    }
  } else if ( numFriendlies > ( numFoes * 2 ) ) {
    for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
      if ( decisionMatrix[ AI_STATE_FEW_ENEMIES ][ i ] > decisionWeights[ i ] ) decisionWeights[ i ] = decisionMatrix[ AI_STATE_FEW_ENEMIES ][ i ];
    }
  }

  // STEP 2: Process the accumulated weights.

  // Normalize the collected weights so their sum is 1.

  float weightSum = 0.0f;

  for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
    weightSum += decisionWeights[ i ];
  }
  for ( int i = 0; i < AI_ACTION_COUNT; i++ ) {
    decisionWeights[ i ] /= weightSum;
  }

  // TODO:Shift the weights towards their average for chaotic creatures.

  // STEP 3: Determine the action to do.

  int action;

  // Throw a dice against randomly picked nonzero weights.
  // NOTE: This will result in an endless loop if all weights are zero!

  while ( true ) {
    action = Util::pickOne( 0, AI_ACTION_COUNT - 1 );
    if ( decisionWeights[ action ] > 0.0f ) {
      if ( Util::roll( 0.0f, 1.0f ) <= decisionWeights[ action ] ) break;
    }
  }
  
#ifdef DEBUG_CREATURE_AI  
  cerr << getName() << " action=" << action << endl;
#endif
  
  switch ( action ) {
    case AI_ACTION_ATTACK_CLOSEST_ENEMY:
    	attackClosestTarget();
      break;
    case AI_ACTION_CAST_ATTACK_SPELL:
      castOffensiveSpell();
      break;
    case AI_ACTION_CAST_AREA_SPELL:
      castAreaSpell();
      break;
    case AI_ACTION_HEAL:
      if ( !castHealingSpell() ) useMagicItem();
      break;
    case AI_ACTION_CAST_AC_SPELL:
      castACSpell();
      break;
    case AI_ACTION_ATTACK_RANDOM_ENEMY:
      attackRandomTarget();
      break;
    case AI_ACTION_START_LOITERING:
      pathManager->findWanderingPath( CREATURE_LOITERING_RADIUS, session->getParty()->getPlayer(), session->getMap() );
      setMotion( Constants::MOTION_LOITER );
      break;
    case AI_ACTION_STOP_MOVING:
      setMotion( Constants::MOTION_STAND );
      stopMoving();
      break;
    case AI_ACTION_GO_ON: // GoOn (a no-op!)
      break;
  }

}

/// Returns the closest suitable battle target, NULL if no target found.

Creature *Creature::getClosestTarget() {
	return closestEnemy;
}

/// Returns a random suitable battle target, NULL if no target found.

Creature *Creature::getRandomTarget() {
	if( closestEnemies.size() == 0 ) return NULL;
	int n = Util::dice( closestEnemies.size() );
	set<Creature*>::iterator i = closestEnemies.begin();
	for( int t = 0; t < n; t++ ) {
		++i;
	}
	return *i;
}

Creature *Creature::getClosestFriend() {
	return closestFriend;
}

/// Returns a random suitable battle target, NULL if no target found.

Creature *Creature::getRandomFriend() {
	if( closestFriends.size() == 0 ) return NULL;
	int n = Util::dice( closestFriends.size() );
	set<Creature*>::iterator i = closestFriends.begin();
	for( int t = 0; t < n; t++ ) {
		++i;
	}
	return *i;
}

/// Makes the creature attack the closest suitable target. Returns true if target found.

bool Creature::attackClosestTarget() {

  Creature *p = getClosestTarget();

  if ( p ) {
    // attack with item
    setTargetCreature ( p );
  }

  return ( p != NULL );
}

/// Makes the creature attack a random suitable target. Returns true if target found.

bool Creature::attackRandomTarget() {
  Creature *p = getRandomTarget();

  if ( p ) {
    // attack with item
    setTargetCreature ( p );
  }

  return ( p != NULL );
}

/// Try to heal someone; returns true if someone was found.

bool Creature::castHealingSpell( bool checkOnly ) {
  vector<Spell*> healingSpells;
  Spell *spell;

  for ( int i = 0; i < getSpellCount(); i++ ) {
    spell = getSpell ( i );
    if ( spell->isFriendly() && ( spell->getMp() < getMp() ) && spell->hasStateModPrereq() && ( findClosestTargetWithPrereq( spell ) != NULL ) && ( spell->isStateModPrereqAPotionSkill() ? ( spell->getStateModPrereq() != Constants::AC ) : true ) ) {
      healingSpells.push_back ( spell );
    }
  }

  // We can't cast a healing spell, exit
  if ( healingSpells.empty() ) return false;

  if ( !checkOnly ) {
    spell = healingSpells[ Util::pickOne ( 0, healingSpells.size() - 1 ) ];
    setAction ( Constants::ACTION_CAST_SPELL, NULL, spell );
    setTargetCreature ( findClosestTargetWithPrereq( spell ) );
    setMotion( Constants::MOTION_MOVE_TOWARDS );
  }

  return true;
}

/// Tries to cast an offensive spell onto the targetted or nearest suitable creature.

bool Creature::castOffensiveSpell( bool checkOnly ) {
  vector<Spell*> offensiveSpells;
  Spell *spell;

  // Gather spells that are suitable for auto-casting.

  for ( int i = 0; i < getSpellCount(); i++ ) {
    spell = getSpell ( i );
    if ( !spell->isFriendly() && ( spell->getMp() < getMp() ) && spell->isCreatureTargetAllowed() && ( spell->hasStateModPrereq() ? findClosestTargetWithPrereq( spell ) != NULL : getClosestTarget() != NULL ) ) {
      offensiveSpells.push_back ( spell );
    }
  }

  // We can't cast an offensive spell, exit
  if ( offensiveSpells.empty() ) return false;

  if ( !checkOnly ) {
    spell = offensiveSpells[ Util::pickOne ( 0, offensiveSpells.size() - 1 ) ];
    setAction ( Constants::ACTION_CAST_SPELL, NULL, spell );
    if ( spell->hasStateModPrereq() ) setTargetCreature( findClosestTargetWithPrereq( spell ) ); else setTargetCreature( getClosestTarget() );
    setMotion( Constants::MOTION_MOVE_TOWARDS );
  }

  return true;
}

/// Tries to cast an area-affecting spell near the targetted or nearest suitable creature.

bool Creature::castAreaSpell( bool checkOnly ) {
  vector<Spell*> areaSpells;
  Spell *spell;

  // Gather spells that are suitable for auto-casting.

  for ( int i = 0; i < getSpellCount(); i++ ) {
    spell = getSpell ( i );
    if ( !spell->isFriendly() && ( spell->getMp() < getMp() ) && spell->isLocationTargetAllowed() && ( spell->hasStateModPrereq() ? findClosestTargetWithPrereq( spell ) != NULL : getClosestTarget() != NULL ) ) {
      areaSpells.push_back ( spell );
    }
  }

  // We can't cast an offensive spell, exit
  if ( areaSpells.empty() ) return false;

  if ( !checkOnly ) {
    spell = areaSpells[ Util::pickOne ( 0, areaSpells.size() - 1 ) ];
    setAction ( Constants::ACTION_CAST_SPELL, NULL, spell );
    if ( spell->hasStateModPrereq() ) setTargetCreature( findClosestTargetWithPrereq( spell ) ); else setTargetCreature( getClosestTarget() );
    setMotion( Constants::MOTION_MOVE_TOWARDS );
  }

  return true;
}

/// Try to raise someone's AC using a spell; returns true if someone was found.

bool Creature::castACSpell( bool checkOnly ) {
  vector<Spell*> acSpells;
  Spell *spell;

  for ( int i = 0; i < getSpellCount(); i++ ) {
    spell = getSpell ( i );
    if ( spell->isFriendly() && ( spell->getMp() < getMp() ) && spell->hasStateModPrereq() && ( findClosestTargetWithPrereq( spell ) != NULL ) && ( spell->isStateModPrereqAPotionSkill() ? ( spell->getStateModPrereq() == Constants::AC ) : false ) ) {
      acSpells.push_back ( spell );
    }
  }

  // We can't cast an AC raising spell, exit
  if ( acSpells.empty() ) return false;

  if ( !checkOnly ) {
    spell = acSpells[ Util::pickOne ( 0, acSpells.size() - 1 ) ];
    setAction ( Constants::ACTION_CAST_SPELL, NULL, spell );
    setTargetCreature ( findClosestTargetWithPrereq( spell ) );
    setMotion( Constants::MOTION_MOVE_TOWARDS );
  }

  return true;
}

/// Tries to use a healing magical item from the backpack on oneself.

bool Creature::useMagicItem( bool checkOnly ) {

  // If there are no magical items in the backpack, get outta here.
  if ( !backpack->getContainedItemCount() || !backpack->getContainsMagicItem() ) return false;

  vector<Item*> usefulItems;
  Item *item;

  // Search the backpack for potions and items with healing spells.
  for ( int i = 0; i < backpack->getContainedItemCount(); i++ ) {
    item = backpack->getContainedItem( i );
    if ( isWithPrereq( item ) ) {
      usefulItems.push_back( item );
    }
  }

  // If no items found, leave.
  if ( usefulItems.empty() ) return false;

  if ( !checkOnly ) {
    item = usefulItems [ Util::pickOne( 0, usefulItems.size() - 1 ) ];
    setAction( ( ( item->getRpgItem()->getPotionSkill() < 0 ) ? Constants::ACTION_EAT_DRINK : Constants::ACTION_CAST_SPELL ), item, item->getSpell() );
  }

  return true;
}

/// Returns the distance to the selected target spot.

float Creature::getDistanceToSel() {
	if ( selX > -1 && selY > -1 ) {
		return Constants::distance( getX(),  getY(), getShape()->getWidth(), getShape()->getDepth(), selX, selY, 1, 1 );
	}

return 0;
}

/// Returns the distance to another creature.

float Creature::getDistance( RenderedCreature *other ) {
	return Constants::distance( getX(),  getY(),
	                            getShape()->getWidth(), getShape()->getDepth(),
	                            other->getX(),
	                            other->getY(),
	                            other->getShape()->getWidth(),
	                            other->getShape()->getDepth() );
}

/// Returns the distance to a creature or if not given, the selected target.

float Creature::getDistanceToTarget( RenderedCreature *creature ) {
	if ( creature ) return getDistance( creature );

	if ( !hasTarget() ) return 0.0f;
	if ( getTargetCreature() ) {
		return getDistance( getTargetCreature() );
	} else if ( getTargetX() || getTargetY() ) {
		if ( getTargetItem() ) {
			return Constants::distance( getX(),  getY(),
			                            getShape()->getWidth(), getShape()->getDepth(),
			                            getTargetX(), getTargetY(),
			                            getTargetItem()->getShape()->getWidth(),
			                            getTargetItem()->getShape()->getDepth() );
		} else {
			return Constants::distance( getX(),  getY(),
			                            getShape()->getWidth(), getShape()->getDepth(),
			                            getTargetX(), getTargetY(), 1, 1 );
		}
	} else {
		return 0.0f;
	}
}

/// Sets the experience required for the character to level up.

void Creature::setExp() {
	if ( !( isPartyMember() || isWanderingHero() ) ) return;
	expOfNextLevel = 0;
	for ( int i = 0; i < level - 1; i++ ) {
		expOfNextLevel += ( ( i + 1 ) * character->getLevelProgression() );
	}
}

/// Calculates how far the creature has moved since the last frame.

GLfloat Creature::getStep() {
	GLfloat fps = session->getGameAdapter()->getFps();
	GLfloat div = FPS_ONE + static_cast<float>( ( 4 - session->getPreferences()->getGameSpeedLevel() ) * 3.0f );
	if ( fps < div ) return 0.8f;
	GLfloat step = 1.0f / ( fps / div  );
	if ( pathManager->getSpeed() <= 0 ) {
		step *= 1.0f - ( 1.0f / 10.0f );
	} else if ( pathManager->getSpeed() >= 10 ) {
		step *= 1.0f - ( 9.9f / 10.0f );
	} else {
		step *= ( 1.0f - ( ( GLfloat )( pathManager->getSpeed() ) / 10.0f ) );
	}
	return step;
}

/// Returns a brief description of the creature.

void Creature::getDetailedDescription( std::string& s ) {
	char tempdesc[256] = {0};

	int alignmentPercent = (int)( getAlignment() * 100 );
	char alignmentDesc[32];

	if ( alignmentPercent < 20 ) {
		snprintf( alignmentDesc, 32, _( "Utterly chaotic" ) );
	} else if ( alignmentPercent >= 20 && alignmentPercent < 40 ) {
		snprintf( alignmentDesc, 32, _( "Chaotic" ) );
	} else if ( alignmentPercent >= 40 && alignmentPercent < 60 ) {
		snprintf( alignmentDesc, 32, _( "Neutral" ) );
	} else if ( alignmentPercent >= 60 && alignmentPercent < 80 ) {
		snprintf( alignmentDesc, 32, _( "Lawful" ) );
	} else if ( alignmentPercent >= 80 ) {
		snprintf( alignmentDesc, 32, _( "Overly lawful" ) );
	}

	snprintf( tempdesc, 255, _( "%s (L:%d HP:%d/%d MP:%d/%d AL:%d%%(%s))" ), _( getName() ), getLevel(), getHp(), getMaxHp(), getMp(), getMaxMp(), alignmentPercent, alignmentDesc );

	s = tempdesc;

	if ( session->getCurrentMission() && session->getCurrentMission()->getMissionId() == getMissionId() ) {
		s += _( " *Mission*" );
	}
	if ( boss ) {
		s += _( " *Boss*" );
	}

}

/// Draws the creature.

void Creature::draw() {
	getShape()->draw();
}

/// Adds additional NPC-only info.

void Creature::setNpcInfo( NpcInfo *npcInfo ) {
	this->npcInfo = npcInfo;
	setName( npcInfo->name );

	// for merchants, re-create backpack with the correct types
	if ( npcInfo->type == Constants::NPC_TYPE_MERCHANT ) {
		// drop everything beyond the basic backpack
		for ( int i = getMonster()->getStartingItemCount(); i < this->getBackpackContentsCount(); i++ ) {
			this->removeFromBackpack( getMonster()->getStartingItemCount() );
		}

		std::vector<int> types( npcInfo->getSubtype()->size() + 1 );
		int typeCount = 0;
		for ( set<int>::iterator e = npcInfo->getSubtype()->begin(); e != npcInfo->getSubtype()->end(); ++e ) {
			types[ typeCount++ ] = *e;
		}

		// equip merchants at the party's level
		int level = ( session->getParty() ?
		              toint( ( static_cast<float>( session->getParty()->getTotalLevel() ) ) / ( static_cast<float>( session->getParty()->getPartySize() ) ) ) :
		              getLevel() );
		if ( level < 0 ) level = 1;

		// add some loot
		int nn = Util::pickOne( 3, 7 );
		//cerr << "Adding loot:" << nn << endl;
		for ( int i = 0; i < nn; i++ ) {
			Item *loot;
			if ( npcInfo->isSubtype( RpgItem::SCROLL ) ) {
				Spell *spell = MagicSchool::getRandomSpell( level );
				loot = session->newItem( RpgItem::getItemByName( "Scroll" ),
				                         level,
				                         spell );
			} else {
				loot =
				  session->newItem(
				    RpgItem::getRandomItemFromTypes( session->getGameAdapter()->
				        getCurrentDepth(),
				        &types[0], typeCount ),
				    level );
			}
			//cerr << "\t" << loot->getRpgItem()->getName() << endl;
			// make it contain all items, no matter what size
			addToBackpack( loot );
		}
	}
}

/// Sets up the special capabilities of the creature.

void Creature::evalSpecialSkills() {
	
	if( !( isPartyMember() || isWanderingHero() ) || !SQUIRREL_ENABLED ) return;
	
	set<SpecialSkill*> oldSpecialSkills;
	for ( int t = 0; t < SpecialSkill::getSpecialSkillCount(); t++ ) {
		SpecialSkill *ss = SpecialSkill::getSpecialSkill( t );
		if ( specialSkills.find( ss ) != specialSkills.end() ) {
			oldSpecialSkills.insert( ss );
		}
	}
	enum {TMP_SIZE = 120};
	char tmp[TMP_SIZE];
	specialSkills.clear();
	specialSkillNames.clear();
	HSQOBJECT *param = session->getSquirrel()->getCreatureRef( this );
	if ( param ) {
		bool result;
		for ( int t = 0; t < SpecialSkill::getSpecialSkillCount(); t++ ) {
			SpecialSkill *ss = SpecialSkill::getSpecialSkill( t );
			session->getSquirrel()->
			callBoolMethod( ss->getSquirrelFunctionPrereq(),
			                param,
			                &result );
			if ( result ) {
				specialSkills.insert( ss );
				string skillName = ss->getName();
				specialSkillNames.insert( skillName );
				if ( oldSpecialSkills.find( ss ) == oldSpecialSkills.end() ) {
					if ( session->getParty()->isPartyMember( this ) ) {
						snprintf( tmp, TMP_SIZE, _( "%1$s gains the %2$s special ability!" ), getName(), ss->getDisplayName() );
						session->getGameAdapter()->writeLogMessage( tmp, Constants::MSGTYPE_STATS );
					}
				}
			}
		}
	}
	for ( int t = 0; t < SpecialSkill::getSpecialSkillCount(); t++ ) {
		SpecialSkill *ss = SpecialSkill::getSpecialSkill( t );
		if ( specialSkills.find( ss ) == specialSkills.end() &&
		        oldSpecialSkills.find( ss ) != oldSpecialSkills.end() ) {
			if ( session->getParty()->isPartyMember( this ) ) {
				snprintf( tmp, TMP_SIZE, _( "%1$s looses the %2$s special ability!" ), getName(), ss->getDisplayName() );
				session->getGameAdapter()->writeLogMessage( tmp, Constants::MSGTYPE_STATS );
			}
		}
	}
}

/// Sets the value of a skill.

void Creature::setSkill( int index, int value ) {
	int oldValue = getSkill( index );
	skills[index] = ( value < 0 ? 0 : value > 100 ? 100 : value );
	skillChanged( index, oldValue, getSkill( index ) );
	evalSpecialSkills();
	session->getParty()->recomputeMaxSkills();
}

/// Sets the additional skill bonus (on top of the base value).

void Creature::setSkillBonus( int index, int value ) {
	int oldValue = getSkill( index );
	skillBonus[index] = value;
	skillChanged( index, oldValue, getSkill( index ) );
	session->getParty()->recomputeMaxSkills();
}

/// Sets the not yet applied skill amount.

void Creature::setSkillMod( int index, int value ) {
	int oldValue = getSkill( index );
	skillMod[ index ] = ( value < 0 ? 0 : value );
	skillChanged( index, oldValue, getSkill( index ) );
	session->getParty()->recomputeMaxSkills();
}

/// Recalculates skills when stats change.

void Creature::skillChanged( int index, int oldValue, int newValue ) {
	// while loading don't update skill values.
	if ( loading ) return;

	if ( Skill::skills[ index ]->getGroup()->isStat() && character ) {
		for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
			int oldPrereq = 0;
			int newPrereq = 0;
			for ( int t = 0; t < Skill::skills[i]->getPreReqStatCount(); t++ ) {
				int statIndex = Skill::skills[i]->getPreReqStat( t )->getIndex();
				if ( statIndex == index ) {
					oldPrereq += oldValue;
					newPrereq += newValue;
				} else {
					oldPrereq += getSkill( statIndex );
					newPrereq += getSkill( statIndex );
				}
			}
			oldPrereq = static_cast<int>( ( oldPrereq / static_cast<float>( Skill::skills[i]->getPreReqStatCount() ) ) *
			                              static_cast<float>( Skill::skills[i]->getPreReqMultiplier() ) );
			newPrereq = static_cast<int>( ( newPrereq / static_cast<float>( Skill::skills[i]->getPreReqStatCount() ) ) *
			                              static_cast<float>( Skill::skills[i]->getPreReqMultiplier() ) );
			if ( oldPrereq != newPrereq ) {
				setSkill( i, getSkill( i ) + ( newPrereq - oldPrereq ) );
				armorChanged = true;
			}
		}
	}
}

/// Applies the skill point based changes to skills.

void Creature::applySkillMods() {
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		if ( skillMod[ i ] > 0 ) {
			setSkill( i, skills[ i ] + skillMod[ i ] );
			skillMod[ i ] = 0;
		}
	}
	availableSkillMod = 0;
	hasAvailableSkillPoints = false;
}

/// Sets the active state of a state mod.

void Creature::setStateMod( int mod, bool setting ) {
	if ( setting ) stateMod |= ( 1 << mod );
	else stateMod &= ( ( GLuint )0xffff - ( GLuint )( 1 << mod ) );
	evalSpecialSkills();
}

/// Sets the "protected" state of a state mod (not influenceable by spells etc.)

void Creature::setProtectedStateMod( int mod, bool setting ) {
	if ( setting ) protStateMod |= ( 1 << mod );
	else protStateMod &= ( ( GLuint )0xffff - ( GLuint )( 1 << mod ) );
	evalSpecialSkills();
}

/// Applies the effects of recurring (periodic) special capabilities.

void Creature::applyRecurringSpecialSkills() {
	for ( int t = 0; t < SpecialSkill::getSpecialSkillCount(); t++ ) {
		SpecialSkill *skill = SpecialSkill::getSpecialSkill( t );
		if ( skill->getType() == SpecialSkill::SKILL_TYPE_RECURRING &&
		        hasSpecialSkill( skill ) ) {
			useSpecialSkill( skill, false );
		}
	}
}

/// Applies the effects of automatic special capabilities.
float Creature::applyAutomaticSpecialSkills( int event, char *varName, float varValue ) {
	if( !( isPartyMember() || isWanderingHero() ) || !SQUIRREL_ENABLED ) return varValue;
	
#ifdef DEBUG_CAPABILITIES
	cerr << "Using automatic capabilities for event type: " << event << endl;
#endif
	for ( int t = 0; t < SpecialSkill::getSpecialSkillCount(); t++ ) {
		SpecialSkill *skill = SpecialSkill::getSpecialSkill( t );
		if ( skill->getEvent() == event &&
		        skill->getType() == SpecialSkill::SKILL_TYPE_AUTOMATIC &&
		        hasSpecialSkill( skill ) ) {
#ifdef DEBUG_CAPABILITIES
			cerr << "\tusing capability: " << skill->getDisplayName() <<
			" and setting var: " <<
			varName << "=" << varValue << endl;
#endif
			session->getSquirrel()->setGlobalVariable( varName, varValue );
			useSpecialSkill( skill, false );
			varValue = session->getSquirrel()->getGlobalVariable( varName );
#ifdef DEBUG_CAPABILITIES
			cerr << "\t\tgot back " << varValue << endl;
#endif
		}
	}
#ifdef DEBUG_CAPABILITIES
	cerr << "final value=" << varValue << " ===============================" << endl;
#endif
	return varValue;
}

/// Uses a special capability.

char *Creature::useSpecialSkill( SpecialSkill *specialSkill, bool manualOnly ) {
	if( monster || !SQUIRREL_ENABLED ) return NULL;
	
	if ( !hasSpecialSkill( specialSkill ) ) {
		return Constants::getMessage( Constants::UNMET_CAPABILITY_PREREQ_ERROR );
	} else if ( manualOnly &&
	            specialSkill->getType() !=
	            SpecialSkill::SKILL_TYPE_MANUAL ) {
		return Constants::getMessage( Constants::CANNOT_USE_AUTO_CAPABILITY_ERROR );
	}
	HSQOBJECT *param = session->getSquirrel()->getCreatureRef( this );
	if ( param ) {
		session->getSquirrel()->
		callNoArgMethod( specialSkill->getSquirrelFunctionAction(),
		                 param );
		return NULL;
	} else {
		cerr << "*** Error: can't find squarrel reference for creature: " << getName() << endl;
		return NULL;
	}
}






/**
 * ============================================================
 * ============================================================
 *
 * New battle system calls
 *
 * damage=attack_roll - ac
 * attack_roll=(item_action + item_level) % skill
 * ac=(armor_total + avg_armor_level) % skill
 * skill=avg. of ability skill (power,coord, etc.), item skill, luck
 *
 * ============================================================
 * ============================================================
 *
 * Fixme:
 * -currently, extra attacks are not used. (use SPEED skill to eval?)
 * -magic item armor
 * -automatic capabilities
 *
 * move here from battle.cpp:
 * -critical hits (2x,3x,damage,etc.)
 * -conditions modifiers
 *
 * ** Look in old methods and battle.cpp for more info
 */

// 1 item level equals this many character levels in combat
#define ITEM_LEVEL_DIVISOR 8.0f

// base weapon damage of an attack with bare hands
#define HAND_ATTACK_DAMAGE Dice(1,4,0)

/// Returns the creature's chance to dogde the specified attack.

float Creature::getDodge( Creature *attacker, Item *weapon ) {
	// the target's dodge if affected by angle of attack
	bool inFOV =
	  Util::isInFOV( getX(), getY(), getTargetAngle(),
	                 attacker->getX(), attacker->getY() );

	// the target's dodge
	float armor, dodgePenalty;
	getArmor( &armor, &dodgePenalty,
	          weapon ? weapon->getRpgItem()->getDamageType() : 0,
	          weapon );
	float dodge = getSkill( Skill::DODGE_ATTACK ) - dodgePenalty;
	if ( !inFOV ) {
		dodge /= 2.0f;
		if ( getCharacter() ) {
			session->getGameAdapter()->writeLogMessage( _( "...Attack from blind-spot!" ), Constants::MSGTYPE_PLAYERBATTLE );
		} else {
			session->getGameAdapter()->writeLogMessage( _( "...Attack from blind-spot!" ), Constants::MSGTYPE_NPCBATTLE );
		}
	}
	return dodge;
}

/// Returns the chance the creature's armor deflects the damage from the specified attack.

float Creature::getArmor( float *armorP, float *dodgePenaltyP,
                          int damageType, Item *vsWeapon ) {
	calcArmor( damageType, &armor, dodgePenaltyP, ( vsWeapon ? true : false ) );

	// negative feedback: for monsters only, allow hits now and then
	// -=K=-: the sentence in if seems screwed up
	if ( monster &&
	        ( Util::mt_rand() <
	          monsterToughness[ session->getPreferences()->getMonsterToughness() ].
	          armorMisfuction ) ) {
		// 3.0f * rand() / RAND_MAX < 1.0f ) {
		armor = Util::roll( 0.0f, armor / 2.0f );
	} else if( !monster && SQUIRREL_ENABLED ) {
		// apply any armor enhancing capabilities
		if ( vsWeapon ) {
			session->getSquirrel()->setCurrentWeapon( vsWeapon );
			armor = applyAutomaticSpecialSkills( SpecialSkill::SKILL_EVENT_ARMOR,
			        "armor", armor );
		}
	}

	*armorP = armor;

	return armor;
}

/// Calculates armor and dodge penalty.

void Creature::calcArmor( int damageType,
                          float *armorP,
                          float *dodgePenaltyP,
                          bool callScript ) {
	if ( armorChanged ) {
		for ( int t = 0; t < RpgItem::DAMAGE_TYPE_COUNT; t++ ) {
			lastArmor[ t ] = ( monster ? monster->getBaseArmor() : 0 );
			lastDodgePenalty[ t ] = 0;
			int armorCount = 0;
			for(int i = 0; i < Constants::EQUIP_LOCATION_COUNT; i++) {
				if ( equipped[i] != MAX_BACKPACK_SIZE ) {
					Item *item = backpack->getContainedItem( equipped[ i ] );
					if ( item->getRpgItem()->getType() == RpgItem::ARMOR ||
					        ( item->isMagicItem() && item->getBonus() > 0 && !item->getRpgItem()->isWeapon() ) ) {

						int n = ( item->getRpgItem()->getType() == RpgItem::ARMOR ?
						          item->getRpgItem()->getDefense( t ) :
						          item->getBonus() );

						if ( callScript && !monster && SQUIRREL_ENABLED ) {
							session->getSquirrel()->setGlobalVariable( "armor", lastArmor[ t ] );
							session->getSquirrel()->callItemEvent( this, item, "useItemInDefense" );
							lastArmor[ t ] = session->getSquirrel()->getGlobalVariable( "armor" );
						}
						lastArmor[ t ] += n;
						lastDodgePenalty[ t ] += item->getRpgItem()->getDodgePenalty();

						// item's level has a small influence.
						lastArmor[ t ] += item->getLevel() / 8;

						// apply the armor influence... it uses the first
						// influence slot (AP_INFLUENCE)
						lastArmor[ t ] +=
						  getInfluenceBonus( item, AP_INFLUENCE,
						                     ( callScript ? "CTH" : NULL ) );

						armorCount++;
					}
				}
			}
			lastArmor[ t ] += bonusArmor;
			if ( lastArmor[ t ] < 0 ) lastArmor[ t ] = 0;
		}
		armorChanged = false;
	}

	*armorP = lastArmor[ damageType ];
	*dodgePenaltyP = lastDodgePenalty[ damageType ];
}

#define MAX_RANDOM_DAMAGE 2.0f

float power( float base, int e ) {
	float n = 1;
	for ( int i = 0; i < e; i++ ) {
		n *= base;
	}
	return n;
}

float Creature::getInfluenceBonus( Item *weapon,
    int influenceType,
    char const* debugMessage ) {

	if ( !weapon ) return 0;

	float bonus = 0;
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
		WeaponInfluence *minInfluence = weapon->getRpgItem()->getWeaponInfluence( i, influenceType, MIN_INFLUENCE );
		WeaponInfluence *maxInfluence = weapon->getRpgItem()->getWeaponInfluence( i, influenceType, MAX_INFLUENCE );

		float n = 0;
		int value = getSkill( i );
		if ( minInfluence->limit > -1 && minInfluence->limit > value ) {
			switch ( minInfluence->type ) {
			case 'E' :
				// exponential malus
				n = -power( minInfluence->base, minInfluence->limit - value );
				break;
			case 'L' :
				// linear
				n = -( minInfluence->limit - value ) * minInfluence->base;
				break;
			default:
				cerr << "*** Error: unknown influence type for item: " << weapon->getRpgItem()->getDisplayName() << endl;
			}
		} else if ( maxInfluence->limit > -1 && maxInfluence->limit < value ) {
			switch ( maxInfluence->type ) {
			case 'E' :
				// exponential bonus
				n = power( maxInfluence->base, value - maxInfluence->limit );
				break;
			case 'L' :
				// linear bonus
				n = ( value - maxInfluence->limit ) * maxInfluence->base;
				break;
			default:
				cerr << "*** Error: unknown influence type for item: " << weapon->getRpgItem()->getDisplayName() << endl;
			}
		}

		if ( n != 0 && debugMessage &&
		        session->getPreferences()->getCombatInfoDetail() > 0 ) {
			char message[120];
			snprintf( message, 120, "...%s %s:%s %d-%d %s %d, %s=%.2f",
			          debugMessage,
			          _( "skill" ),
			          Skill::skills[i]->getDisplayName(),
			          minInfluence->limit,
			          maxInfluence->limit,
			          _( "vs." ),
			          value,
			          _( "bonus" ),
			          n );
			session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_SYSTEM );
		}

		bonus += n;
	}
	return bonus;
}

/// Returns the chance to hit with a weapon, as well as the skill used for the weapon.

void Creature::getCth( Item *weapon, float *cth, float *skill, bool showDebug ) {
	// the attacker's skill
	*skill = getSkill( weapon ?
	                   weapon->getRpgItem()->getDamageSkill() :
	                   Skill::HAND_TO_HAND_COMBAT );

	// The max cth is closer to the skill to avoid a lot of misses
	// This is ok, since dodge is subtracted from it anyway.
	float maxCth = *skill * 1.5f;
	if ( maxCth > 100 ) maxCth = 100;
	if ( maxCth < 40 ) maxCth = 40;

	// roll chance to hit (CTH)
	*cth = Util::roll( 0.0f, maxCth );

	// item's level has a small influence
	if ( weapon ) *skill += weapon->getLevel() / 2;

	// Apply COORDINATION influence to skill
	// (As opposed to subtracting it from cth. This is b/c
	// skill is shown in the characterInfo as the cth.)
	*skill += getInfluenceBonus( weapon, CTH_INFLUENCE,
	                             ( showDebug ? "CTH" : NULL ) );
	if ( *skill < 0 ) *skill = 0;

	if ( showDebug && session->getPreferences()->getCombatInfoDetail() > 0 ) {
		char message[120];
		snprintf( message, 120, "...%s:%.2f (%s:%.2f) %s %s:%.2f",
		          _( "CTH" ),
		          *cth,
		          _( "max" ),
		          maxCth,
		          _( "vs." ),
		          _( "skill" ),
		          *skill );
		session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_SYSTEM );
	}
}

/// Returns the chance to successfully attack with a weapon, as well as the min and max damage.

float Creature::getAttack( Item *weapon,
                           float *maxP,
                           float *minP,
                           bool callScript ) {

	float power;
	if ( weapon && weapon->getRpgItem()->isRangedWeapon() ) {
		power = getSkill( Skill::POWER ) / 2.0f +
		        getSkill( Skill::COORDINATION ) / 2.0f;
	} else {
		power = getSkill( Skill::POWER );
	}

	// the min/max power value
	float minPower, maxPower;
	if ( power < 10 ) {
		// 1d10
		minPower = 1; maxPower = 10;
	} else if ( power < 15 ) {
		// 2d10
		minPower = 2; maxPower = 20;
	} else {
		// 3d10
		minPower = 3; maxPower = 30;
	}

	// What percent of power is given by weapon?
	// (For unarmed combat it's a coordination bonus.)
	float damagePercent = ( weapon ?
	                        weapon->getRpgItem()->getDamage() :
	                        getSkill( Skill::COORDINATION ) +
	                        getSkill( Skill::POWER ) );

	// item's level has a small influence
	if ( weapon ) damagePercent += weapon->getLevel() / 2;

	// apply POWER influence
	damagePercent += getInfluenceBonus( weapon, DAM_INFLUENCE,
	                 ( callScript ? "DAM" : NULL ) );
	if ( damagePercent < 0 ) damagePercent = 0;

	if ( minP ) {
		*minP = ( minPower / 100.0f ) * damagePercent;
	}
	if ( maxP ) {
		*maxP = ( maxPower / 100.0f ) * damagePercent;
	}

	// roll the power
	float roll = Util::roll( minPower, maxPower );

	// take the weapon's skill % of the max power
	roll = ( roll / 100.0f ) * damagePercent;

	// apply damage enhancing capabilities
	if ( callScript && isPartyMember() && SQUIRREL_ENABLED ) {
		session->getSquirrel()->setCurrentWeapon( weapon );
		roll = applyAutomaticSpecialSkills( SpecialSkill::SKILL_EVENT_DAMAGE,
		       "damage", roll );
		if ( weapon )
			session->getSquirrel()->setGlobalVariable( "damage", roll );
		session->getSquirrel()->callItemEvent( this, weapon, "useItemInAttack" );
		roll = session->getSquirrel()->getGlobalVariable( "damage" );
	}

	return roll;
}

/// The chance to parry a successful attack before it hits armor.

float Creature::getParry( Item **parryItem ) {
	int location[] = {
		Constants::EQUIP_LOCATION_RIGHT_HAND,
		Constants::EQUIP_LOCATION_LEFT_HAND,
		Constants::EQUIP_LOCATION_WEAPON_RANGED,
		-1
	};
	float ret = 0;
	float maxParry = 0;
	for ( int i = 0; location[i] > -1; i++ ) {
		Item *item = getEquippedItem( location[i] );
		if ( !item ) continue;
		if ( item->getRpgItem()->getDefenseSkill() == Skill::SHIELD_DEFEND ) {
			// parry using a shield: use shield skill to parry
			maxParry = getSkill( item->getRpgItem()->getDefenseSkill() );
		} else if ( item->getRpgItem()->getParry() > 0 ) {
			// parry using a weapon: get max parry skill amount (% of weapon skill)
			maxParry =
			  ( getSkill( item->getRpgItem()->getDamageSkill() ) / 100.0f ) *
			  item->getRpgItem()->getParry();

			// use the item's CTH skill to modify parry also
			maxParry += getInfluenceBonus( item, CTH_INFLUENCE, "PARRY" );
			if ( maxParry < 0 ) maxParry = 0;

		} else {
			// no parry with this hand
			continue;
		}
		// roll to parry
		float parry = Util::roll( 0.0f, maxParry );
		// select the highest value
		if ( ret == 0 || ret < parry ) {
			ret = parry;
			*parryItem = item;
		}
	}
	return ret;
}

/// Modifies the attack roll according to the attacked creature's active state mods.

float Creature::getDefenderStateModPercent( bool magical ) {
	/*
	  apply state_mods:
	  (Done here so it's used for spells too)
	  
	  blessed,
	  empowered,
	  enraged,
	  ac_protected,
	  magic_protected,

	  drunk,

	  poisoned,
	  cursed,
	  possessed,
	  blinded,
	  charmed,
	  changed,
	  overloaded,
	*/
	float delta = 0.0f;
	if ( getStateMod( StateMod::blessed ) ) {
		delta += Util::roll( 0.0f, 10.0f );
	}
	if ( getStateMod( StateMod::empowered ) ) {
		delta += Util::roll( 5.0f, 15.0f );
	}
	if ( getStateMod( StateMod::enraged ) ) {
		delta += Util::roll( 8.0f, 18.0f );
	}
	if ( getStateMod( StateMod::drunk ) ) {
		delta += Util::roll( -7.0f, 7.0f );
	}
	if ( getStateMod( StateMod::cursed ) ) {
		delta -= Util::roll( 5.0f, 15.0f );
	}
	if ( getStateMod( StateMod::blinded ) ) {
		delta -= Util::roll( 0.0f, 10.0f );
	}
	if ( !magical && getTargetCreature()->getStateMod( StateMod::ac_protected ) ) {
		delta -= Util::roll( 0.0f, 7.0f );
	}
	if ( magical && getTargetCreature()->getStateMod( StateMod::magic_protected ) ) {
		delta -= ( 7.0f * Util::mt_rand() );
	}
	if ( getTargetCreature()->getStateMod( StateMod::blessed ) ) {
		delta -= Util::roll( 0.0f, 5.0f );
	}
	if ( getTargetCreature()->getStateMod( StateMod::cursed ) ) {
		delta += Util::roll( 0.0f, 5.0f );
	}
	if ( getTargetCreature()->getStateMod( StateMod::overloaded ) ) {
		delta += Util::roll( 0.0f, 2.0f );
	}
	if ( getTargetCreature()->getStateMod( StateMod::blinded ) ) {
		delta += Util::roll( 0.0f, 2.0f );
	}
	if ( getTargetCreature()->getStateMod( StateMod::invisible ) ) {
		delta -= Util::roll( 0.0f, 10.0f );
	}
	return delta;
}

/// Modifies the attack roll according to the creature's active state mods.

float Creature::getAttackerStateModPercent() {
	/*
	  apply state_mods:
	  blessed,
	 empowered,
	 enraged,
	 ac_protected,
	 magic_protected,
	  
	 drunk,
	  
	 poisoned,
	 cursed,
	 possessed,
	 blinded,
	 charmed,
	 changed,
	 overloaded,
	*/
	float delta = 0.0f;
	if ( getStateMod( StateMod::blessed ) ) {
		delta += Util::roll( 0.0f, 15.0f );
	}
	if ( getStateMod( StateMod::empowered ) ) {
		delta += Util::roll( 10.0f, 25.0f );
	}
	if ( getStateMod( StateMod::enraged ) ) {
		delta -= Util::roll( 0.0f, 10.0f );
	}
	if ( getStateMod( StateMod::drunk ) ) {
		delta += Util::roll( -15.0f, 15.0f );
	}
	if ( getStateMod( StateMod::cursed ) ) {
		delta -= Util::roll( 10.0f, 25.0f );
	}
	if ( getStateMod( StateMod::blinded ) ) {
		delta -= Util::roll( 0.0f, 15.0f );
	}
	if ( getStateMod( StateMod::overloaded ) ) {
		delta -= Util::roll( 0.0f, 10.0f );
	}
	if ( getStateMod( StateMod::invisible ) ) {
		delta += Util::roll( 5.0f, 10.0f );
	}
	return delta;
}

/// Returns a semi-random amount of magical damage for an item.

float Creature::rollMagicDamagePercent( Item *item ) {
	float itemLevel = ( item->getLevel() - 1 ) / ITEM_LEVEL_DIVISOR;
	return item->rollMagicDamage() + itemLevel;
}

/// Returns the number of action points the creature receives at each battle turn.

float Creature::getMaxAP( ) {
	return( static_cast<float>( getSkill( Skill::COORDINATION ) ) + static_cast<float>( getSkill( Skill::SPEED ) ) ) / 2.0f;
}

/// Returns the attacks per round possible with item.

float Creature::getAttacksPerRound( Item *item ) {
	return( getMaxAP() / getWeaponAPCost( item, false ) );
}

/// Returns the action point cost of using a specific item.

float Creature::getWeaponAPCost( Item *item, bool showDebug ) {
	float baseAP = ( item ?
	                 item->getRpgItem()->getAP() :
	                 Constants::HAND_WEAPON_SPEED );
	// never show debug (called a lot)
	// Apply a max 3pt influence to weapon AP
	baseAP -= getInfluenceBonus( item, AP_INFLUENCE, NULL );
	// can't be free..
	if ( baseAP < 1 ) baseAP = 1;
	return baseAP;
}

/// Checks whether an item can be equipped, returns error message or NULL if successful.

char *Creature::canEquipItem( Item *item, bool interactive ) {

	// check item tags to see if this item can be equipped.
	if ( character ) {
		if ( !character->canEquip( item->getRpgItem() ) ) {
			return Constants::getMessage( Constants::ITEM_ACL_VIOLATION );
		}
	}

	// check the level
	if ( getLevel() < item->getLevel() ) {
		return Constants::getMessage( Constants::ITEM_LEVEL_VIOLATION );
	}

	// two handed weapon violations
  if( item->getRpgItem()->getEquip() & Constants::EQUIP_LOCATION_LEFT_HAND ||
      item->getRpgItem()->getEquip() & Constants::EQUIP_LOCATION_RIGHT_HAND ) {
    Item *leftHandWeapon = getEquippedItem( Constants::EQUIP_LOCATION_LEFT_HAND );
    Item *rightHandWeapon = getEquippedItem( Constants::EQUIP_LOCATION_RIGHT_HAND );
		bool bothHandsFree = !( leftHandWeapon || rightHandWeapon );
		bool holdsTwoHandedWeapon =
		  ( ( leftHandWeapon && leftHandWeapon->getRpgItem()->getTwoHanded() == RpgItem::ONLY_TWO_HANDED ) ||
		    ( rightHandWeapon && rightHandWeapon->getRpgItem()->getTwoHanded() == RpgItem::ONLY_TWO_HANDED ) );

		if ( holdsTwoHandedWeapon ||
		        ( !bothHandsFree &&
		          item->getRpgItem()->getTwoHanded() == RpgItem::ONLY_TWO_HANDED ) ) {
			if ( interactive ) {
				return Constants::getMessage( Constants::ITEM_TWO_HANDED_VIOLATION );
			}
		}
	}
	return NULL;
}

/// Sets character info (if not monster/NPC).

void Creature::setCharacter( Character *c ) {
	assert( isPartyMember() || isWanderingHero() );
	character = c;
}

/// Plays a character sound of the specified type.

void Creature::playCharacterSound( int soundType, int panning ) {
	if ( !monster )
		session->getSound()->playCharacterSound( model_name.c_str(), soundType, panning );
}

/// Does a roll against a skill, optionally with a luck modifier.

bool Creature::rollSkill( int skill, float luckDiv ) {
	float f = static_cast<float>( getSkill( skill ) );
	if ( luckDiv > 0 )
		f += static_cast<float>( getSkill( Skill::LUCK ) ) / luckDiv;
	return( Util::roll( 0.0f, 100.0f ) <= f );
}

/// Does a secret door discovery roll, returns true if successful.

#define SECRET_DOOR_ATTEMPT_INTERVAL 5000
bool Creature::rollSecretDoor( Location *pos ) {
	if ( secretDoorAttempts.find( pos ) != secretDoorAttempts.end() ) {
		Uint32 lastTime = secretDoorAttempts[ pos ];
		if ( SDL_GetTicks() - lastTime < SECRET_DOOR_ATTEMPT_INTERVAL ) return false;
	}
	bool ret = rollSkill( Skill::FIND_SECRET_DOOR, 4.0f );
	if ( !ret ) {
		secretDoorAttempts[ pos ] = SDL_GetTicks();
	}
	return ret;
}

void Creature::resetSecretDoorAttempts() {
	secretDoorAttempts.clear();
}

/// Unused.

#define TRAP_FIND_ATTEMPT_INTERVAL 500
bool Creature::rollTrapFind( Trap *trap ) {
	if ( trapFindAttempts.find( trap ) != trapFindAttempts.end() ) {
		Uint32 lastTime = trapFindAttempts[ trap ];
		if ( SDL_GetTicks() - lastTime < TRAP_FIND_ATTEMPT_INTERVAL ) return false;
	}
	bool ret = rollSkill( Skill::FIND_TRAP, 0.5f ); // traps are easy to notice
	if ( !ret ) {
		trapFindAttempts[ trap ] = SDL_GetTicks();
	}
	return ret;
}

void Creature::resetTrapFindAttempts() {
	trapFindAttempts.clear();
}

/// Does a perception roll, discovers secret doors and traps if successful.

void Creature::rollPerception() {

	Uint32 now = SDL_GetTicks();
	if ( now - lastPerceptionCheck < PERCEPTION_DELTA ) return;
	lastPerceptionCheck = now;

	// find traps
	set<Uint8> *trapsShown = session->getMap()->getTrapsShown();
	for ( set<Uint8>::iterator e = trapsShown->begin(); e != trapsShown->end(); ++e ) {
		Uint8 trapIndex = *e;
		Trap *trap = session->getMap()->getTrapLoc( trapIndex );
		if ( trap->discovered == false ) {
			float dist = Constants::distance( getX(), getY(), getShape()->getWidth(), getShape()->getDepth(),
			             trap->r.x, trap->r.y, trap->r.w, trap->r.h );
			if ( dist < 10 && !session->getMap()->isWallBetween( toint( getX() ), toint( getY() ), 0, trap->r.x, trap->r.y, 0 ) ) {
				trap->discovered = rollSkill( Skill::FIND_TRAP, 0.5f ); // traps are easy to notice
				if ( trap->discovered ) {
					char message[ 120 ];
					snprintf( message, 120, _( "%s notices a trap!" ), getName() );
					int panning = session->getMap()->getPanningFromMapXY( trap->r.x, trap->r.y );
					session->getSound()->playSound( "notice-trap", panning );
					session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_MISSION );
					addExperienceWithMessage( 50 );
					setMotion( Constants::MOTION_STAND );
					stopMoving();
				}
			}
		}
	}

	// find secret doors
	for ( int xx = toint( getX() ) - 10; xx < toint( getX() ) + 10; xx++ ) {
		for ( int yy = toint( getY() ) - 10; yy < toint( getY() ) + 10; yy++ ) {
			Location *pos = session->getMap()->getLocation( xx, yy, 0 );
			if ( pos && session->getMap()->isSecretDoor( pos ) && !session->getMap()->isSecretDoorDetected( pos ) ) {
				if ( rollSkill( Skill::FIND_SECRET_DOOR, 4.0f ) ) {
					session->getMap()->setSecretDoorDetected( pos );
					char message[ 120 ];
					snprintf( message, 120, _( "%s notices a secret door!" ), getName() );
					int panning = session->getMap()->getPanningFromMapXY( pos->x, pos->y );
					session->getSound()->playSound( "notice-trap", panning );
					session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_MISSION );
					addExperienceWithMessage( 50 );
				}
			}
		}
	}
}

/// Checks whether the creature has stepped into a trap and handles the effects.

void Creature::evalTrap() {
	int trapIndex = session->getMap()->getTrapAtLoc( toint( getX() ), toint( getY() ) );
	if ( trapIndex != -1 ) {
		Trap *trap = session->getMap()->getTrapLoc( trapIndex );
		if ( trap->enabled ) {
			trap->discovered = true;
			trap->enabled = false;
			int damage = static_cast<int>( Util::getRandomSum( 10, session->getCurrentMission()->getLevel() ) );
			char message[ 120 ];
			snprintf( message, 120, _( "%1$s blunders into a trap and takes %2$d points of damage!" ), getName(), damage );
			int panning = session->getMap()->getPanningFromMapXY( trap->r.x, trap->r.y );
			session->getSound()->playSound( "trigger-trap", panning );
			if ( getCharacter() ) {
				session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_PLAYERDAMAGE );
			} else {
				session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_NPCDAMAGE );
			}
			takeDamage( damage );
		}
	}
}

/// Handles trap disarming.

void Creature::disableTrap( Trap *trap ) {
	if ( trap->enabled ) {
		trap->discovered = true;
		trap->enabled = false;
		enum { MSG_SIZE = 120 };
		char message[ MSG_SIZE ];
		snprintf( message, MSG_SIZE, _( "%s attempts to disable the trap:" ), getName() );
		session->getGameAdapter()->writeLogMessage( message );
		bool ret = rollSkill( Skill::FIND_TRAP, 5.0f );
		if ( ret ) {
			session->getGameAdapter()->writeLogMessage( _( "   and succeeds!" ), Constants::MSGTYPE_MISSION );
			int panning = session->getMap()->getPanningFromMapXY( trap->r.x, trap->r.y );
			session->getSound()->playSound( "disarm-trap", panning );
			int exp = static_cast<int>( Util::getRandomSum( 50, session->getCurrentMission()->getLevel() ) );
			addExperienceWithMessage( exp );
		} else {
			int damage = static_cast<int>( Util::getRandomSum( 10, session->getCurrentMission()->getLevel() ) );
			char message[ MSG_SIZE ];
			snprintf( message, MSG_SIZE, _( "    and fails! %1$s takes %2$d points of damage!" ), getName(), damage );
			int panning = session->getMap()->getPanningFromMapXY( trap->r.x, trap->r.y );
			session->getSound()->playSound( "trigger-trap", panning );
			session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_FAILURE );
			takeDamage( damage );
		}
	} else {
		session->getGameAdapter()->writeLogMessage( _( "This trap is already disabled." ) );
	}
}

/// Sets the motion type (stand, run, loiter around...) for the creature.

void Creature::setMotion( int motion ) {
	this->motion = motion;
}

/// Will this creature stay visible in movie mode?

void Creature::setScripted( bool b ) {
	this->scripted = b;
	if ( scripted ) stopMoving();
}

/// Draws the creature's portrait in movie mode.

void Creature::drawMoviePortrait( int width, int height ) {
	// todo: should be next power of 2 after width/height (maybe cap-ed at 256)
	int textureSizeW = 128;
	int textureSizeH = 128;

	if ( !portrait.isSpecified() ) {
		if ( getCharacter() != NULL ) {
			getSession()->getShapePalette()->getPortraitTexture( getSex(), getPortraitTextureIndex() ).glBind();
		} else {
			getMonster()->getPortraitTexture().glBind();
		}

		portrait.createAlpha( session->getShapePalette()->getNamedTexture( "conv_filter" ), getCharacter() ? getSession()->getShapePalette()->getPortraitTexture( getSex(), getPortraitTextureIndex() ) : getMonster()->getPortraitTexture(), 128, 128, width, height );
	}

	glsDisable( GLS_DEPTH_TEST | GLS_CULL_FACE );
	glsEnable( GLS_TEXTURE_2D | GLS_BLEND );
	glBlendFunc( Scourge::blendA, Scourge::blendB );

	portrait.glBind();
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 1 );
	glVertex2i( textureSizeW, 0 );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, textureSizeH );
	glTexCoord2i( 1, 0 );
	glVertex2i( textureSizeW, textureSizeH );
	glEnd();

	session->getShapePalette()->getNamedTexture( "conv" ).glBind();
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glPushMatrix();
	glTranslatef( -10.0f, -20.0f, 0.0f );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2i( 0, 0 );
	glVertex2i( 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex2i( width + 20, 0 );
	glTexCoord2i( 0, 1 );
	glVertex2i( 0, height + 40 );
	glTexCoord2i( 1, 1 );
	glVertex2i( width + 20, height + 40 );
	glEnd();

	glPopMatrix();

	glsDisable( GLS_BLEND );
	glsEnable( GLS_DEPTH_TEST );
}

/// Draws the creature's portrait, if it exists, else it draws a little 3D view of the creature.

void Creature::drawPortrait( int width, int height, bool inFrame ) {
	if ( getCharacter() || ( getMonster() && getMonster()->getPortraitTexture().isSpecified() ) ) {

		glsEnable( GLS_TEXTURE_2D );

		glPushMatrix();

		if (getCharacter() != NULL) {
			getSession()->getShapePalette()->getPortraitTexture( getSex(), getPortraitTextureIndex() ).glBind();
		} else {
			getMonster()->getPortraitTexture().glBind();
		}

		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

		glBegin( GL_TRIANGLE_STRIP );
		glTexCoord2i( 0, 0 );
		glVertex2i( 0, 0 );
		glTexCoord2i( 1, 0 );
		glVertex2i( width, 0 );
		glTexCoord2i( 0, 1 );
		glVertex2i( 0, height );
		glTexCoord2i( 1, 1 );
		glVertex2i( width, height );
		glEnd();

		glPopMatrix();

		glsDisable( GLS_TEXTURE_2D );

	} else if ( getMonster() ) {
		
		glsEnable( GLS_DEPTH_TEST | GLS_TEXTURE_2D );

		Texture* textureGroup = session->getMap()->getShapes()->getCurrentTheme()->getTextureGroup( WallTheme::themeRefName[ WallTheme::THEME_REF_WALL ] );
		Texture texture = textureGroup[ GLShape::FRONT_SIDE ];

		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

		glPushMatrix();

		texture.glBind();

		glBegin( GL_TRIANGLE_STRIP );
		glTexCoord2i( 0, 0 );
		glVertex2i( 20, 0 );
		glTexCoord2i( 1, 0 );
		glVertex2i( 170, 0 );
		glTexCoord2i( 0, 1 );
		glVertex2i( 20, 150 );
		glTexCoord2i( 1, 1 );
		glVertex2i( 170, 150 );
		glEnd();

		glPopMatrix();

		glsDisable( GLS_TEXTURE_2D );

		shape = getShape();
		shape->setCurrentAnimation( MD2_STAND, true );
		
		// seems to have no effect...
		((AnimatedShape*)shape)->setAlpha( 180.0f );

		glPushMatrix();

		glTranslatef( 135.0f, 190.0f, 100.0f );
		glRotatef( 90.0f, 1.0f, 0.0f, 0.0f );
		glRotatef( 180.0f, 0.0f, 0.0f, 1.0f );
		glScalef( 2.0f, 2.0f, 2.0f );
		shape->draw();

		glPopMatrix();

		glScalef( 1.0f, 1.0f, 1.0f );
		
	}
}

/// The item at a specified backpack index.
Item *Creature::getBackpackItem( int backpackIndex ) {
	return backpack->getContainedItem( backpackIndex );
}
/// Number of items carried in backpack.
int Creature::getBackpackContentsCount() {
	return backpack->getContainedItemCount();
}





/// Returns the max hit points of the creature.

int Creature::getMaxHp() {
	return getStartingHp() * getLevel();
}

/// Returns the max magic points of the creature.

int Creature::getMaxMp() {
	return getStartingMp() * getLevel();
}

/// Sets the full amount of hit points.

void Creature::setHp() {
	int total = getLevel() * getStartingHp();
	hp = Util::pickOne( (int)( total * 0.75f ), total );
}

/// Sets the full amount of magic points.

void Creature::setMp() {
	int total = getLevel() * getStartingMp();
	mp = Util::pickOne( (int)( total * 0.75f ), total );
}

int Creature::getStartingHp() {
	return getCharacter() ? getCharacter()->getStartingHp() : startingHp;
}

int Creature::getStartingMp() {
	return getCharacter() ? getCharacter()->getStartingMp() : startingMp;
}

void Creature::monsterInit() {

	setLevel( monster->getLevel() );


	//cerr << "------------------------------------" << endl << "Creature: " << monster->getType() << endl;
	for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {

		//int n = Creature::rollStartingSkill( scourge->getSession(), LEVEL, i );
		int n;
		if ( Skill::skills[i]->getGroup()->isStat() ) {
			MonsterToughness *mt = &( monsterToughness[ session->getPreferences()->getMonsterToughness() ] );
			n = static_cast<int>( 20.0f * Util::roll( mt->minSkillBase, mt->maxSkillBase ) );
		} else {
			// create the starting value as a function of the stats
			n = 0;
			for ( int t = 0; t < Skill::skills[i]->getPreReqStatCount(); t++ ) {
				int index = Skill::skills[i]->getPreReqStat( t )->getIndex();
				n += getSkill( index );
			}
			n = static_cast<int>( ( n / static_cast<float>( Skill::skills[i]->getPreReqStatCount() ) ) * static_cast<float>( Skill::skills[i]->getPreReqMultiplier() ) );
		}

		// special: adjust magic resistance... makes game too hard otherwise
		if ( i == Skill::RESIST_AWARENESS_MAGIC ||
		        i == Skill::RESIST_CONFRONTATION_MAGIC ||
		        i == Skill::RESIST_DECEIT_MAGIC ||
		        i == Skill::RESIST_HISTORY_MAGIC ||
		        i == Skill::RESIST_LIFE_AND_DEATH_MAGIC ||
		        i == Skill::RESIST_NATURE_MAGIC ) {
			n /= 2;
		}

		// apply monster settings
		int minSkill = monster->getSkillLevel( Skill::skills[i]->getName() );
		if ( minSkill > n ) n = minSkill;

		setSkill( i, n );
		//cerr << "\t" << Skill::skills[ i ]->getName() << "=" << getSkill( i ) << endl;

		stateMod = monster->getStartingStateMod();
	}

	// equip starting backpack
	for ( int i = 0; i < getMonster()->getStartingItemCount(); i++ ) {
		int itemLevel = getMonster()->getLevel() - Util::dice(  2 );
		if ( itemLevel < 1 ) itemLevel = 1;
		Item *item = session->newItem( getMonster()->getStartingItem( i ), itemLevel );
		if( addToBackpack( item ) ) {
			equipFromBackpack( backpack->getContainedItemCount() - 1 );
		}
	}

	// add some loot
	int nn = Util::pickOne( 3, 7 );
	//cerr << "Adding loot:" << nn << endl;
	for ( int i = 0; i < nn; i++ ) {
		Item *loot;
		if ( 0 == Util::dice(  10 ) ) {
			Spell *spell = MagicSchool::getRandomSpell( getLevel() );
			loot = session->newItem( RpgItem::getItemByName( "Scroll" ),
			                         getLevel(),
			                         spell );
		} else {
			loot = session->newItem( RpgItem::getRandomItem( session->getGameAdapter()->getCurrentDepth() ),
			                         getLevel() );
		}
		//cerr << "\t" << loot->getRpgItem()->getName() << endl;
		// make it contain all items, no matter what size
		addToBackpack( loot );
	}

	// add spells
	for ( int i = 0; i < getMonster()->getStartingSpellCount(); i++ ) {
		addSpell( getMonster()->getStartingSpell( i ) );
	}

	// add some hp and mp
	if( monster->isNpc() ) {
		// npc-s are initialized to be similar to pc-s
		startingHp = monster->getHp();
		startingMp = monster->getMp();
		setHp();
		setMp();
	} else {
		// monsters initialization
		MonsterToughness mt = monsterToughness[ session->getPreferences()->getMonsterToughness() ];
		
		startingHp = monster->getHp();
		float n = static_cast<float>( startingHp * level );
		hp = static_cast<int>( n * Util::roll( mt.minHpMpBase, mt.maxHpMpBase ) );
	
		startingMp = monster->getMp();
		n = static_cast<float>( startingMp * level );
		mp = static_cast<int>( n * Util::roll( mt.minHpMpBase, mt.maxHpMpBase ) );
	}
}

Creature *Creature::summonCreature( bool friendly ) {
	Creature *creature = NULL;
	if( (int)(getSummoned()->size()) >= getMaxSummonedCreatures() ) {
		session->getGameAdapter()->writeLogMessage( _( "You may not summon any more creatures." ), Constants::MSGTYPE_STATS );
	} else {
		Monster *monster = Monster::getRandomMonster( Util::pickOne( (int)( getLevel() * 0.5f ), (int)( getLevel() * 0.75f ) ) );
		if( monster ) {
			cerr << "*** summoning monster: " << monster->getType() << endl;
			creature = doSummon( monster, toint( getX() ), toint( getY() ), 0, 0, friendly );
		} else {
			cerr << "*** no monster summoned." << endl;
		}
	}
	return creature;
}

Creature *Creature::doSummon( Monster *monster, int cx, int cy, int ex, int ey, bool friendly ) {
	GLShape *shape = session->getShapePalette()->
	                 getCreatureShape( monster->getModelName(),
	                                   monster->getSkinName(),
	                                   monster->getScale(),
	                                   monster );
	Creature *creature = session->newCreature( monster, shape );
	
	// try to place on map
	bool placed;
	if( ex <= 0 ) {
		cerr << "*** summon via findPlace: pos=" << cx << "," << cy << endl;
		int x, y;
		placed = creature->findPlace( cx, cy, &x, &y );
		if( placed ) {
			cerr << "\t*** final pos=" << x << "," << y << endl;
		}
	} else {
		cerr << "*** summon via findPlaceBounded: region: " << cx << "," << cy << "-" << ex << "," << ey << endl;
		placed = creature->findPlaceBounded( cx, cy, ex, ey );
	}
	if( !placed ) {
		cerr << "*** warning: unable to place summoned creature." << endl;
		return NULL;
	}
	
	addSummoned( creature );
	creature->setSummoner( this );

	// monster summons friendly or pc summons friendly: possess it
	if( isMonster() != friendly ) {
			// maybe make a new state mod for 'summonned'? Maybe not... all the battle code to update... argh
			creature->setStateMod( StateMod::possessed, true );
	}
	
	// register with squirrel
	session->getSquirrel()->registerCreature( creature );
	for ( int i = 0; i < creature->getBackpackContentsCount(); i++ ) {
		session->getSquirrel()->registerItem( creature->getBackpackItem( i ) );
	}	
	
	creature->cancelTarget();
	creature->startEffect( Constants::EFFECT_TELEPORT, ( Constants::DAMAGE_DURATION * 4 ) );
	
	char message[200];
	snprintf( message, 200, _( "%s calls for help: a %s appears!" ), getName(), monster->getType() );
	session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_PLAYERMAGIC );
	
	return creature;
}

void Creature::dismissSummonedCreatures() {
	while( summoned.size() > 0 ) {
		Creature *c = summoned[0];
		c->dismissSummonedCreature();
	}
}

void Creature::dismissSummonedCreature() {
	if( isSummoned() ) {
		// remove from summoner's list
		for( vector<Creature*>::iterator e = getSummoner()->getSummoned()->begin(); e != getSummoner()->getSummoned()->end(); ++e ) { 
			Creature *c = *e;
			if( c == this ) {
				getSummoner()->getSummoned()->erase( e );
				break;
			}
		}
		setSummoner( NULL );
		
		// remove from the map; the object will be cleaned up at the end of the mission
		session->getMap()->removeCreature( toint( getX() ), toint( getY() ), toint( getZ() ) );
		setStateMod( StateMod::dead, true );
		// cancel target, otherwise segfaults on resurrection
		cancelTarget();
		
		session->getMap()->startEffect( toint( getX() ), toint( getY() ), toint( getZ() ), 
		                                Constants::EFFECT_DUST, ( Constants::DAMAGE_DURATION * 4 ) );
		
		char message[200];
		snprintf( message, 200, _( "%s turns to vapors and disappears!" ), getName() );
		session->getGameAdapter()->writeLogMessage( message, Constants::MSGTYPE_PLAYERMAGIC );
	}
}

/// Returns the alignment of the creature as a float between 0.0 (fully chaotic) and 1.0 (totally lawful).

float Creature::getAlignment() {
  int totalSpellCount, chaoticSpellCount, lawfulSpellCount;
  int totalSkillPointCount, chaoticSkillPointCount, lawfulSkillPointCount;
  float chaoticness, lawfulness;
  Spell *spell;
  Skill *skill;

  totalSpellCount = getSpellCount();

  if ( totalSpellCount == 0 ) {
    // If the creature has no spells, determine alignment by the distribution of certain stats.
    totalSkillPointCount = chaoticSkillPointCount = lawfulSkillPointCount = 0;
    string skillName;

    for ( int i = 0; i < Skill::SKILL_COUNT; i++ ) {
      skill = Skill::skills[ i ];
      skillName = skill->getName();

      // Evaluate only basic stats and resistances.
      if ( skill->getGroup()->isStat() || ( skillName.compare( 0, 7, "RESIST_" ) == 0 ) ) {
        totalSkillPointCount += getSkill( i, false );

        if ( skill->getAlignment() == ALIGNMENT_CHAOTIC ) {
          chaoticSkillPointCount += getSkill( i, false );
        } else if ( skill->getAlignment() == ALIGNMENT_LAWFUL ) {
          lawfulSkillPointCount += getSkill( i, false );
        }

      }

    }

    chaoticness = (float)chaoticSkillPointCount / (float)totalSkillPointCount;
    lawfulness = (float)lawfulSkillPointCount / (float)totalSkillPointCount;

  } else {
    // If the creature has spells, determine alignment by their selection.
    chaoticSpellCount = lawfulSpellCount = 0;

    for ( int i = 0; i < totalSpellCount; i++ ) {
      spell = getSpell( i );

      if ( spell->getSchool()->getBaseAlignment() == ALIGNMENT_CHAOTIC ) {
        chaoticSpellCount++;
      } else if ( spell->getSchool()->getBaseAlignment() == ALIGNMENT_LAWFUL ) {
        lawfulSpellCount++;
      }

    }

    // We don't need a neutralness value because it is already implied by the math done here.
    chaoticness = (float)chaoticSpellCount / (float)totalSpellCount;
    lawfulness = (float)lawfulSpellCount / (float)totalSpellCount;
  }

  return ( ( -chaoticness + lawfulness ) + 1 ) / 2;
}

Conversation *Creature::setConversation( string filename ) {
	if( conversation ) {
		Conversation::unref( this, conversation );
		conversation = NULL;
	}
	conversation = Conversation::ref( this, filename, getSession()->getGameAdapter() );
	return conversation;
}
