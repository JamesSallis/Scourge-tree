/***************************************************************************
                          projectilerenderer.cpp  -  description
                             -------------------
    begin                : Sat May 3 2003
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

#include "projectilerenderer.h"
#include "map.h"
#include "renderedprojectile.h"
#include "render.h"

using namespace std;

void ShapeProjectileRenderer::drawPath( Map *map, 
																				RenderedProjectile *proj, 
																				std::vector<CVector3> *path ) {
	// draw the last step only
	CVector3 last = path->back();

	glPushMatrix();
	shape->setupToDraw();
	glTranslatef( last.x, last.y, last.z + ( 7.0f / DIV ) );
	glColor4f(1, 1, 1, 0.9f);
	glDisable( GL_CULL_FACE );
	((GLShape*)shape)->setCameraRot( map->getXRot(), 
																	 map->getYRot(),
																	 map->getZRot() );
	((GLShape*)shape)->setCameraPos( map->getXPos(), 
																	 map->getYPos(), 
																	 map->getZPos(), 
																	 last.x, last.y, last.z );

	// orient and draw the projectile
	float f = proj->getAngle() + 90;
	if(f < 0) f += 360;
	if(f >= 360) f -= 360;
	glRotatef( f, 0, 0, 1 );
	
	// for projectiles, set the correct camera angle
	if( proj->getAngle() < 90 ) {
		((GLShape*)shape)->setCameraRot( map->getXRot(), 
																		 map->getYRot(),
																		 map->getZRot() + proj->getAngle() + 90 );		
	} else if( proj->getAngle() < 180) {
		((GLShape*)shape)->setCameraRot( map->getXRot(), 
																		 map->getYRot(),
																		 map->getZRot() - proj->getAngle() );		
	}

	if( shape->drawLater() ) {
		glEnable( GL_BLEND );
		glDepthMask( GL_FALSE );
		shape->setupBlending();
	}
	shape->draw();
	if( shape->drawLater() ) {
		shape->endBlending();
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
	glPopMatrix();
}

EffectProjectileRenderer::EffectProjectileRenderer( Map *map, Preferences *prefs, Shapes *shapes, int effectType, int timeToLive ) {
  this->effectType = effectType;
  this->timeToLive = timeToLive;
	this->effects = (Effect**)malloc( MAX_EFFECT_COUNT * sizeof( Effect* ) );
	for( int i = 0; i < MAX_EFFECT_COUNT; i++ ) {
		effects[i] = new Effect( map, prefs, shapes, 2, 2 );
	}
}

EffectProjectileRenderer::~EffectProjectileRenderer() {
	for( int i = 0; i < MAX_EFFECT_COUNT; i++ ) {
		delete effects[i];
	}
	free( effects );
}

void EffectProjectileRenderer::draw() {
}

void EffectProjectileRenderer::setCameraRot( float x, float y, float z ) {
}

bool EffectProjectileRenderer::drawLater() {
  return true;
}

void EffectProjectileRenderer::setupBlending() {
  //glEnable( GL_BLEND );
  //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE );
}

void EffectProjectileRenderer::endBlending() {
  glDisable( GL_BLEND );
}

float EffectProjectileRenderer::getZ() { 
  return 0; 
}

int EffectProjectileRenderer::getStepsDrawn() { 
  return 3; 
}

int EffectProjectileRenderer::getTimeToLiveAfterImpact() { 
  return timeToLive; 
}

bool EffectProjectileRenderer::engulfTarget() { 
  return true; 
}

int EffectProjectileRenderer::getStepInc() { 
  return 2; 
}

bool EffectProjectileRenderer::needsRotation() {
  return false;
}

void EffectProjectileRenderer::drawPath( Map *map, 
																				 RenderedProjectile *proj, 
																				 std::vector<CVector3> *path ) {
	for( int i = 0; i < (int)path->size() && i < MAX_EFFECT_COUNT; i++ ) {
		CVector3 v = (*path)[i];
		glPushMatrix();
		glTranslatef( v.x, v.y, v.z + ( 1.0f / DIV ) );
		glColor4f( 1, 1, 1, 0.9f );
		glDisable( GL_CULL_FACE );
		glEnable( GL_BLEND );
		glDepthMask( GL_FALSE );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );

		effects[i]->draw( effectType, 0, ( (float)i / (float)( path->size() ) ) );
		
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glPopMatrix();
	}
}
