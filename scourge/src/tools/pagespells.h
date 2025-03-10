#ifndef PAGESPELLS_H
#define PAGESPELLS_H

#include "page.h"

/** Forward Declarations **/
class DFSpells;
class wxTextCtrl;
class wxScrollBar;
class wxComboBox;
class wxSlider;
class wxWindow;
class subPageSchools;
class subPageSpells;
class wxCommandEvent;

class PageSpells : public Page
{
protected:

public:
	DFSpells *dfSpells;
	subPageSchools *pageSchools;
	subPageSpells *pageSpells;

public:
	PageSpells();
	virtual ~PageSpells();

	void Init(wxNotebook*, DF*);

	void UpdatePage();


	void Prev(int=1);
	void Next(int=1);
	void New();
	void Del();

	void UpdatePageNumber();

/*	void LoadAll();*/
/*	void SaveAll();*/
	void GetCurrent();
	void SetCurrent();
	void ClearCurrent();


	void OnSubPageChange(wxCommandEvent& event);

protected:
	wxNotebook *subNotebook;
	Page *currentSubPage;
};

#endif // PAGESPELLS_H
