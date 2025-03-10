/***************************************************************************
                    date.h  -  Manages ingame date/time
                             -------------------
    begin                : Wed April 7 2004
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

#ifndef DATE_H
#define DATE_H
#pragma once

//uncomment to debug
//#define DEBUG_DATE


/**
  *@author Daroth-U
  */


/// The "ingame time" object.
class Date {
private:

	int sec;      // 0 to 59
	int min;      // 0 to 59
	int hour;     // 0 to 23
	int day;      // 1 to 31 or 30 or 29
	int month;    // 1 to 12
	int year;     // -32000 to +32000

	enum { SHORT_SIZE = 30, DATE_SIZE = 100 };
	char dateString[ DATE_SIZE ];
	void buildDateString();
	char shortString[ SHORT_SIZE ];

public:

	static int dayInMonth[13];
	static const char * monthName[13];
	static const char * dayName[8];

	void addDate( Date d );
	void setDate( int s, int m, int h, int day, int month, int year );
	void setDate( char *shortString );

	void addSec( int n );
	void addMin( int n );
	void addHour( int n );
	void addDay( int n );
	void addMonth( int n );
	void addYear( int n );

	inline int getYear()        {
		return year;
	}
	inline int getMonth()       {
		return month;
	}
	inline int getDay()         {
		return day;
	}
	inline int getHour()        {
		return hour;
	}
	inline int getMin()         {
		return min;
	}
	inline int getSec()         {
		return sec;
	}
	/// Returns date as a localized string.
	inline char * getDateString()   {
		buildDateString();return dateString;
	}
	void reset( char *shortString = NULL );

	/*bool operator==(const Date &d);
	bool operator<=(const Date &d);
	bool operator<(const Date &d);*/

	bool isInferiorTo( Date d );
	bool operator==( const Date& d ) const;
	void print();
	bool isADayLater( Date date );
	bool isAnHourLater( Date date );
	char *getShortString();

	Date();
	Date( int sec, int min, int hour, int day, int month, int year );
	Date( char *shortString );
	~Date();

protected:


};

#endif
