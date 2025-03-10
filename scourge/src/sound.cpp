/***************************************************************************
                    sound.cpp  -  Sound and music manager
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
#include <iomanip>
#include <string>
#include "sound.h"
#include "battle.h"
#include "render/renderlib.h"
#include "rpg/rpglib.h"
#include "rpg/rpgitem.h"
#include "creature.h"
#include "session.h"
#include "sqbinding/sqbinding.h"

// ###### MS Visual C++ specific ###### 
#if defined(_MSC_VER) && defined(_DEBUG)
# define new DEBUG_NEW
# undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif 

using namespace std;

#define MISSION_MUSIC_COUNT 6
#define FIGHT_MUSIC_COUNT 2

char *Sound::TELEPORT = "teleport";
char *Sound::OPEN_DOOR = "open door";
char *Sound::OPEN_BOX = "open box";

Sound::Sound( Preferences *preferences, Session *session ) {
	this->session = session;
	haveSound = false;
	ambientPause = false;

	if ( preferences->isSoundEnabled() ) {
#ifdef HAVE_SDL_MIXER
		if ( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 ) ) {
			cerr << "*** Error opening audio: " << Mix_GetError() << endl;
			cerr << "\tDisabling sound." << endl;
		} else {
			haveSound = true;
		}

		int allocated_count;
		allocated_count = Mix_AllocateChannels( 32 );
		if ( allocated_count != 32 ) {
			cerr << "allocated " << allocated_count << " channels from default mixing." << endl;
			cerr << "32 channels were not allocated!" << endl;
			// this might be a critical error...
		}

		int reserved_count;
		reserved_count = Mix_ReserveChannels( 8 );
		if ( reserved_count != 8 ) {
			cerr << "reserved " << reserved_count << " channels from default mixing." << endl;
			cerr << "8 channels were not reserved!" << endl;
			// this might be a critical error...
		}

		lastChapter = -1;
		missionMusicIndex = -1;
		currentMusic = currentLevelMusic = menuMusic = landMusic = missionMusic = fightMusic = currentFightMusic = chapterMusic = outroMusic = NULL;
		strcpy( landMusicName, "" );
		strcpy( landMusicKey, "" );
		musicStartTime = 0;
		musicPosition = 0;
		if ( haveSound ) {
			selectMusic( preferences );
		}
#endif
	}
}

Sound::~Sound() {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		// delete music
		if ( menuMusic ) {
			Mix_FreeMusic( menuMusic );
			menuMusic = NULL;
		}
		if ( landMusic ) {
			Mix_FreeMusic( landMusic );
			landMusic = NULL;
		}
		if ( missionMusic ) {
			Mix_FreeMusic( missionMusic );
			missionMusic = NULL;
		}
		if ( fightMusic ) {
			Mix_FreeMusic( fightMusic );
			fightMusic = NULL;
		}
		if ( !chapterMusic ) {
			Mix_FreeMusic( chapterMusic );
			chapterMusic = NULL;
		}
		if ( !outroMusic ) {
					Mix_FreeMusic( outroMusic );
					outroMusic = NULL;
				}
		// delete sounds
		for ( map<string, Mix_Chunk*>::iterator i = soundMap.begin(); i != soundMap.end(); ++i ) {
			Mix_Chunk *sample = i->second;
			Mix_FreeChunk( sample );
			sample = NULL;
		}
		if ( !soundMap.empty() ) soundMap.clear();
		if ( !soundNameMap.empty() ) soundNameMap.clear();
		for ( map<string, Mix_Chunk*>::iterator i = ambient_objects.begin(); i != ambient_objects.end(); ++i ) {
			Mix_Chunk *sample = i->second;
			Mix_FreeChunk( sample );
			sample = NULL;
		}
		for ( std::map<std::string, AmbientSound*>::iterator i = ambients.begin(); i != ambients.end(); ++i ) {
			delete i->second;
		}
		for ( std::map<string, Mix_Music*>::iterator i = bossCombatMusics.begin(); i != bossCombatMusics.end(); ++i ) {
			Mix_Music *mm = i->second;
			Mix_FreeMusic( mm );
		}
		// stop audio system
		Mix_CloseAudio();
	}
#endif
}

// randomly select mission music, load others if needed
void Sound::selectMusic( Preferences *preferences, Mission * mission ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		// load fixed musics if needed
		if ( !menuMusic ) {
			string fn = rootDir + "/sound/music/menu.ogg";
			menuMusic = Mix_LoadMUS( fn.c_str() );
			if ( !menuMusic ) {
				cerr << "*** 1 Error: couldn't load music: " << fn << endl;
				cerr << "\t" << Mix_GetError() << endl;
			}
		}
		if ( mission && mission->isStoryLine() ) {
			if ( lastChapter != mission->getChapter() ) {
				if ( !chapterMusic ) {
					Mix_FreeMusic( chapterMusic );
					chapterMusic = NULL;
				}
				lastChapter = mission->getChapter();
				stringstream filename;
				filename << rootDir << "/sound/music/chapters/chapter" << lastChapter << ".ogg";
				string fn = filename.str();
				chapterMusic = Mix_LoadMUS( fn.c_str() );
				if ( !chapterMusic ) {
					cerr << "*** 3 Error: couldn't load music: " << fn << endl;
					cerr << "\t" << Mix_GetError() << endl;
					cerr << "\tfalling back to chapter 1 music." << endl;
					fn = rootDir + "/sound/music/chapters/chapter1.ogg";
					chapterMusic = Mix_LoadMUS( fn.c_str() );
					if ( !chapterMusic ) {
						cerr << "*** 4 Error: couldn't load music: " << fn << endl;
						cerr << "\t" << Mix_GetError() << endl;
					}
				}
			}
		}

		if( !outroMusic ) {
			stringstream filename;
			filename << rootDir << "/sound/music/outro.ogg";
			string fn = filename.str();
			outroMusic = Mix_LoadMUS( fn.c_str() );
		}

		//select mission music
		// unload the current one
		if ( missionMusic ) {
			Mix_FreeMusic( missionMusic );
			missionMusic = NULL;
		}

		stringstream filename;
		if ( mission && mission->getMusicTrack() ) {
			//selects mission specific track
			filename << rootDir << "/sound/music/" << mission->getMusicTrack() << ".ogg";
		} else {
			//selects random one
			missionMusicIndex = Util::pickOne( 1, MISSION_MUSIC_COUNT );
			filename << rootDir << "/sound/music/track" << setw( 2 ) << setfill( '0' ) << missionMusicIndex << ".ogg";
		}
		string fn;
		filename >> fn;

		// load the new one
		missionMusic = Mix_LoadMUS( fn.c_str() );
		if ( !missionMusic ) {
			cerr << "*** 5 Error: couldn't load music: " << fn << endl;
			cerr << "\t" << Mix_GetError() << endl;
		}

		// selects fight music
		int n = Util::pickOne( 1, FIGHT_MUSIC_COUNT );
		if ( fightMusicIndex != n ) {
			fightMusicIndex = n;

			// unload the current one
			if ( fightMusic ) {
				Mix_FreeMusic( fightMusic );
				fightMusic = NULL;
			}


			// load the new one
			stringstream filename2;
			//filename.str("");
			filename2 << rootDir << "/sound/music/fight" << setw( 2 ) << setfill( '0' ) << fightMusicIndex << ".ogg";
			string fn;
			filename2 >> fn;
			fightMusic = Mix_LoadMUS( fn.c_str() );
			if ( !fightMusic ) {
				cerr << "*** 6 Error: couldn't load music: " << fn << endl;
				cerr << "\t" << Mix_GetError() << endl;
			}
		}
		
		// load the boss combat musics
		if( bossCombatMusics.empty() ) {
			for( map<string, Monster*>::iterator i = Monster::monstersByName.begin(); i != Monster::monstersByName.end(); ++i ) {
				Monster *monster = i->second;
				if( strlen( monster->getCombatMusic() ) ) {
					string musicName = monster->getCombatMusic();
					if( bossCombatMusics.find( musicName ) == bossCombatMusics.end() ) {
						stringstream filename3;
						filename3 << rootDir << "/sound/music/" << musicName;
						string fn;
						filename3 >> fn;
						//cerr << "+++ loading combat music for " << monster->getType() << ": " << fn << endl;
						bossCombatMusics[ musicName ] = Mix_LoadMUS( fn.c_str() );	
					} else {
						//cerr << "+++ reusing combat music for " << monster->getType() << ": " << endl;
					}					 
				}
			}
		}

		setMusicVolume( preferences->getMusicVolume() );
	}
#endif
}


void Sound::playMusic( Mix_Music *music, int ms, int loopCount ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound && music ) {
		currentMusic = music;
		if ( currentMusic != currentFightMusic ) {
			if ( currentLevelMusic != currentMusic ) {
				currentLevelMusic = currentMusic;
				musicStartTime = SDL_GetTicks();
				musicPosition = 0;
			}
			if ( Mix_FadeInMusicPos( music, loopCount, ms, musicPosition ) ) {
				cerr << "*** Error playing music: " << Mix_GetError() << endl;
			}
		} else {
			if ( Mix_FadeInMusic( music, loopCount, ms ) ) {
				cerr << "*** Error playing music: " << Mix_GetError() << endl;
			}
		}
	}
#endif
}

void Sound::stopMusic( int ms ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {

		// remember where the level music is stopped
		if ( currentMusic != currentFightMusic ) {
			musicPosition = static_cast<double>( SDL_GetTicks() - musicStartTime );
		}

		if ( !Mix_FadeOutMusic( ms ) ) {
			//  cerr << "*** Error stopping music: " << Mix_GetError() << endl;
			// force stop music
			Mix_HaltMusic();
		}
	}
#endif
}


void Sound::checkMusic( bool inCombat, const char *currentCombatMusic ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		currentFightMusic = fightMusic;
		if( currentCombatMusic && strlen( currentCombatMusic ) ) {
			string s = currentCombatMusic;
			currentFightMusic = bossCombatMusics[ s ];
			if( !currentFightMusic ) {
				currentFightMusic = fightMusic;
			}
		}
		Mix_Music *should = ( inCombat ? currentFightMusic : currentLevelMusic );
		if ( should != currentMusic ) {
			if ( currentMusic )
				stopMusic( 500 );
			playMusic( should, 500 );
		}
	}
#endif
}

void Sound::loadUISounds( Preferences *preferences ) {
	if ( !haveSound ) return;

//  cerr << "Loading UI sounds..." << endl;
	storeSound( 0, Window::ROLL_OVER_SOUND );
	storeSound( 0, Window::ACTION_SOUND );
	storeSound( 0, Window::DROP_SUCCESS );
	storeSound( 0, Window::DROP_FAILED );

	storeSound( 0, TELEPORT );
	storeSound( 0, OPEN_DOOR );
	storeSound( 0, OPEN_BOX );

	setEffectsVolume( preferences->getEffectsVolume() );

}

void Sound::loadSounds( Preferences *preferences ) {
	if ( !haveSound )
		return;

	//cerr << "Loading battle sounds..." << endl;
	for ( int i = 0; i < Battle::getSoundCount(); i++ ) {
		storeSound( 0, Battle::getSound( i ) );
	}

	//cerr << "Loading item sounds..." << endl;
	for ( map<int, vector<string>*>::iterator i = RpgItem::soundMap.begin();
	        i != RpgItem::soundMap.end(); ++i ) {
		//Creature *creature = i->first;
		vector<string> *v = i->second;
		for ( vector<string>::iterator r = v->begin(); r != v->end(); r++ ) {
			storeSound( 0, *r );
		}
	}

	//cerr << "Loading spell sounds..." << endl;
	for ( int i = 0; i < static_cast<int>( MagicSchool::getMagicSchoolCount() ); i++ ) {
		for ( int t = 0; t < static_cast<int>( MagicSchool::getMagicSchool( i )->getSpellCount() ); t++ ) {
			storeSound( 0, MagicSchool::getMagicSchool( i )->getSpell( t )->getSound() );
		}
	}

	// FIXME: Put thunder sounds in an array
	storeSound( "rain", "/sound/weather/rain.wav" );
	storeSound( "thunder1", "/sound/weather/thunder1.wav" );
	storeSound( "thunder2", "/sound/weather/thunder2.wav" );
	storeSound( "thunder3", "/sound/weather/thunder3.wav" );
	storeSound( "thunder4", "/sound/weather/thunder4.wav" );

	setEffectsVolume( preferences->getEffectsVolume() );
}

void Sound::storeAmbientObjectSound( std::string const& sound ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		stringstream filename;
		filename << rootDir << "/sound/objects/" << sound;
		Mix_Chunk *sample = Mix_LoadWAV( filename.str().c_str() );
		if ( sample ) {
			if ( ambient_objects.find( sound ) == ambient_objects.end() ) {
				//cerr << "\tstoring " << sound << endl;
				ambient_objects[sound] = sample;
			}
		} else {
			cerr << "*** Error: unable to load ambient object sound: " << sound << endl;
		}
	}
#endif
}

void Sound::playObjectSound( std::string& name, int percent, int panning ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		int volume = static_cast<int>( ( MIX_MAX_VOLUME / 100.0f ) * static_cast<float>( percent ) );

		//cerr << "vol=" << volume << endl;
		Mix_Volume( Constants::OBJECT_CHANNEL, volume );
		Mix_SetPanning( Constants::OBJECT_CHANNEL, 255 - panning, panning );
		if ( !Mix_Playing( Constants::OBJECT_CHANNEL ) ) {
			for ( int i = 0; i < 5; i++ ) {
				if ( Mix_PlayChannel( Constants::OBJECT_CHANNEL, ambient_objects[ name ], 0 ) != -1 ) break;
			}
		}
	}
#endif
}

void Sound::loadMonsterSounds( char const* monsterType, map<int, vector<string>*> *m, Preferences *preferences ) {
	//  cerr << "Loading monster sounds for " << monsterType << "..." << endl;
	if ( m ) {
		for ( map<int, vector<string>*>::iterator i2 = m->begin(); i2 != m->end(); ++i2 ) {
			vector<string> *v = i2->second;
			for ( vector<string>::iterator i = v->begin(); i != v->end(); i++ ) {
				storeSound( 0, *i );
			}
		}
	}
	setEffectsVolume( preferences->getEffectsVolume() );
}

void Sound::unloadMonsterSounds( char const* monsterType, map<int, vector<string>*> *m ) {
	//  cerr << "Unloading monster sounds for " << monsterType << "..." << endl;
	if ( m ) {
		for ( map<int, vector<string>*>::iterator i2 = m->begin(); i2 != m->end(); ++i2 ) {
			vector<string> *v = i2->second;
			for ( vector<string>::iterator i = v->begin(); i != v->end(); i++ ) {
				unloadSound( 0, *i );
			}
		}
	}
}

void Sound::loadCharacterSounds( char const* type ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		//std::map<std::string, std::map<int, std::vector<Mix_Chunk*>* >* > characterSounds;
		string typeStr = type;
		map<int, vector<Mix_Chunk*>*>* charSoundMap;
		if ( characterSounds.find( typeStr ) == characterSounds.end() ) {
			charSoundMap = new map<int, vector<Mix_Chunk*>*>();
			characterSounds[ typeStr ] = charSoundMap;
		} else {
			charSoundMap = characterSounds[ typeStr ];
		}

		// load the sounds
		storeCharacterSounds( charSoundMap, type, GameAdapter::COMMAND_SOUND, "command" );
		storeCharacterSounds( charSoundMap, type, GameAdapter::HIT_SOUND, "hit" );
		storeCharacterSounds( charSoundMap, type, GameAdapter::SELECT_SOUND, "select" );
	}
#endif
}

#ifdef HAVE_SDL_MIXER
void Sound::storeCharacterSounds( map<int, vector<Mix_Chunk*>*> *charSoundMap,
    char const* type, int soundType, char const* filePrefix ) {
	stringstream filename;
	vector<Mix_Chunk*> *v;
	if ( charSoundMap->find( soundType ) == charSoundMap->end() ) {
		v = new vector<Mix_Chunk*>();
		( *charSoundMap )[ soundType ] = v;
	} else {
		v = ( *charSoundMap )[ soundType ];
	}
	for ( int i = 0; i < 100; i++ ) {
		filename << rootDir << type << "/sound/" << filePrefix << ( i + 1 ) << ".wav";
		//cerr << "Looking for character sound: " << filename << endl;
		Mix_Chunk *sample = Mix_LoadWAV( filename.str().c_str() );
		if ( !sample ) break;
		//cerr << "Loaded character sound: " << filename << endl;
		v->push_back( sample );
	}
}
#endif

void Sound::playCharacterSound( char const* type, int soundType, int panning ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		string typeStr = type;
		if ( characterSounds.find( typeStr ) != characterSounds.end() ) {
			map<int, vector<Mix_Chunk*>*> *charSoundMap = characterSounds[ typeStr ];
			if ( charSoundMap->find( soundType ) != charSoundMap->end() ) {
				vector<Mix_Chunk*> *v = ( *charSoundMap )[ soundType ];
				if ( v->size() > 0 ) {
					for ( int t = 0; t < 5; t++ ) {
						int index = Util::dice( v->size() );
						int channel = Mix_PlayChannel( -1, ( *v )[ index ], 0 );
						if ( channel != -1 ) {
							Mix_SetPanning( channel, 255 - panning, panning );
							return;
						}
					}
				}
			}
		}
	}
#endif
}

void Sound::unloadCharacterSounds( char const* type ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		string typeStr = type;
		if ( characterSounds.find( typeStr ) != characterSounds.end() ) {
			map<int, vector<Mix_Chunk*>*>* charSoundMap = characterSounds[ typeStr ];
			for ( map<int, vector<Mix_Chunk*>*>::iterator i = charSoundMap->begin();
			        i != charSoundMap->end(); ++i ) {
				vector<Mix_Chunk*> *v = i->second;
				for ( vector<Mix_Chunk*>::iterator i2 = v->begin(); i2 != v->end(); ++i2 ) {
					Mix_Chunk *sample = *i2;
					Mix_FreeChunk( sample );
				}
				delete v;
			}
			delete charSoundMap;
			characterSounds.erase( typeStr );
		}
	}
#endif
}

void Sound::storeSound( const string& name, const string& file ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		if ( soundNameMap.find( name ) == soundNameMap.end() ) {
			soundNameMap[name] = file;
			storeSound( 0, file );
		}
	}
#endif
}

void Sound::storeSound( int type, const string& file ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		string fileStr = file;
		if ( soundMap.find( fileStr ) == soundMap.end() ) {
			string fn;
			if ( file[0] == '/' || file[0] == '\\' ) {
				fn = rootDir + file;
			} else {
				fn = rootDir + ( "/" + file );
			}
//      cerr << "*** Loading sound file: " << fn << endl;
			Mix_Chunk *sample = Mix_LoadWAV( fn.c_str() );
			if ( !sample ) {
				// commented out; happens too often to be meaningful
				// cerr << "*** Error loading WAV file (" << file << "): " << Mix_GetError() << endl;
			} else {
				soundMap[fileStr] = sample;
			}
		}
	}
#endif
}

void Sound::unloadSound( int type, const string& file ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		if ( soundMap.find( file ) != soundMap.end() ) {
//   cerr << "*** Freeing sound: " << fileStr << endl;
			Mix_Chunk *sample = soundMap[ file ];
			Mix_FreeChunk( sample );
			soundMap.erase( file );
		}
	}
#endif
}

// ######################################
void Sound::pauseAmbientSounds() {
#ifdef HAVE_SDL_MIXER
	ambientPause = true;
	Mix_HaltChannel( Constants::RAIN_CHANNEL );
	Mix_HaltChannel( Constants::FOOTSTEP_CHANNEL );
	Mix_HaltChannel( Constants::AMBIENT_CHANNEL );
#endif
}

void Sound::unpauseAmbientSounds() {
#ifdef HAVE_SDL_MIXER
	ambientPause = false;
#endif
}

int Sound::playSound( const string& file, int panning ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		//cerr << "*** Playing WAV: " << file << endl;
		string s = ( soundNameMap.find( file ) != soundNameMap.end() ? soundNameMap[file] : file );
		if ( soundMap.find( s ) != soundMap.end() ) {
			for ( int t = 0; t < 5; t++ ) {
				int channel = Mix_PlayChannel( -1, soundMap[s], 0 );
				if ( channel != -1 ) {
					Mix_SetPanning( channel, 255 - panning, panning );
					return channel;
				}
			}
		}
	}
#endif
	return -1;
}

void Sound::startRain() {
#ifdef HAVE_SDL_MIXER
	if ( haveSound && !ambientPause ) {
		int panning = 127;
		//cerr << "*** Playing WAV: " << file << endl;
		string s = soundNameMap["rain"];
		if ( soundMap.find( s ) != soundMap.end() ) {
			for ( int t = 0; t < 5; t++ ) {
				if ( Mix_FadeInChannel( Constants::RAIN_CHANNEL, soundMap[s], -1, WEATHER_CHANGE_DURATION ) == Constants::RAIN_CHANNEL ) {
					Mix_SetPanning( Constants::RAIN_CHANNEL, 255 - panning, panning );
					return;
				}
			}
		}
	}
#endif
}

void Sound::stopRain() {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		Mix_FadeOutChannel( Constants::RAIN_CHANNEL, WEATHER_CHANGE_DURATION );
	}
#endif
}

void Sound::startFootsteps( std::string& name, int depth, int panning ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound && !ambientPause ) {
		AmbientSound *as = getAmbientSound( name, depth );
		if ( as ) as->playFootsteps( panning );
	}
#endif
}

void Sound::stopFootsteps() {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		Mix_HaltChannel( Constants::FOOTSTEP_CHANNEL );
	}
#endif
}

void Sound::addAmbientSound( std::string& name, std::string& ambient, std::string& footsteps, std::string& afterFirstLevel ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		ambients[name] = new AmbientSound( name, ambient, footsteps, afterFirstLevel );
	}
#endif
}
void Sound::startAmbientSound( std::string& name, int depth ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound && !ambientPause ) {
		//cerr << "Playing " << name << " depth=" << depth << endl;
		AmbientSound *as = getAmbientSound( name, depth );
		if ( as ) as->playRandomAmbientSample();
	}
#endif
}

void Sound::stopAmbientSound() {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		Mix_HaltChannel( Constants::AMBIENT_CHANNEL );
	}
#endif
}

AmbientSound *Sound::getAmbientSound( std::string& name, int depth ) {
	AmbientSound *as = NULL;
	if ( ambients.find( name ) != ambients.end() ) {
		as = ambients[ name ];
		if ( depth > 0 ) {
			if ( ambients.find( as->getAfterFirstLevel() ) != ambients.end() ) {
				as = ambients[ as->getAfterFirstLevel() ];
			} else {
				cerr << "2 Ambient not found: " << as->getAfterFirstLevel() << endl;
				as = NULL;
			}
		}
	} else {
		cerr << "1 Ambient not found: " << name << endl;
	}
	return as;
}

// ######################################

AmbientSound::AmbientSound( std::string& name, std::string& ambient, std::string& footsteps, std::string& afterFirstLevel ) {
	this->name = name;
	this->afterFirstLevel = afterFirstLevel;
#ifdef HAVE_SDL_MIXER
	updateAmbients( ambient );
	stringstream filename;
	filename << rootDir << "/sound/footsteps/" << footsteps;
	//cerr << "\t" << filename.str() << endl;
	this->footsteps = Mix_LoadWAV( filename.str().c_str() );
	if ( !this->footsteps ) {
		cerr << "*** Error cannot load sound sample: " << filename.str() << " reason=" << Mix_GetError() << endl;
	}
#endif
}

void AmbientSound::updateAmbients( std::string& ambient ) {
	for ( unsigned n = 0; n < ambients.size(); n++ ) {
		Mix_FreeChunk( this->ambients[ n ] );
	}
	ambients.clear();
	
	size_t start = 0;
	size_t found = ambient.find( "," );

	//cerr << "Loading ambients: " << name << " stored ambients: " << ambients.size() << endl;
	//cerr << "ambients=" << ambient << endl;
	bool first = true;
	while ( !( found == string::npos && first ) ) {
		string s;
		if ( found != string::npos ) {
			s = ambient.substr( start, found - start );
		} else {
			s = ambient.substr( start );
		}
//  cerr << "\tstart=" << start << " found=" << found << " s=" << s << " first=" << first << endl;

		stringstream filename;
		filename << rootDir << "/sound/ambient/" << s;
		//cerr << "\tLOADING " << filename.str() << endl;
		Mix_Chunk *sample = Mix_LoadWAV( filename.str().c_str() );
		if ( !sample ) {
			cerr << "*** Error cannot load sound sample: " << filename.str() << " reason=" << Mix_GetError() << endl;
		} else {
			ambients.push_back( sample );
		}

		if ( found == string::npos ) {
			break;
		} else {
			start = found + 1;
			found = ambient.find( ",", start );
			first = false;
		}
	}	
}

AmbientSound::~AmbientSound() {
#ifdef HAVE_SDL_MIXER
	// halt playback on all channels
	Mix_HaltChannel( -1 );
	Mix_FreeChunk( this->footsteps );
	for ( unsigned n = 0; n < ambients.size(); n++ ) {
		Mix_FreeChunk( this->ambients[ n ] );
	}
#endif
}

/// Plays a random sample from the map's ambient sound set.

int AmbientSound::playRandomAmbientSample() {
#ifdef HAVE_SDL_MIXER
	// Abort if already playing an ambient
	if ( Mix_Playing( Constants::AMBIENT_CHANNEL ) ) return -1;
	int panning = Util::pickOne( 41, 213 );
	Mix_SetPanning( Constants::AMBIENT_CHANNEL, 255 - panning, panning );
	int n = Util::dice( ambients.size() );
	//cerr << "\t" << n << " out of " << ambients.size() << endl;
	for ( int t = 0; t < 5; t++ ) {
		if ( Mix_PlayChannel( Constants::AMBIENT_CHANNEL, ambients[ n ], 0 ) ) return 1;
	}
#endif
	return -1;
}

/// Plays the footstep sound.

int AmbientSound::playFootsteps( int panning ) {
#ifdef HAVE_SDL_MIXER
	Mix_SetPanning( Constants::FOOTSTEP_CHANNEL, 255 - panning, panning );
	for ( int i = 0; i < 5; i++ ) {
		if ( Mix_PlayChannel( Constants::FOOTSTEP_CHANNEL, footsteps, 0 ) != -1 ) return 1;
	}
#endif
	return -1;
}

// ######################################

void Sound::setMusicVolume( int volume ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound )
		Mix_VolumeMusic( volume );
#endif
}

void Sound::setEffectsVolume( int volume ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		for ( map<string, Mix_Chunk*>::iterator i = soundMap.begin(); i != soundMap.end(); ++i ) {
			Mix_Chunk *sample = i->second;
			Mix_VolumeChunk( sample, volume );
		}
	}
#endif
}

void Sound::setWeatherVolume( bool underRoof ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		// Dim the weather sounds when inside a house.
		if ( underRoof ) {
			Mix_Volume( Constants::RAIN_CHANNEL, 40 );
			Mix_Volume( Constants::AMBIENT_CHANNEL, 40 );
		} else {
			Mix_Volume( Constants::RAIN_CHANNEL, 128 );
			Mix_Volume( Constants::AMBIENT_CHANNEL, 128 );
		}
	}
#endif	
}

void Sound::playThunderSound( bool underRoof ) {
#ifdef HAVE_SDL_MIXER
	if ( haveSound ) {
		int channel;
		int volume = ( underRoof ? 40 : 128 );
		int thunderSound = Util::pickOne( 1, 4 );
		if ( thunderSound == 1 ) {
			channel = playSound( "thunder1", Util::pickOne( 41, 213 ) );
			if ( channel > -1 ) Mix_Volume( channel, volume );
		} else if ( thunderSound == 2 ) {
			channel = playSound( "thunder2", Util::pickOne( 41, 213 ) );
			if ( channel > -1 ) Mix_Volume( channel, volume );
		} else if ( thunderSound == 3 ) {
			channel = playSound( "thunder3", Util::pickOne( 41, 213 ) );
			if ( channel > -1 ) Mix_Volume( channel, volume );
		} else if ( thunderSound == 4 ) {
			channel = playSound( "thunder4", Util::pickOne( 41, 213 ) );
			if ( channel > -1 ) Mix_Volume( channel, volume );
		}
	}
#endif					
}

void Sound::playLandMusic() {
#ifdef HAVE_SDL_MIXER	
	if ( haveSound ) {
		char music_key[255];
		int params[6];
		session->getGameAdapter()->getMapRegionAndPos( params, true );
		session->getSquirrel()->callIntArgStringReturnMethod( "get_music_key", music_key, 6, params );
	
		switchLandMusic( music_key );
		
		switchLandAmbients( music_key );
	
		strcpy( landMusicKey, music_key );
	}
#endif	
}

void Sound::switchLandAmbients( char *music_key ) {
	if( strcmp( music_key, landMusicKey ) ) {
		cerr << "Switching ambients to key=" << music_key << endl;
		
		string name = "outdoors";
		char music_name[255];
		char *args[1];
		args[0] = music_key;
		session->getSquirrel()->callStringArgStringReturnMethod( "get_ambients", music_name, 1, args );
		if( strlen( music_name ) ) {
			cerr << "\tSelected ambient files=" << music_name << endl;
			
			// modify the current "outdoor" ambient sound
			stopAmbientSound();
			AmbientSound *a = getAmbientSound( name, 0 );
			string ambient_sounds = music_name;
			if( a ) a->updateAmbients( ambient_sounds );

		} else {
			cerr << "*** error no ambients were returned for music key: " << music_key << endl;
		}
			
		// play it
		startAmbientSound( name, 0 );			
	}	
}

void Sound::switchLandMusic( char *music_key ) {
	// if the music has stopped or the music key has changed, load a new tune
	if( !Mix_PlayingMusic() || strcmp( music_key, landMusicKey ) ) {
		cerr << "Switching musics to key=" << music_key << endl;
		
		char music_name[255];
		char *args[1];
		args[0] = music_key;
		session->getSquirrel()->callStringArgStringReturnMethod( "get_music_name", music_name, 1, args );
		if( strcmp( music_name, landMusicName ) ) {
			cerr << "\tSelected music file=" << music_name << endl;
			
			// unload the old
			stopMusic( 500 );
			if ( landMusic ) {
				Mix_FreeMusic( landMusic );
				landMusic = NULL;
			}
			
			if( strlen( music_name ) ) {
				// load the new
				string fn = rootDir + "/sound/music/" + music_name;
				cerr << "\tLoading music " << fn << endl;
				landMusic = Mix_LoadMUS( fn.c_str() );
				if ( !landMusic ) {
					cerr << "*** Error: couldn't load music: " << fn << endl;
					cerr << "\t" << Mix_GetError() << endl;
				}
			} else {
				cerr << "*** error no music file was returned for music key: " << music_key << endl;
			}
			strcpy( landMusicName, music_name );
			
			// play it
			if( landMusic ) playMusic( landMusic, 500, 1 );			
		}
	}	
}
