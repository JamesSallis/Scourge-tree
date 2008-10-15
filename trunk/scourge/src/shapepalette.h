/***************************************************************************
            shapepalette.h  -  Loads and inits textures/models
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
#pragma once

#include <string>
#include <vector>
#include <map>
#include "render/shapes.h"

/**
  *@author Gabor Torok
  */

class GLShape;
class GLTorch;
class Session;
class Monster;
class ModelWrapper;
class ConfigLang;
class ConfigNode;
class SDLScreenView;

/// A location on the world map.
struct MapGridLocation {
	char name[80];
	int x, y;
	bool random;
	char type;
};

/// An outdoor texture that can be referenced by name.
struct NamedOutdoorTexture {
	char name[80];
	Texture* tex;
	int width, height;
};

/// Interface to the game's texture pool.
class ShapePalette : public Shapes {
private:
	ModelLoader *loader;

	Texture gui_texture, gui_texture2, hand_attack_icon;
	Texture* gui_wood_texture;
	std::map<int, Texture*> statModIcons;
	Texture thirstIcon, hungerIcon;

	Session *session;

	std::vector<Texture*> portraitTextures[2];
	Texture deathPortraitTexture;
	Texture progressTexture, progressHighlightTexture;

	char aboutText[3000];

	Texture mapGrid[ Constants::MAP_GRID_TILE_WIDTH ][ Constants::MAP_GRID_TILE_HEIGHT ];
	std::map<char, std::vector<MapGridLocation*>*> mapGridLocationByType;

	std::map<std::string, Texture*> namedTextures;
	std::map<std::string, NamedOutdoorTexture> outdoorNamedTextures;
	SDL_Rect equipLocationHoles[ Constants::EQUIP_LOCATION_COUNT ];

public:
	ShapePalette( Session *session );
	~ShapePalette();

	inline Session *getSession() {
		return session;
	}

	inline  Texture* getProgressTexture() {
		return &progressTexture;
	}
	inline Texture* getProgressHighlightTexture() {
		return &progressHighlightTexture;
	}

	inline Texture const* getMapGridTile( int x, int y ) {
		return &mapGrid[ x ][ y ];
	}

	inline Texture* getHandsAttackIcon() {
		return &hand_attack_icon;
	}

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

	inline char *getAboutText() {
		return aboutText;
	}

	void preInitialize();
	virtual void initialize();

	void loadNpcPortraits();

	GLuint formationTexIndex;

	inline Texture* getStatModIcon( int statModIndex ) {
		if ( statModIcons.find( statModIndex ) == statModIcons.end() ) return NULL; else return statModIcons[statModIndex];
	}
	inline Texture* getThirstIcon() {
		return &thirstIcon;
	}
	inline Texture* getHungerIcon() {
		return &hungerIcon;
	}

	// cursor
	SDL_Surface *tiles, *spells;
	Texture tilesTex[20][20];
	Texture spellsTex[20][20];

	Texture logo_texture;
	Texture chain_texture;
	Texture scourgeBackdrop_texture;

	Texture cloud, candle, highlight;
	Texture border, border2, gargoyle;
	Texture minimap, minimapMask, dismiss, exitTexture, options, group, inventory;
	Texture waitTexture, startTexture, realTimeTexture, pausedTexture;
	Texture systemTexture;
	Texture ioTexture;

	Texture raindropTexture;

	inline Texture const* getGuiTexture() {
		return &gui_texture;
	}
	inline Texture const* getGuiTexture2() {
		return &gui_texture2;
	}
	inline Texture* getGuiWoodTexture() {
		return gui_wood_texture;
	}
	//inline GLuint getPaperDollTexture() { return paper_doll_texture; }
	inline Texture const* getHighlightTexture() {
		return &highlight;
	}
	inline Texture* getBorderTexture() {
		return &border;
	}
	inline Texture* getBorder2Texture() {
		return &border2;
	}
	inline Texture* getGargoyleTexture() {
		return &gargoyle;
	}
	inline Texture* getMinimapTexture() {
		return &minimap;
	}
	inline Texture* getMinimapMaskTexture() {
		return &minimapMask;
	}
	inline Texture* getDismissTexture() {
		return &dismiss;
	}
	inline Texture* getExitTexture() {
		return &exitTexture;
	}
	inline Texture* getOptionsTexture() {
		return &options;
	}
	inline Texture* getGroupTexture() {
		return &group;
	}
	inline Texture* getInventoryTexture() {
		return &inventory;
	}

	inline Texture* getPausedTexture() {
		return &pausedTexture;
	}
	inline Texture* getRealTimeTexture() {
		return &realTimeTexture;
	}
	inline Texture* getStartTexture() {
		return &startTexture;
	}
	inline Texture* getWaitTexture() {
		return &waitTexture;
	}

	inline Texture* getIoTexture() {
		return &ioTexture;
	}

	inline Texture* getSystemIconTexture() {
		return &systemTexture;
	}

	inline Texture* getRaindropTexture() {
		return &raindropTexture;
	}

	inline int getPortraitCount( int sex ) {
		return portraitTextures[sex].size();
	}
	inline Texture* getPortraitTexture( int sex, int index ) {
		return portraitTextures[sex][ index ];
	}
	inline Texture* getDeathPortraitTexture() {
		return &deathPortraitTexture;
	}

	// Md2 shapes
	GLShape *getCreatureShape( char *model_name, char *skin_name, float scale = 0.0f,
	                           Monster *monster = NULL );
	void decrementSkinRefCountAndDeleteShape( char *model_name,
	                                          char *skin_name,
	                                          GLShape *shape,
	                                          Monster *monster = NULL );
	void debugLoadedModels();

	inline Texture* getNamedTexture( std::string name ) {
		return( namedTextures.find( name ) == namedTextures.end() ? 0 : namedTextures[ name ] );
	}
	inline NamedOutdoorTexture *getOutdoorNamedTexture( std::string name ) {
		return( outdoorNamedTextures.find( name ) == outdoorNamedTextures.end() ? NULL : &( outdoorNamedTextures[ name ] ) );
	}
	inline std::map<std::string, NamedOutdoorTexture> *getOutdoorNamedTextures() {
		return &outdoorNamedTextures;
	}
	inline SDL_Rect *getInventoryHole( int inventoryLocation ) {
		return( inventoryLocation >= 0 &&
						inventoryLocation < Constants::EQUIP_LOCATION_COUNT ? 
						&( equipLocationHoles[ inventoryLocation ] ) : 
		        NULL );
	}

protected:
	void initFonts( ConfigLang *config );
	void initCursor( ConfigLang *config );
	void initAbout( ConfigLang *config );
	void initPcPortraits( ConfigLang *config );
	void initPcModels( ConfigLang *config );
	void initRugs( ConfigLang *config );
	void initSystemTextures( ConfigLang *config );
	void initNativeShapes( ConfigLang *config );
	void initVirtualShapes( ConfigLang *config );
	void init3dsShapes( ConfigLang *config );
	void initOccurance( ConfigNode *node, ShapeValues *sv );
	void initThemes( ConfigLang *config );
	void initDescriptions( ConfigLang *config );
	void initSounds( ConfigLang *config );
	void initNamedTextures( ConfigLang *config );
	void initInventory( ConfigLang *config );

	ShapeValues *createShapeValues( ConfigNode *node );
	DECLARE_NOISY_OPENGL_SUPPORT();
};

#endif

