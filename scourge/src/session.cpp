/***************************************************************************
                     session.cpp  -  Game session manager
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
#include "session.h"
#include "render/renderlib.h"
#include "rpg/rpglib.h"
#include "item.h"
#include "creature.h"
#include "sqbinding/sqbinding.h"
#include "test/combattest.h"
#include "shapepalette.h"
#include "render/cutscene.h"
#include "render/weather.h"
#include <iostream>
#include <stdlib.h>
//#include <strings.h>

#include "creature.h"
#include "persist.h"
#include "io/file.h"
#include "configlang.h"
#include "sound.h"

// ###### MS Visual C++ specific ###### 
#if defined(_MSC_VER) && defined(_DEBUG)
# define new DEBUG_NEW
# undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif 

using namespace std;

Session *Session::instance = NULL;

/**
 *@author Gabor Torok
 */
Session::Session( GameAdapter *adapter )
		: chapterImage()
		, characters( NULL ) {
	this->exiting = false;
	this->adapter = adapter;
	sound = NULL;
	shapePal = NULL;
	party = NULL;
	map = NULL;
	board = NULL;
	cutscene = NULL;
	weather = NULL;
#ifdef HAVE_SDL_NET
	server = NULL;
	client = NULL;
#endif
	multiplayerGame = false;
	currentMission = -1;
	chapterImageWidth = chapterImageHeight = 0;
	showChapterIntro = false;
	squirrel = NULL;
	savegame = "";
	loadgame = "";
	strcpy( scoreid, "" );
	dataInitialized = NOT_INITIALIZED;
	Session::instance = this;
	autosave = false;
	terrainGenerator = NULL;
	strcpy( interruptFunction, "" );
}

Session::~Session() {
	if( !isExiting() ) {
		reset();
		SpecialSkill::unInitSkills();
		Monster::unInitMonsters();
		delete characters;
		MagicSchool::unInitMagic();
		Rpg::unInitRpg();
		if ( squirrel ) delete squirrel;
		deleteCreaturesAndItems();
		delete party;
		party = NULL; // adapter destruction may need it
		delete sound;
		if ( board ) delete board;
		if ( cutscene ) delete cutscene;
		if ( weather ) delete weather;
	#ifdef HAVE_SDL_NET
		delete server;
		delete client;
	#endif
		if ( map ) delete map;
		delete shapePal;
		shapePal = NULL; // adapter destruction may need it
		delete adapter;
	}
}

void Session::initialize() {
	sound = new Sound( adapter->getPreferences(), this );
	shapePal = new ShapePalette( this );
	cutscene = new Cutscene( this );
	weather = new Weather ( this );
	adapter->setSession( this );
	char* str = new char[100]; strcpy(str,"#1 no leaks before"); // a test leak
	adapter->initVideo();
	shapePal->preInitialize();
	// init the fonts and ui
	//adapter->initStart(12, "Loading shapes...");
}

void Session::start() {
	adapter->start();
}

void Session::doInitData() {
	adapter->setUpdate( _( "Loading Shapes" ) );
	shapePal->initialize();

	// read the skills, etc.
	adapter->setUpdate( _( "Loading Professions" ) );
	Rpg::initRpg();

	// initialize the items
	adapter->setUpdate( _( "Loading Items" ) );
	RpgItem::initItems( shapePal );

	// initialize magic
	adapter->setUpdate( _( "Loading Spells" ) );
	MagicSchool::initMagic();

	// init professions
	adapter->setUpdate( _( "Loading Characters" ) );
	characters = new Characters;

	// initialize the monsters (they use items, magic)
	adapter->setUpdate( _( "Loading Creatures" ) );
	Monster::initMonsters();

	adapter->setUpdate( _( "Loading Portraits" ) );
	shapePal->loadNpcPortraits();


	// create the mission board
	adapter->setUpdate( _( "Loading Missions" ) );
	board = new Board( this );


	// do this before the backpack and optionsdialog (so Z is less than of those)
	adapter->setUpdate( _( "Initializing Party" ) );
	party = new Party( this );

	adapter->setUpdate( _( "Initializing Scripting" ) );
	squirrel = new SqBinding( this );

	adapter->setUpdate( _( "Loading Skills" ) );
	SpecialSkill::initSkills();

	adapter->setUpdate( _( "Creating Map" ) );
	map = new Map( adapter, adapter->getPreferences(), getShapePalette() );

	adapter->setUpdate( _( "Initializing UI" ) );
	adapter->initUI();

	adapter->setUpdate( "" );
}

void Session::initData() {
	if ( dataInitialized == NOT_INITIALIZED ) {
		dataInitialized = INIT_STARTED;
		doInitData();
		dataInitialized = INIT_DONE;
	}
}

void Session::quit( int value ) {
	// FIXME: if(getSDLHandler()) getSDLHandler()->quit(value);
	exit( value );
}

#ifdef HAVE_SDL_NET
void Session::runServer( int port ) {
	GameStateHandler *gsh = new TestGameStateHandler();
	server = new Server( port ? port : DEFAULT_SERVER_PORT );
	server->setGameStateHandler( gsh );

	// wait for the server to quit
	int status;
	SDL_WaitThread( server->getThread(), &status );

	delete gsh;
}

void Session::runClient( char const* host, int port, char const* userName ) {
	CommandInterpreter *ci = new TestCommandInterpreter();
	GameStateHandler *gsh = new TestGameStateHandler();
	client = new Client( host, port, userName, ci );
	client->setGameStateHandler( gsh );
	if ( !client->login() ) {
		cerr << Constants::getMessage( Constants::CLIENT_CANT_CONNECT_ERROR ) << endl;
		return;
	}

	// connect as a character
//  Party *party = new Party(this);
	Creature *pc[MAX_PARTY_SIZE];
	int pcCount;
	Party::createHardCodedParty( this, pc, &pcCount );
	cerr << "Sending character: " << pc[0]->getName() << endl;
	getParty()->resetMultiplayer( pc[0] );

	char message[80];
	while ( true ) {
		cout << "> ";
		int c;
		int n = 0;
		while ( n < 79 && ( c = getchar() ) != '\n' ) message[n++] = c;
		message[n] = 0;
		client->sendChatTCP( message );
		//client->sendRawTCP(message);
	}

	delete ci;
	delete gsh;
}

void Session::startServer( GameStateHandler *gsh, int port ) {
	server = new Server( port );
	server->setGameStateHandler( gsh );
	multiplayerGame = true;
}

void Session::startClient( GameStateHandler *gsh, CommandInterpreter *ci, char const* host, int port, char const* username ) {
	client = new Client( host, port, username, ci );
	client->setGameStateHandler( gsh );
	multiplayerGame = true;
}

void Session::stopClientServer() {
	if ( server ) {
		delete server;
		server = NULL;
	}
	if ( client ) {
		delete client;
		client = NULL;
	}
	multiplayerGame = false;
}

#endif

Item *Session::newItem( RpgItem *rpgItem, int level, Spell *spell, bool loading ) {
	// don't randomize special items
	if ( rpgItem->isSpecial() ) loading = true;
	int itemLevel = level;
	if ( !loading ) {
		itemLevel += Util::dice( 6 ) - 3;
	}
	if ( itemLevel < 1 ) itemLevel = 1;
	Item *item = new Item( this, rpgItem, itemLevel );
	if ( spell ) item->setSpell( spell );
	newItems.push_back( item );
	if ( rpgItem->isSpecial() ) setSpecialItem( rpgItem, item );
	return item;
}

Item *Session::addItemFromScript( char *name, int x, int y, int z, bool isContainer, int level, int depth ) {
	RpgItem *rpgItem = RpgItem::getItemByName( name );
	if ( !rpgItem ) {
		cerr << "*** Error: no item named " << name << endl;
		return NULL;
	}
	Item *item = newItem( rpgItem );
	// register with squirrel
	getSquirrel()->registerItem( item );

	if ( isContainer ) {
		getGameAdapter()->fillContainer( item, level, depth );
	}

	getMap()->setItem( x, y, z, item );

	return item;
}

Creature *Session::addWanderingHeroFromScript( int level, int cx, int cy, int cz ) {
	Creature *creature = (Creature*)SqBinding::sessionRef->getGameAdapter()->createWanderingHero( level );

	// register with squirrel
	registerWithSquirrel( creature );

//	if ( fx && fy ) {
		int fx, fy;
		creature->findPlace( cx, cy, &fx, &fy );
//	} else {
//		//int ffx, ffy;
//		//replacement->findPlace( cx, cy, &ffx, &ffy );
//		creature->moveTo( cx, cy, 0 );
//		getMap()->setCreature( cx, cy, 0, creature );
//	}
	creature->cancelTarget();

	return creature;
}

Creature *Session::addCreatureFromScript( char *creatureType, int cx, int cy, int *fx, int *fy, int r ) {
	Monster *monster = Monster::getMonsterByName( creatureType );
	if ( !monster ) {
		cerr << "*** Error: no monster named " << creatureType << endl;
		return NULL;
	}
	GLShape *shape = getShapePalette()->
	                 getCreatureShape( monster->getModelName(),
	                                   monster->getSkinName(),
	                                   monster->getScale(),
	                                   monster );
	Creature *replacement = newCreature( monster, shape );

	// register with squirrel
	registerWithSquirrel( replacement );

	bool b = true;
	if( r > 0 ) {
		b = replacement->findPlaceBoundedRadial( cx, cy, r );
	} else if ( fx && fy ) {
		b = replacement->findPlace( cx, cy, fx, fy );
	} else {
		//int ffx, ffy;
		//replacement->findPlace( cx, cy, &ffx, &ffy );
		replacement->moveTo( cx, cy, 0 );
		getMap()->setCreature( cx, cy, 0, replacement );
	}
	replacement->cancelTarget();

	return replacement;
}

Creature *Session::replaceCreature( Creature *creature, char *newCreatureType ) {
	int cx = toint( creature->getX() );
	int cy = toint( creature->getY() );
	int cz = toint( creature->getZ() );
	getMap()->removeCreature( cx, cy, cz );
	creature->moveTo( -1, -1, 0 ); // remove its marker
	creature->setStateMod( StateMod::dead, true ); // make sure it doesn't move

	int fx, fy;
	Creature *replacement = addCreatureFromScript( newCreatureType, cx, cy, &fx, &fy );
	if ( replacement ) {
		getMap()->startEffect( fx, fy, 1,
		                       Constants::EFFECT_DUST,
		                       ( Constants::DAMAGE_DURATION * 4 ),
		                       replacement->getShape()->getWidth(),
		                       replacement->getShape()->getDepth() );
		char msg[120];
		snprintf( msg, 120, _( "%s transforms into another shape in front of your very eyes!" ),
		          creature->getName() );
		getGameAdapter()->writeLogMessage( msg );

		cerr << "is npc? " << replacement->isNpc() << endl;
	}
	return replacement;
}

void Session::setVisible( Creature *creature, bool b ) {
	if ( b && !isVisible( creature ) ) {
		nonVisibleCreatures.erase( creature );
		getMap()->setCreature( toint( creature->getX() ), toint( creature->getY() ), toint( creature->getZ() ), creature );
	} else if ( !b && isVisible( creature ) ) {
		nonVisibleCreatures.insert( creature );
		getMap()->removeCreature( toint( creature->getX() ), toint( creature->getY() ), toint( creature->getZ() ) );
	}
}

bool Session::isVisible( Creature *creature ) {
	return( nonVisibleCreatures.find( creature ) == nonVisibleCreatures.end() );
}

// creatures created for the mission
Creature *Session::newCreature( Monster *monster, GLShape *shape, bool loaded ) {
	Creature *c = new Creature( this, monster, shape, !loaded );
	creatures.push_back( c );
	return c;
}

Creature *Session::newCreature( Character *character, char const* name, int sex, int model ) {
	Creature *c = new Creature( this, character, name, sex, model );
	creatures.push_back( c );
	return c;
}

bool Session::removeCreatureRef( Creature *creature, int index ) {
	for ( vector<Creature*>::iterator i = creatures.begin(); i != creatures.end(); ++i ) {
		Creature *c = *i;
		if ( c == creature ) {
			creatures.erase( i );
			return true;
		}
	}
	return false;
}

void Session::addCreatureRef( Creature *creature, int index ) {
	creatures.push_back( creature );
}

void Session::deleteCreaturesAndItems( bool missionItemsOnly ) {
	// delete the items and creatures created for this mission
	// (except items in backpack)
	if ( !missionItemsOnly ) {
		for ( int i = 0; i < static_cast<int>( newItems.size() ); i++ ) {
			if ( newItems[i]->isSpecial() ) {
				// put special item back into play
				special.erase( newItems[i]->getRpgItem() );
			}
			delete newItems[i];
		}
		newItems.clear();
	} else {
		for ( int i = 0; i < static_cast<int>( newItems.size() ); i++ ) {
			bool inBackpack = false;
			for ( int t = 0; t < getParty()->getPartySize(); t++ ) {
				if ( getParty()->getParty( t )->isItemInBackpack( newItems[i] ) ) {
					inBackpack = true;
					break;
				}
			}
			if ( !inBackpack ) {
				if ( newItems[i]->isSpecial() ) {
					// put special item back into play
					special.erase( newItems[i]->getRpgItem() );
				}
				delete newItems[i];
				for ( int t = i + 1; t < static_cast<int>( newItems.size() ); t++ ) {
					newItems[t - 1] = newItems[t];
				}
				newItems.pop_back();
				i--;
			}
		}
	}
	for ( int i = 0; i < static_cast<int>( creatures.size() ); i++ ) {
		delete creatures[i];
	}
	creatures.clear();
	nonVisibleCreatures.clear();

	getShapePalette()->debugLoadedModels();
}

bool Session::removeCreature( Creature *creature ) {
	getMap()->removeCreature( toint( creature->getX() ), toint( creature->getY() ), toint( creature->getZ() ) );
	creature->moveTo( -1, -1, 0 ); // remove its marker
	creature->setStateMod( StateMod::dead, true ); // make sure it doesn't move
	return removeCreatureRef( creature, 0 );
}

void Session::creatureDeath( Creature *creature ) {
	bool result;
	squirrel->callBoolMethod( "creatureDeath",
	                          squirrel->getCreatureRef( creature ),
	                          &result );
	// FIXME: not used currently
	//if( !result ) return;
	
	// dismiss summoned creatures
	creature->dismissSummonedCreatures();	
	
	if ( creature == party->getPlayer() ) {
		party->switchToNextLivePartyMember();
	}
	// remove from the map; the object will be cleaned up at the end of the mission
	map->removeCreature( toint( creature->getX() ),
	                     toint( creature->getY() ),
	                     toint( creature->getZ() ) );
	// add a container object instead
	//if(battleRound.size() > 0) creature->getShape()->setCurrentAnimation(MD2_DEATH1);
	Item *item = newItem( RpgItem::getItemByName( "Corpse" ) );
	// add creature's backpack to container
	map->setItem( toint( creature->getX() ),
	              toint( creature->getY() ),
	              toint( creature->getZ() ), item );
	int n = creature->getBackpackContentsCount();
	for ( int i = 0; i < n; i++ ) {
		// make it contain all items, no matter what size
		item->addContainedItem( creature->removeFromBackpack( 0 ), true );
	}
	creature->setStateMod( StateMod::dead, true );
	
	runGenerators();

	creature->setCauseOfDeath( creature->getPendingCauseOfDeath() );

	// cancel target, otherwise segfaults on resurrection
	creature->cancelTarget();

	if ( creature->isPartyMember() ) {
		char message[255];
		snprintf( message, 255, _( "  %s dies!" ), creature->getName() );
		getGameAdapter()->startTextEffect( message );
	}

#ifdef HAVE_SDL_NET
	bool foundLivePlayer = false;
	for ( int i = 0; i < getParty()->getPartySize(); i++ ) {
		if ( !getParty()->getParty( i )->getStateMod( StateMod::dead ) ) {
			foundLivePlayer = true;
			break;
		}
	}
	if ( !foundLivePlayer ) {
		/*
		Hack: only info about the lead player is uploaded. Set the cause of death on that
		player now. This is a hack so cod need not be persisted in savegame.
		*/
		getParty()->getParty( 0 )->setCauseOfDeath( creature->getCauseOfDeath() );
		getGameAdapter()->askToUploadScore();
	}
#endif
}

// define below to enable savegame testing
//#define TESTING_SAVEGAME 1

void testSaveGame( Session *session ) {
	cerr << "Loading savegame." << endl;
	Uint32 storylineIndex;
	Uint32 partySize;
	Creature *pc[MAX_PARTY_SIZE];

	{
		FILE *fp = fopen( "/home/gabor/.scourge/savegame.dat", "rb" );
		File *file = new File( fp );
		Uint32 n = PERSIST_VERSION;
		file->read( &n );
		file->read( &storylineIndex );
		file->read( &partySize );
		cerr << "LOADING: " << endl;
		for ( int i = 0; i < static_cast<int>( partySize ); i++ ) {
			CreatureInfo *info = Persist::loadCreature( file );
			pc[i] = Creature::load( session, info );
			if ( i == 0 ) {
				for ( int t = 0; t < Skill::SKILL_COUNT; t++ ) {
					cerr << "\tinfo=" << info->skills[t] <<
					" infoMOD=" << info->skillMod[t] <<
					" SK=" << pc[i]->getSkill( t, false ) <<
					" MOD=" << pc[i]->getSkillMod( t ) <<
					" BON=" << pc[i]->getSkillBonus( t ) <<
					endl;
				}
			}
			Persist::deleteCreatureInfo( info );
		}
		delete file;
	}

	cerr << "Saving savegame." << endl;
	{
		FILE *fp = fopen( "/home/gabor/.scourge/out.dat", "wb" );
		if ( !fp ) {
			cerr << "Error creating savegame file!" << endl;
			return;
		}
		File *file = new File( fp );
		Uint32 n = PERSIST_VERSION;
		file->write( &n );
		file->write( &storylineIndex );
		file->write( &partySize );
		for ( int i = 0; i < static_cast<int>( partySize ); i++ ) {
			CreatureInfo *info = pc[i]->save();
			Persist::saveCreature( file, info );
			Persist::deleteCreatureInfo( info );
		}
		delete file;
	}

	cerr << "AFTER SAVING: " << endl;
	for ( int t = 0; t < Skill::SKILL_COUNT; t++ ) {
		cerr << "\tSK=" << pc[0]->getSkill( t, false ) << endl;
	}

	cerr << "Done." << endl;
}

//#define TESTING_CONFIG

int Session::runGame( GameAdapter *adapter, int argc, char *argv[] ) {

	int err = Constants::initRootDir( argc, argv );
	if ( err ) return err;

#ifdef TESTING_CONFIG
	ConfigLang *config = ConfigLang::load( "config/scourge.cfg" );
	config->debug();
	delete config;
	exit( 0 );
#endif

#ifdef TESTING_SAVEGAME
	adapter = new GameAdapter( adapter->getPreferences() );
#endif
	// -=K=-: its sole session ... so i make it static
	static Session session( adapter );
	session.initialize();
	if ( argc >= 2 && !strcmp( argv[1], "--run-tests" ) ) {
		char const* path = ( argc >= 3 ?
		                     argv[2] :
		                     "/home/gabor/sourceforge/scourge/api/tests" );
		if ( CombatTest::executeTests( &session, path ) ) {
			cout << "Tests were succesfully written to: " << path << endl;
			return 0;
		} else {
			cerr << "Error while running tests." << endl;
			return 1;
		}
		return 0;
	}
#ifndef TESTING_SAVEGAME
	char* str = new char[100]; strcpy(str,"leak #7 (5 unknown leaks)"); // a test leak
	session.start();
#else
	testSaveGame( &session );
#endif

	return EXIT_SUCCESS;
}

int Session::getCountForDate( char *key, bool withinLastHour ) {
	int count = 0;
	char const* value = getSquirrel()->getValue( key );
	if ( value != NULL ) {
		char s[255];
		strcpy( s, value );
		char *p = strtok( s, "+" );
		if ( p != NULL ) {
			char *q = strtok( NULL, "+" );
			Date *lastUsed = new Date( p );
			Date now = getParty()->getCalendar()->getCurrentDate();

			bool withinDate = ( ( withinLastHour && now.isAnHourLater( *lastUsed ) ) || ( !withinLastHour && now.isADayLater( *lastUsed ) ) );

			// did specified amount of time pass?
			if ( !withinDate ) {
				if ( q ) {
					count = atoi( q );
				}
			}
			delete lastUsed;
		}
	}
	return count;
}

void Session::setCountForDate( char *key, int value ) {
	char s[255];
	snprintf( s, 255, "%s+%d",
	          getParty()->getCalendar()->getCurrentDate().getShortString(),
	          value );
	getSquirrel()->setValue( key, s );
}

void Session::setSavegameName( string& s ) {
	savegame = s;
}

void Session::setSavegameTitle( string& s ) {
	savetitle = s;
}

Creature *Session::getCreatureByName( char const* name ) {
	for ( unsigned int i = 0; i < creatures.size(); i++ ) {
		if ( !strcmp( creatures[i]->getName(), name ) ) return creatures[i];
	}
	return NULL;
}

void Session::setCurrentMission( Mission *mission ) {
	Mission *oldMission = getCurrentMission();
	
	currentMission = -1;
	if( mission ) {
		for( unsigned int i = 0; i < missions.size(); i++ ) {
			if( missions[i] == mission ) {
				cerr << "\tfound mission." << endl;
				currentMission = i;
				break;
			}
		}
		if( currentMission == -1 ) {
			cerr << "\tmission not found, adding it now" << endl;
			// if not found, add it
			missions.push_back( mission );
			currentMission = missions.size() - 1;
		}
	} 
	
	getGameAdapter()->refreshBackpackUI();
	if ( oldMission != mission && mission && mission->isStoryLine() && !mission->isReplay() ) {
		char filename[300];
		snprintf( filename, 300, "chapter%d.png", mission->getChapter() );
		setChapterImage( filename );
	}
}

void Session::debugMissions( char *message ) {
	cerr << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << endl;
	cerr << ">>> " << message << " <<<" << endl;
	cerr << "mission count: " << missions.size() << " current index: " << currentMission << endl;
	for( unsigned int i = 0; i < missions.size(); i++ ) {
		cerr << "\ti=" << i << " mission: " << missions[i]->getName() << " level: " << missions[i]->getLevel() << " depth: " << missions[i]->getDepth() << " map: " << missions[i]->getMapName() << endl;
	}
	cerr << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << endl;
}

void Session::setChapterImage( char *image ) {
	char filename[300];
	snprintf( filename, 300, "/chapters/%s", image );
	chapterImageTexture.load( filename );
	if ( !chapterImageTexture.isSpecified() ) {
		cerr << "Error loading chapter image " << image << endl;
		chapterImage.clear();
		// its clear anyway, chapterImageTexture.clear();
		chapterImageWidth = chapterImageHeight = 0;
	} else {
		//chapterImageTexture = shapePal->loadGLTextures(filename, true);
		GLclampf pri = 0.1f; chapterImageTexture.glPrioritize( pri );
		chapterImageWidth = 1000;
		chapterImageHeight = 458;
		cerr << "***********************************" << endl;
		cerr << "Loaded chapter art: " << filename <<
		" dimensions=" << chapterImageWidth << "," << chapterImageHeight << endl;
		cerr << "***********************************" << endl;
	}
}

std::string HQ_AMBIENT_SOUND = "outdoors";
std::string& Session::getAmbientSoundName() {
	return getCurrentMission() ? getCurrentMission()->getAmbientSoundName() : HQ_AMBIENT_SOUND;
}

void Session::playSound( const std::string& sound, int panning ) {
	getSound()->playSound( sound, panning );
}

void Session::addVisitedRegion( int regionX, int regionY ) {
	visitedRegions.insert( (Uint16)( regionX + regionY * REGIONS_PER_ROW ) );
}

bool Session::isRegionVisited( int regionX, int regionY ) {
	return visitedRegions.find( (Uint16)( regionX + regionY * REGIONS_PER_ROW ) ) != visitedRegions.end();
}

void Session::reset() {
	visitedRegions.clear();
	clearMissions();
}

void Session::clearMissions() {
	for( unsigned int i = 0; i < missions.size(); i++ ) {
		Mission *mission = missions[i];
		if( mission ) delete mission;
	}
	missions.clear();
	currentMission = -1;
}

void Session::runGenerators() {
	if( !getMap()->isHeightMapEnabled() ) return;
	
	for( unsigned int i = 0; i < generators.size(); i++ ) {
		CreatureGenerator *generator = generators[i];
		generator->generate( this );
	}	
}
	
void Session::registerWithSquirrel( Creature *creature ) {
	getSquirrel()->registerCreature( creature );
	for ( int i = 0; i < creature->getBackpackContentsCount(); i++ ) {
		getSquirrel()->registerItem( creature->getBackpackItem( i ) );
	}
}

int Session::getGeneratorCount( int rx, int ry ) {
	int count = 0;
	for( unsigned int i = 0; i < generators.size(); i++ ) {
		CreatureGenerator *generator = generators[i];
		if( generator->rx == rx && generator->ry == ry ) {
			count++;
		}
	}
	//cerr << "++++ getGeneratorCount: region " << rx << "," << ry << " has " << count << " generators." << endl;
	return count;
}

GeneratorInfo *Session::getGeneratorInfo( int rx, int ry, int index ) {
	//cerr << "++++ getGeneratorInfo: looking for generator in region " << rx << "," << ry << " index " << index << " generators." << endl;
	int count = 0;
	for( unsigned int i = 0; i < generators.size(); i++ ) {
		CreatureGenerator *generator = generators[i];
		if( generator->rx == rx && generator->ry == ry ) {
			if( count == index ) {
				//cerr << "\t++++ found it." << endl; 
				return Persist::createGeneratorInfo( generator->rx, generator->ry, generator->x, generator->y, generator->count, generator->monster );
			}
			count++;
		}
	}
	cerr << "*** Error: couldn't find generator index=" << index << " for region: " << rx << "," << ry << endl;
	return NULL;
}

void Session::loadGenerator( GeneratorInfo *info ) {
	CreatureGenerator *generator = new CreatureGenerator();
	generator->rx = info->rx;
	generator->ry = info->ry;
	generator->x = info->x;
	generator->y = info->y;
	generator->count = info->count;
	strcpy( generator->monster, (char*)info->monster );
	//cerr << "++++ loadGenerator: Loaded generator at " << generator->x << "," << generator->y << " count=" << generator->count << endl;
	generators.push_back( generator );
}

void Session::assignCreaturesToGenerators() {
	if( !getMap()->isHeightMapEnabled() ) return;
	
	// find the creatures not assigned to a generator
	std::vector<Creature*> c;
	for( unsigned int i = 0; i < creatures.size(); i++ ) {
		Creature *creature = creatures[i];
		if( creature->isMonster() && !creature->getStateMod( StateMod::dead ) ) { 
			bool found = false;
			for( unsigned int t = 0; !found && t < generators.size(); t++ ) {
				CreatureGenerator *generator = generators[t];
				for( unsigned int r = 0; !found && r < generator->creatures.size(); r++ ) {
					if( creature == generator->creatures[r] ) {
						found = true;
					}
				}
			}
			if( !found ) {
				c.push_back( creature );
			}
		}
	}

	//cerr << "assigning " << c.size() << " creatures to " << generators.size() << " generators..." << endl;
	
	// remove the dead creatures from each generator (maybe there can't be any dead creatures at this point?)
	for( unsigned int t = 0; t < generators.size(); t++ ) {
		CreatureGenerator *generator = generators[t];
		//cerr << "\tgenerator at " << generator->x << "," << generator->y << " count=" << generator->count << endl;
		generator->removeDead();
	}
		
	// assign creatures to closest generator
	for( unsigned int i = 0; i < c.size(); i++ ) {
		Creature *creature = c[i];
		
		CreatureGenerator *closest = NULL;
		float dist = 0;
		for( unsigned int t = 0; t < generators.size(); t++ ) {
			CreatureGenerator *generator = generators[t];
			if( generator->count - generator->creatures.size() > 0 ) {
				float d = Constants::distance( creature->getX(), creature->getY(), creature->getShape()->getWidth(), creature->getShape()->getDepth(), 
				                               generator->x, generator->y, 1, 1 );
				if( closest == NULL || d < dist ) {
					closest = generator;
					dist = d;
				}
			}
		}
		
		if( closest ) {
			//cerr << "\t\tadding " << creature->getType() << " at " << creature->getX() << "," << creature->getY() << 
			//	" to generator at " << closest->x << "," << closest->y << endl; 
			closest->creatures.push_back( creature );
		} else {
			//cerr << "\t\tno generators w/o creatures were found." << endl;
			break;
		}
	}
}
