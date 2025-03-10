#ifndef SUBPAGESCHOOLS_H
#define SUBPAGESCHOOLS_H

#include "page.h"

/** Forward Declarations **/
class DFSpells;
class ColorSelector;
class wxTextCtrl;
class wxStaticText;
class wxScrollBar;
class wxComboBox;
class wxSlider;
class wxWindow;
class wxListBox;
class wxArrayString;
class PageSpells;

class subPageSchools : public Page
{
protected:

public:
	PageSpells *parent;
	DFSpells *dfSpells;

public:
	subPageSchools();
	virtual ~subPageSchools();

	void Init(wxNotebook*, DF*) {}		// DO NOT USE
	void Init(wxNotebook*, DF*, PageSpells*);

	void UpdatePage();

	void New();

	void UpdatePageNumber();

	void GetCurrent();
	void SetCurrent();
	void ClearCurrent();

	void OnAddMessage();
	void OnDelMessage();

protected:
	ColorSelector *colorSelector;

	// List of editable controls
	wxTextCtrl *nameEdit;
	wxTextCtrl *deityEdit;
	wxTextCtrl *skillEdit;
	wxTextCtrl *resistSkillEdit;
	wxSlider *rColorSlider;
	wxSlider *gColorSlider;
	wxSlider *bColorSlider;
		wxStaticText *rColorText;
		wxStaticText *gColorText;
		wxStaticText *bColorText;
	wxTextCtrl *symbolEdit;
	wxTextCtrl *descEdit;
	wxListBox *donationList;
};

#endif // SUBPAGESCHOOLS_H
