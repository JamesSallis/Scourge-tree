/***************************************************************************
                          scourge.cpp  -  description
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

#include "scourge.h"

#define GUI_WIDTH 220
#define GUI_HEIGHT 125


// 2,3  2,6  3,6*  5,1+  6,3   8,3*


// good for debugging blending
int Scourge::blendA = 2;
int Scourge::blendB = 6;     // 3
int Scourge::blend[] = { 
    GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, 
    GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA_SATURATE
};

void Scourge::setBlendFunc() {
    glBlendFunc(blend[blendA], blend[blendB]);
}

Scourge::Scourge(int argc, char *argv[]){
  lastTick = 0;
  mainWin = NULL;
  messageWin = NULL;
  movingX = movingY = movingZ = MAP_WIDTH + 1;
  movingItem = NULL;

  isInfoShowing = true;
  
  // Reads the user configuration from a file      
  userConfiguration = new UserConfiguration();  
  userConfiguration->loadConfiguration();    
  userConfiguration->parseCommandLine(argc, argv);  
   
  // Initialize the video mode
  sdlHandler = new SDLHandler(); 
  sdlHandler->setVideoMode(userConfiguration); 
  
  shapePal = sdlHandler->getShapePalette();

  // initialize the monsters
  Monster::initMonsters();

  // init the party; hard code for now
  Creature **pc = Creature::createHardCodedParty(this);
  party[0] = player = pc[0];
  party[1] = pc[1];
  party[2] = pc[2];
  party[3] = pc[3];

  player_only = false;
  inventory = new Inventory(this);
  move = 0;
  startRound = true;
  battleCount = 0;
  gameSpeed = 70; // the greater the slower the game (in ticks) 

  createUI();
  
  // show the main menu
  mainMenu = new MainMenu(this);  
  optionsMenu = new OptionsMenu(this);

  while(true) {
	mainMenu->show();    
	sdlHandler->setHandlers((SDLEventHandler *)mainMenu, (SDLScreenView *)mainMenu);
	sdlHandler->mainLoop();
	mainMenu->hide();

    // evaluate results and start a missions
    fprintf(stderr, "value=%d\n", mainMenu->getValue());
    if(mainMenu->getValue() == NEW_GAME) {
      //charBuilder-
      startMission();
	} else if(mainMenu->getValue() == OPTIONS) {
	  optionsMenu->show();
    } else if(mainMenu->getValue() == QUIT) {
      sdlHandler->quit(0);
    }
  }
}

Scourge::~Scourge(){
  delete mainMenu;
  delete optionsMenu;
  delete userConfiguration;
}

void Scourge::startMission() {
  // add gui
  mainWin->setVisible(true);
  messageWin->setVisible(true);

  // new item and creature references
  itemCount = creatureCount = 0;
  
  // create the map
  map = new Map(this);
  miniMap = new MiniMap(this); 
  
  // Initialize the map with a random dunegeon	
  dg = new DungeonGenerator(this, level);
  Sint16 startx, starty;
  dg->toMap(map, &startx, &starty, getShapePalette());

  // position the players
  player_only = false;
  move = 0;
  startRound = true;
  battleCount = 0;
  setPlayer(getParty(0));
  setFormation(Constants::DIAMOND_FORMATION - Constants::DIAMOND_FORMATION);
  getPlayer()->moveTo(startx, starty, 0);
  getPlayer()->setTargetCreature(NULL);
  map->setCreature(startx, starty, 0, getPlayer()); 

  // init the rest of the party
  for(int i = 1; i < 4; i++) {
	getParty(i)->setNext(getPlayer(), i);
	map->setCreature(getParty(i)->getX(), 
					 getParty(i)->getY(), 
					 getParty(i)->getZ(), 
					 getParty(i));
	getParty(i)->setTargetCreature(NULL);
  }
 
  // center map on the player
  map->center(startx, starty);
  
  // Must be called after MiniMap has been built by dg->toMap() !!! 
  miniMap->computeDrawValues();

  // set to receive events here
  sdlHandler->setHandlers((SDLEventHandler *)this, (SDLScreenView *)this);



  // run mission
  sdlHandler->mainLoop();



  // remove gui
  mainWin->setVisible(false);
  messageWin->setVisible(false);

  // clean up after the mission
  delete map;
  delete miniMap;
  delete dg;

  // delete the items and creatures created for this mission
  // (except items in inventory) 
  // FIXME: may still be a memory leak... the ones in the inventory should be
  // kept in items w. the right itemCount
  for(int i = 0; i < itemCount; i++) {
	bool inInventory = false;
	for(int t = 0; t < 4; t++) {
	  if(getParty(t)->isItemInInventory(items[i])) {
		inInventory = true;
		break;
	  }
	}
	if(!inInventory) {
	  //fprintf(stderr, "Freeing item: i=%d name=%s\n", i, items[i]->getRpgItem()->getName());
	  delete items[i];
	}
  }
  for(int i = 0; i < creatureCount; i++) {
	//fprintf(stderr, "Freeing creature: i=%d name=%s\n", i, creatures[i]->getCharacter()->getName());
	delete creatures[i];
  }
}

// items created for the mission
Item *Scourge::newItem(RpgItem *rpgItem) {
  items[itemCount++] = new Item(rpgItem);
  return items[itemCount - 1];
}

// creatures created for the mission
Creature *Scourge::newCreature(Character *character, char *name) {
  creatures[creatureCount++] = new Creature(this, character, name);
  return creatures[creatureCount - 1];
}

// creatures created for the mission
Creature *Scourge::newCreature(Monster *monster) {
  creatures[creatureCount++] = new Creature(this, monster);
  return creatures[creatureCount - 1];
}

void Scourge::drawView(SDL_Surface *screen) {
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // make a move (player, monsters, etc.)
  playRound();
  
  map->draw(screen);
  
  glDisable( GL_DEPTH_TEST );
  glDisable( GL_TEXTURE_2D );

  if(isInfoShowing) {
    map->initMapView();  
    for(int i = 0; i < 4; i++) {
	  map->showCreatureInfo(party[i], (player == party[i]), 
							(map->getSelectedDropTarget() && 
							 map->getSelectedDropTarget()->creature == party[i]),
							!player_only);
    }
  }

  map->drawDescriptions(messageList);

  miniMap->draw(0, 400);

  if(inventory->isVisible()) inventory->drawInventory();

  glEnable( GL_DEPTH_TEST );
  glEnable( GL_TEXTURE_2D );      
}

bool Scourge::handleEvent(SDL_Event *event) {
  int ea;  

  if(inventory->isVisible()) {
	inventory->handleEvent(event);
	return false;
  }

  if(optionsMenu->isVisible()) {
	optionsMenu->handleEvent(event);
	return false;
  }


  switch(event->type) {
  case SDL_MOUSEMOTION:
    if(event->motion.x < 10) {
      map->setZRot(5.0f);
    } else if(event->motion.x >= sdlHandler->getScreen()->w - 10) {
      map->setZRot(-5.0f);
    } else {
      map->setZRot(0.0f);
    }
    if(event->motion.y < 10) {
      map->setYRot(5.0f);
    } else if(event->motion.y >= sdlHandler->getScreen()->h - 10) {
      map->setYRot(-5.0f);
    } else {
      map->setYRot(0.0f);
    }
    processGameMouseMove(event->motion.x, event->motion.y);
    break;
  case SDL_MOUSEBUTTONDOWN:
    if(event->button.button) {
	  processGameMouseDown(event->button.x, event->button.y, event->button.button);
    }
    break;	
  case SDL_MOUSEBUTTONUP:
    if(event->button.button) {
	  processGameMouseClick(event->button.x, event->button.y, event->button.button);
    }
    break;
  case SDL_KEYDOWN:
  case SDL_KEYUP:
  
    if(event->key.keysym.sym == SDLK_ESCAPE){
        player->setSelXY(-1, -1);   // stop moving
        movingItem = NULL;          // stop moving items
				move = 0;
        return true;
    }
    
    // xxx_yyy_stop means : "do this action when the corresponding key is up"
    ea = userConfiguration->getEngineAction(event);    
    if(ea == SET_MOVE_DOWN){        
        setMove(Constants::MOVE_DOWN);
    }
    else if(ea == SET_MOVE_UP){
        setMove(Constants::MOVE_UP);
    }
    else if(ea == SET_MOVE_RIGHT){
        setMove(Constants::MOVE_RIGHT);
    }
    else if(ea == SET_MOVE_LEFT){
        setMove(Constants::MOVE_LEFT);
    }
    else if(ea == SET_MOVE_DOWN_STOP){        
        removeMove(Constants::MOVE_DOWN);
    }
    else if(ea == SET_MOVE_UP_STOP){
        removeMove(Constants::MOVE_UP);
    }
    else if(ea == SET_MOVE_RIGHT_STOP){
        removeMove(Constants::MOVE_RIGHT);
    }
    else if(ea == SET_MOVE_LEFT_STOP){
        removeMove(Constants::MOVE_LEFT);
    }            
    else if(ea == SET_PLAYER_0){
	  setPlayer(0);
    }
    else if(ea == SET_PLAYER_1){
	  setPlayer(1);
    }
    else if(ea == SET_PLAYER_2){
	  setPlayer(2);
    }
    else if(ea == SET_PLAYER_3){
	  setPlayer(3);
    }
    else if(ea == SET_PLAYER_ONLY){
	  togglePlayerOnly();
    }    
    else if(ea == BLEND_A){
        blendA++; if(blendA >= 11) blendA = 0;
        fprintf(stderr, "blend: a=%d b=%d\n", blendA, blendB);
    }
    else if(ea == BLEND_B){    
        blendB++; if(blendB >= 11) blendB = 0;
        fprintf(stderr, "blend: a=%d b=%d\n", blendA, blendB);
    }
    else if(ea == SHOW_INVENTORY){
        inventory->show();        
    }
    else if(ea == SHOW_OPTIONS_MENU){
        optionsMenu->show();
    }
    else if(ea == USE_ITEM_STOP){
        useItem();
    }
    else if(ea == SET_NEXT_FORMATION_STOP){
        if(getFormation() < Creature::FORMATION_COUNT - 1) setFormation(getFormation() + 1);
	else setFormation(Constants::DIAMOND_FORMATION - Constants::DIAMOND_FORMATION);
    }   
    else if(ea == SET_X_ROT_PLUS){        
        map->setXRot(1.0f);
    }
    else if(ea == SET_X_ROT_MINUS){
        map->setXRot(-1.0f);
    }
    else if(ea == SET_Y_ROT_PLUS){
        map->setYRot(1.0f);
    }
    else if(ea == SET_Y_ROT_MINUS){
        map->setYRot(-1.0f);
    }
    else if(ea == SET_Z_ROT_PLUS){
        map->setZRot(1.0f);
    }
    else if(ea == SET_Z_ROT_MINUS){
        map->setZRot(-1.0f);
    }    
    else if(ea == SET_X_ROT_PLUS_STOP){
        map->setXRot(0.0f);
    }
    else if(ea == SET_X_ROT_MINUS_STOP){
        map->setXRot(0.0f);
    }
    else if(ea == SET_Y_ROT_PLUS_STOP){
        map->setYRot(0.0f);
    }
    else if(ea == SET_Y_ROT_MINUS_STOP){
        map->setYRot(0.0f);
    }
    else if(ea == SET_Z_ROT_PLUS_STOP){
        map->setZRot(0.0f);
    }
    else if(ea == SET_Z_ROT_MINUS_STOP){
        map->setZRot(0.0f);
    }
    
    else if(ea == ADD_X_POS_PLUS){
        map->addXPos(10.0f);
    }
    else if(ea == ADD_X_POS_MINUS){
        map->addXPos(-10.0f);
    }
    else if(ea == ADD_Y_POS_PLUS){
        map->addYPos(10.0f);
    }
    else if(ea == ADD_Y_POS_MINUS){
        map->addYPos(-10.0f);
    }
    else if(ea == ADD_Z_POS_PLUS){
        map->addZPos(10.0f);
    }
    else if(ea == ADD_Z_POS_MINUS){
        map->addZPos(-10.0f);
    } 
    
    else if(ea == MINIMAP_ZOOM_IN){
        miniMap->zoomIn();
    }
    else if(ea == MINIMAP_ZOOM_OUT){
        miniMap->zoomOut();
    }
    else if(ea == MINIMAP_TOGGLE){
        miniMap->toggle();
    }
    else if(ea == SET_ZOOM_IN){
        map->setZoomIn(true);
    }
    else if(ea == SET_ZOOM_OUT){
        map->setZoomOut(true);
    }
    else if(ea == SET_ZOOM_IN_STOP){
        map->setZoomIn(false);
    }
    else if(ea == SET_ZOOM_OUT_STOP){
        map->setZoomOut(false);
    }
	else if(ea == START_ROUND) {
	  toggleRound();
	}
    break;
  default: break;
  }

  return false;  
}

void Scourge::processGameMouseMove(Uint16 x, Uint16 y) {
  Uint16 mapx, mapy;
  getMapXYAtScreenXY(x, y, &mapx, &mapy);
  if(mapx < MAP_WIDTH) {

	// find the drop target
	Location *dropTarget = NULL;
	if(movingItem) {
	  Uint16 mapx, mapy, mapz;
	  getMapXYZAtScreenXY(x, y, &mapx, &mapy, &mapz);
	  if(mapx < MAP_WIDTH) {
		dropTarget = map->getLocation(mapx, mapy, mapz);
		// FIXME: add container handling code
		if(!(dropTarget && dropTarget->creature)) dropTarget = NULL;
	  }
	}
	map->setSelectedDropTarget(dropTarget);

	map->handleMouseMove(mapx, mapy, 0);
  }
}

void Scourge::processGameMouseDown(Uint16 x, Uint16 y, Uint8 button) {
  Uint16 mapx, mapy, mapz;
  if(button == SDL_BUTTON_LEFT) {
	// click on an item
	getMapXYZAtScreenXY(x, y, &mapx, &mapy, &mapz);
	if(mapx > MAP_WIDTH) getMapXYAtScreenXY(x, y, &mapx, &mapy);
	if(startItemDrag(mapx, mapy)) return;
  }
}

void Scourge::processGameMouseClick(Uint16 x, Uint16 y, Uint8 button) {
  Uint16 mapx, mapy, mapz;
  if(button == SDL_BUTTON_LEFT) {
	getMapXYZAtScreenXY(x, y, &mapx, &mapy, &mapz);
	
	// clicking on a creature
	if(!movingItem && mapx < MAP_WIDTH) {
	  Location *loc = map->getLocation(mapx, mapy, mapz);
	  if(loc && loc->creature) {
		if(loc->creature->isMonster()) {
		  // follow this creature
		  player->setTargetCreature(loc->creature);
		  if(!player_only) {
			for(int i = 0; i < 4; i++) 
			  party[i]->setTargetCreature(loc->creature);
		  }
		  return;
		} else {
		  // select player
		  for(int i = 0; i < 4; i++) {
			if(party[i] == loc->creature) {
			  setPlayer(i);
			  return;
			}
		  }
		}
	  }
	}	

	// click on an item
	if(mapx > MAP_WIDTH) getMapXYAtScreenXY(x, y, &mapx, &mapy);
	if(useItem(mapx, mapy)) return;

	// click on the map
	getMapXYAtScreenXY(x, y, &mapx, &mapy);
	player->setSelXY(mapx, mapy);
	if(player_only) {
	  player->setTargetCreature(NULL);
	} else {
	  for(int i = 0; i < 4; i++) {
	    party[i]->setTargetCreature(NULL);
	    if(party[i] != player) party[i]->follow(map);
	  }
	}


  } else if(button == SDL_BUTTON_RIGHT) {
	getMapXYZAtScreenXY(x, y, &mapx, &mapy, &mapz);
	if(mapx < MAP_WIDTH) {    
	  map->handleMouseClick(mapx, mapy, mapz, button);
	}
  }
}

void Scourge::getMapXYAtScreenXY(Uint16 x, Uint16 y,
                                 Uint16 *mapx, Uint16 *mapy) {
    glPushMatrix();

    // Initialize the scene w/o y rotation.
    map->initMapView(true);

    double obj_x, obj_y, obj_z;
    double win_x = (double)x;
    double win_y = (double)sdlHandler->getScreen()->h - y - 1;
    double win_z = 0.0;

    double projection[16];
    double modelview[16];
    GLint viewport[4];

    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetIntegerv(GL_VIEWPORT, viewport);

    int res = gluUnProject(win_x, win_y, win_z,
                           modelview,
                           projection,
                           viewport,
                           &obj_x, &obj_y, &obj_z);
    if(res) {
        *mapx = map->getX() + (Uint16)(((obj_x) * GLShape::DIV)) - 1;
        *mapy = map->getY() + (Uint16)(((obj_y) * GLShape::DIV)) + 2;
        //*mapz = (Uint16)0;
        //*mapz = (Uint16)(obj_z * GLShape::DIV);
    } else {
        //*mapx = *mapy = *mapz = MAP_WIDTH + 1;
        *mapx = *mapy = MAP_WIDTH + 1;
    }

    glPopMatrix();
}

void Scourge::getMapXYZAtScreenXY(Uint16 x, Uint16 y,
                                  Uint16 *mapx, Uint16 *mapy, Uint16 *mapz) {
  GLuint buffer[512];
  GLint  hits, viewport[4];

  glGetIntegerv(GL_VIEWPORT, viewport);
  glSelectBuffer(512, buffer);
  glRenderMode(GL_SELECT);
  glInitNames();
  glPushName(0);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluPickMatrix(x, viewport[3]-y, 1, 1, viewport);
  sdlHandler->setOrthoView();

  glMatrixMode(GL_MODELVIEW);
  map->selectMode = true;
  map->draw(sdlHandler->getScreen());
  map->selectMode = false;

  glFlush();    
  hits = glRenderMode(GL_RENDER);
  //cerr << "hits=" << hits << endl;
  if (hits > 0)	{					  // If There Were More Than 0 Hits
	int choose = buffer[4];					// Make Our Selection The First Object
	int depth = buffer[1];					// Store How Far Away It Is

	for (int loop = 0; loop < hits; loop++)	{		// Loop Through All The Detected Hits

	  //            fprintf(stderr, "\tloop=%d 0=%u 1=%u 2=%u 3=%u 4=%u \n", loop, 
	  //                    buffer[loop*5+0], buffer[loop*5+1], buffer[loop*5+2], 
	  //                    buffer[loop*5+3],  buffer[loop*5+4]);
	  if (buffer[loop*5+4] > 0) {
		decodeName(buffer[loop*5+4], mapx, mapy, mapz);
	  }

	  // If This Object Is Closer To Us Than The One We Have Selected
	  if (buffer[loop*5+1] < GLuint(depth))	{
		choose = buffer[loop*5+4];			  // Select The Closer Object
		depth = buffer[loop*5+1];			// Store How Far Away It Is
	  }
	}

	//cerr << "choose=" << choose << endl;
	decodeName(choose, mapx, mapy, mapz);
  } else {
	*mapx = *mapy = MAP_WIDTH + 1;
  }

  // Restore the projection matrix
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  // Go back to modelview for normal rendering
  glMatrixMode(GL_MODELVIEW);
}

void Scourge::decodeName(int name, Uint16* mapx, Uint16* mapy, Uint16* mapz) {
    char *s;
    if(name > 0) {
        // decode the encoded map coordinates
        *mapz = name / (MAP_WIDTH * MAP_DEPTH);
        if(*mapz > 0)
            name %= (MAP_WIDTH * MAP_DEPTH);
        *mapx = name % MAP_WIDTH;
        *mapy = name / MAP_WIDTH;
        Location *pos = map->getPosition(*mapx, *mapy, 0);
        if(pos) {
            if(pos->shape) s = pos->shape->getName();
            else if(pos->item && pos->item->getShape()) {
                s = pos->item->getShape()->getName();
            }
        } else s = NULL;
		//        fprintf(stderr, "\tmap coordinates: pos null=%s shape null=%s item null=%s %u,%u,%u name=%s\n", 
		//                (pos ? "no" : "yes"), (pos && pos->shape ? "no" : "yes"), (pos && pos->item ? "no" : "yes"), *mapx, *mapy, *mapz, (s ? s : "NULL"));
    } else {
        *mapx = MAP_WIDTH + 1;
        *mapy = 0;
        *mapz = 0;
		//        fprintf(stderr, "\t---\n");
    }
}

void Scourge::setPartyMotion(int motion) {
  for(int i = 0; i < 4; i++) {
	if(party[i] != player) party[i]->setMotion(motion);
  }
}

bool Scourge::startItemDrag(int x, int y) {
  if(movingItem) return false;
  Location *pos = map->getPosition(x, y, player->getZ());
  if(pos) {
	if(getItem(pos)) {  
	  return true;
	}
  }
  return false;
}

bool Scourge::useItem() {
  for(int x = player->getX() - 2; 
	  x < player->getX() + player->getShape()->getWidth() + 2; 
	  x++) {
	for(int y = player->getY() + 2; 
		y > player->getY() - player->getShape()->getDepth() - 2; 
		y--) {
	  if(useItem(x, y)) return true;
	}
  }
  return false;
}

bool Scourge::useItem(int x, int y) {
  if(movingItem) {
	//	dropItem(x, y);
	dropItem(map->getSelX(), map->getSelY());
	return true;
  }
  
  Location *pos = map->getPosition(x, y, player->getZ());
  if(pos) {
	if(useDoor(pos)) {
	  map->updateLightMap();
	  return true;
	}
  }
  return false;
}

bool Scourge::getItem(Location *pos) {
    if(pos->item) {
	  if(map->isWallBetween(pos->x, pos->y, pos->z, 
							getPlayer()->getX(),
							getPlayer()->getY(),
							0)) {
		map->addDescription(Constants::getMessage(Constants::ITEM_OUT_OF_REACH));
	  } else {
        movingX = pos->x;
        movingY = pos->y;
        movingZ = pos->z;
        movingItem = pos->item;
        map->removeItem(pos->x, pos->y, pos->z);
		// draw the item as 'selected'
		map->setSelectedDropTarget(NULL);
		map->handleMouseMove(movingX, movingY, movingZ);
	  }
	  return true;
    }
    return false;
}

// drop an item from the inventory
void Scourge::setMovingItem(Item *item, int x, int y, int z) {
  movingX = x;
  movingY = y;
  movingZ = z;
  movingItem = item;
}

void Scourge::dropItem(int x, int y) {
  if(map->getSelectedDropTarget()) {
	char message[120];
	Creature *c = map->getSelectedDropTarget()->creature;
	if(c) {
	  c->addInventory(movingItem);
	  sprintf(message, "%s picks up %s.", 
			  c->getName(), 
			  movingItem->getRpgItem()->getName());
	  map->addDescription(message);
	  movingItem = NULL;
	  movingX = movingY = movingZ = MAP_WIDTH + 1;
	}
  } else if(!map->isBlocked(x, y, 0,
					 movingX, movingY, movingZ,
					 movingItem->getShape())) {
	map->setItem(x, y, 0, movingItem);
	//	movingItem->moveTo(x, y, 0);
	movingItem = NULL;
	movingX = movingY = movingZ = MAP_WIDTH + 1;
  }
}

bool Scourge::useDoor(Location *pos) {
    Shape *newDoorShape = NULL;
    if(pos->shape == shapePal->getShape(Constants::EW_DOOR_INDEX)) {
        newDoorShape = shapePal->getShape(Constants::NS_DOOR_INDEX);
    } else if(pos->shape == shapePal->getShape(Constants::NS_DOOR_INDEX)) {
        newDoorShape = shapePal->getShape(Constants::EW_DOOR_INDEX);
    }
    if(newDoorShape) {
        // switch door
        Sint16 ox = pos->x;
        Sint16 oy = pos->y;
        Sint16 nx = pos->x;
        Sint16 ny = (pos->y - pos->shape->getDepth()) + newDoorShape->getDepth();
        Shape *oldDoorShape = map->removePosition(ox, oy, player->getZ());
        if(!map->isBlocked(nx, ny, player->getZ(),
                           ox, oy, player->getZ(),
                           newDoorShape)) {
            map->setPosition(nx, ny, player->getZ(), newDoorShape);
            return true;
        } else {
          // rollback
          map->setPosition(ox, oy, player->getZ(), oldDoorShape);
          map->addDescription(Constants::getMessage(Constants::DOOR_BLOCKED));
          return true;
        }
    }
    return false;
}

Creature *Scourge::isPartyMember(Location *pos) {
  for(int i = 0; i < 4; i++) {
	  if(pos->x == party[i]->getX() &&
       pos->y == party[i]->getY() &&
       pos->z == party[i]->getZ()) return party[i];
  }
	return NULL;
}

bool Scourge::handleEvent(Widget *widget, SDL_Event *event) {
  if(inventory->isVisible()) {
	inventory->handleEvent(widget, event);
	return false;
  }

  if(optionsMenu->isVisible()) {
	optionsMenu->handleEvent(widget, event);
	return false;
  }


  if(widget == inventoryButton) {
	if(inventory->isVisible()) inventory->hide();
	else inventory->show();
  } else if(widget == optionsButton) {
	if(optionsMenu->isVisible()) optionsMenu->hide();
	else optionsMenu->show();
  } else if(widget == quitButton) {
	return true;
  } else if(widget == diamondButton) {
	setFormation(Constants::DIAMOND_FORMATION - Constants::DIAMOND_FORMATION);
  } else if(widget == staggeredButton) {
	setFormation(Constants::STAGGERED_FORMATION - Constants::DIAMOND_FORMATION);
  } else if(widget == squareButton) {
	setFormation(Constants::SQUARE_FORMATION - Constants::DIAMOND_FORMATION);
  } else if(widget == rowButton) {
	setFormation(Constants::ROW_FORMATION - Constants::DIAMOND_FORMATION);
  } else if(widget == scoutButton) {
	setFormation(Constants::SCOUT_FORMATION - Constants::DIAMOND_FORMATION);
  } else if(widget == crossButton) {
	setFormation(Constants::CROSS_FORMATION - Constants::DIAMOND_FORMATION);
  } else if(widget == player1Button) {
	setPlayer(Constants::PLAYER_1 - Constants::PLAYER_1);
  } else if(widget == player2Button) {
	setPlayer(Constants::PLAYER_2 - Constants::PLAYER_1);
  } else if(widget == player3Button) {
	setPlayer(Constants::PLAYER_3 - Constants::PLAYER_1);
  } else if(widget == player4Button) {
	setPlayer(Constants::PLAYER_4 - Constants::PLAYER_1);
  } else if(widget == groupButton) {
	togglePlayerOnly();
  } else if(widget == roundButton) {
	toggleRound();
  }
  return false;
}

void Scourge::createUI() {
  // create the ui
  char version[100];
  sprintf(version, "S.C.O.U.R.G.E. version %7.2f", SCOURGE_VERSION);
  mainWin = new Window( getSDLHandler(),
						sdlHandler->getScreen()->w - GUI_WIDTH, 
						sdlHandler->getScreen()->h - GUI_HEIGHT, 
						GUI_WIDTH, GUI_HEIGHT, 
						strdup(version), 
						getShapePalette()->getGuiTexture() );
  int gx = sdlHandler->getScreen()->w - GUI_WIDTH;
  int gy = sdlHandler->getScreen()->h - GUI_HEIGHT;
  inventoryButton = new Button( 0, 0, 100, 25, strdup("Party Info") );
  mainWin->addWidget((Widget*)inventoryButton);
  optionsButton = new Button( 0, 25,  100, 50, strdup("Options") );
  mainWin->addWidget((Widget*)optionsButton);
  quitButton = new Button( 0, 50,  100, 75, strdup("Quit") );
  mainWin->addWidget((Widget*)quitButton);
  roundButton = new Button( 0, 75,  100, 100, strdup("Real-Time") );
  roundButton->setToggle(true);
  roundButton->setSelected(true);
  mainWin->addWidget((Widget*)roundButton);


  diamondButton = new Button( 100, 0,  120, 20 );
  diamondButton->setToggle(true);
  mainWin->addWidget((Widget*)diamondButton);  
  staggeredButton = new Button( 120, 0,  140, 20 );
  staggeredButton->setToggle(true);
  mainWin->addWidget((Widget*)staggeredButton);
  squareButton = new Button( 140, 0,  160, 20 );
  squareButton->setToggle(true);
  mainWin->addWidget((Widget*)squareButton);
  rowButton = new Button( 160, 0,  180, 20 );
  rowButton->setToggle(true);
  mainWin->addWidget((Widget*)rowButton);
  scoutButton = new Button( 180, 0,  200, 20 );
  scoutButton->setToggle(true);
  mainWin->addWidget((Widget*)scoutButton);
  crossButton = new Button( 200, 0,  220, 20 );
  crossButton->setToggle(true);
  mainWin->addWidget((Widget*)crossButton);

  player1Button = new Button( 100, 20,  124, 40, strdup("1") );
  player1Button->setToggle(true);
  mainWin->addWidget((Widget*)player1Button);  
  player2Button = new Button( 124, 20,  148, 40, strdup("2") );
  player2Button->setToggle(true);
  mainWin->addWidget((Widget*)player2Button);
  player3Button = new Button( 148, 20,  172, 40, strdup("3") );
  player3Button->setToggle(true);
  mainWin->addWidget((Widget*)player3Button);
  player4Button = new Button( 172, 20,  196, 40, strdup("4") );
  player4Button->setToggle(true);
  mainWin->addWidget((Widget*)player4Button);
  groupButton = new Button( 196, 20,  220, 40, strdup("G") );
  groupButton->setToggle(true);
  groupButton->setSelected(true);
  mainWin->addWidget((Widget*)groupButton);

  messageWin = new Window( getSDLHandler(),
						   0, 0, 400, 120, 
						   strdup("Messages"), 
						   getShapePalette()->getGuiTexture() );
  messageWin->setBackground(0, 0, 0);
  messageList = new ScrollingList(0, 0, 400, 95);
  messageList->setSelectionColor( 0.15f, 0.15f, 0.3f );
  messageWin->addWidget(messageList);
}


void Scourge::playRound() {
  // move the player's selX,selY in a direction as specified by keystroke
  handleKeyboardMovement();

  // hound your targets
  for(int i = 0; i < 4; i++) {
	if(party[i]->getTargetCreature()) {
	  party[i]->setSelXY(party[i]->getTargetCreature()->getX(),
						 party[i]->getTargetCreature()->getY());
	}
  }
  
  // round starts if:
  // -in group mode
  // -(or) the round was manually started
  GLint t = SDL_GetTicks();
  if(startRound && 
	 (lastTick == 0 || t - lastTick > gameSpeed)) {
	lastTick = t;
		
	// move the party members
	movePlayers();
	
	// move visible monsters
	for(int i = 0; i < creatureCount; i++) {
	  if(map->isLocationVisible(creatures[i]->getX(), creatures[i]->getY())) {
		moveMonster(creatures[i]);
	  }
	}

	// set up for battle
	battleCount = 0;

	// attack targeted monster if close enough
	for(int i = 0; i < 4; i++) {
	  if(party[i]->getTargetCreature()) {
		battle[battleCount].creature = party[i];
		battleCount++;
	  }
	}
	for(int i = 0; i < creatureCount; i++) {
	  if(map->isLocationVisible(creatures[i]->getX(), creatures[i]->getY()) &&
		 creatures[i]->getTargetCreature()) {
		battle[battleCount].creature = creatures[i];
		battleCount++;
	  }
	}

	// fight one round of the epic battle
	if(battleCount > 0) fightBattle();
  }
}

void Scourge::fightBattle() {
  int initiative = -10;
  char message[200];
  bool fightingStarted = false;
  // this is O(n^2) unfortunately... maybe we could use a linked list or something here
  while(battleCount > 0) {
	bool creatureFound = false;
	for(int i = 0; i < battleCount; i++) {
	  Creature *creature = battle[i].creature;
	  float dist = Util::distance(creature->getX(), 
								  creature->getY(), 
								  creature->getShape()->getWidth(),
								  creature->getShape()->getDepth(),
								  creature->getTargetCreature()->getX(),
								  creature->getTargetCreature()->getY(),
								  creature->getTargetCreature()->getShape()->getWidth(),
								  creature->getTargetCreature()->getShape()->getDepth());
	  // get the best weapon given the distance from the target
	  Item *item = creature->getBestWeapon(dist);
	  // creature won't fight if too far from the action 
	  // (!item) is a bare-hands attack
	  if(item || dist <= 1.0f) {
		// not time for this creature's turn yet
		int creatureInitiative = creature->getInitiative(item);
		if(creatureInitiative > initiative) continue;
		creatureFound = true;

		if(!fightingStarted) {
		  map->addDescription("A round of battle begins...", 1, 1, 1);
		  fightingStarted = true;
		}

		if(item) {
		  sprintf(message, "%s attacks %s with %s! (initiative: %d)", 
				  creature->getName(), 
				  creature->getTargetCreature()->getName(),
				  item->getRpgItem()->getName(),
				  creatureInitiative);
		  map->addDescription(message);
		} else if(dist <= 1.0f) {
		  sprintf(message, "%s attacks %s with bare hands! (initiative: %d)", 
				  creature->getName(), 
				  creature->getTargetCreature()->getName(),
				  creatureInitiative);
		  map->addDescription(message);
		}

		// take a swing
		int tohit = creature->getToHit(item);
		int ac = creature->getTargetCreature()->getArmor();
		if(tohit > ac) {

		  // deal out the damage
		  int damage = creature->getDamage(item);
		  if(damage) {
			sprintf(message, "...and hits! (toHit=%d vs. AC=%d) for %d points of damage", 
					tohit, ac, damage);
			map->addDescription(message, 1.0f, 0.5f, 0.5f);

			// FIXME: implement me!
			// creature->getTargetCreature()->takeDamage(damage);

		  } else {
			sprintf(message, "...and hits! (toHit=%d vs. AC=%d) but causes no damage", 
					tohit, ac);
			map->addDescription(message);
		  }
		  
		} else {
		  // missed
		  sprintf(message, "...and misses! (toHit=%d vs. AC=%d)", tohit, ac);
		  map->addDescription(message);
		}
	  }
	  // remove this creature from the turn
	  for(int t = i; t < battleCount - 1; t++) {
		battle[t].creature = battle[t + 1].creature;
	  }
	  battleCount--;
	  i--;
	}
	if(!creatureFound) initiative++;
  }
}

// move the player in a direction as specified by keystroke
void Scourge::handleKeyboardMovement() {
  if(!move) return;

  // when moving w. the keyboard don't attack creatures
  for(int i = 0; i < 4; i++) party[i]->setTargetCreature(NULL);

  // decode keyboard movement
  if(move & Constants::MOVE_UP) {
	if(getPlayer()->getSelX() == -1) 
	  getPlayer()->setSelXY(getPlayer()->getX(), getPlayer()->getY() - 1);
	else
	  getPlayer()->setSelXY(getPlayer()->getSelX(), getPlayer()->getSelY() - 1);
  }
  if(move & Constants::MOVE_DOWN) {
	if(getPlayer()->getSelX() == -1) 
	  getPlayer()->setSelXY(getPlayer()->getX(), getPlayer()->getY() + 1);
	else
	  getPlayer()->setSelXY(getPlayer()->getSelX(), getPlayer()->getSelY() + 1);
  }
  if(move & Constants::MOVE_LEFT) {
	if(getPlayer()->getSelX() == -1) 
	  getPlayer()->setSelXY(getPlayer()->getX() - 1, getPlayer()->getY());
	else
	  getPlayer()->setSelXY(getPlayer()->getSelX() - 1, getPlayer()->getSelY());
  }
  if(move & Constants::MOVE_RIGHT) {
	if(getPlayer()->getSelX() == -1) 
	  getPlayer()->setSelXY(getPlayer()->getX() + 1, getPlayer()->getY());
	else
	  getPlayer()->setSelXY(getPlayer()->getSelX() + 1, getPlayer()->getSelY());
  }

  if(!player_only) {
    for(int i = 0; i < 4; i++) {
      party[i]->setTargetCreature(NULL);
      if(party[i] != player) party[i]->follow(map);
    }
  }
}

void Scourge::movePlayers() {   
  if(player_only) {	
	// in single-step mode:
	for(int i = 0; i < 4; i++) {
	  // move the current player
	  player->moveToLocator(map, player_only);
	  map->center(player->getX(), player->getY());
	  
	  // switch to next player
	  for(int t = 0; t < 4; t++) {
		if(party[t] == player) {
		  setPlayer(t < 3 ? t + 1 : 0);
		  break;
		}
	  }
	}
	
  } else {
	// In group mode:
	
	// move the leader
	player->moveToLocator(map, player_only);
	map->center(player->getX(), player->getY());
	
	// in keyboard mode, don't move the selection
	if(move) player->setSelXY(-1, -1);
	
	// others follow the player
	for(int t = 0; t < 4; t++) {
	  if(party[t] != player) {
		if(party[t]->getTargetCreature()) {
		  party[t]->moveToLocator(map, player_only);
		} else {
		  //party[t]->follow(map);
		  party[t]->moveToLocator(map, player_only);
		}
	  }
	}	
  }
}

void Scourge::setPlayer(int n) {
  player = party[n];
  player->setNextDontMove(NULL, 0);
  move = 0;
  //  player->setSelXY(-1, -1); // don't move
  // init the rest of the party
  int count = 1;
  for(int i = 0; i < 4; i++) {
	if(i != n) party[i]->setNextDontMove(player, count++);
  }
  map->refresh();
  map->center(player->getX(), player->getY());
  player1Button->setSelected(false);
  player2Button->setSelected(false);
  player3Button->setSelected(false);
  player4Button->setSelected(false);
  switch(n) {
  case 0 : player1Button->setSelected(true); break;
  case 1 : player2Button->setSelected(true); break;
  case 2 : player3Button->setSelected(true); break;
  case 3 : player4Button->setSelected(true); break;
  }
}

/**
   Setting the formation ends single-step mode (back to group mode).
 */
void Scourge::setFormation(int formation) {
  this->formation = formation;
  for(int i = 0; i < 4; i++) {
	party[i]->setFormation(formation);
  }
  player_only = false;
  groupButton->setSelected(!player_only);
  startRound = true;
  roundButton->setSelected(startRound);


  diamondButton->setSelected(false);
  staggeredButton->setSelected(false);
  squareButton->setSelected(false);
  rowButton->setSelected(false);
  scoutButton->setSelected(false);
  crossButton->setSelected(false);
  switch(formation + Constants::DIAMOND_FORMATION) {
  case Constants::DIAMOND_FORMATION:
    diamondButton->setSelected(true); break;
  case Constants::STAGGERED_FORMATION:
    staggeredButton->setSelected(true); break;
  case Constants::SQUARE_FORMATION:
    squareButton->setSelected(true); break;
  case Constants::ROW_FORMATION:
    rowButton->setSelected(true); break;
  case Constants::SCOUT_FORMATION:
    scoutButton->setSelected(true); break;
  case Constants::CROSS_FORMATION:
    crossButton->setSelected(true); break;
  }

}

/**
   When pausing the round, we enter single-step mode.
 */
void Scourge::toggleRound() {
  startRound = (startRound ? false : true);
  if(startRound)
	map->addDescription(Constants::getMessage(Constants::REAL_TIME_MODE), 0.5f, 0.5f, 1.0f);
  else
	map->addDescription(Constants::getMessage(Constants::TURN_MODE), 0.5f, 0.5f, 1.0f);
  roundButton->setSelected(startRound);
}

void Scourge::togglePlayerOnly() {
  player_only = (player_only ? false : true);
  // in group mode everyone hunts the same creature
  if(!player_only) {
	for(int i = 0; i < 4; i++) {
	  if(party[i] != player) 
		party[i]->setTargetCreature(player->getTargetCreature());
	}
  }
  if(player_only)
	map->addDescription(Constants::getMessage(Constants::SINGLE_MODE), 0.5f, 0.5f, 1.0f);
  else
	map->addDescription(Constants::getMessage(Constants::GROUP_MODE), 0.5f, 0.5f, 1.0f);
  groupButton->setSelected(!player_only);
}
  
void Scourge::moveMonster(Creature *monster) {	
  // for now just twitch around
  // FIXME: this needs to be a lot more intelligent!
  for(int i = 0; i < 4; i++) {
	int n = (int)(10.0f * rand()/RAND_MAX);
	if(n == 0 || !monster->move(monster->getDir(), map)) {
	  int dir = (int)(4.0f * rand()/RAND_MAX);
	  switch(dir) {
	  case 0: monster->setDir(Constants::MOVE_UP); break;
	  case 1: monster->setDir(Constants::MOVE_DOWN); break;
	  case 2: monster->setDir(Constants::MOVE_LEFT); break;
	  case 3: monster->setDir(Constants::MOVE_RIGHT); break;
	  }
	} else {
	  break;
	}
  }
}
