/***************************************************************************
                          protocol.h  -  description
                             -------------------
    begin                : Sun Sep 28 2003
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

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../constants.h"
#include "../scourge.h"

#ifdef HAVE_SDL_NET
#include <SDL_net.h>
#include <SDL_thread.h>
#include "client.h"
#include "server.h"
#endif

/**
 *@author Gabor Torok
 */
class Protocol {
private:
  Scourge *scourge;

#ifdef HAVE_SDL_NET

  static const int DEFAULT_SERVER_PORT = 6543;
  Server *server;
  Client *client;
#endif

 public:
   static const char *localhost;
   static const char *adminUserName;

  Protocol(Scourge *scourge);
  ~Protocol(); 
#ifdef HAVE_SDL_NET
  int startServer(int port=DEFAULT_SERVER_PORT);
  void stopServer();
  Uint32 login(char *server, int port, char *name);
  void logout();
  void sendChat(char *message);

#endif
};

#endif
