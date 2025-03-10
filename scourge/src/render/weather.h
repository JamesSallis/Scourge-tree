/***************************************************************************
                        weather.h  -  Weather engine
                             -------------------
    begin                : Wed Apr 29 2009
    copyright            : (C) 2009 by Dennis Murczak
    email                : dmurczak@googlemail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WEATHER_H
#define WEATHER_H
#pragma once

#include "../session.h"

class Session;

/// A "weather" object that manages and controls the weather in the outdoors.

class Weather {
private:
	Session *session;

	float rainByHours[24][CLIMATE_COUNT];
	float rainByMonths[12][CLIMATE_COUNT];

	float snowByHours[24][CLIMATE_COUNT];
	float snowByMonths[12][CLIMATE_COUNT];

	float thunderByHours[24][CLIMATE_COUNT];
	float thunderByMonths[12][CLIMATE_COUNT];

	float fogByHours[24][CLIMATE_COUNT];
	float fogByMonths[12][CLIMATE_COUNT];

	int oldWeather;
	int currentWeather;

#define RAIN_DROP_COUNT 300
#define RAIN_DROP_SIZE 32
	float rainDropX[RAIN_DROP_COUNT];
	float rainDropY[RAIN_DROP_COUNT];
	float rainDropZ[RAIN_DROP_COUNT];
#define SNOW_FLAKE_COUNT 120
#define SNOW_FLAKE_SIZE 16
	float snowFlakeX[SNOW_FLAKE_COUNT];
	float snowFlakeY[SNOW_FLAKE_COUNT];
	float snowFlakeZ[SNOW_FLAKE_COUNT];
	float snowFlakeXSpeed[SNOW_FLAKE_COUNT];
#define CLOUD_COUNT 15
	float cloudX[CLOUD_COUNT];
	float cloudY[CLOUD_COUNT];
	float cloudSize[CLOUD_COUNT];
	int cloudSpeed[CLOUD_COUNT];

	Uint32 lastWeatherChange;
	Uint32 lastWeatherRoll;
	Uint32 lastWeatherUpdate;
	Uint32 lastLightning;
	Uint32 lastLightningRoll;
	float lightningBrightness;
	bool thunderOnce;

public:
	Weather( Session *session );
	~Weather();

	void drawWeather();

	/// Initiate a fluid weather change.
	void changeWeather( int newWeather );
	/// Change weather instantly without a transition.
	inline void setWeather( int i ) { currentWeather = i; }

	inline int getOldWeather() { return oldWeather; }
	inline int getCurrentWeather() { return currentWeather; }
	/// Returns whether the weather is currently changing.
	inline bool isWeatherChanging() { return ( ( SDL_GetTicks() - lastWeatherChange ) > WEATHER_CHANGE_DURATION ); }

	void thunder();

	/// Generate weather for the given climate zone.
	int generateWeather( int climate = CLIMATE_INDEX_TEMPERATE );
	void generateRain();
	void generateSnow();
	void generateClouds();

	DECLARE_NOISY_OPENGL_SUPPORT();
};

#endif
