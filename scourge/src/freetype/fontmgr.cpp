/***************************************************************************
             fontmgr.cpp  -  Freetype font rendering
               -------------------
  begin                : Tue Jan 23 2007
  copyright            : (C) 2003 by Gabor Torok
  email                : cctorok@yahoo.com

  The original code for this class was written by Bob Pendleton and is
  covered under the LGPL. Thanks Bob!

  http://www.gamedev.net/community/forums/topic.asp?topic_id=284259

  Parts of this code were taken from the SDL_ttf source (under GPL):
  LATIN1_to_UNICODE()
  UTF8_to_UNICODE()

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
#include "fontmgr.h"

// ###### MS Visual C++ specific ###### 
#if defined(_MSC_VER) && defined(_DEBUG)
# define new DEBUG_NEW
# undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif 

using namespace std;

int FontMgr::initCounter = 0;

FontMgr::FontMgr( TTF_Font *font, int shadowX, int shadowY ) :
		ttfFont( font ), shadowX( shadowX ), shadowY( shadowY ) {
	initCounter++;
	foreground.r = 255;
	foreground.g  = 255;
	foreground.b = 255;
	foreground.unused = 0;

	shadowColor.r = ( Uint8 )10;
	shadowColor.g = ( Uint8 )10;
	shadowColor.b = ( Uint8 )10;
	shadowColor.unused = 0;

	height = TTF_FontHeight( ttfFont );
	ascent = TTF_FontAscent( ttfFont );
	descent = TTF_FontDescent( ttfFont );
	lineSkip = TTF_FontLineSkip( ttfFont );
}

FontMgr::~FontMgr() {

	for ( map<Uint16, GlyphInfo*>::iterator i = glyphs.begin(); i != glyphs.end(); ++i ) {
		delete i->second;
	}
	glyphs.clear();

	initCounter--;
	if ( 0 == initCounter ) {
		TTF_Quit();
	}
}

void FontMgr::textSizeUTF8( char *text, SDL_Rect *r ) {
	unicodeBuffer[0] = UNICODE_BOM_NATIVE;
	UTF8_to_UNICODE( unicodeBuffer, text, strlen( text ) );
	textSizeUNICODE( unicodeBuffer, r );
}

void FontMgr::textSize( char *text, SDL_Rect *r ) {
	unicodeBuffer[0] = UNICODE_BOM_NATIVE;
	LATIN1_to_UNICODE( unicodeBuffer, text, strlen( text ) );
	textSizeUNICODE( unicodeBuffer, r );
}

void FontMgr::textSizeUNICODE( Uint16 *p, SDL_Rect *r ) {

	int maxx = 0;
	int advance = 0;

	r->x = 0;
	r->y = 0;
	r->w = 0;
	r->h = height;

	while ( 0 != *p ) {
		GlyphInfo *g = loadChar( *p );
		if ( g ) {
			maxx = g->maxx;
			advance = g->advance;
			r->w += advance;
		}

		p++;
	}

	r->w = r->w - advance + maxx;
}


void FontMgr::drawTextUTF8( char *text, int x, int y ) {
	glsEnable( GLS_BLEND | GLS_TEXTURE_2D | GLS_ALPHA_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	// NOTE: important: GL_REPLACE causes colors to not render correctly. The madness!
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	unicodeBuffer[0] = UNICODE_BOM_NATIVE;
	UTF8_to_UNICODE( unicodeBuffer, text, strlen( text ) );
	drawUNICODE( unicodeBuffer, x, y );

	glsDisable( GLS_BLEND | GLS_ALPHA_TEST );
}

void FontMgr::drawText( char *text, int x, int y ) {
	glsEnable( GLS_BLEND | GLS_TEXTURE_2D | GLS_ALPHA_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	unicodeBuffer[0] = UNICODE_BOM_NATIVE;
	LATIN1_to_UNICODE( unicodeBuffer, text, strlen( text ) );
	drawUNICODE( unicodeBuffer, x, y );

	glsDisable( GLS_BLEND | GLS_ALPHA_TEST );
}

void FontMgr::drawUNICODE( Uint16 *p, int x, int y ) {
	GLfloat left, right;
	GLfloat top, bottom;
	GLfloat texMinX, texMinY;
	GLfloat texMaxX, texMaxY;

	while ( 0 != *p ) {
		GlyphInfo *g = loadChar( *p );
		if ( g ) {

			texMinX = g->texMinX;
			texMinY = g->texMinY;
			texMaxX = g->texMaxX;
			texMaxY = g->texMaxY;

			left   = x;
			right  = x + g->w;
			top    = y;
			bottom = y + g->h;

			glBindTexture( GL_TEXTURE_2D, g->tex );
			glBegin( GL_TRIANGLE_STRIP );
			glTexCoord2f( texMinX, texMinY ); glVertex2i( left,    top );
			glTexCoord2f( texMaxX, texMinY ); glVertex2i( right,    top );
			glTexCoord2f( texMinX, texMaxY ); glVertex2i( left, bottom );
			glTexCoord2f( texMaxX, texMaxY ); glVertex2i( right, bottom );
			glEnd();

			x += g->advance;
		}

		p++;
	}
}

// taken from SDL_ttf
Uint16 *FontMgr::LATIN1_to_UNICODE( Uint16 *unicode, const char *text, int len ) {
	int i;

	for ( i = 0; i < len; ++i ) {
		unicode[i] = ( ( const unsigned char * )text )[i];
	}
	unicode[i] = 0;

	return unicode;
}

// taken from SDL_ttf
Uint16 *FontMgr::UTF8_to_UNICODE( Uint16 *unicode, const char *utf8, int len ) {
	int i, j;
	Uint16 ch;

	for ( i = 0, j = 0; i < len; ++i, ++j ) {
		ch = ( ( const unsigned char * )utf8 )[i];
		if ( ch >= 0xF0 ) {
			ch  =  ( Uint16 )( utf8[i] & 0x07 ) << 18;
			ch |=  ( Uint16 )( utf8[++i] & 0x3F ) << 12;
			ch |=  ( Uint16 )( utf8[++i] & 0x3F ) << 6;
			ch |=  ( Uint16 )( utf8[++i] & 0x3F );
		} else
			if ( ch >= 0xE0 ) {
				ch  =  ( Uint16 )( utf8[i] & 0x0F ) << 12;
				ch |=  ( Uint16 )( utf8[++i] & 0x3F ) << 6;
				ch |=  ( Uint16 )( utf8[++i] & 0x3F );
			} else
				if ( ch >= 0xC0 ) {
					ch  =  ( Uint16 )( utf8[i] & 0x1F ) << 6;
					ch |=  ( Uint16 )( utf8[++i] & 0x3F );
				}
		unicode[j] = ch;
	}
	unicode[j] = 0;

	return unicode;
}

static int power_of_two( int input ) {
	int value = 1;

	while ( value < input ) {
		value <<= 1;
	}
	return value;
}

SDL_Color background = { 0, 0, 0, 0 };

GlyphInfo *FontMgr::loadChar( Uint16 c ) {
	if ( glyphs.find( c ) == glyphs.end() ) {
		GLfloat texcoord[4];
		Uint16 letter[2] = {0, 0};
		letter[0] = c;

		GlyphInfo *g = new GlyphInfo();
		TTF_GlyphMetrics( ttfFont,
		                  ( Uint16 )c,
		                  &( g->minx ),
		                  &( g->maxx ),
		                  &( g->miny ),
		                  &( g->maxy ),
		                  &( g->advance ) );

		SDL_Surface *shadow = TTF_RenderUNICODE_Shaded( ttfFont,
		                      letter,
		                      shadowColor,
		                      background );

		SDL_Surface *surface = TTF_RenderUNICODE_Shaded( ttfFont,
		                       letter,
		                       foreground,
		                       background );

		if ( surface ) {
			g->w = surface->w + shadowX;
			g->h = surface->h + shadowY;
			g->tex = loadTextureColorKey( surface, shadow, texcoord, 0, 0, 0 );
			g->texMinX = texcoord[ 0 ];
			g->texMinY = texcoord[ 1 ];
			g->texMaxX = texcoord[ 2 ];
			g->texMaxY = texcoord[ 3 ];
			glyphs[ c ] = g;
			// if( glyphs.size() % 10 == 0 ) cerr << " glyph count=" << glyphs.size() << endl;
			SDL_FreeSurface( surface );
			if ( shadow ) SDL_FreeSurface( shadow );
		} else {
			cerr << "*** Unable to render glyph: c=" << c << endl;
		}
	}
	return glyphs[ c ];
}



//  Create a texture from a surface. Set the alpha according
//  to the color key. Pixels that match the color key get an
//  alpha of zero while all other pixels get an alpha of
//  one.
GLuint FontMgr::loadTextureColorKey( SDL_Surface *surface,
                                     SDL_Surface *shadow,
                                     GLfloat *texcoord,
                                     int ckr,
                                     int ckg,
                                     int ckb ) {
	GLuint texture;
	int w, h;
	SDL_Surface *image;
	SDL_Rect area, area2;
	Uint32 colorkey;

	// Use the surface width and height expanded to powers of 2
	w = power_of_two( surface->w + shadowX );
	h = power_of_two( surface->h + shadowY );
	texcoord[0] = 0.0f;           // Min X
	texcoord[1] = 0.0f;           // Min Y
	texcoord[2] = ( GLfloat )( surface->w + shadowX ) / w; // Max X
	texcoord[3] = ( GLfloat )( surface->h + shadowY ) / h; // Max Y

	image = SDL_CreateRGBSurface(
	          SDL_SWSURFACE,
	          w, h,
	          32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN // OpenGL RGBA masks
	          0x000000FF,
	          0x0000FF00,
	          0x00FF0000,
	          0xFF000000
#else
	          0xFF000000,
	          0x00FF0000,
	          0x0000FF00,
	          0x000000FF
#endif
	        );
	if ( image == NULL ) {
		return 0;
	}

	// Set up so that colorkey pixels become transparent
	colorkey = SDL_MapRGBA( image->format, ckr, ckg, ckb, 0 );
	SDL_FillRect( image, NULL, colorkey );

	colorkey = SDL_MapRGBA( surface->format, ckr, ckg, ckb, 0 );
	SDL_SetColorKey( surface, SDL_SRCCOLORKEY, colorkey );
	SDL_SetColorKey( shadow, SDL_SRCCOLORKEY, colorkey );

	// Copy the surface into the GL texture image
	area.x = 0;
	area.y = 0;
	area.w = surface->w;
	area.h = surface->h;

	// render the shadow offset...
	// FIXME: shadow font is not perfect but close
	area2.x = shadowX;
	area2.y = shadowY;
	area2.w = surface->w;
	area2.h = surface->h;

	SDL_BlitSurface( shadow, &area, image, &area2 );
	SDL_BlitSurface( surface, &area, image, &area );

	// Create an OpenGL texture for the image
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );

	SDL_FreeSurface( image ); // No longer needed

	return texture;
}

GlyphInfo::GlyphInfo() :
		minx( 0 ), maxx( 0 ), miny( 0 ), maxy( 0 ), advance( 0 ),
		w( 0 ), h( 0 ),
		tex( 0 ), texMinX( 0 ), texMinY( 0 ), texMaxX( 0 ), texMaxY( 0 ) {
}

GlyphInfo::~GlyphInfo() {
}

