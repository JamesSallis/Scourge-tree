/***************************************************************************
                          shapepalette.h  -  description
                             -------------------
    begin                : Sat Jun 14 2003
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

#ifndef SHAPEPALETTE_H
#define SHAPEPALETTE_H

#include <string>
#include <vector>
#include <map>
#include "common/constants.h"
#include "render/shapes.h"

/**
  *@author Gabor Torok
  */

class GLShape;
class GLTorch;
class Session;
class Monster;
class ModelWrapper;

typedef struct _MapGridLocation {
  char name[80];
  int x, y;
  bool random;
  char type;
} MapGridLocation;

class ShapePalette : public Shapes {
private:
	ModelLoader *loader;

  GLuint gui_texture, gui_wood_texture, gui_texture2;
  std::map<int, GLuint> statModIcons;
	GLuint thirstIcon, hungerIcon;

  Session *session;
  
  std::vector<GLuint> portraitTextures[2];
  GLuint deathPortraitTexture;
  GLuint progressTexture, progressHighlightTexture;

  char aboutText[3000];

  GLuint mapGrid[ Constants::MAP_GRID_TILE_WIDTH ][ Constants::MAP_GRID_TILE_HEIGHT ];
  std::map<char, std::vector<MapGridLocation*>*> mapGridLocationByType;

public: 
  ShapePalette( Session *session );
  ~ShapePalette();
  
  inline Session *getSession() { return session; }

  inline GLuint getProgressTexture() { return progressTexture; }
  inline GLuint getProgressHighlightTexture() { return progressHighlightTexture; }

  inline GLuint getMapGridTile( int x, int y ) { return mapGrid[ x ][ y ]; }

  void initMapGrid();

  /**
   * Find a random location on the scourge map.
   * @param type a char depicting an arbitrary map type (eg.: C-city, D-dungeon, etc.)
   * @param name will point to the name of the location found
   * @param x the x coordinate
   * @param y the y coordinate
   * @return true if a location of type was found.
   */
  bool getRandomMapLocation( char type, char **name, int *x, int *y );
  
  inline char *getAboutText() { return aboutText; }

  void preInitialize();
  void initialize();

  void loadNpcPortraits();

  GLuint formationTexIndex;

  inline GLuint getStatModIcon(int statModIndex) { if(statModIcons.find(statModIndex) == statModIcons.end()) return (GLuint)0; else return statModIcons[statModIndex]; }
	inline GLuint getThirstIcon() { return thirstIcon; }
	inline GLuint getHungerIcon() { return hungerIcon; }

  // cursor
  SDL_Surface *tiles, *spells;
  GLubyte *tilesImage[20][20], *spellsImage[20][20];
  GLuint tilesTex[20][20], spellsTex[20][20];
  SDL_Surface *paperDoll;
  GLubyte *paperDollImage;

  SDL_Surface *logo;
  GLubyte *logoImage;   
  GLuint logo_texture;

  SDL_Surface *chain;
  GLubyte *chainImage;   
  GLuint chain_texture;

  SDL_Surface *scourge;
  GLubyte *scourgeImage;

  GLuint cloud, candle, torchback, highlight;
  GLuint border, border2, gargoyle;
  GLuint minimap, minimapMask, dismiss, exitTexture, options, group, inventory;
	GLuint waitTexture, startTexture, realTimeTexture, pausedTexture;
	GLuint systemTexture;

  inline GLuint getGuiTexture() { return gui_texture; }
  inline GLuint getGuiTexture2() { return gui_texture2; }
  inline GLuint getGuiWoodTexture() { return gui_wood_texture; }
  //inline GLuint getPaperDollTexture() { return paper_doll_texture; }
  inline GLuint getHighlightTexture() { return highlight; }
  inline GLuint getBorderTexture() { return border; }
  inline GLuint getBorder2Texture() { return border2; }
  inline GLuint getGargoyleTexture() { return gargoyle; }
  inline GLuint getMinimapTexture() { return minimap; }
  inline GLuint getMinimapMaskTexture() { return minimapMask; }
	inline GLuint getDismissTexture() { return dismiss; }
	inline GLuint getExitTexture() { return exitTexture; }
	inline GLuint getOptionsTexture() { return options; }
	inline GLuint getGroupTexture() { return group; }
	inline GLuint getInventoryTexture() { return inventory; }
	
	inline GLuint getPausedTexture() { return pausedTexture; }	
	inline GLuint getRealTimeTexture() { return realTimeTexture; }	
	inline GLuint getStartTexture() { return startTexture; }	
	inline GLuint getWaitTexture() { return waitTexture; }	

	inline GLuint getSystemIconTexture() { return systemTexture; }

  inline int getPortraitCount( int sex ) { return portraitTextures[sex].size(); }
  inline GLuint getPortraitTexture( int sex, int index ) { return portraitTextures[sex][ index ]; }
  inline GLuint getDeathPortraitTexture() { return deathPortraitTexture; }

  // Md2 shapes
  GLShape *getCreatureShape(char *model_name, char *skin_name, float scale=0.0f, 
                            Monster *monster=NULL);
  void decrementSkinRefCount(char *model_name, char *skin_name, 
                             Monster *monster=NULL);
	void debugLoadedModels();

protected:
  virtual int interpretShapesLine( FILE *fp, int n );
};

#endif

