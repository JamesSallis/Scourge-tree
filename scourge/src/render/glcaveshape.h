/***************************************************************************
            glcaveshape.h  -  extends GLShape for rendering caves
                             -------------------
    begin                : Thu Jul 10 2003
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

#ifndef GLCAVE_SHAPE_H
#define GLCAVE_SHAPE_H
#pragma once

#include "render.h"
#include "glshape.h"
#include <vector>

class Shapes;

/// A triangle face in a cave.

class CaveFace {
public:
	int p1, p2, p3; // point indexes
	CVector3 normal;
	GLfloat tex[3][2]; // texture coordinates per point
	enum {
		WALL = 0,
		TOP,
		FLOOR
	};
	int textureType;
	GLfloat shade;

	CaveFace( int p1, int p2, int p3, GLfloat u1, GLfloat v1, GLfloat u2, GLfloat v2, GLfloat u3, GLfloat v3, int textureType )
			: normal( 0, 0, 0 ) {
		this->p1 = p1;
		this->p2 = p2;
		this->p3 = p3;
		tex[0][0] = u1;
		tex[0][1] = v1;
		tex[1][0] = u2;
		tex[1][1] = v2;
		tex[2][0] = u3;
		tex[2][1] = v3;
		this->textureType = textureType;
		this->shade = 1;
	}

	~CaveFace() {
	}
};

/// Creates the shapes (walls, lava, floor etc.) that are used inside caves.

class GLCaveShape : public GLShape {
private:
	int mode;
	int dir;
	Shapes *shapes;
	Texture* wallTextureGroup;
	Texture* topTextureGroup;
	Texture* floorTextureGroup;
	int caveIndex;
	int stencilIndex;
	int stencilAngle;

	enum {
		MODE_FLAT = 0,
		MODE_CORNER,
		MODE_BLOCK,
		MODE_FLOOR,
		MODE_INV,
		MODE_LAVA
	};

	enum {
		DIR_N = 0,
		DIR_E,
		DIR_S,
		DIR_W,
		DIR_NE,
		DIR_SE,
		DIR_SW,
		DIR_NW,
		DIR_CROSS_NW,
		DIR_CROSS_NE
	};


	GLCaveShape( Shapes *shapes, Texture texture[], int width, int depth, int height, char const* name, int index,
	             int mode, int dir, int caveIndex, int stencilIndex = 0, int stencilAngle = 0 );
	virtual ~GLCaveShape();

	virtual void initialize();

public:


	void draw();

	virtual inline bool isFlatCaveshape() {
		return caveIndex >= LAVA_SIDE_W;
	}

	enum {
		CAVE_INDEX_N = 0,
		CAVE_INDEX_E,
		CAVE_INDEX_S,
		CAVE_INDEX_W,
		CAVE_INDEX_NE,
		CAVE_INDEX_SE,
		CAVE_INDEX_SW,
		CAVE_INDEX_NW,
		CAVE_INDEX_INV_NE,
		CAVE_INDEX_INV_SE,
		CAVE_INDEX_INV_SW,
		CAVE_INDEX_INV_NW,
		CAVE_INDEX_CROSS_NW,
		CAVE_INDEX_CROSS_NE,
		CAVE_INDEX_BLOCK,
		CAVE_INDEX_FLOOR,

		LAVA_SIDE_W,
		LAVA_SIDE_E,
		LAVA_SIDE_N,
		LAVA_SIDE_S,
		LAVA_OUTSIDE_TURN_NW,
		LAVA_OUTSIDE_TURN_NE,
		LAVA_OUTSIDE_TURN_SE,
		LAVA_OUTSIDE_TURN_SW,
		LAVA_U_N,
		LAVA_U_E,
		LAVA_U_S,
		LAVA_U_W,
		LAVA_SIDES_NS,
		LAVA_SIDES_EW,
		LAVA_ALL,
		LAVA_NONE,

		CAVE_INDEX_COUNT
	};


	static void createShapes( Texture texture[], int shapeCount, Shapes *shapes );
	static void initializeShapes( Shapes *shapes );
	static inline GLCaveShape *getShape( int index ) {
		return our.shapeList[ index ];
	}

	virtual inline bool isShownInMapEditor() {
		return false;
	}

protected:
	void drawFaces();
	void drawBlock( float w, float h, float d );
	void drawFloor( float w, float h, float d );
	void drawLava( float w, float h, float d );

private:
	static char const* names[CAVE_INDEX_COUNT];
	class Common {
	public:
		Common();
		~Common();
		void poly( int index, int p1, int p2, int p3, GLfloat u1, GLfloat v1, GLfloat u2, GLfloat v2, GLfloat u3, GLfloat v3, int textureType ) {
			CaveFace* cf = new CaveFace( p1, p2, p3, u1, v1, u2, v2, u3, v3, textureType );
			polys[index]->push_back( cf );
		}
		void removeDupPoints();
		void updatePointIndexes( int oldIndex, int newIndex );
		void dividePolys();
		CVector3 *divideSegment( CVector3 *v1, CVector3 *v2 );
		void bulgePoints( CVector3 *n1, CVector3 *n2, CVector3 *n3 );
		void calculateNormals();
		void calculateLight();
		void createFloorTexture( Shapes *shapes, int stencilIndex );
		void createLavaTexture( int index, int stencilIndex, int rot );

		GLuint floorTex[Shapes::STENCIL_COUNT];
		TextureData floorData[Shapes::STENCIL_COUNT];
		GLCaveShape* shapeList[CAVE_INDEX_COUNT];
		std::vector<CVector3*> points;
		std::vector<std::vector<CaveFace*>*> polys;
	};
	static Common our; 
	DECLARE_NOISY_OPENGL_SUPPORT();
};

#endif

