/***************************************************************************
  savegamedialog.h  -  The save/load game dialog
-------------------
    begin                : 9/9/2005
    copyright            : (C) 2005 by Gabor Torok
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

#ifndef SAVEGAME_DIALOG_H
#define SAVEGAME_DIALOG_H
#pragma once

#include <map>
#include <vector>
#include <set>
#include <sstream>
#include "gui/eventhandler.h"

class Scourge;
class Window;
class Label;
class ScrollingList;
class ScrollingLabel;
class Widget;
class Button;
class TextField;
class ConfirmDialog;
class Texture;

using namespace std;

#define MAX_SAVEGAME_COUNT 100

/// Information about a savegame.
class SavegameInfo {
public:
	std::string path;
	std::string title;
};

/// The save/load game dialog.
class SavegameDialog : public EventHandler {
private:
	Scourge *scourge;
	Window *win;
	ScrollingList *files;
	Button *cancel, *save, *load, *newSave, *deleteSave;
	std::vector<SavegameInfo*> fileInfos;
	std::vector<std::string> filenames;
	std::vector<Texture> screens; // owns the textures
	ConfirmDialog *confirm;

	bool matches_save_( std::string& match );

public:
	SavegameDialog( Scourge *scourge );
	~SavegameDialog();
	inline Window *getWindow() {
		return win;
	}
	bool handleEvent( Widget *widget, SDL_Event *event );
	void show( bool inSaveMode = true );

	bool createNewSaveGame();

	void deleteUnvisitedMaps( const string& dirName, std::set<std::string> *visitedMaps );

	bool quickSave( const string& dirName, const string& title );
	void quickLoad();

protected:
	bool findFiles();
	bool readFileDetails( const string& path );
	void makeDirectory( const string& path );
	void findFilesInDir( const string& path, vector<string> *fileNameList );
	bool checkIfFileExists( const string& filename );
	void saveScreenshot( const string& dirName );
	bool copyMaps( const string& fromDirName, const string& toDirName );
	bool copyFile( const string& fromDirName, const string& toDirName, const string& fileName );
	// unused: terror GLuint loadScreenshot( const string& path );
	bool saveGameInternal( SavegameInfo *info );
	bool createSaveGame( SavegameInfo *info );
	void loadGame( int n );
	void setSavegameInfoTitle( SavegameInfo *info );
	bool deleteDirectory( const string& path );
	void deleteUnreferencedMaps( const string& dirName );
	bool isRegionVisited( std::string filename );
	DECLARE_NOISY_OPENGL_SUPPORT();
};

#endif

