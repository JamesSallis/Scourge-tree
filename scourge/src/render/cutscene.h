/***************************************************************************
                   cutscene.h  -  Utilities for movie mode
                             -------------------
    begin                : Tue May 13 2008
    copyright            : (C) 2008 by Dennis Murczak
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

#ifndef CUTSCENE_H
#define CUTSCENE_H
#pragma once

#include "../session.h"

class Session;

/// A "movie" object that controls camera movement, widescreen effect etc.

class Cutscene {
private:
	Session *session;

	Uint32 cameraStartTime, cameraDuration;
	Uint32 letterboxStartTime, letterboxEndTime;

	float originalX, fromX, toX;
	float originalY, fromY, toY;
	float originalZ, fromZ, toZ;

	float originalXRot, fromXRot, toXRot;
	float originalYRot, fromYRot, toYRot;
	float originalZRot, fromZRot, toZRot;

	float originalZoom, fromZoom, toZoom;

	bool cameraMoving;

	bool inMovieMode;
	bool endingMovie;

	int letterboxHeight;

	void startLetterbox();
	void endLetterbox();

public:
	Cutscene( Session *session );
	~Cutscene();

	void startMovieMode();
	void endMovieMode();

	void placeCamera( float x, float y, float z, float xRot, float yRot, float zRot, float zoom );
	void animateCamera( float targetX, float targetY, float targetZ, float targetXRot, float targetYRot, float targetZRot, float targetZoom, Uint32 duration );
	void updateCameraPosition();

	bool isInMovieMode();
	bool isCameraMoving();

	float getCameraX();
	float getCameraY();
	float getCameraZ();
	float getCameraXRot();
	float getCameraYRot();
	float getCameraZRot();
	float getCameraZoom();

	/// Maximum height of the black bars.
	inline int getLetterboxHeight() {
		return letterboxHeight;
	};
	int getCurrentLetterboxHeight();

	void drawLetterbox();
	DECLARE_NOISY_OPENGL_SUPPORT();
};

#endif
