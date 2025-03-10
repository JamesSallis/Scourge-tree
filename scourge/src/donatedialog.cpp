/***************************************************************************
  donatedialog.cpp  -  The donation dialog
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

#include "common/constants.h"
#include "donatedialog.h"
#include "scourge.h"
#include "creature.h"
#include "rpg/rpglib.h"
#include "gui/window.h"
#include "gui/button.h"
#include "gui/textfield.h"
#include "gui/scrollinglabel.h"

using namespace std;

DonateDialog::DonateDialog( Scourge *scourge ) {
	this->scourge = scourge;
	this->creature = NULL;
	int w = 400;
	int h = 200;
	win =
	  scourge->createWindow( 50, 50,
	                         w, h,
	                         Constants::getMessage( Constants::DONATE_DIALOG_TITLE ) );
	creatureLabel = win->createLabel( 10, 15, "" );
	coinLabel = win->createLabel( 10, 30, _( "Coins Available:" ) );
	win->createLabel( 10, 55, _( "Amount:" ) );
	amount = new TextField( 70, 45, 28 );
	win->addWidget( amount );
	applyButton = win->createButton( 320, 45, 390, 70, _( "Donate!" ) );

	result = new ScrollingLabel( 10, 75, w - 20, 65, "" );
	win->addWidget( result );

	h = 20;
	int y = win->getHeight() - h - 30;
	closeButton = win->createButton( w - 80, y, w - 10, y + h, _( "Close" ) );
	win->registerEventHandler( this );
}

DonateDialog::~DonateDialog() {
	delete win;
}

void DonateDialog::setCreature( Creature *creature ) {
	this->creature = creature;
	updateUI();
	win->setVisible( true );
}

void DonateDialog::updateUI() {

	MagicSchool *ms =
	  MagicSchool::getMagicSchoolByName( creature->getNpcInfo()->subtypeStr );
	enum { TXT_SIZE = 255 };
	char tmp[TXT_SIZE];
	snprintf( tmp, TXT_SIZE, _( "Temple of %s" ), ms->getDeity() );
	char s[TXT_SIZE];
	snprintf( s, TXT_SIZE, "%s (%s %d), %s.",
	          _( creature->getName() ),
	          _( "level" ),
	          creature->getNpcInfo()->level,
	          tmp );
	creatureLabel->setText( s );
	snprintf( s, TXT_SIZE, "%s %d",
	          _( "Coins Available:" ),
	          scourge->getParty()->getPlayer()->getMoney() );
	coinLabel->setText( s );

	result->setText( "" );
}

bool DonateDialog::handleEvent( Widget *widget, SDL_Event *event ) {
	if ( widget == closeButton || widget == win->closeButton ) {
		win->setVisible( false );
	} else if ( widget == applyButton ) {
		donate( atoi( amount->getText() ) );
	}
	return false;
}

void DonateDialog::donate( int amount ) {
	if ( amount <= 0 ) {
		scourge->showMessageDialog( _( "Enter the amount you want to donate." ) );
		return;
	}

	// take the $$$
	scourge->getParty()->getPlayer()->setMoney(
	  scourge->getParty()->getPlayer()->getMoney() - amount );
	// update the coin label
	char s[255];
	snprintf( s, 255, "%s %d",
	          _( "Coins Available:" ),
	          scourge->getParty()->getPlayer()->getMoney() );
	coinLabel->setText( s );
	this->amount->clearText();

	// do something... ( add to value per selected god. Get special powers, exp., etc.)
	cerr << "FIXME: add code to see if user gains a special power." << endl;

	// show results of donation
	MagicSchool *ms =
	  MagicSchool::getMagicSchoolByName( creature->getNpcInfo()->subtypeStr );
	int level = scourge->getParty()->getPlayer()->getLevel();
	int low = ( level > 1 ? ( level - 1 ) * 100 : 0 );
	int high = ( level + 1 ) * 1000;
	//cerr << "low: " << low << " amount: " << amount << " high: " << high << endl;
	result->setText( amount < low ? ms->getLowDonateMessage()
	                 : amount > high ? ms->getHighDonateMessage()
	                 : ms->getNeutralDonateMessage() );
}

