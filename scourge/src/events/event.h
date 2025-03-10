/***************************************************************************
                  event.h  -  Base class for ingame events
                             -------------------
    begin                : Thu Apr 8 2004
    copyright            : (C) 2004 by Daroth-U
    email                : daroth-u@ifrance.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef EVENT_H
#define EVENT_H
#pragma once

#include "../date.h"


/**
  *@author Daroth-U
  */

class Date;
class Creature;

/// A scheduled ingame event (hunger, thirst, potion expires etc.)
class Event  {

private:
	Date eventDate;
	Date timeOut;
	long nbExecutionsToDo;
	long nbExecutions;

	// For debug purpose
	static int globalId;
	int eventId;  // unique
	bool cancelEvent;

public:


	enum {
		INFINITE_EXECUTIONS = -1
	};

	// This event will occur nbExecutionsTD time every tmOut from currentDate.
	Event( Date currentDate, Date tmOut, long nbExecutionsToDo );

	// This event will occur only one time at the given date
	Event( Date eventDate );

	Event();
	virtual ~Event();

	//virtual void execute()=0;
	virtual void execute() {
		std::cout << "Event.cpp : execute function should'nt be called by event base class!" << std::endl;
	}

	// this is called before the event is deleted (It's only called once.)
	virtual void executeBeforeDelete() { }

	inline long getNbExecutionsToDo() {
		return nbExecutionsToDo;
	}
	inline long getNbExecutions()     {
		return nbExecutions;
	}
	inline int getEventId()           {
		return eventId;
	}
	inline void increaseNbExecutions() {
		nbExecutions ++ ;
	}
	inline void setNbExecutionsToDo( int nb ) {
		if ( nb >= -1 ) nbExecutionsToDo = nb; else nbExecutions = 0;
	}
	inline bool isCancelEventSet() {
		return cancelEvent;
	}


	Date getEventDate() {
		return eventDate;
	}
	Date getTimeOut()   {
		return timeOut;
	}
	void setEventDate( Date d ) {
		eventDate = d;
	}
	void scheduleDeleteEvent();

	// does this event reference this creature?
	virtual inline bool doesReferenceCreature( Creature *creature ) {
		return false;
	};

	virtual const char *getName() = 0;

	virtual Creature *getCreature() {
		return NULL;
	}


};

#endif
