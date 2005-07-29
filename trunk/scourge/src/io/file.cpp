/***************************************************************************
                          file.cpp  -  description
                             -------------------
    begin                : Sat Jul 29, 2005
    copyright            : (C) 2005 by Gabor Torok
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

#include "file.h"

#define MAX_BUFF_SIZE 1024
        
File::File( FILE *fp ) {
  this->fp = fp;
  this->rwops = SDL_RWFromFP( fp, 1 );
  tmp32 = (Uint32*)malloc( MAX_BUFF_SIZE * sizeof( Uint32 ) );
  tmp16 = (Uint16*)malloc( MAX_BUFF_SIZE * sizeof( Uint16 ) );
}

File::~File() {
  close();
  free( tmp32 );
  free( tmp16 );
}

int File::write( Uint32 *n, int count ) {
  // always save as big endian
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      *( tmp32 + i ) = SDL_Swap32( *( n + i ) );
    }
    int ret = SDL_RWwrite( rwops, tmp32, sizeof( Uint32 ), count );
    return ret;
  } else {
    return SDL_RWwrite( rwops, n, sizeof( Uint32 ), count );
  }
}

int File::write( Uint16 *n, int count ) {
  // always save as big endian
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      *( tmp16 + i ) = SDL_Swap16( *( n + i ) );
    }
    int ret = SDL_RWwrite( rwops, tmp16, sizeof( Uint16 ), count );
    return ret;
  } else {
    return SDL_RWwrite( rwops, n, sizeof( Uint16 ), count );
  }
}

int File::write( Uint8 *n, int count ) {
  return SDL_RWwrite( rwops, n, sizeof( Uint8 ), count );
}


int File::read( Uint32 *n, int count ) {
  int ret = SDL_RWread( rwops, n, sizeof( Uint32 ), count );
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      *( n + i ) = SDL_Swap32( *( n + i ) );
    }
  }
  return ret;
}

int File::read( Uint16 *n, int count ) {
  int ret = SDL_RWread( rwops, n, sizeof( Uint16 ), count );
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      *( n + i ) = SDL_Swap16( *( n + i ) );
    }
  }
  return ret;
}

int File::read( Uint8 *n, int count ) {
  return SDL_RWread( rwops, n, sizeof( Uint8 ), count );
}

void File::close() {
  // this closes fp too because rwops was created with 'autoclose'.
  SDL_RWclose( rwops );
}

