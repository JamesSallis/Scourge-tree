/***************************************************************************
             projectile.cpp  -  Class representing a projectile
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

#include "common/constants.h"
#include "projectile.h"
#include "render/renderlib.h"
#include "rpg/rpglib.h"
#include "creature.h"
#include "item.h"
#include "session.h"

using namespace std;

Uint32 Projectile::lastProjectileTick = 0;

#define DELTA 1.0f

// Uncomment line below for debug trace
//#define DEBUG_MOVEMENT 1

Projectile::Projectile( Creature *creature, Creature *target, Item *item, ProjectileRenderer *renderer, float parabolic, bool stopOnImpact, bool seeker ) {
	this->creature = creature;
	this->tx = target->getX();
	this->ty = target->getY();
	this->tw = target->getShape()->getWidth();
	this->td = target->getShape()->getDepth();
	this->target = target;
	this->item = item;
	this->spell = NULL;
	this->renderer = renderer;
	this->parabolic = parabolic;
	this->stopOnImpact = stopOnImpact;
	this->seeker = seeker;

	commonInit();
}

Projectile::Projectile( Creature *creature, Creature *target, Spell *spell, ProjectileRenderer *renderer,
                        float parabolic, bool stopOnImpact, bool seeker ) {
	this->creature = creature;
	this->tx = target->getX();
	this->ty = target->getY();
	this->tw = target->getShape()->getWidth();
	this->td = target->getShape()->getDepth();
	this->target = target;
	this->item = NULL;
	this->spell = spell;
	this->renderer = renderer;
	this->parabolic = parabolic;
	this->stopOnImpact = stopOnImpact;
	this->seeker = seeker;

	commonInit();
}

Projectile::Projectile( Creature *creature, int x, int y, int w, int d,
                        Spell *spell, ProjectileRenderer *renderer,
                        float parabolic, bool stopOnImpact ) {
	this->creature = creature;
	this->tx = x;
	this->ty = y;
	this->tw = w;
	this->td = d;
	this->target = NULL;
	this->item = NULL;
	this->spell = spell;
	this->renderer = renderer;
	this->parabolic = parabolic;
	this->stopOnImpact = stopOnImpact;
	this->seeker = false;

	commonInit();
}

void Projectile::commonInit() {
	this->casterLevel = creature->getLevel();
	this->reachedTarget = false;
	this->timeToLive = 0;
	this->sx.push_back( creature->getX() + ( creature->getShape()->getWidth() / 2 ) );
	this->sy.push_back( creature->getY() - ( creature->getShape()->getDepth() / 2 ) );

	calculateAngle( sx.back(), sy.back() );

	//  cerr << "NEW PROJECTILE: (" << sx << "," << sy << ")-(" << ex << "," << ey << ") angle=" << angle << " q=" << q << endl;
	cx = cy = 0;
	steps = 0;

	maxDist = ( spell ? spell->getDistance() : item->getRange() ) + tw;
	startX = sx.back();
	startY = sy.back();
	distToTarget = Constants::distance( startX,  startY,
	               1, 1,
	               tx, ty, tw, td );
}

Projectile::~Projectile() {
	renderer->removeProjectile( this );
	if ( !renderer->hasProjectiles() ) {
		//cerr << "Deleting renderer!" << endl;
		delete renderer;
	}
}

bool Projectile::atTargetLocation() {
	if ( reachedTarget ) return true;
	reachedTarget = ( fabs( ex - sx.back() ) <= 1.0f + DELTA &&
	                  fabs( ey - sy.back() ) <= 1.0f + DELTA );
	return reachedTarget;
}

void Projectile::debug() {
	cerr << "Projectile at: " << sx.back() << "," << sy.back() << " target: " << ex << "," << ey <<
	" at target? " << atTargetLocation() << endl;
}

bool Projectile::move() {
	// are we at the target location?
	// return false to let the map class handle the attack.
	if ( this->atTargetLocation() ) {
		// clamp to target
		sx.push_back( ex );
		sy.push_back( ey );
		return false;
	}

	// return true to let this class handle the attack
	if ( !reachedTarget && steps++ >= maxDist + 2 ) return true;

	float ssx = sx.back();
	float ssy = sy.back();

	float oldAngle = angle;
	if ( parabolic != 0.0f ) {
		float a = ( 179.0f * steps ) / distToTarget;
		angle = angle + parabolic * 40 * Constants::sinFromAngle( a );
		ssx += ( Constants::cosFromAngle( angle ) * DELTA );
		ssy += ( Constants::sinFromAngle( angle ) * DELTA );
		angle = oldAngle;
	} else {
		// angle-based floating pt. movement
		if ( ssx == ex ) {
			// vertical movement
			if ( ssy < ey ) ssy += DELTA;
			else ssy -= DELTA;
		} else if ( ssy == ey ) {
			// horizontal movement
			if ( ssx < ex ) ssx += DELTA;
			else ssx -= DELTA;
		} else {
			/*
			cerr << "before: " << sx << "," << sy <<
			  " angle=" << angle <<
			  " rad=" << Constants::toRadians(angle) <<
			  " cos=" << cos(Constants::toRadians(angle)) <<
			  " sin=" << sin(Constants::toRadians(angle)) << endl;
			  */
			ssx += ( Constants::cosFromAngle( angle ) * DELTA );
			ssy += ( Constants::sinFromAngle( angle ) * DELTA );
			//cerr << "after: " << sx << "," << sy << endl;
		}
	}

	// target-seeking missiles re-adjust their flight paths
	if ( seeker && target ) {
		this->tx = target->getX();
		this->ty = target->getY();
		this->tw = target->getShape()->getWidth();
		this->td = target->getShape()->getDepth();
		if ( !( toint( ssx ) == toint( ex ) && toint( ssy ) == toint( ey ) ) ) calculateAngle( ssx, ssy );
	}

	// sanity checking: are we off the map?
	if ( toint( ssx ) < 0 || toint( ssy ) < 0 ||
	        toint( ssx ) >= MAP_WIDTH ||
	        toint( ssy ) >= MAP_DEPTH ) {
		return true;
	}

	// recalculate the distance
	distToTarget = Constants::distance( startX,  startY,
	               1, 1,
	               tx, ty, tw, td );

	// store this position
	sx.push_back( ssx );
	sy.push_back( ssy );

	// we're not at the target yet
	return false;
}

void Projectile::calculateAngle( float sx, float sy ) {
	this->ex = tx + ( tw / 2 );
	this->ey = ty - ( td / 2 );

	int x = static_cast<int>( ex - sx );
	int y = static_cast<int>( ey - sy );
	if ( !x ) this->angle = ( y <= 0 ? ( 90.0f + 180.0f ) : 90.0f );
	else this->angle = Constants::toAngle( atan( static_cast<float>( y ) / static_cast<float>( x ) ) );
	//cerr << "x=" << x << " y=" << y << " angle=" << angle << endl;

	// read about the arctan problem:
	// http://hyperphysics.phy-astr.gsu.edu/hbase/ttrig.html#c3
	q = 1;
	if ( x < 0 ) {  // Quadrant 2 & 3
		q = ( y >= 0 ? 2 : 3 );
		angle += 180;
	} else if ( y < 0 ) { // Quadrant 4
		q = 4;
		angle += 360;
	}
}

// return null if the projectile cannot be launched
Projectile *Projectile::addProjectile( Creature *creature, Creature *target,
    Item *item, ProjectileRenderer *renderer,
    int maxProjectiles, bool stopOnImpact ) {
	Projectile *p = new Projectile( creature, target, item, renderer, 0.0f, stopOnImpact );
	renderer->addProjectile( p );
	RenderedProjectile::addProjectile( p );
	return p;
}

Projectile *Projectile::addProjectile( Creature *creature, Creature *target,
    Spell *spell, ProjectileRenderer *renderer,
    int maxProjectiles, bool stopOnImpact ) {
	Projectile *p = new Projectile( creature, target, spell, renderer, 0.0f, stopOnImpact, true );
	renderer->addProjectile( p );
	RenderedProjectile::addProjectile( p );

	// add extra projectiles w. parabolic curve
	float r = 0.5f;
	for ( int i = 0; i < maxProjectiles - 1; i += 2 ) {
		if ( i < maxProjectiles - 1 ) {
			Projectile *pp = new Projectile( creature, target, spell, renderer, r, stopOnImpact, true );
			renderer->addProjectile( pp );
			RenderedProjectile::addProjectile( pp );
		}
		if ( ( i + 1 ) < maxProjectiles - 1 ) {
			Projectile *pp = new Projectile( creature, target, spell, renderer, -r, stopOnImpact, true );
			renderer->addProjectile( pp );
			RenderedProjectile::addProjectile( pp );
		}
		r += ( r / 2.0f );
	}

	return p;
}

Projectile *Projectile::addProjectile( Creature *creature, int x, int y, int w, int d,
    Spell *spell, ProjectileRenderer *renderer,
    int maxProjectiles, bool stopOnImpact ) {
	// add a straight-flying projectile
	Projectile *p = new Projectile( creature, x, y, w, d, spell, renderer, 0.0f, stopOnImpact );
	renderer->addProjectile( p );
	RenderedProjectile::addProjectile( p );

	// add extra projectiles w. parabolic curve
	float r = 0.5f;
	for ( int i = 0; i < maxProjectiles - 1; i += 2 ) {
		if ( i < maxProjectiles - 1 ) {
			Projectile *pp = new Projectile( creature, x, y, w, d, spell, renderer, r, stopOnImpact );
			renderer->addProjectile( pp );
			RenderedProjectile::addProjectile( pp );
		}
		if ( ( i + 1 ) < maxProjectiles - 1 ) {
			Projectile *pp = new Projectile( creature, x, y, w, d, spell, renderer, -r, stopOnImpact );
			renderer->addProjectile( pp );
			RenderedProjectile::addProjectile( pp );
		}
		r += ( r / 2.0f );
	}

	return p;
}

void Projectile::moveProjectiles( Session *session ) {
	Uint32 t = SDL_GetTicks();
	if ( lastProjectileTick == 0 ||
	        t - lastProjectileTick > ( Uint32 )( session->getPreferences()->getGameSpeedTicks() / 50 ) ) {
		lastProjectileTick = t;

		if ( getProjectileMap()->empty() ) return;

		map<Projectile*, Creature*> battleProjectiles;

		vector<Projectile*> removedProjectiles;
#ifdef DEBUG_MOVEMENT
		cerr << "Projectiles:" << endl;
#endif
		for ( map<RenderedCreature *, vector<RenderedProjectile*>*>::iterator i = getProjectileMap()->begin();
		        i != getProjectileMap()->end();
		        ++i ) {
			vector<RenderedProjectile*> *p = i->second;
			for ( vector<RenderedProjectile*>::iterator e = p->begin(); e != p->end(); ++e ) {
				Projectile *proj = ( Projectile* )( *e );


				if ( proj->timeToLive > 0 ) {
					removedProjectiles.push_back( proj );
				} else {

#ifdef DEBUG_MOVEMENT
					cerr << "\t\tprojectile at: (" << proj->getCurrentX() << "," << proj->getCurrentY() <<
					") target: (" << proj->ex << "," << proj->ey << ")" <<
					" target creature: " << ( proj->target ? proj->target->getName() : "NULL" ) << endl;
#endif
					if ( proj->move() ) {
#ifdef DEBUG_MOVEMENT
						cerr << "PROJ: max steps, from=" << proj->getCreature()->getName() << endl;
#endif
						session->getGameAdapter()->writeLogMessage( _( "Projectile did not reach the target (max steps)." ) );
						removedProjectiles.push_back( proj );
					}

					// collision detection
					bool blocked = false;

					// proj reached the target
					if ( proj->reachedTarget ) {
						if ( proj->getSpell() &&
						        proj->getSpell()->isLocationTargetAllowed() ) {
#ifdef DEBUG_MOVEMENT
							cerr << "PROJ: reached location target, from=" << proj->getCreature()->getName() << endl;
#endif
							session->getGameAdapter()->fightProjectileHitTurn( proj, static_cast<int>( proj->getCurrentX() ), static_cast<int>( proj->getCurrentY() ) );
							blocked = true;
						} else if ( proj->target ) {

							bool b = SDLHandler::intersects( toint( proj->ex ), toint( proj->ey ),
							         toint( 1 + DELTA ), toint( 1 + DELTA ),
							         toint( proj->target->getX() ), toint( proj->target->getY() ),
							         proj->target->getShape()->getWidth(),
							         proj->target->getShape()->getHeight() );
#ifdef DEBUG_MOVEMENT
							cerr << "PROJ: checking intersection of " <<
							"proj: (" << toint( proj->ex ) << "," << toint( proj->ey ) << "," <<
							toint( 1 + DELTA ) << "," << toint( 1 + DELTA ) <<
							" and creature: " << toint( proj->target->getX() ) << "," << toint( proj->target->getY() ) <<
							"," << proj->target->getShape()->getWidth() << "," << proj->target->getShape()->getHeight() <<
							" intersects? " << b << endl;
#endif
							if ( b ) {
#ifdef DEBUG_MOVEMENT
								cerr << "PROJ: attacks target creature, from=" << proj->getCreature()->getName() << endl;
#endif
								battleProjectiles[ proj ] = proj->target;
							}
							blocked = true;
#ifdef DEBUG_MOVEMENT
						} else {
							cerr << "PROJ: target creature moved?" << endl;
#endif
						}
					}

					// proj stopped, due to something else
					if ( !blocked ) {

						Location *loc = session->getMap()->getLocation( toint( proj->getCurrentX() ),
						                toint( proj->getCurrentY() ),
						                0 );
						if ( loc ) {
							if ( loc->creature &&
							        proj->getCreature()->canAttack( loc->creature ) &&
							        proj->timeToLive == 0 ) {
#ifdef DEBUG_MOVEMENT
								cerr << "PROJ: attacks non-target creature: " << loc->creature->getName() << ", from=" << proj->getCreature()->getName() << endl;
#endif
								battleProjectiles[ proj ] = ( Creature* )( loc->creature );
								blocked = true;
							} else if ( proj->doesStopOnImpact() && ( ( loc->item && loc->item->getShape()->getHeight() >= 6 ) ||
							            ( !loc->creature && !loc->item && loc->shape && loc->shape->getHeight() >= 6 ) ) ) {
#ifdef DEBUG_MOVEMENT
								cerr << "PROJ: blocked by item or shape, from=" << proj->getCreature()->getName() << endl;
#endif
								// hit something
								session->getGameAdapter()->writeLogMessage( _( "Projectile did not reach the target (blocked)." ) );
								blocked = true;
							}
						}
					}

					// remove finished projectiles
					if ( blocked || proj->reachedTarget || proj->timeToLive > 0 ) {

#ifdef DEBUG_MOVEMENT
						// DEBUG INFO
						if ( !blocked ) {
							cerr << "*** Warning: projectile didn't hit target ***" << endl;
							cerr << "Creature: " << proj->getCreature()->getName() << endl;
							proj->debug();
						}
#endif

						removedProjectiles.push_back( proj );
					}
				}
			}
		}

		// fight battles
		for ( map<Projectile*, Creature*>::iterator i = battleProjectiles.begin(); i != battleProjectiles.end(); ++i ) {
			Projectile *proj = i->first;
			Creature *creature = i->second;
			session->getGameAdapter()->fightProjectileHitTurn( proj, creature );
		}

		// remove projectiles
		Uint32 now = SDL_GetTicks();
		for ( vector<Projectile*>::iterator e = removedProjectiles.begin(); e != removedProjectiles.end(); ++e ) {
			Projectile *proj = *e;
			if ( proj->timeToLive == 0 && proj->getRenderer()->getTimeToLiveAfterImpact() > 0 ) {
				proj->timeToLive = now;
			} else if ( proj->getRenderer()->getTimeToLiveAfterImpact() < static_cast<int>( now - proj->timeToLive ) ) {
				Projectile::removeProjectile( proj );
				// Is it ok to destroy proj. here?
				delete proj;
			}
		}
	}
}

