/***************************************************************************
                          map.cpp  -  description
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

#include "map.h"

#define DEBUG_MOUSE_POS

// only use 1 (disabled) or 0 (enabled)
#define LIGHTMAP_ENABLED 1

const float Map::shadowTransformMatrix[16] = { 
	1, 0, 0, 0,
	0, 1, 0, 0,
	0.5f, -0.5f, 0, 0,
	0, 0, 0, 1 };

Map::Map(Scourge *scourge){
  zoom = 1.0f;
  zoomIn = zoomOut = false;
  x = y = 0;
  selectMode = false;
  floorOnly = false;
  selX = selY = selZ = MAP_WIDTH + 1;
  oldLocatorSelX = oldLocatorSelY = oldLocatorSelZ = selZ;
  useShadow = false;
  //alwaysCenter = true;

  debugX = debugY = debugZ = -1;
  
  mapChanged = true;
  
  descriptionCount = 0;
  descriptionsChanged = false;
  for(int i = 0; i < MAX_DESCRIPTION_COUNT; i++)
	descriptions[i] = (char*)malloc(120 * sizeof(char));
  
  this->xrot = 0.0f; // if 0, artifacts appear
  this->yrot = 30.0f;
  this->zrot = 45.0f;
  this->xRotating = this->yRotating = this->zRotating = 0.0f;

  float adjust = (float)scourge->getSDLHandler()->getScreen()->w / 800.0f;
  this->xpos = (float)(scourge->getSDLHandler()->getScreen()->w) / 2.0f / adjust;
  this->ypos = (float)(scourge->getSDLHandler()->getScreen()->h) / 2.0f / adjust;
  this->zpos = 0.0f;  
  
  this->scourge = scourge;  
  this->debugGridFlag = false;
  this->drawGridFlag = false;

  targetWidth = 0.0f;
  targetWidthDelta = 0.05f / GLShape::DIV;
  lastTick = SDL_GetTicks();
  
  // initialize shape graph of "in view shapes"
  for(int x = 0; x < MAP_WIDTH; x++) {
	for(int y = 0; y < MAP_DEPTH; y++) {
      floorPositions[x][y] = NULL;
	  for(int z = 0; z < MAP_VIEW_HEIGHT; z++) {
        pos[x][y][z] = NULL;
      }      
    }
  }

  lightMapChanged = true;  
  colorAlreadySet = false;
  selectedDropTarget = NULL;

  createOverlayTexture();

  addDescription(Constants::getMessage(Constants::WELCOME), 1.0f, 0.5f, 1.0f);
  addDescription("----------------------------------", 1.0f, 0.5f, 1.0f);
}

Map::~Map(){
  for(int xp = 0; xp < MAP_WIDTH; xp++) {
	for(int yp = 0; yp < MAP_DEPTH; yp++) {
	  for(int zp = 0; zp < MAP_VIEW_HEIGHT; zp++) {
		if(pos[xp][yp][zp]) {
		  delete pos[xp][yp][zp];
		}
	  }
	}
  }
  for(int i = 0; i < MAX_DESCRIPTION_COUNT; i++)
	free(descriptions[i]);
}

void Map::center(Sint16 x, Sint16 y) { 
  Sint16 nx = x - MAP_VIEW_WIDTH / 2; 
  Sint16 ny = y - MAP_VIEW_DEPTH / 2; 

  if(scourge->getUserConfiguration()->getAlwaysCenterMap() ||
     abs(this->x - nx) > X_CENTER_TOLERANCE ||
     abs(this->y - ny) > Y_CENTER_TOLERANCE) {
    // relocate
    this->x = nx;
    this->y = ny;
  }
}

/**
   If 'ground' is true, it draws the ground layer.
   Otherwise the shape arrays (other, stencil, later) are populated.
*/
void Map::setupShapes(bool ground) {
  if(!ground) {
	laterCount = stencilCount = otherCount = damageCount = 0;
	mapChanged = false;
  }

  int chunkOffsetX = 0;
  int chunkStartX = (getX() - MAP_OFFSET) / MAP_UNIT;
  int mod = (getX() - MAP_OFFSET) % MAP_UNIT;
  if(mod) {
	chunkOffsetX = -mod;
  }
  int chunkEndX = MAP_VIEW_WIDTH / MAP_UNIT + chunkStartX;

  int chunkOffsetY = 0;
  int chunkStartY = (getY() - MAP_OFFSET) / MAP_UNIT;
  mod = (getY() - MAP_OFFSET) % MAP_UNIT;
  if(mod) {
	chunkOffsetY = -mod;
  }
  int chunkEndY = MAP_VIEW_DEPTH / MAP_UNIT + chunkStartY;

  Shape *shape;
  int posX, posY;
  float xpos2, ypos2, zpos2;
  for(int chunkX = chunkStartX; chunkX < chunkEndX; chunkX++) {
	for(int chunkY = chunkStartY; chunkY < chunkEndY; chunkY++) {
	  int doorValue = 0;

	  // if this chunk is not lit, don't draw it
	  if(!lightMap[chunkX][chunkY]) {
		if(ground) continue; 
		else {
		  // see if a door has to be drawn
		  for(int yp = MAP_UNIT_OFFSET + 1; yp < MAP_UNIT; yp++) {
			bool found = false;
			if(chunkX - 1 >= 0 && lightMap[chunkX - 1][chunkY]) {
			  for(int xp = MAP_UNIT - MAP_UNIT_OFFSET; xp < MAP_UNIT; xp++) {
				posX = (chunkX - 1) * MAP_UNIT + xp + MAP_OFFSET;
				posY = chunkY * MAP_UNIT + yp + MAP_OFFSET + 1;
				if(posX >= 0 && posX < MAP_WIDTH && 
				   posY >= 0 && posY < MAP_DEPTH &&
				   pos[posX][posY][0]) {
				  found = true;
				  break;
				}
			  }
			}
			if(!found) doorValue |= Constants::MOVE_LEFT;
			found = false;
			if(chunkX + 1 < MAP_WIDTH / MAP_UNIT && lightMap[chunkX + 1][chunkY]) {
			  for(int xp = 0; xp < MAP_UNIT_OFFSET; xp++) {
				posX = (chunkX + 1) * MAP_UNIT + xp + MAP_OFFSET;
				posY = chunkY * MAP_UNIT + yp + MAP_OFFSET + 1;
				if(posX >= 0 && posX < MAP_WIDTH && 
				   posY >= 0 && posY < MAP_DEPTH &&
				   pos[posX][posY][0]) {
				  found = true;
				  break;
				}
			  }
			}
			if(!found) doorValue |= Constants::MOVE_RIGHT;
		  }
		  for(int xp = MAP_UNIT_OFFSET; xp < MAP_UNIT - MAP_UNIT_OFFSET; xp++) {
			bool found = false;
			if(chunkY - 1 >= 0 && lightMap[chunkX][chunkY - 1]) {
			  for(int yp = MAP_UNIT - MAP_UNIT_OFFSET; yp < MAP_UNIT; yp++) {
				posX = chunkX * MAP_UNIT + xp + MAP_OFFSET;
				posY = (chunkY - 1) * MAP_UNIT + yp + MAP_OFFSET + 1;
				if(posX >= 0 && posX < MAP_WIDTH && 
				   posY >= 0 && posY < MAP_DEPTH &&
				   pos[posX][posY][0]) {
				  found = true;
				  break;
				}
			  }
			}
			if(!found) doorValue |= Constants::MOVE_UP;
			found = false;
			if(chunkY + 1 >= 0 && lightMap[chunkX][chunkY + 1]) {
			  for(int yp = 0; yp < MAP_UNIT_OFFSET; yp++) {
				posX = chunkX * MAP_UNIT + xp + MAP_OFFSET;
				posY = (chunkY + 1) * MAP_UNIT + yp + MAP_OFFSET + 1;
				if(posX >= 0 && posX < MAP_WIDTH && 
				   posY >= 0 && posY < MAP_DEPTH &&
				   pos[posX][posY][0]) {
				  found = true;
				  break;
				}
			  }
			}
			if(!found) doorValue |= Constants::MOVE_DOWN;
		  }
		  if(doorValue == 0) continue;
		}
	  }
	  
	  for(int yp = 0; yp < MAP_UNIT; yp++) {
		for(int xp = 0; xp < MAP_UNIT; xp++) {

		  /**
			 In scourge, shape coordinates are given by their
			 left-bottom corner. So the +1 for posY moves the
			 position 1 unit down the Y axis, which is the
			 unit square's bottom left corner.
		   */
		  posX = chunkX * MAP_UNIT + xp + MAP_OFFSET;
		  posY = chunkY * MAP_UNIT + yp + MAP_OFFSET + 1;

		  if(ground) {
			shape = floorPositions[posX][posY];
			if(shape) {
			  xpos2 = (float)((chunkX - chunkStartX) * MAP_UNIT + 
							  xp + chunkOffsetX) / GLShape::DIV;
			  ypos2 = (float)((chunkY - chunkStartY) * MAP_UNIT - 
							  shape->getDepth() +
							  yp + chunkOffsetY) / GLShape::DIV;
			  drawGroundPosition(posX, posY,
								 xpos2, ypos2,
								 shape); 		  
			}
		  } else {
			for(int zp = 0; zp < MAP_VIEW_HEIGHT; zp++) {
			  if(pos[posX][posY][zp] && 
				 pos[posX][posY][zp]->x == posX &&
				 pos[posX][posY][zp]->y == posY &&
				 pos[posX][posY][zp]->z == zp) {
				shape = pos[posX][posY][zp]->shape;

				// FIXME: this draws more doors than needed... 
				// it should use doorValue to figure out what needs to be drawn
				if((!lightMap[chunkX][chunkY] && 
					(shape == scourge->getShapePalette()->getShape(Constants::NS_DOOR_INDEX) ||
					 shape == scourge->getShapePalette()->getShape(Constants::EW_DOOR_INDEX) ||
					 shape == scourge->getShapePalette()->getShape(Constants::NS_DOOR_TOP_INDEX) ||
					 shape == scourge->getShapePalette()->getShape(Constants::EW_DOOR_TOP_INDEX) ||
					 shape == scourge->getShapePalette()->getShape(Constants::DOOR_SIDE_INDEX))) ||
				   (lightMap[chunkX][chunkY] && shape)) {
				  xpos2 = (float)((chunkX - chunkStartX) * MAP_UNIT + 
								  xp + chunkOffsetX) / GLShape::DIV;
				  ypos2 = (float)((chunkY - chunkStartY) * MAP_UNIT - 
								  shape->getDepth() + 
								  yp + chunkOffsetY) / GLShape::DIV;
				  zpos2 = (float)(zp) / GLShape::DIV;
				  setupPosition(posX, posY, zp,
								xpos2, ypos2, zpos2,
								shape, pos[posX][posY][zp]->item, 
								pos[posX][posY][zp]->creature); 		  
				}
			  }
			}
		  }
		}
	  }
	}
  }
}

void Map::drawGroundPosition(int posX, int posY,
							 float xpos2, float ypos2,
							 Shape *shape) {
  GLuint name;
  // encode this shape's map location in its name
  name = posX + (MAP_WIDTH * posY);			
  glTranslatef( xpos2, ypos2, 0.0f);
  glPushName( name );
  shape->draw();
  glPopName();
  glTranslatef( -xpos2, -ypos2, 0.0f);
}

void Map::setupPosition(int posX, int posY, int posZ,
						float xpos2, float ypos2, float zpos2,
						Shape *shape, Item *item, Creature *creature) {
  GLuint name;
  name = posX + (MAP_WIDTH * (posY)) + (MAP_WIDTH * MAP_DEPTH * posZ);		
  
  // special effects
  GLint t = SDL_GetTicks();
  if(creature && t - creature->getDamageEffect() < Constants::DAMAGE_DURATION) {
	damage[damageCount].xpos = xpos2;
	damage[damageCount].ypos = ypos2;
	damage[damageCount].zpos = zpos2;
	damage[damageCount].shape = shape;
	damage[damageCount].item = item;
	damage[damageCount].creature = creature;
	damage[damageCount].name = name;
	damageCount++;
  }

  if(shape->isStencil()) {
	stencil[stencilCount].xpos = xpos2;
	stencil[stencilCount].ypos = ypos2;
	stencil[stencilCount].zpos = zpos2;
	stencil[stencilCount].shape = shape;
	stencil[stencilCount].item = item;
	stencil[stencilCount].creature = creature;
	stencil[stencilCount].name = name;
	stencilCount++;
  } else if(!shape->isStencil()) {
	if(shape->drawFirst()) {
	  other[otherCount].xpos = xpos2;
	  other[otherCount].ypos = ypos2;
	  other[otherCount].zpos = zpos2;
	  other[otherCount].shape = shape;
	  other[otherCount].item = item;
	  other[otherCount].creature = creature;
	  other[otherCount].name = name;
	  otherCount++;
	}
	if(shape->drawLater()) {
	  later[laterCount].xpos = xpos2;
	  later[laterCount].ypos = ypos2;
	  later[laterCount].zpos = zpos2;
	  later[laterCount].shape = shape;
	  later[laterCount].item = item;
	  later[laterCount].creature = creature;
	  later[laterCount].name = name;
	  laterCount++;
	}
  }
}

void Map::drawDraggedItem() {
  if(scourge->getMovingItem()) {
	// glDisable(GL_DEPTH_TEST);
	//	glDepthMask(GL_FALSE);
	glPushMatrix();
	glLoadIdentity();	
	glTranslatef( scourge->getSDLHandler()->mouseX, scourge->getSDLHandler()->mouseY, 500);
	glRotatef( xrot, 0.0f, 1.0f, 0.0f );  
	glRotatef( yrot, 1.0f, 0.0f, 0.0f );  
	glRotatef( zrot, 0.0f, 0.0f, 1.0f );
	doDrawShape(0, 0, 0, scourge->getMovingItem()->getShape(), 0);
	glPopMatrix();
	//	glEnable(GL_DEPTH_TEST);
	//glDepthMask(GL_TRUE);
  }


  /*
  float xpos2, ypos2, zpos2;  
  Shape *shape = NULL;  

  if(selX >= getX() && selX < getX() + MAP_VIEW_WIDTH &&
	 selY >= getY() && selY < getY() + MAP_VIEW_DEPTH &&
	 selZ < MAP_VIEW_HEIGHT &&
	 scourge->getMovingItem()) {

	shape = scourge->getMovingItem()->getShape();	
	int newz = selZ;
	Location *dropLoc = isBlocked(selX, selY, selZ, -1, -1, -1, shape, &newz);
	selZ = newz;

	// only let drop on other creatures and containers
	// unfortunately I have to call isWallBetween(), so objects aren't dragged behind walls
	// this makes moving items slow
	if(dropLoc || 
	   (oldLocatorSelX < MAP_WIDTH && 
		isWallBetween(selX, selY, selZ, 
					  oldLocatorSelX, 
					  oldLocatorSelY, 
					  oldLocatorSelZ))) {
	  selX = oldLocatorSelX;
	  selY = oldLocatorSelY;
	  selZ = oldLocatorSelZ;
	}
	
	xpos2 = ((float)(selX - getX()) / GLShape::DIV);
	ypos2 = (((float)(selY - getY() - 1) - (float)shape->getDepth()) / GLShape::DIV);
	zpos2 = (float)(selZ) / GLShape::DIV;
	
	doDrawShape(xpos2, ypos2, zpos2, shape, 0);

	oldLocatorSelX = selX;
	oldLocatorSelY = selY;
	oldLocatorSelZ = selZ;
  }
  */

}

void Map::draw() {
  if(zoomIn) {
	if(zoom <= 0.5f) {
	  zoomOut = false;
    } else {
	  zoom /= ZOOM_DELTA; 
	  xpos = (int)((float)scourge->getSDLHandler()->getScreen()->w / zoom / 2.0f);
	  ypos = (int)((float)scourge->getSDLHandler()->getScreen()->h / zoom / 2.0f);
    }
  } else if(zoomOut) {
	if(zoom >= 2.8f) {
	  zoomOut = false;
	} else {
	  zoom *= ZOOM_DELTA; 
	  xpos = (int)((float)scourge->getSDLHandler()->getScreen()->w / zoom / 2.0f);
	  ypos = (int)((float)scourge->getSDLHandler()->getScreen()->h / zoom / 2.0f);
    }
  }

  float oldrot;

  oldrot = yrot;
  if(yRotating != 0) yrot+=yRotating;
  if(yrot >= 55 || yrot < 0) yrot = oldrot;

  oldrot = zrot;
  if(zRotating != 0) zrot+=zRotating;
  if(zrot >= 360) zrot -= 360;
  if(zrot < 0) zrot = 360 + zrot;
  

  initMapView();
  if(lightMapChanged) configureLightMap();
  // populate the shape arrays
  if(mapChanged) setupShapes(false);
  if(selectMode) {
      for(int i = 0; i < otherCount; i++) doDrawShape(&other[i]);
  } else {  



#ifdef DEBUG_MOUSE_POS
	// debugging mouse position
	if(debugX < MAP_WIDTH && debugX >= 0) {
	  DrawLater later2;
	  
	  later2.shape = scourge->getShapePalette()->getShape(Constants::LAMP_BASE_INDEX);

	  later2.xpos = ((float)(debugX - getX()) / GLShape::DIV);
	  later2.ypos = (((float)(debugY - getY() - 1) - (float)((later2.shape)->getDepth())) / GLShape::DIV);
	  later2.zpos = (float)(debugZ) / GLShape::DIV;
	  
	  later2.item = NULL;
	  later2.creature = NULL;
	  later2.name = 0;	 
	  doDrawShape(&later2);
	}
#endif



	// draw the creatures/objects/doors/etc.
	for(int i = 0; i < otherCount; i++) {
	  if(selectedDropTarget && 
		 ((selectedDropTarget->creature && selectedDropTarget->creature == other[i].creature) ||
		  (selectedDropTarget->item && selectedDropTarget->item == other[i].item))) {
		colorAlreadySet = true;
		glColor4f(0, 1, 1, 1);
	  }
	  doDrawShape(&other[i]);
	}

    if(scourge->getUserConfiguration()->getStencilbuf() &&
			 scourge->getUserConfiguration()->getStencilBufInitialized()) {
	  // create a stencil for the walls
	  glDisable(GL_DEPTH_TEST);
	  glColorMask(0,0,0,0);
	  glEnable(GL_STENCIL_TEST);
	  glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	  glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
	  for(int i = 0; i < stencilCount; i++) doDrawShape(&stencil[i]);
	  
	  // Use the stencil to draw
	  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	  glEnable(GL_DEPTH_TEST);
	  // decr where the floor is (results in a number = 1)
	  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	  glStencilFunc(GL_EQUAL, 0, 0xffffffff);  // draw if stencil=0
	  // draw the ground  
	  setupShapes(true);
	  
	  // shadows
      if(scourge->getUserConfiguration()->getShadows() >= Constants::OBJECT_SHADOWS) {
	    glStencilFunc(GL_EQUAL, 0, 0xffffffff);  // draw if stencil=0
	    // GL_INCR makes sure to only draw shadow once
	    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);	
	    glDisable(GL_TEXTURE_2D);
	    glDepthMask(GL_FALSE);
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	    useShadow = true;
        if(scourge->getUserConfiguration()->getShadows() == Constants::ALL_SHADOWS) {
	      for(int i = 0; i < stencilCount; i++) {
		    doDrawShape(&stencil[i]);
	      }
        }
	    for(int i = 0; i < otherCount; i++) {
		  doDrawShape(&other[i]);
	    }
	    useShadow = false;
	    glDisable(GL_BLEND);
	    glEnable(GL_TEXTURE_2D);
	    glDepthMask(GL_TRUE);
      }	  
	  glDisable(GL_STENCIL_TEST); 
	
	  // draw the blended walls
	  glEnable(GL_BLEND);  
	  glDepthMask(GL_FALSE);
	  //glDisable(GL_LIGHTING);
	  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	  for(int i = 0; i < stencilCount; i++) doDrawShape(&stencil[i]);
	  //glEnable(GL_LIGHTING);
	  glDepthMask(GL_TRUE);    
	  glDisable(GL_BLEND);
	} else {
	  // draw the walls
	  for(int i = 0; i < stencilCount; i++) doDrawShape(&stencil[i]);
	  // draw the ground  
	  setupShapes(true);
	}
	
	// draw the effects
	glEnable(GL_BLEND);  
	glDepthMask(GL_FALSE);
	//      glDisable(GL_LIGHTING);
	for(int i = 0; i < laterCount; i++) {
	  later[i].shape->setupBlending();
	  doDrawShape(&later[i]);
	  later[i].shape->endBlending();
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	for(int i = 0; i < damageCount; i++) {
	  doDrawShape(&damage[i], 1);
	}
	drawShade();

	//      glEnable(GL_LIGHTING);
	glDepthMask(GL_TRUE);    
	glDisable(GL_BLEND);

	//drawDraggedItem();
  }
}

void Map::drawShade() {
  glPushMatrix();
  glLoadIdentity();
  //  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  //glDisable( GL_TEXTURE_2D );
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  //scourge->setBlendFunc();
  
  glColor4f( 1, 1, 1, 0.5f);

  glBindTexture( GL_TEXTURE_2D, overlay_tex );
  glBegin( GL_QUADS );
  //  glNormal3f(0.0f, 1.0f, 0.0f);
  glTexCoord2f( 0.0f, 0.0f );
  glVertex3f(0, 0, 0);
  glTexCoord2f( 0.0f, 1.0f );
  glVertex3f(0, 
			 scourge->getSDLHandler()->getScreen()->h, 0);
  glTexCoord2f( 1.0f, 1.0f );
  glVertex3f(scourge->getSDLHandler()->getScreen()->w, 
			 scourge->getSDLHandler()->getScreen()->h, 0);
  glTexCoord2f( 1.0f, 0.0f );
  glVertex3f(scourge->getSDLHandler()->getScreen()->w, 0, 0);
  glEnd();
  //glEnable( GL_TEXTURE_2D );
  glEnable(GL_DEPTH_TEST);
  glPopMatrix();
}

void Map::createOverlayTexture() {
  // create the dark texture
  unsigned int i, j;
  glGenTextures(1, (GLuint*)&overlay_tex);
//  float tmp = 0.7f;
  for(i = 0; i < OVERLAY_SIZE; i++) {
	for(j = 0; j < OVERLAY_SIZE; j++) {
	  float half = ((float)OVERLAY_SIZE - 0.5f) / 2.0f;
	  float id = (float)i - half;
	  float jd = (float)j - half;
	  float dd = 255.0f - ((255.0f / (half * half / 1.2f)) * (id * id + jd * jd));
	  if(dd < 0.0f) dd = 0.0f;
	  if(dd > 255.0f) dd = 255.0f;
	  unsigned char d = (unsigned char)dd;
	  overlay_data[i * OVERLAY_SIZE * 3 + j * 3 + 0] = d;
	  overlay_data[i * OVERLAY_SIZE * 3 + j * 3 + 1] = d;
	  overlay_data[i * OVERLAY_SIZE * 3 + j * 3 + 2] = d;
	}
  }
  glBindTexture(GL_TEXTURE_2D, overlay_tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, OVERLAY_SIZE, OVERLAY_SIZE, 0, 
			   GL_RGB, GL_UNSIGNED_BYTE, overlay_data);
}

void Map::doDrawShape(DrawLater *later, int effect) {
    doDrawShape(later->xpos, later->ypos, later->zpos, later->shape, later->name, effect, later);
}

void Map::doDrawShape(float xpos2, float ypos2, float zpos2, Shape *shape, 
					  GLuint name, int effect, DrawLater *later) {
  glPushMatrix();
  if(useShadow) {
	// put shadow above the floor a little
	glTranslatef( xpos2, ypos2, 0.26f / GLShape::DIV);
	glMultMatrixf(shadowTransformMatrix);
	glColor4f( 0, 0, 0, 0.5f );

	// FIXME: this is a nicer shadow color, but it draws outside the walls too.
	// need to fix the stencil ops to restrict shadow drawing to the floor.
	//glColor4f( 0.04f, 0, 0.07f, 0.6f );
	((GLShape*)shape)->useShadow = true;
  } else {
	glTranslatef( xpos2, ypos2, zpos2);
	if(colorAlreadySet) {
	  colorAlreadySet = false;
	} else {
	  glColor4f(0.72f, 0.65f, 0.55f, 0.5f);
	}
  }

  // encode this shape's map location in its name
  glPushName( name );
  //glPushName( (GLuint)((GLShape*)shape)->getShapePalIndex() );
  ((GLShape*)shape)->setCameraRot(xrot, yrot, zrot);
  ((GLShape*)shape)->setCameraPos(xpos, ypos, zpos, xpos2, ypos2, zpos2);
  if(effect && later && later->creature) {
	later->creature->getEffect()->draw((GLShape*)shape, 
									   later->creature->getEffectType(),
									   later->creature->getDamageEffect());
  } else {
	shape->draw();
  }
  ((GLShape*)shape)->useShadow = false;
  glPopName();
  glPopMatrix();
}

void Map::showInfoAtMapPos(Uint16 mapx, Uint16 mapy, Uint16 mapz, char *message) {
  float xpos2 = ((float)(mapx - getX()) / GLShape::DIV);
  float ypos2 = ((float)(mapy - getY()) / GLShape::DIV);
  float zpos2 = (float)(mapz) / GLShape::DIV;
  glTranslatef( xpos2, ypos2, zpos2 + 100);

  //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  //glRasterPos2f( 0, 0 );
  scourge->getSDLHandler()->texPrint(0, 0, "%s", message);

  glTranslatef( -xpos2, -ypos2, -(zpos2 + 100));
}

void Map::showCreatureInfo(Creature *creature, bool player, bool selected, bool groupMode) {
  glPushMatrix();
  //showInfoAtMapPos(creature->getX(), creature->getY(), creature->getZ(), creature->getName());

  glEnable( GL_DEPTH_TEST );
  glDepthMask(GL_FALSE);
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glDisable( GL_CULL_FACE );

  // draw circle
  double w = (double)creature->getShape()->getWidth() / GLShape::DIV;
  double s = 0.35f / GLShape::DIV;
  
  float xpos2, ypos2, zpos2;

  GLint t = SDL_GetTicks();
  if(t - lastTick > 45) {
	// initialize target width
	if(targetWidth == 0.0f) {
	  targetWidth = s;
	  targetWidthDelta *= -1.0f;
	}
	// targetwidth oscillation
	targetWidth += targetWidthDelta;
	if((targetWidthDelta < 0 && targetWidth < s) || 
	   (targetWidthDelta > 0 && targetWidth >= s + (5 * targetWidthDelta))) 
	  targetWidthDelta *= -1.0f;
	lastTick = t;
  }

  if(player && creature->getSelX() > -1 && 
	 !creature->getTargetCreature() &&
	 !(creature->getSelX() == creature->getX() && creature->getSelY() == creature->getY()) ) {
	// draw target
	glColor4f(1.0f, 0.75f, 0.0f, 0.5f);
	xpos2 = ((float)(creature->getSelX() - getX()) / GLShape::DIV);
	ypos2 = ((float)(creature->getSelY() - getY()) / GLShape::DIV);
	zpos2 = 0.0f / GLShape::DIV;  
	glPushMatrix();
	glTranslatef( xpos2 + w / 2.0f, ypos2 - w, zpos2 + 5);
	gluDisk(creature->getQuadric(), w / 1.8f - targetWidth, w / 1.8f, 12, 1);
	glPopMatrix();
  }

  if(player && creature->getTargetCreature()) {
	glColor4f(1.0f, 0.15f, 0.0f, 0.5f);
	xpos2 = ((float)(creature->getTargetCreature()->getX() - getX()) / GLShape::DIV);
	ypos2 = ((float)(creature->getTargetCreature()->getY() - getY()) / GLShape::DIV);
	zpos2 = 0.0f / GLShape::DIV;  
	glPushMatrix();
	glTranslatef( xpos2 + w / 2.0f, ypos2 - w, zpos2 + 5);
	gluDisk(creature->getQuadric(), w / 1.8f - targetWidth, w / 1.8f, 12, 1);
	glPopMatrix();
  }

  if(selected) {
	glColor4f(0, 1, 1, 0.5f);
  } else if(player) {
	glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
  } else {
	glColor4f(0.7f, 0.7f, 0.7f, 0.25f);
  }
  
  xpos2 = ((float)(creature->getX() - getX()) / GLShape::DIV);
  ypos2 = ((float)(creature->getY() - getY()) / GLShape::DIV);
  zpos2 = (float)(creature->getZ()) / GLShape::DIV;  
  glTranslatef( xpos2 + w / 2.0f, ypos2 - w, zpos2 + 5);
  if(groupMode || player) gluDisk(creature->getQuadric(), w / 1.8f - s, w / 1.8f, 12, 1);

  glEnable( GL_CULL_FACE );
  glDisable( GL_BLEND );
  glDisable( GL_DEPTH_TEST );
  glDepthMask(GL_TRUE);
  
  // draw name
  glTranslatef( 0, 0, 100);
  scourge->getSDLHandler()->texPrint(0, 0, "%s", creature->getName());

  //glTranslatef( -xpos2, -ypos2, -(zpos2 + 100));
  glPopMatrix();
}

/**
 * Initialize the map view (translater, rotate)
 */
void Map::initMapView(bool ignoreRot) {
  glLoadIdentity();

  // adjust for screen size
  float adjust = (float)scourge->getSDLHandler()->getScreen()->w / 800.0f;
  glScalef(adjust, adjust, adjust);

  glScalef(zoom, zoom, zoom);
  
  // translate the camera and rotate
  // the offsets ensure that the center of rotation is under the player
  glTranslatef( this->xpos, this->ypos, 0);  
  glRotatef( xrot, 0.0f, 1.0f, 0.0f );
  if(!ignoreRot) {
      glRotatef( yrot, 1.0f, 0.0f, 0.0f );  
  }  
  glRotatef( zrot, 0.0f, 0.0f, 1.0f );
  glTranslatef( 0, 0, this->zpos);  

  float startx = -((float)MAP_VIEW_WIDTH / 2.0) / GLShape::DIV;
  float starty = -((float)MAP_VIEW_DEPTH / 2.0) / GLShape::DIV;
  //float startz = -(float)(MAP_VIEW_HEIGHT) / GLShape::DIV;
  float startz = 0.0;

  glTranslatef( startx, starty, startz );
}

Location *Map::moveCreature(Sint16 x, Sint16 y, Sint16 z, Uint16 dir,Creature *newCreature) {
	Sint16 nx = x;
	Sint16 ny = y;
	Sint16 nz = z;
	switch(dir) {
	case Constants::MOVE_UP: ny--; break;
	case Constants::MOVE_DOWN: ny++; break;
	case Constants::MOVE_LEFT: nx--; break;
	case Constants::MOVE_RIGHT: nx++; break;
	}
	return moveCreature(x, y, z, nx, ny, nz, newCreature);
}

Location *Map::moveCreature(Sint16 x, Sint16 y, Sint16 z, 
							Sint16 nx, Sint16 ny, Sint16 nz,
							Creature *newCreature) {
  Location *position = isBlocked(nx, ny, nz, x, y, z, newCreature->getShape());
  if(position) return position;
  // move position
  removeCreature(x, y, z);
  setCreature(nx, ny, nz, newCreature);
  return NULL;
}

void Map::setFloorPosition(Sint16 x, Sint16 y, Shape *shape) {
  floorPositions[x][y] = shape;
  for(int xp = 0; xp < shape->getWidth(); xp++) {
	for(int yp = 0; yp < shape->getDepth(); yp++) {
	  scourge->getMiniMap()->colorMiniMapPoint(x + xp, y - yp, shape);
	}
  }
}

Shape *Map::removeFloorPosition(Sint16 x, Sint16 y) {
	Shape *shape = NULL;
  if(floorPositions[x][y]) {
    shape = floorPositions[x][y];
	floorPositions[x][y] = 0;
	for(int xp = 0; xp < shape->getWidth(); xp++) {
	  for(int yp = 0; yp < shape->getDepth(); yp++) {
		// fixme : is it good or not to erase the minimap too ???       
		scourge->getMiniMap()->eraseMiniMapPoint(x, y);
	  }
	}
  }
	return shape;
}

Location *Map::isBlocked(Sint16 x, Sint16 y, Sint16 z, 
						 Sint16 shapeX, Sint16 shapeY, Sint16 shapeZ, 
						 Shape *s, 
						 int *newz) {
  int zz = z;
  for(int sx = 0; sx < s->getWidth(); sx++) {
	for(int sy = 0; sy < s->getDepth(); sy++) {
	  // find the lowest location where this item fits
	  int sz = z;
	  while(sz < zz + s->getHeight()) {
		Location *loc = pos[x + sx][y - sy][z + sz];
		if(loc && loc->shape && 
		   !(loc->x == shapeX && loc->y == shapeY && loc->z == shapeZ)) {
		  if(newz && (loc->item || loc->creature)) {
			int tz = loc->z + loc->shape->getHeight();
			if(tz > zz) zz = tz;
			if(zz + s->getHeight() >= MAP_VIEW_HEIGHT) {
			  return pos[x + sx][y - sy][z + sz];
			}
			if(zz > sz) sz = zz;
			else break;
		  } else if(newz && loc) {
			return pos[x + sx][y - sy][z + sz];
		  } else if(!newz && !(loc && loc->item && !loc->item->isBlocking())) {
			return pos[x + sx][y - sy][z + sz];
		  } else {
			sz++;
		  }
		} else {
		  sz++;
		}
	  }
	}
  }
  if(newz) *newz = zz;
  return NULL;
}

void Map::switchPlaces(Sint16 x1, Sint16 y1, Sint16 z1,
											 Sint16 x2, Sint16 y2, Sint16 z2) {
	Shape *shape1 = removePosition(x1, y1, z1);
	Shape *shape2 = removePosition(x2, y2, z2);

  Location *position = isBlocked(x2, y2, z2, x1, y1, z1, shape1);
	if(position) {
    // can't do it
  	setPosition(x1, y1, z1, shape1);
	  setPosition(x2, y2, z2, shape2);
    return;
  }
	// move it
	setPosition(x2, y2, z2, shape1);

  position = isBlocked(x1, y1, z1, x2, y2, z2, shape2);
	if(position) {
    // can't do it
    removePosition(x2, y2, z2); // undo previous step
  	setPosition(x1, y1, z1, shape1);
	  setPosition(x2, y2, z2, shape2);
    return;
  }
	// move it
	setPosition(x1, y1, z1, shape2);  
}

Location *Map::getPosition(Sint16 x, Sint16 y, Sint16 z) {
  if(pos[x][y][z] &&
     ((pos[x][y][z]->shape &&
      pos[x][y][z]->x == x &&
      pos[x][y][z]->y == y &&
      pos[x][y][z]->z == z))) return pos[x][y][z];
  return NULL;
}

void Map::addDescription(char *desc, float r, float g, float b) {
  if(descriptionCount > 0) {
	int last = descriptionCount;
	if(last >= MAX_DESCRIPTION_COUNT) last--;
    for(int i = last; i >= 1; i--) {
	  strcpy(descriptions[i], descriptions[i - 1]);
	  descriptionsColor[i].r = descriptionsColor[i - 1].r;
	  descriptionsColor[i].g = descriptionsColor[i - 1].g;
	  descriptionsColor[i].b = descriptionsColor[i - 1].b;
    }
  }
  if(descriptionCount < MAX_DESCRIPTION_COUNT) descriptionCount++;
  strcpy(descriptions[0], desc);
  descriptionsColor[0].r = r;
  descriptionsColor[0].g = g;
  descriptionsColor[0].b = b;
  descriptionsChanged = true;
}

void Map::drawDescriptions(ScrollingList *list) {
  if(descriptionsChanged) {
    descriptionsChanged = false;
    list->setLines(descriptionCount, (const char**)descriptions, descriptionsColor);
  }

  /*
  glPushMatrix();
  glLoadIdentity();
  //glColor4f(1.0f, 1.0f, 0.4f, 1.0f);
  int y = TOP_GUI_HEIGHT - 5;
  if(descriptionCount <= 5) y = descriptionCount * 15;
  int index =  0;
  while(y > 5 && index < descriptionCount) {    
	glColor4f(descriptions[index].r, descriptions[index].g, descriptions[index].b, 1.0f);
    glRasterPos2f( (float)5, (float)y );
    scourge->getSDLHandler()->texPrint(5, y, "%s", descriptions[index].text);

    y -= 15;
    index++;
  }
  glPopMatrix();
  */
}

void Map::handleMouseClick(Uint16 mapx, Uint16 mapy, Uint16 mapz, Uint8 button) {
	char s[300];
  if(mapx < MAP_WIDTH) {
	if(button == SDL_BUTTON_RIGHT) {
	  //	  fprintf(stderr, "\tclicked map coordinates: x=%u y=%u z=%u\n", mapx, mapy, mapz);
	  Location *loc = getPosition(mapx, mapy, mapz);
	  if(loc) {
		char *description = NULL;
		Creature *creature = loc->creature;
		//fprintf(stderr, "\tcreature?%s\n", (creature ? "yes" : "no"));
		if(creature) {
			creature->getDetailedDescription(s);
		  description = s;
		} else {
		  Item *item = loc->item;
		  //fprintf(stderr, "\titem?%s\n", (item ? "yes" : "no"));
		  if( item ) {
				item->getDetailedDescription(s, false);
				description = s;
		  } else {
			Shape *shape = loc->shape;
			//fprintf(stderr, "\tshape?%s\n", (shape ? "yes" : "no"));
			if(shape) {
			  description = shape->getRandomDescription();
			}        
		  }
		}
		if(description) {
		  //            map->addDescription(x, y, mapx, mapy, mapz, description);
		  addDescription(description);
		}
	  }
    }
  }
}

void Map::handleMouseMove(Uint16 mapx, Uint16 mapy, Uint16 mapz) {
  if(mapx < MAP_WIDTH) {
	selX = mapx;
	selY = mapy;
	selZ = mapz;
  }
}

void Map::setPosition(Sint16 x, Sint16 y, Sint16 z, Shape *shape) {
  if(shape) {
	mapChanged = true;
	for(int xp = 0; xp < shape->getWidth(); xp++) {
	  for(int yp = 0; yp < shape->getDepth(); yp++) {
	    scourge->getMiniMap()->colorMiniMapPoint(x + xp, y - yp, shape);
		for(int zp = 0; zp < shape->getHeight(); zp++) {
		  
		  if(!pos[x + xp][y - yp][z + zp]) {
			pos[x + xp][y - yp][z + zp] = new Location();
		  }
		  
		  pos[x + xp][y - yp][z + zp]->shape = shape;
		  pos[x + xp][y - yp][z + zp]->item = NULL;
		  pos[x + xp][y - yp][z + zp]->creature = NULL;
		  pos[x + xp][y - yp][z + zp]->x = x;
		  pos[x + xp][y - yp][z + zp]->y = y;
		  pos[x + xp][y - yp][z + zp]->z = z;
		}
	  }
	}
	
  }
}

Shape *Map::removePosition(Sint16 x, Sint16 y, Sint16 z) {
  Shape *shape = NULL;
  if(pos[x][y][z] &&
     pos[x][y][z]->shape &&
     pos[x][y][z]->x == x &&
     pos[x][y][z]->y == y &&
     pos[x][y][z]->z == z) {
	mapChanged = true;
    shape = pos[x][y][z]->shape;
    for(int xp = 0; xp < shape->getWidth(); xp++) {
      for(int yp = 0; yp < shape->getDepth(); yp++) {
        // fixme : is it good or not to erase the minimap too ???       
        scourge->getMiniMap()->eraseMiniMapPoint(x + xp, y - yp);
        for(int zp = 0; zp < shape->getHeight(); zp++) {
		  delete pos[x + xp][y - yp][z + zp];
		  pos[x + xp][y - yp][z + zp] = NULL;          
        }
      }
    }
  }
  return shape;
}

void Map::setItem(Sint16 x, Sint16 y, Sint16 z, Item *item) {
  if(item) {
    if(item->getShape()) {
	  mapChanged = true;
      for(int xp = 0; xp < item->getShape()->getWidth(); xp++) {
        for(int yp = 0; yp < item->getShape()->getDepth(); yp++) {          
          for(int zp = 0; zp < item->getShape()->getHeight(); zp++) {
			
            if(!pos[x + xp][y - yp][z + zp]) {
              pos[x + xp][y - yp][z + zp] = new Location();
            }

            pos[x + xp][y - yp][z + zp]->item = item;
			pos[x + xp][y - yp][z + zp]->shape = item->getShape();
			pos[x + xp][y - yp][z + zp]->creature = NULL;
            pos[x + xp][y - yp][z + zp]->x = x;
            pos[x + xp][y - yp][z + zp]->y = y;
            pos[x + xp][y - yp][z + zp]->z = z;
          }
        }
      }
  	}
  }
}

Item *Map::removeItem(Sint16 x, Sint16 y, Sint16 z) {
  Item *item = NULL;
  if(pos[x][y][z] &&
     pos[x][y][z]->item &&
     pos[x][y][z]->x == x &&
     pos[x][y][z]->y == y &&
     pos[x][y][z]->z == z) {
	mapChanged = true;
	item = pos[x][y][z]->item;
    for(int xp = 0; xp < item->getShape()->getWidth(); xp++) {
      for(int yp = 0; yp < item->getShape()->getDepth(); yp++) {       
        for(int zp = 0; zp < item->getShape()->getHeight(); zp++) {
		  
		  delete pos[x + xp][y - yp][z + zp];
		  pos[x + xp][y - yp][z + zp] = NULL;		
        }
      }
    }
  }
  return item;
}

// drop items above this one
void Map::dropItemsAbove(int x, int y, int z, Item *item) {
  int count = 0;
  Location drop[100];
  for(int tx = 0; tx < item->getShape()->getWidth(); tx++) {
	for(int ty = 0; ty < item->getShape()->getDepth(); ty++) {
	  for(int tz = z + item->getShape()->getHeight(); tz < MAP_VIEW_HEIGHT; tz++) {
		Location *loc2 = pos[x + tx][y - ty][tz];
		if(loc2 && loc2->item) {
		  drop[count].x = loc2->x;
		  drop[count].y = loc2->y;
		  drop[count].z = loc2->z - item->getShape()->getHeight();
		  drop[count].item = loc2->item;
		  count++;
		  removeItem(loc2->x, loc2->y, loc2->z);
		  tz += drop[count - 1].item->getShape()->getHeight() - 1;
		}
	  }
	}
  }
  for(int i = 0; i < count; i++) {
	cerr << "item " << drop[i].item->getRpgItem()->getName() << " new z=" << drop[i].z << endl;
	setItem(drop[i].x, drop[i].y, drop[i].z, drop[i].item);
  }
}

void Map::setCreature(Sint16 x, Sint16 y, Sint16 z, Creature *creature) {
  char message[120];  
  if(creature) {
    if(creature->getShape()) {
	  mapChanged = true;
	  while(true) {
		for(int xp = 0; xp < creature->getShape()->getWidth(); xp++) {
		  for(int yp = 0; yp < creature->getShape()->getDepth(); yp++) {
			for(int zp = 0; zp < creature->getShape()->getHeight(); zp++) {
			  if(!pos[x + xp][y - yp][z + zp]) {
				pos[x + xp][y - yp][z + zp] = new Location();
			  } else if(pos[x + xp][y - yp][z + zp]->item) {
				// creature picks up non-blocking item (this is the only way to handle 
				// non-blocking items. It's also very 'roguelike'.
				Item *item = pos[x + xp][y - yp][z + zp]->item;
				removeItem(pos[x + xp][y - yp][z + zp]->x,
						   pos[x + xp][y - yp][z + zp]->y,
						   pos[x + xp][y - yp][z + zp]->z);
				creature->addInventory(item);
				sprintf(message, "%s picks up %s.", 
						creature->getName(), 
						item->getRpgItem()->getName());
				addDescription(message);				
				// since the above will have removed some locations, try adding the creature again
				continue;
			  }
			  pos[x + xp][y - yp][z + zp]->item = NULL;
			  pos[x + xp][y - yp][z + zp]->shape = creature->getShape();
			  pos[x + xp][y - yp][z + zp]->creature = creature;
			  pos[x + xp][y - yp][z + zp]->x = x;
			  pos[x + xp][y - yp][z + zp]->y = y;
			  pos[x + xp][y - yp][z + zp]->z = z;
			  //creature->moveTo(x, y, z);
			}
		  }
		}
		break;
	  }
	}
  }
}

Creature *Map::removeCreature(Sint16 x, Sint16 y, Sint16 z) {
  Creature *creature = NULL;
  if(pos[x][y][z] &&
     pos[x][y][z]->creature &&
     pos[x][y][z]->x == x &&
     pos[x][y][z]->y == y &&
     pos[x][y][z]->z == z) {
	mapChanged = true;
    creature = pos[x][y][z]->creature;
    for(int xp = 0; xp < creature->getShape()->getWidth(); xp++) {
      for(int yp = 0; yp < creature->getShape()->getDepth(); yp++) {
        for(int zp = 0; zp < creature->getShape()->getHeight(); zp++) {	  
		  delete pos[x + xp][y - yp][z + zp];
		  pos[x + xp][y - yp][z + zp] = NULL;	
        }
      }
    }
  }
  return creature;
}


// FIXME: only uses x,y for now
bool Map::isWallBetween(int x1, int y1, int z1,
						int x2, int y2, int z2) {

  if(x1 == x2 && y1 == y2) return isWall(x1, y1, z1);
  if(x1 == x2) {
	if(y1 > y2) SWAP(y1, y2);
	for(int y = y1; y <= y2; y++) {
	  if(isWall(x1, y, z1)) return true;
	}
	return false;
  }
  if(y1 == y2) {
	if(x1 > x2) SWAP(x1, x2);
	for(int x = x1; x <= x2; x++) {
	  if(isWall(x, y1, z1)) return true;
	}
	return false;
  }
  

  //  fprintf(stderr, "Checking for wall: from: %d,%d to %d,%d\n", x1, y1, x2, y2);
  bool ret = false;
  bool yDiffBigger = (abs(y2 - y1) > abs(x2 - x1));
  float m = (float)(y2 - y1) / (float)(x2 - x1);
  int steps = (yDiffBigger ? abs(y2 - y1) : abs(x2 - x1));
  float x = x1;
  float y = y1;
  for(int i = 0; i < steps; i++) {
	//	fprintf(stderr, "\tat=%f,%f\n", x, y);
	if(isWall((int)x, (int)y, z1)) {
	  //fprintf(stderr, "wall at %f, %f\n", x, y);
	  ret = true;
	  break;
	}
	if(yDiffBigger) {
	  if(y1 < y2) y += 1.0f;
	  else y -= 1.0f;
	  if(x1 < x2) x += 1.0f / abs(m);
	  else x += -1.0f / abs(m);
	} else {
	  if(x1 < x2) x += 1.0f;
	  else x -= 1.0f;
	  if(y1 < y2) y += abs(m);
	  else y += -1.0 * abs(m);
	}
  }
  //  fprintf(stderr, "wall in between? %s\n", (ret ? "TRUE": "FALSE"));
  return ret;
}

bool Map::isWall(int x, int y, int z) {
  Location *loc = getLocation((int)x, (int)y, z);
  return (loc && 
		  (!loc->item || loc->item->getShape() != loc->shape) && 
		  (!loc->creature || loc->creature->getShape() != loc->shape));
}

// FIXME: only uses x, y for now
bool Map::shapeFits(Shape *shape, int x, int y, int z) {
  for(int tx = 0; tx < shape->getWidth(); tx++) {
	for(int ty = 0; ty < shape->getDepth(); ty++) {
	  if(getLocation(x + tx, y - ty, 0)) {
		return false;
	  }
	}
  }
  return true;
}

// FIXME: only uses x, y for now
Location *Map::getBlockingLocation(Shape *shape, int x, int y, int z) {
  for(int tx = 0; tx < shape->getWidth(); tx++) {
	for(int ty = 0; ty < shape->getDepth(); ty++) {
	  Location *loc = getLocation(x + tx, y - ty, 0);
	  if(loc) return loc;
	}
  }
  return NULL;
}

/**
   Return the drop location, or NULL if none
 */
Location *Map::getDropLocation(Shape *shape, int x, int y, int z) {
  for(int tx = 0; tx < shape->getWidth(); tx++) {
	for(int ty = 0; ty < shape->getDepth(); ty++) {
	  Location *loc = getLocation(x + tx, y - ty, z);
	  if(loc) {
		return loc;
	  }
	}
  }
  return NULL;
}

// the world has changed...
void Map::configureLightMap() {
  lightMapChanged = false;

  // draw nothing at first
  for(int x = 0; x < MAP_WIDTH / MAP_UNIT; x++) {
	for(int y = 0; y < MAP_DEPTH / MAP_UNIT; y++) {
	  lightMap[x][y] = (LIGHTMAP_ENABLED ? 0 : 1);
	}
  }
  if(!LIGHTMAP_ENABLED) return;

  int chunkX = (scourge->getPlayer()->getX() + 
				(scourge->getPlayer()->getShape()->getWidth() / 2) - 
				MAP_OFFSET) / MAP_UNIT;
  int chunkY = (scourge->getPlayer()->getY() - 
				(scourge->getPlayer()->getShape()->getDepth() / 2) - 
				MAP_OFFSET) / MAP_UNIT;

  traceLight(chunkX, chunkY);
}

void Map::traceLight(int chunkX, int chunkY) {
  if(chunkX < 0 || chunkX >= MAP_WIDTH / MAP_UNIT ||
	 chunkY < 0 || chunkY >= MAP_DEPTH / MAP_UNIT)
	return;

  // already visited?
  if(lightMap[chunkX][chunkY]) return;

  // let there be light
  lightMap[chunkX][chunkY] = 1;
  
  // can we go N?
  int x, y;
  bool blocked = false;
  x = chunkX * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
  for(y = chunkY * MAP_UNIT + MAP_OFFSET - (MAP_UNIT / 2);
	  y < chunkY * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
	  y++) {
   	if(isLocationBlocked(x, y, 0)) {
	  blocked = true;
	  break;
	}
  }
  if(!blocked) traceLight(chunkX, chunkY - 1);
  
  // can we go E?
  blocked = false;
  y = chunkY * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
  for(x = chunkX * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
	  x < chunkX * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2) + MAP_UNIT;
	  x++) {
	if(isLocationBlocked(x, y, 0)) {
	  blocked = true;
	  break;
	}
  }
  if(!blocked) traceLight(chunkX + 1, chunkY);
  
  // can we go S?
  blocked = false;
  x = chunkX * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
  for(y = chunkY * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
	  y < chunkY * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2) + MAP_UNIT;
	  y++) {
	if(isLocationBlocked(x, y, 0)) {
	  blocked = true;
	  break;
	}
  }
  if(!blocked) traceLight(chunkX, chunkY + 1);

  // can we go W?
  blocked = false;
  y = chunkY * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
  for(x = chunkX * MAP_UNIT + MAP_OFFSET - (MAP_UNIT / 2);
	  x < chunkX * MAP_UNIT + MAP_OFFSET + (MAP_UNIT / 2);
	  x++) {
	if(isLocationBlocked(x, y, 0)) {
	  blocked = true;
	  break;
	}
  }
  if(!blocked) traceLight(chunkX - 1, chunkY);
}

bool Map::isLocationBlocked(int x, int y, int z) {
  if(x >= 0 && x < MAP_WIDTH && 
	 y >= 0 && y < MAP_DEPTH && 
	 z >= 0 && z < MAP_VIEW_HEIGHT) {
	Location *pos = getLocation(x, y, z);
	if(pos == NULL || pos->item || pos->creature) { 
	  return false;
	}
  }
  return true;
}

void Map::drawCube(float x, float y, float z, float r) {
  glBegin(GL_QUADS);
  // front
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(-r+x, -r+y, r+z);
  glVertex3f(r+x, -r+y, r+z);
  glVertex3f(r+x, r+y, r+z);
  glVertex3f(-r+x, r+y, r+z);
  
  // back
  glNormal3f(0.0, 0.0, -1.0);
  glVertex3f(r+x, -r+y, -r+z);
  glVertex3f(-r+x, -r+y, -r+z);
  glVertex3f(-r+x, r+y, -r+z);
  glVertex3f(r+x, r+y, -r+z);
  
  // top
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(-r+x, r+y, r+z);
  glVertex3f(r+x, r+y, r+z);
  glVertex3f(r+x, r+y, -r+z);
  glVertex3f(-r+x, r+y, -r+z);
  
  // bottom
  glNormal3f(0.0, -1.0, 0.0);
  glVertex3f(-r+x, -r+y, -r+z);
  glVertex3f(r+x, -r+y, -r+z);
  glVertex3f(r+x, -r+y, r+z);
  glVertex3f(-r+x, -r+y, r+z);
  
  // left
  glNormal3f(-1.0, 0.0, 0.0);
  glVertex3f(-r+x, -r+y, -r+z);
  glVertex3f(-r+x, -r+y, r+z);
  glVertex3f(-r+x, r+y, r+z);
  glVertex3f(-r+x, r+y, -r+z);
  
  // right
  glNormal3f(1.0, 0.0, 0.0);
  glVertex3f(r+x, -r+y, r+z);
  glVertex3f(r+x, -r+y, -r+z);
  glVertex3f(r+x, r+y, -r+z);
  glVertex3f(r+x, r+y, r+z);
  
  glEnd();
  
}
