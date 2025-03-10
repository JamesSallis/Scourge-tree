/***************************************************************************
                   file.cpp  -  Platform agnostic file I/O
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

        
File::File( FILE *fp ) {
  this->fp = fp;
  this->rwops = SDL_RWFromFP( fp, 1 );
}

File::~File() {
  //cerr << "in ~File()" << endl;
  close();
}

int File::write( Uint32 *n, int count ) {
  // always save as big endian
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      tmp32[ i ] = SDL_Swap32( *( n + i ) );
    }
    return write( tmp32, sizeof( Uint32 ), count );
  } else {
    return write( n, sizeof( Uint32 ), count );
  }
}

int File::write( Uint16 *n, int count ) {
  // always save as big endian
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      tmp16 [ i ] = SDL_Swap16( *( n + i ) );
    }
    return write( tmp16, sizeof( Uint16 ), count );
  } else {
    return write( n, sizeof( Uint16 ), count );
  }
}

int File::write( Uint8 *n, int count ) {
  return write( n, sizeof( Uint8 ), count );
}


int File::read( Uint32 *n, int count ) {
  int ret = read( n, sizeof( Uint32 ), count );
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      *( n + i ) = SDL_Swap32( *( n + i ) );
    }
  }
  return ret;
}

int File::read( Uint16 *n, int count ) {
  int ret = read( n, sizeof( Uint16 ), count );
  if( SDL_BYTEORDER	!= SCOURGE_BYTE_ORDER ) {
    for( int i = 0; i < count; i++ ) {
      *( n + i ) = SDL_Swap16( *( n + i ) );
    }
  }
  return ret;
}

int File::read( Uint8 *n, int count ) {
  return read( n, sizeof( Uint8 ), count );
}

void File::close() {
  //cerr << "File::close()" << endl;
  // this closes fp too because rwops was created with 'autoclose'.
  SDL_RWclose( rwops );
}

int File::write( void *buff, size_t size, int count ) {
  return SDL_RWwrite( rwops, buff, size, count );
}

int File::read( void *buff, size_t size, int count ) {
  return SDL_RWread( rwops, buff, size, count );
}

