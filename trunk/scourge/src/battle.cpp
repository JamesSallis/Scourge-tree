/***************************************************************************
                          battle.cpp  -  description
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

#include "battle.h"

//#define DEBUG_BATTLE
#define GOD_MODE 0
#define MONSTER_IMORTALITY 0

enum {
  NO_ACTION = 0,
  LOITER,
  ATTACK,
  MOVE,
  NO_TARGET,
  WAIT,
};

char *actionName[] = { "NO ACTION", "LOITER", "ATTACK", "MOVE", "NO_TARGET", "WAIT"};

Battle::Battle() {
  this->creature = NULL;
}

Battle::Battle(Scourge *scourge, Creature *creature) {
  this->scourge = scourge;
  this->creature = creature;
  this->item = NULL;
  this->creatureInitiative = 0;
  this->initiativeCheck = false;
  this->speed = 0;
  this->dist = creature->getDistanceToTarget();
  this->spell = NULL;
  this->empty = false;
}

Battle::~Battle() {
}

void Battle::setupBattles(Scourge *scourge, Battle *battle[], int count, vector<Battle *> *turns) {

  if(count == 0) return;

  bool battleStarted = false;
  int battleCount = count;  
  int initiative = -10;
//char message[200];
  int turn = 0;

#ifdef DEBUG_BATTLE
  cerr << "==================================================" << endl;
  cerr << "battleCount=" << battleCount << endl;
  for(int i = 0; i < battleCount; i++) {
    cerr << "\t(" << battle[i]->creature->getName() << 
    "->" << (battle[i]->creature->getTargetCreature() ? 
             battle[i]->creature->getTargetCreature()->getName() :
             "NULL") << 
    ")" << endl;
  }
#endif

// this is O(n^2) unfortunately... maybe we could use a linked list or something here
  while(battleCount > 0) {
    bool creatureFound = false; 

    for(int i = 0; i < battleCount; i++) {
      int action = NO_ACTION;

      battle[i]->empty = false;
      battle[i]->projectileHit = false;

      if(!battle[i]->creature->hasTarget()) {
        // remove creatures with no target creature from this round
        action = NO_TARGET;
      } else if(!battle[i]->creature->isTargetValid()) {
        // if someone already killed this target, skip it
        battle[i]->creature->cancelTarget();
        if(battle[i]->creature->isMonster()) {
          battle[i]->creature->setMotion(Constants::MOTION_LOITER);     
          action = LOITER;
        } else {
          // if party member re-join the battle
          Creature *c = battle[i]->creature;
          c->setTargetCreature(scourge->getClosestVisibleMonster(c->getX(), 
                                                                 c->getY(), 
                                                                 c->getShape()->getWidth(),
                                                                 c->getShape()->getDepth(),
                                                                 20));
        }
      } else {
        GLint t = SDL_GetTicks();

        // get the best weapon given the distance from the target
        battle[i]->initTurn();

        // check the creature's initiative
        if(!battle[i]->initiativeCheck || 
           battle[i]->creatureInitiative < initiative) {

          // remember that it passed (creature can attack next time if timing (below) is good)
          battle[i]->initiativeCheck = true;
          creatureFound = true;

          // this is to slow down battle, depending on your weapon
          // getLastTurn is 0 when an action is decided to be executed
          // this way it's two trips through this method forcing a spell (e.g.)
          // to use the clock.
          bool initialActionTurn = (battle[i]->creature->getLastTurn() == 0);
#ifdef DEBUG_BATTLE                                                                   
          cerr << "speed=" << battle[i]->speed << " last=" << battle[i]->creature->getLastTurn() << 
          " diff=" << (t - battle[i]->creature->getLastTurn()) << endl;
#endif                                                                                        
          if(initialActionTurn ||
             battle[i]->speed < t - battle[i]->creature->getLastTurn()) {

            // remember the last active turn			
            battle[i]->creature->setLastTurn(t);

            if(!battleStarted) {
              scourge->getMap()->addDescription("A round of battle begins...", 1, 1, 1);
              battleStarted = true;
            }

            if(initialActionTurn) {
              // add a waiting term
              battle[i]->empty = true;
              action = WAIT;
              turns->push_back(battle[i]);
            } else {
              // save this turn
              turns->push_back(battle[i]);
              action = ATTACK;
            }
          } else {
            // add a waiting term
            battle[i]->empty = true;
            action = WAIT;
            turns->push_back(battle[i]);
          }
        }
      }

      if(action != NO_ACTION) {

#ifdef DEBUG_BATTLE                 
        cerr << "Turn: " << turn << " " << actionName[action] << ", " <<
        battle[i]->creature->getName() << "(" << battle[i]->creatureInitiative << ")" <<
        "->" << (battle[i]->creature->getTargetCreature() ? 
                 battle[i]->creature->getTargetCreature()->getName() : 
                 "<no target>") <<
        endl;
#endif                                                    

        // remove this battle from the turn
        Battle *tmp = battle[i];
        for(int t = i; t < battleCount - 1; t++) {
          battle[t] = battle[t + 1];
        }
        // move current one to the end of the list so we don't loose it
        battle[battleCount - 1] = tmp;
        battleCount--;
        i--;
        turn++;
      }
    }

    // if there are no creatures for this initiative slot, go to the next one
    if(!creatureFound) initiative++;
  }
}                 

void Battle::fightTurn() {

  // waiting to cast a spell?
  if(creature->getAction() == Constants::ACTION_CAST_SPELL) {
    creature->startEffect(Constants::EFFECT_CAST_SPELL, 
                          Constants::DAMAGE_DURATION * 4);
  }

  // waiting to attack?
  if(isEmpty()) return;

  // target killed?
  if(!creature->hasTarget()) return;

  // if there was no 'item' selected it's because we're too far from the target
  // (getBestWeapon returned NULL) and we're not casting a spell, follow the target
  //
  // If it's an empty-handed attack, follow the target
  //
  // If it's a ranged attack and we're not in range, follow the target (will move away from target)
  //
  // This sure is confusing code...
  /*
  cerr << "projectileHit=" << projectileHit <<
        " dist=" << dist <<
        " creature->isInRange()=" << creature->isInRange() <<
        " item=" << item <<
        " creature->getActionSpell()=" << creature->getActionSpell() <<
        " spell=" << spell << endl;
        */
  if(!projectileHit &&
     ((dist > Constants::MIN_DISTANCE && !item && !creature->getActionSpell() && !spell) ||  
      !creature->isInRange())) { 
    //cerr << "\tfollowing..." << endl;
    creature->followTarget();
    return;
  }
  //cerr << "\tcontinuing" << endl;

  // handle action: eat/drink items
  if(creature->getAction() == Constants::ACTION_EAT_DRINK) {
    executeEatDrinkAction();
    return;
  }

  // the attacked target may get upset
  creature->makeTargetRetaliate();

  // handle the action
  if(!projectileHit && item && item->getRpgItem()->isRangedWeapon()) {
    launchProjectile();
  } else if(spell) {
    // a spell projectile hit
    SpellCaster *sc = new SpellCaster(this, spell, true); 
    sc->spellSucceeded();
    delete sc;
  } else if(creature->getActionSpell()) {
    // casting a spell for the first time
    castSpell();
  } else {
    hitWithItem();
  }
}

void Battle::castSpell() {
  //  cerr << "FIXME: implement Battle::castSpell()" << endl;
  //  cerr << "\tdynamic effects (flame, explode, etc.), saving throws, multiple targets, projectile hits, mana usage, etc." << endl;
  //  cerr << "\tcall Battle::dealDamage() for each target." << endl;

  sprintf(message, "%s casts %s!", 
          creature->getName(), 
          creature->getActionSpell()->getName());
  scourge->getMap()->addDescription(message, 1, 0.15f, 1);
  ((MD2Shape*)(creature->getShape()))->setAttackEffect(true);

  // use up some MP
  int n = creature->getMp() - creature->getActionSpell()->getMp();
  if(n < 0) n = 0;
  creature->setMp( n );

  // spell succeeds?
  // FIXME: use stats like IQ here to modify spell success rate...
  if(!projectileHit && 
     (int)(100.0f * rand() / RAND_MAX) < creature->getActionSpell()->getFailureRate()) {
    SpellCaster *sc = new SpellCaster(this, creature->getActionSpell(), false);
    sc->spellFailed();
    delete sc;
  } else {

    // get exp for casting the spell
    bool b = creature->getStateMod(Constants::leveled);
    if(!creature->getStateMod(Constants::dead)) {
      int n = creature->addExperience(creature->getActionSpell()->getExp());
      if(n > 0) {
        sprintf(message, "%s gains %d experience points.", creature->getName(), n);
        scourge->getMap()->addDescription(message);
        if(!b && creature->getStateMod(Constants::leveled)) {
          sprintf(message, "%s gains a level!", creature->getName());
          scourge->getMap()->addDescription(message, 1.0f, 0.5f, 0.5f);
        }
      }
    }

    SpellCaster *sc = new SpellCaster(this, creature->getActionSpell(), false);
    sc->spellSucceeded();
    delete sc;
  }

  // cancel action
  creature->cancelTarget();
}

void Battle::launchProjectile() {
  sprintf(message, "...%s shoots a projectile", creature->getName());
  scourge->getMap()->addDescription(message); 
  if(!Projectile::addProjectile(creature, creature->getTargetCreature(), item, 
                                scourge->getShapePalette()->findShapeByName("ARROW"),
                                creature->getMaxProjectileCount(item))) {
    // max number of projectiles in the air
    // FIXME: do something... 
    // (like print message: can't launch projectile due to use of fixed-sized array in code?)
  }
}

void Battle::projectileHitTurn(Scourge *scourge, Projectile *proj, Creature *target) {

  // FIXME: get rid of references to get/setTargetCreature in this method.
  // instead, use creature's target methods (or make new ones)

  // configure a turn
  Creature *oldTarget = proj->getCreature()->getTargetCreature();
  proj->getCreature()->setTargetCreature(target);
  Battle *battle = new Battle(scourge, proj->getCreature());
  battle->projectileHit = true;

  if(proj->getItem()) {
    battle->initItem(proj->getItem());
  } else if(proj->getSpell()) {
    battle->spell = proj->getSpell();
  }
  // play it
  battle->fightTurn();

  delete battle;
  proj->getCreature()->setTargetCreature(oldTarget);
}

void Battle::hitWithItem() {

  if(item) {
    sprintf(message, "%s attacks %s with %s! (I:%d,S:%d)", 
            creature->getName(), 
            creature->getTargetCreature()->getName(),
            item->getRpgItem()->getName(),
            creatureInitiative, speed);
    scourge->getMap()->addDescription(message);
    ((MD2Shape*)(creature->getShape()))->setAttackEffect(true);
  } else if(dist <= Constants::MIN_DISTANCE) {
    sprintf(message, "%s attacks %s with bare hands! (I:%d,S:%d)", 
            creature->getName(), 
            creature->getTargetCreature()->getName(),
            creatureInitiative, speed);
    scourge->getMap()->addDescription(message);
    ((MD2Shape*)(creature->getShape()))->setAttackEffect(true);
  }

  // take a swing
  int tohit = creature->getToHit(item);
  int ac = creature->getTargetCreature()->getSkillModifiedArmor();
  sprintf(message, "...%s defends with armor=%d", creature->getTargetCreature()->getName(), ac);
  scourge->getMap()->addDescription(message);
  sprintf(message, "...toHit=%d vs. AC=%d", tohit, ac);
  scourge->getMap()->addDescription(message);
  if(tohit > ac) {
    // deal out the damage
    dealDamage(creature->getDamage(item));
  } else {
    // missed
    sprintf(message, "...and misses! (toHit=%d vs. AC=%d)", tohit, ac);
    scourge->getMap()->addDescription(message);
  }
}

void Battle::dealDamage(int damage, int effect) {
  if(damage) {  
    sprintf(message, "...and hits! for %d points of damage", damage);
    scourge->getMap()->addDescription(message, 1.0f, 0.5f, 0.5f);

    // target creature death
    if(creature->getTargetCreature()->takeDamage(damage, effect)) {         
      creature->getShape()->setCurrentAnimation((int)MD2_TAUNT);  
      sprintf(message, "...%s is killed!", creature->getTargetCreature()->getName());
      scourge->getMap()->addDescription(message, 1.0f, 0.5f, 0.5f);

      if((creature->getTargetCreature()->isMonster() && !MONSTER_IMORTALITY) || 
         !GOD_MODE)
        scourge->creatureDeath(creature->getTargetCreature());

      // add exp. points and money
      if(!creature->isMonster()) {


        // FIXME: try to move to party.cpp
        for(int i = 0; i < scourge->getParty()->getPartySize(); i++) {
          bool b = scourge->getParty()->getParty(i)->getStateMod(Constants::leveled);
          if(!scourge->getParty()->getParty(i)->getStateMod(Constants::dead)) {
            int n = scourge->getParty()->getParty(i)->addExperience(creature->getTargetCreature());
            if(n > 0) {
              sprintf(message, "%s gains %d experience points.", scourge->getParty()->getParty(i)->getName(), n);
              scourge->getMap()->addDescription(message);
              if(!b && scourge->getParty()->getParty(i)->getStateMod(Constants::leveled)) {
                sprintf(message, "%s gains a level!", scourge->getParty()->getParty(i)->getName());
                scourge->getMap()->addDescription(message, 1.0f, 0.5f, 0.5f);
              }
            }

            n = scourge->getParty()->getParty(i)->addMoney(creature->getTargetCreature());
            if(n > 0) {
              sprintf(message, "%s finds %d coins!", scourge->getParty()->getParty(i)->getName(), n);
              scourge->getMap()->addDescription(message);
            }
          }
        }
        // end of FIXME

        // see if this is a mission objective
        if(scourge->getCurrentMission() && 
           creature->getTargetCreature()->getMonster() &&
           scourge->getCurrentMission()->monsterSlain(creature->getTargetCreature()->getMonster())) {
          scourge->missionCompleted();
        }
      }
    }
  } else {
    sprintf(message, "...and hits! but causes no damage");
    scourge->getMap()->addDescription(message);
  }
}

void Battle::initTurn() {
  float range = 0.0f;

  // select a weapon
  if(creature->getAction() == Constants::ACTION_NO_ACTION) {

    // already selected weapon?
    if(item) return;
    Item *i = creature->getBestWeapon(dist);  
    if(i) range = i->getRpgItem()->getDistance();
    // set up distance range for ranged weapons (do it here so it only happens when the action changes)
    creature->setDistanceRange(0, Constants::MIN_DISTANCE);
    if(range >= 8) {
      creature->setDistanceRange(range * 0.5f, range);
    }
    initItem(i);
  } else {
    // or init action
    switch(creature->getAction()) {
    case Constants::ACTION_CAST_SPELL:
      range = creature->getActionSpell()->getDistance();
      speed = creature->getActionSpell()->getSpeed() * 
              (scourge->getUserConfiguration()->getGameSpeedTicks() + 80);
      creatureInitiative = creature->getInitiative(NULL, creature->getActionSpell());
      // set up distance range for ranged weapons (do it here so it only happens when the action changes)
      creature->setDistanceRange(0, Constants::MIN_DISTANCE);
      if(range >= 8) {
        creature->setDistanceRange(range * 0.5f, range);
      }
      break;
    case Constants::ACTION_EAT_DRINK:
      range = creature->getActionItem()->getRpgItem()->getDistance();
      speed = creature->getActionItem()->getRpgItem()->getSpeed() *
              (scourge->getUserConfiguration()->getGameSpeedTicks() + 80);
      creatureInitiative = creature->getInitiative(creature->getActionItem(), NULL);
      // set up distance range for ranged weapons (do it here so it only happens when the action changes)
      creature->setDistanceRange(0, Constants::MIN_DISTANCE);
      if(range >= 8) {
        creature->setDistanceRange(range * 0.5f, range);
      }
      break;
    default:
      cerr << "*** Error: unhandled action: " << creature->getAction() << endl;
    }
  }  
}

void Battle::initItem(Item *item) {
  this->item = item;

  // (!item) is a bare-hands attack		
  speed = (item ? item->getRpgItem()->getSpeed() : Constants::HAND_WEAPON_SPEED) * 
          (scourge->getUserConfiguration()->getGameSpeedTicks() + 80);
  //	(scourge->getUserConfiguration()->getGameSpeedTicks() + 80);

  creatureInitiative = creature->getInitiative(item);
}

void Battle::executeEatDrinkAction() {
  // is it still in the inventory?
  int index = creature->findInInventory(creature->getActionItem());
  if(index > -1) {
    if(creature->eatDrink(creature->getActionItem())){
      creature->removeInventory(index);
    }
  }
  // cancel action
  creature->cancelTarget();
}
