
/***************************************************************************
                   gllocator.cpp  -  Some unused 3D shape
                             -------------------
    begin                : Sat Sep 20 2003
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

#include "../common/constants.h"
#include "gllocator.h"

GLLocator::GLLocator( Texture texture[],
                      int width, int depth, int height,
                      char *name, int descriptionGroup,
                      Uint32 color, Uint8 shapePalIndex ) :
		GLShape( texture, width, depth, height, name, descriptionGroup, color, shapePalIndex ) {
}

GLLocator::~GLLocator() {
}

void GLLocator::draw() {
	float w = static_cast<float>( width ) * MUL;
	float d = static_cast<float>( depth ) * MUL;
	float h = 0.26f * MUL;

	glsDisable( GLS_TEXTURE_2D | GLS_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );

	glColor4f( 0.2f, 0.5f, 0.1f, 0.7f );

	glBegin( GL_TRIANGLE_STRIP );
	glVertex3f( 0, 0, h );
	glVertex3f( w, 0, h );
	glVertex3f( 0, d, h );
	glVertex3f( w, d, h );
	glEnd();

	glColor4f( 1.0f, 1.0f, 0.1f, 1.0f );

	glBegin( GL_LINES );
	glVertex3f( w, d, h );
	glVertex3f( 0, d, h );

	glVertex3f( 0, d, h );
	glVertex3f( 0, 0, h );

	glVertex3f( 0, 0, h );
	glVertex3f( w, 0, h );

	glVertex3f( w, 0, h );
	glVertex3f( w, d, h );
	glEnd();

	glsEnable( GLS_TEXTURE_2D );
}

