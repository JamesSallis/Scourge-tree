/***************************************************************************
                          character.cpp  -  description
                             -------------------
    begin                : Mon Jul 7 2003
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

#include "character.h"

map<string, Character*> Character::character_class;
map<string, Character*> Character::character_class_short;
map<string, int> Character::character_index_short;

void Character::initCharacters() {
  char errMessage[500];
  char s[200];
  sprintf(s, "%s/world/characters.txt", rootDir);
  FILE *fp = fopen(s, "r");
  if(!fp) {        
	sprintf(errMessage, "Unable to find the file: %s!", s);
	cerr << errMessage << endl;
	exit(1);
  }

  Character *last = NULL;
  char name[255], model[255], skin[255];
  char line[255], shortName[10];
  int index = 0;
  int n = fgetc(fp);
  while(n != EOF) {
	if(n == 'C') {
	  // skip ':'
	  fgetc(fp);
	  // read the rest of the line
	  n = Constants::readLine(line, fp);

	  strcpy(name, strtok(line, ","));
	  strcpy(model, strtok(NULL, ","));
	  strcpy(skin, strtok(NULL, ","));
	  int hp =  atoi(strtok(NULL, ","));
	  int mp =  atoi(strtok(NULL, ","));
	  int skill_bonus =  atoi(strtok(NULL, ","));
	  int level_progression = atoi(strtok(NULL, ","));
    strcpy(shortName, strtok(NULL, ","));

	  cerr << "adding character class: " << name << " model: " << model << 
		" skin: " << skin << " hp: " << hp << " mp: " << mp << " skill_bonus: " << 
      skill_bonus << " shortName=" << shortName << endl;

	  last = new Character( strdup(name), hp, mp, strdup(model), strdup(skin), skill_bonus, 
                          level_progression, strdup(shortName) );
	  string s = name;
	  character_class[s] = last;
    string s2 = shortName;
	  character_class_short[s2] = last;
    character_index_short[s2] = index++;
	} else if(n == 'D' && last) {
	  fgetc(fp);
	  n = Constants::readLine(line, fp);
	  if(strlen(last->description)) strcat(last->description, " ");
	  strcat(last->description, strdup(line));
	} else if(n == 'S' && last) {
	  fgetc(fp);
	  n = Constants::readLine(line, fp);
	  strcpy(name, strtok(line, ","));
	  char *p = strtok(NULL, ",");
	  int min = 0;
	  if(strcmp(p, "*")) min = atoi(p);
	  p = strtok(NULL, ",");
	  int max = 100;
	  if(strcmp(p, "*")) max = atoi(p);
	  int skill = Constants::getSkillByName(name);
	  if(skill < 0) {
		cerr << "*** WARNING: cannot find skill: " << name << endl;		
	  } else {		
		last->setMinMaxSkill(skill, min, max);
	  }
	} else {
	  n = Constants::readLine(line, fp);
	}
  }
  fclose(fp);
}

Character::Character(char *name, int startingHp, int startingMp, char *model, 
					 char *skin, int skill_bonus, int level_progression, char *shortName ) {  
  this->name = name;
  this->startingHp = startingHp;
  this->startingMp = startingMp;
  this->model_name = model;
  this->skin_name = skin;
  this->skill_bonus = skill_bonus;
  this->level_progression = level_progression;
  this->shortName = shortName;
  strcpy(description, "");
}

Character::~Character(){  
}
