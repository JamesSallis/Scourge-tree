/***************************************************************************
                          board.cpp  -  description
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

#include "board.h"
#include "item.h"
#include "creature.h"
#include "session.h"

	
/**
  *@author Gabor Torok
  */

/*
char Board::objectiveName[OBJECTIVE_COUNT][80] = {
  "FIND_OBJECT",
  "KILL_MONSTER"
};
*/

Board::Board(Session *session) {
  this->session = session;

  char errMessage[500];
  char s[200];
  sprintf(s, "%s/world/missions.txt", rootDir);
  FILE *fp = fopen(s, "r");
  if(!fp) {        
    sprintf(errMessage, "Unable to find the file: %s!", s);
    cerr << errMessage << endl;
    exit(1);
  }

  Mission *current_mission = NULL;
  char name[255], line[255], description[2000], 
    success[2000], failure[2000];
  int n = fgetc(fp);
  while(n != EOF) {
    if( n == 'M' ) {
      // skip ':'
      fgetc( fp );
      n = Constants::readLine( name, fp );
      strcpy( description, "" );
      while( n == 'D' ) {
        n = Constants::readLine( line, fp );
        if( strlen( description ) ) strcat( description, " " );
        strcat( description, line + 1 );
      }
      templates.push_back( new MissionTemplate( name, description ) );
    } else if( n == 'T' ) {
      // skip ':'
      fgetc( fp );
      // read the name
      n = Constants::readLine( name, fp );
      cerr << "Creating story mission: " << name << endl;

      // read the level and depth
      n = Constants::readLine( line, fp );
      int level = atoi( strtok( line + 1, "," ) );
      int depth = atoi( strtok( NULL, ",") );

      strcpy( description, "" );
      while( n == 'D' ) {
        n = Constants::readLine( line, fp );
        if( strlen( description ) ) strcat( description, " " );
        strcat( description, line + 1 );
      }

      strcpy( success, "" );
      while( n == 'Y' ) {
        n = Constants::readLine( line, fp );
        if( strlen( success ) ) strcat( success, " " );
        strcat( success, line + 1 );
      }

      strcpy( failure, "" );
      while( n == 'N' ) {
        n = Constants::readLine( line, fp );
        if( strlen( failure ) ) strcat( failure, " " );
        strcat( failure, line + 1 );
      }

      current_mission = new Mission( level, depth, name, description, success, failure );
      storylineMissions.push_back( current_mission );

    } else if(n == 'I' && current_mission) {
      fgetc(fp);
      n = Constants::readLine( line, fp );
      Item *item = new Item( RpgItem::getItemByName(line), 
                            current_mission->getLevel() );
      item->setBlocking(true); // don't let monsters pick this up
      current_mission->addItem( item );
    
    } else if(n == 'M' && current_mission) {
      fgetc(fp);
      n = Constants::readLine(line, fp);
      Monster *monster = Monster::getMonsterByName(line);
      GLShape *shape = session->getShapePalette()->
        getCreatureShape(monster->getModelName(), 
                         monster->getSkinName(), 
                         monster->getScale());
      Creature *creature = new Creature( session, monster, shape );
      current_mission->addCreature( creature );
    
    } else {
      n = Constants::readLine(line, fp);
    }
  }
  fclose(fp);
}

Board::~Board() {
  freeListText();
}

void Board::freeListText() {
  if( availableMissions.size() ) {
    for( int i = 0; i < (int)availableMissions.size(); i++ ) {
      free( missionText[i] );
    }
    free( missionText );
    free( missionColor );
  }
}

void Board::reset() {
  for( int i = 0; i < (int)storylineMissions.size(); i++ ) {
    Mission *mission = storylineMissions[i];
    mission->reset();
  }
}

void Board::initMissions() {
  // free ui
  freeListText();

  // find the highest and lowest levels in the party
  int highest = 0;
  int lowest = -1;
  int sum = 0;
  for(int i = 0; i < session->getParty()->getPartySize(); i++) {
    int n = session->getParty()->getParty(i)->getLevel();
    if(n < 1) n = 1;
    if(highest < n) {
      highest = n;
    } else if(lowest == -1 || lowest > n) {
      lowest = n;
    }
    sum += n;
  }
  float ave = ((float)sum / (float)session->getParty()->getPartySize() / 1.0f);

  // maintain a set of missions
  while( availableMissions.size() < 5 ) {
    int level = (int)( ( ave + 2.0f ) * rand()/RAND_MAX ) - 4;
    if( level < 1 ) level = 1;
    int depth =  (int)((float)level / 7.0f ) + 1 + (int)( 3.0f * rand()/RAND_MAX );
    if( depth > 9 ) depth = 9;
    int templateIndex = (int)( (float)( templates.size() ) * rand()/RAND_MAX );
    Mission *mission = templates[ templateIndex ]->createMission( session, level, depth );
    availableMissions.push_back( mission );
  }

  // init ui
  if(availableMissions.size()) {
    missionText = (char**)malloc(availableMissions.size() * sizeof(char*));
    missionColor = (Color*)malloc(availableMissions.size() * sizeof(Color));
    for(int i = 0; i < (int)availableMissions.size(); i++) {
      missionText[i] = (char*)malloc(120 * sizeof(char));
      sprintf(missionText[i], "L:%d, S:%d, %s%s", 
              availableMissions[i]->getLevel(), 
              availableMissions[i]->getDepth(), 
              availableMissions[i]->getName(),
              (availableMissions[i]->isCompleted() ? "(completed)" : ""));
      missionColor[i].r = 1.0f;
      missionColor[i].g = 1.0f;
      missionColor[i].b = 0.0f;
      if(availableMissions[i]->isCompleted()) {
        missionColor[i].r = 0.5f;
        missionColor[i].g = 0.5f;
        missionColor[i].b = 0.5f;
      } else if(availableMissions[i]->getLevel() < ave) {
        missionColor[i].r = 1.0f;
        missionColor[i].g = 1.0f;
        missionColor[i].b = 1.0f;
      } else if(availableMissions[i]->getLevel() > ave) {
        missionColor[i].r = 1.0f;
        missionColor[i].g = 0.0f;
        missionColor[i].b = 0.0f;
      }
      if(i == 0) {
        session->getGameAdapter()->setMissionDescriptionUI((char*)availableMissions[i]->getDescription());
      }
    }

    session->getGameAdapter()->updateBoardUI(availableMissions.size(), 
                                             (const char**)missionText, 
                                             missionColor);
  }
}











MissionTemplate::MissionTemplate( char *name, char *description ) {
  strcpy( this->name, name );
  strcpy( this->description, description );
  cerr << "*** Adding template: " << this->name << endl;
  cerr << this->description << endl;
  cerr << "----------------------------" << endl;
}

MissionTemplate::~MissionTemplate() {
}

Mission *MissionTemplate::createMission( Session *session, int level, int depth ) {

  cerr << "*** Creating level " << level << " mission, using template: " << this->name << endl;

  map<string, Item*> items;
  map<string, Creature*> creatures;

  char parsedName[80];
  char parsedDescription[2000];
  char s[2000];
  strcpy( s, name );
  parseText( session, level, s, parsedName, &items, &creatures );
  strcpy( s, description );
  parseText( session, level, s, parsedDescription, &items, &creatures );
  
  Mission *mission = new Mission( level, depth, parsedName, parsedDescription, 
                                  "You have succeeded in your mission!", 
                                  "You have failed to complete your mission" );
  for(map<string, Item*>::iterator i=items.begin(); i!=items.end(); ++i) {
    Item *item = i->second;
    mission->addItem( item );
  }
  for(map<string, Creature*>::iterator i=creatures.begin(); i!=creatures.end(); ++i) {
    Creature *creature = i->second;
    mission->addCreature( creature );
  }

  return mission;
}                                               

void MissionTemplate::parseText( Session *session, int level, 
                                 char *text, char *parsedText,
                                 map<string, Item*> *items, 
                                 map<string, Creature*> *creatures ) {
  //cerr << "parsing text: " << text << endl;
  strcpy( parsedText, "" );
  char *p = strtok( text, " " );
  while( p ) {
    if( strlen( parsedText ) ) strcat( parsedText, " " );
    char *start = index( p, '{' );
    char *end = rindex( p, '}' );
    if( start && end && end - start < 255 ) {
      char varName[255];
      strncpy( varName, start, (size_t)( end - start ) );
      *(varName + ( end - start )) = '\0';
      
      if( strstr( varName, "item" ) ) {
        string s = varName;
        Item *item;
        if( items->find( s ) == items->end() ) {
          item = new Item( RpgItem::getRandomItem( 1 ), level );
          item->setBlocking(true); // don't let monsters pick this up
          (*items)[ s ] = item;
        } else {
          item = (*items)[s];
        }
        // FIXME: also copy text before and after the variable
        strcat( parsedText, item->getRpgItem()->getName() );
      } else if( strstr( varName, "creature" ) ) {

        // FIXME: must ensure that there is only 1 such monster on this level!
        // Either that or the mission monster must be marked somehow. (outline?)
        // Items don't matter they're on pedestals.

        string s = varName;
        Creature *creature;
        if( creatures->find( s ) == creatures->end() ) {
          
          // find a monster
          Monster *monster = NULL;
          int monsterLevel = level;
          while( monsterLevel > 0 && !monster ) {
            monster = Monster::getRandomMonster( monsterLevel );
            if(!monster) {
              cerr << "Warning: no monsters defined for level: " << level << endl;
              monsterLevel--;
            }
          }
          if( !monster ) {
            cerr << "Error: could not find any monsters." << endl;
            exit( 1 );
          }
          
          // create the shape
          GLShape *shape = session->getShapePalette()->
            getCreatureShape(monster->getModelName(), 
                             monster->getSkinName(), 
                             monster->getScale());
          creature = new Creature( session, monster, shape );
          (*creatures)[ s ] = creature;
        } else {
          creature = (*creatures)[s];
        }
        // FIXME: also copy text before and after the variable
        strcat( parsedText, creature->getMonster()->getType() );
      }
    } else {
      strcat( parsedText, p );
    }
    p = strtok( NULL, " " );
  }
}


Mission::Mission( int level, int depth, char *name, char *description, char *success, char *failure ) {
  this->level = level;
  this->depth = depth;
  strcpy( this->name, name );
  strcpy( this->description, description );
  strcpy( this->success, success );
  strcpy( this->failure, failure );
  this->completed = false;

  cerr << "*** Created mission: " << getName() << endl;
  cerr << getDescription() << endl;
  cerr << "-----------------------" << endl;
}

Mission::~Mission() {
  for(map<Item*, bool >::iterator i=items.begin(); i!=items.end(); ++i) {
    Item *item = i->first;
    delete item;
  }
  items.clear();
  itemList.clear();
  for(map<Creature*, bool >::iterator i=creatures.begin(); i!=creatures.end(); ++i) {
    Creature *creature = i->first;
    delete creature;
  }
  creatures.clear();
  creatureList.clear();
}

bool Mission::itemFound(Item *item) {
  if( items.find( item ) != items.end() ) {
    items[ item ] = true;
    checkMissionCompleted();
  }
  return isCompleted();
}

bool Mission::creatureSlain(Creature *creature) {
  if( creatures.find( creature ) != creatures.end() ) {
    creatures[ creature ] = true;
    checkMissionCompleted();
  }
  return isCompleted();
}

void Mission::checkMissionCompleted() {
  completed = true;
  for(map<Item*, bool >::iterator i=items.begin(); i!=items.end(); ++i) {
    bool b = i->second;
    if( !b ) {
      completed = false;
      return;
    }
  }
  for(map<Creature*, bool >::iterator i=creatures.begin(); i!=creatures.end(); ++i) {
    bool b = i->second;
    if( !b ) {
      completed = false;
      return;
    }
  }
}

void Mission::reset() {
  completed = false;
  // is this ok? (below: to modify the table while iterating it...)
  for(map<Item*, bool >::iterator i=items.begin(); i!=items.end(); ++i) {
    Item *item = i->first;
    items[ item ] = false;
  }
  for(map<Creature*, bool >::iterator i=creatures.begin(); i!=creatures.end(); ++i) {
    Creature *creature = i->first;
    creatures[ creature ] = false;
  }
}








// ------------------------------
// Mission stuff
//
/*
Mission::Mission(char *name, int level, int dungeonStoryCount) {
  strcpy(this->name, name);
  this->level = level;
  this->dungeonStoryCount = dungeonStoryCount;

  strcpy(this->story, "");
  this->completed = false;
  this->objective = NULL;
}

Mission::~Mission() {
  delete objective;
}

bool Mission::itemFound(RpgItem *item) {
  if(!completed && 
	 objective &&
	 item) {
	for(int i = 0; i < objective->itemCount; i++) {
	  if(objective->item[i] == item && !objective->itemHandled[i]) {
		objective->itemHandled[i] = true;
		checkMissionCompleted();
		return isCompleted();
	  }
	}
  }
  return false;
}

bool Mission::monsterSlain(Monster *monster) {
  if(!completed &&
	 objective &&
	 monster) {
	for(int i = 0; i < objective->monsterCount; i++) {
	  if(objective->monster[i] == monster &&
		 !objective->monsterHandled[i]) {
		objective->monsterHandled[i] = true;
		checkMissionCompleted();
		return isCompleted();
	  }
	}
  }
  return false;
}

void Mission::checkMissionCompleted() {
  completed = true;
  if(objective) {
	for(int i = 0; i < objective->itemCount; i++) {
	  if(!objective->itemHandled[i]) {
		completed = false;
		return;
	  }
	}
	for(int i = 0; i < objective->monsterCount; i++) {
	  if(!objective->monsterHandled[i]) {
		completed = false;
		return;
	  }
	}
  }
}

void Mission::reset() {
  completed = false;
  if(objective) {
	for(int i = 0; i < objective->itemCount; i++) {
	  objective->itemHandled[i] = false;
	}
	for(int i = 0; i < objective->monsterCount; i++) {
	  objective->monsterHandled[i] = false;
	}
  }
}
*/

