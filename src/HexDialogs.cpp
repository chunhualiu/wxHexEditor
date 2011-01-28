/***********************************(GPL)********************************
*   wxHexEditor is a hex edit tool for editing massive files in Linux   *
*   Copyright (C) 2010  Erdem U. Altinyurt                              *
*                                                                       *
*   This program is free software; you can redistribute it and/or       *
*   modify it under the terms of the GNU General Public License         *
*   as published by the Free Software Foundation; either version 2      *
*   of the License.                                                     *
*                                                                       *
*   This program is distributed in the hope that it will be useful,     *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of      *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
*   GNU General Public License for more details.                        *
*                                                                       *
*   You should have received a copy of the GNU General Public License   *
*   along with this program;                                            *
*   if not, write to the Free Software Foundation, Inc.,                *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA        *
*                                                                       *
*               home  : wxhexeditor.sourceforge.net                     *
*               email : spamjunkeater at gmail dot com                  *
*************************************************************************/

#define NANINT 0xFFFFFFFFFFFFFFFFLL
#include "HexDialogs.h"
#include <wx/progdlg.h>
GotoDialog::GotoDialog( wxWindow* parent, uint64_t& _offset, uint64_t _cursor_offset, uint64_t _filesize, DialogVector *_myDialogVector=NULL ):GotoDialogGui(parent, wxID_ANY){
	offset = &_offset;
	cursor_offset = _cursor_offset;
	is_olddec =true;
	filesize = _filesize;
	m_textCtrlOffset->SetFocus();
	myDialogVector = _myDialogVector;
	if(myDialogVector != NULL ){
		m_hex->SetValue( myDialogVector->goto_hex );
		if( myDialogVector->goto_hex )
			m_textCtrlOffset->ChangeValue( wxString::Format(wxT("%X"),myDialogVector->goto_input) );
		else
			m_textCtrlOffset->ChangeValue( wxString::Format(wxT("%ld"),myDialogVector->goto_input) );
		m_branch->Select( myDialogVector->goto_branch );
		}
	}

wxString GotoDialog::Filter( wxString text ){
	for( unsigned i = 0 ; i < text.Length() ; i++ ){
		if( m_dec->GetValue() ? isdigit( text.GetChar( i ) ) : isxdigit( text.GetChar( i ) ))
			continue;
		else
			text.Remove( i, 1);
		}
	return text;
	}

void GotoDialog::OnInput( wxCommandEvent& event ){
	if(!m_textCtrlOffset->IsModified())
        return;
    long insertionPoint = m_textCtrlOffset->GetInsertionPoint();
// TODO (death#1#): Copy/Paste/Drop & illegal key walking?
    m_textCtrlOffset->ChangeValue( Filter( m_textCtrlOffset->GetValue() ) );
    m_textCtrlOffset->SetInsertionPoint(insertionPoint);
	event.Skip(false);
	}
void GotoDialog::OnGo( wxCommandEvent& event ){
	wxULongLong_t *wxul = new wxULongLong_t(0);
	m_textCtrlOffset->GetValue().ToULongLong( wxul, m_dec->GetValue() ? 10 : 16 );
	*offset = *wxul;

	if(myDialogVector != NULL ){
		myDialogVector->goto_hex = m_hex->GetValue();
		myDialogVector->goto_branch = m_branch->GetSelection();
		myDialogVector->goto_input = *wxul;
		}

	delete wxul;
	if( m_branch->GetSelection() == 1)
		*offset += cursor_offset;
	else if( m_branch->GetSelection() == 2)
		*offset = filesize - *offset;
	if( *offset < 0 )
		*offset = 0;

	EndModal( wxID_OK );
	}

void GotoDialog::OnConvert( wxCommandEvent& event ){
	if( event.GetId() == m_dec->GetId() && !is_olddec ){	//old value is hex, new value is dec
		wxULongLong_t *wxul = new wxULongLong_t(0);
		m_textCtrlOffset->GetValue().ToULongLong( wxul, 16 );
		*offset = *wxul;
		delete wxul;
		m_textCtrlOffset->SetValue( wxString::Format(_T("%ld"),*offset) );
// TODO (death#1#): myDialogVector->goto_hex = 0;
		is_olddec = true;
		}
	else if( event.GetId() == m_hex->GetId() && is_olddec ){	//old value is dec, new value is hex
		wxULongLong_t *wxul = new wxULongLong_t(0);
		m_textCtrlOffset->GetValue().ToULongLong( wxul, 10 );
		*offset = *wxul;
		delete wxul;
		m_textCtrlOffset->SetValue( wxString::Format(_T("%X"),*offset) );
		is_olddec = false;
		}
	event.Skip(false);
	}

// TODO (death#1#):Paint 4 Find All
// TODO (death#1#):Make ComboBox remember old values
// TODO (death#1#):Remember options last state
FindDialog::FindDialog( wxWindow* _parent, FAL *_findfile, wxString title ):FindDialogGui( _parent, wxID_ANY, title){
	parent = static_cast< HexEditor* >(_parent);
	findfile = _findfile;
	m_comboBoxSearch->SetFocus();
	}

void FindDialog::EventHandler( wxCommandEvent& event ){
	parent->HighlightArray.Clear();
	if( event.GetId() == btnFind->GetId() or event.GetId() == m_comboBoxSearch->GetId())
		OnFind();
	else if( event.GetId() == m_searchtype->GetId()){
		m_searchtype->GetSelection() == 1 ? chkMatchCase->Enable(false) : chkMatchCase->Enable(true) ;
		}
	else if( event.GetId() == btnFindAll->GetId() )
		OnFindAll();
//	else if( event.GetId() == btnFindPrev->GetId() )
//		OnFindPrev();
	else
		wxBell();
	}

bool FindDialog::OnFind( bool internal ) {
	uint64_t found = NANINT;
	uint64_t search_size = 0;
	//prepare Operator
	unsigned options = 0;
	options |= m_searchtype->GetSelection() == 0 ? SEARCH_TEXT : SEARCH_HEX;
	options |= chkWrapAround->GetValue() ? SEARCH_WRAPAROUND : 0;
	options |= chkSearchBackwards->GetValue() ? SEARCH_BACKWARDS : 0;
	if(options & SEARCH_TEXT) {
		options |= chkUTF8->GetValue() ? SEARCH_UTF8 : 0;
		options |= chkMatchCase->GetValue() ? SEARCH_MATCHCASE : 0;
		search_size = m_comboBoxSearch->GetValue().Len();
		found = FindText( m_comboBoxSearch->GetValue(), parent->CursorOffset()+1, options );
		}
	else { //SEARCH_HEX
		wxString hexval = m_comboBoxSearch->GetValue();

		for( unsigned i = 0 ; i < hexval.Len() ; i++ )
			if( !isxdigit( hexval[i] ) or hexval == ' ' ) { //Not hexadecimal!
				wxMessageBox(_("Search value is not hexadecimal!"), _("Format Error!"), wxOK, this );
				wxBell();
				return false;
				}
		//Remove all space chars and update the Search value
		while( hexval.find(' ') != -1 )
			hexval.Remove( hexval.find(' '),1);
		if( hexval.Len() % 2 )//there is odd hex value, must be even for byte search!
			hexval = wxChar('0')+hexval;
		m_comboBoxSearch->SetValue(hexval.Upper());

		wxMemoryBuffer search_binary = wxHexCtrl::HexToBin( m_comboBoxSearch->GetValue());
		search_size = search_binary.GetDataLen();
		found = FindBinary( search_binary, parent->CursorOffset()+1, options );
		}

	if( found != NANINT ) {
		parent->Goto( found );
		parent->Select( found,  found+search_size-1 );
		return true;
		}

	else if( !internal )
		wxMessageBox(_("Search value not found"), _("Nothing found!"), wxOK, this );
	return false;
	}

uint64_t FindDialog::FindText( wxString target, uint64_t start_from, unsigned options ){
	if( target.IsAscii() ){
		wxMemoryBuffer textsrc;
		textsrc.AppendData( target.ToAscii() , target.Length() );
		return FindBinary( textsrc, start_from, options );
		}
	else{
		wxBell();
		return NANINT;
		}
// TODO (death#1#): Find in UTF?
	}

// TODO (death#1#): New Find as "bool FindText/Bin( &uint64_t )
// TODO (death#1#): Implement Search_Backwards
uint64_t FindDialog::FindBinary( wxMemoryBuffer target, uint64_t from, unsigned options ){
	if( target.GetDataLen() == 0 )
		return NANINT;

	uint64_t current_offset = from;
	int search_step = parent->FileLength() < MB ? parent->FileLength() : MB ;
	findfile->Seek( current_offset, wxFromStart );
	char* buffer = new char [search_step];
	if(buffer == NULL) return NANINT;
	// TODO (death#6#): insert error check message here
	int found = -1;
	int readed = 0;
// TODO (death#1#): Seach bar with gauge???
	//Search step 1: From cursor to file end.
	do{
		findfile->Seek( current_offset, wxFromStart );
		readed = findfile->Read( buffer , search_step );
		found = SearchAtBuffer( buffer, readed, static_cast<char*>(target.GetData()),target.GetDataLen(), options );//Makes raw search here
		if(found >= 0){
			delete buffer;
			return current_offset+found;
			}
		else
			current_offset +=readed - target.GetDataLen() - 1; //Unprocessed bytes
		}while(readed >= search_step); //indicate also file end.

	//Search step 2: From start to file end.
	if( options & SEARCH_WRAPAROUND ){
		current_offset = 0;
		do{
			findfile->Seek(current_offset, wxFromStart );
			readed = findfile->Read( buffer , search_step );
			if( readed + current_offset > from )
				search_step = readed + current_offset - from - 1;
			found = SearchAtBuffer( buffer, readed, static_cast<char*>(target.GetData()),target.GetDataLen(), options );//Makes raw search here
			if(found >= 0){
				delete buffer;
				return current_offset+found;
				}
			else
				current_offset +=readed - target.GetDataLen() - 1; //Unprocessed bytes
			}while(current_offset + readed < from); //Search until cursor
		}


	delete buffer;
	return NANINT;
	}

// TODO (death#1#): Needed to be checked.
void FindDialog::OnFindAll() {
	parent->HighlightArray.Clear();
	uint64_t found = NANINT;
	uint64_t search_size = 0;

	wxProgressDialog progress_gauge(_("wxHexEditor Searching") , _("Finding matches... "), 100,  this, wxPD_SMOOTH|wxPD_REMAINING_TIME|wxPD_CAN_ABORT|wxPD_AUTO_HIDE );
	progress_gauge.SetWindowStyleFlag( progress_gauge.GetWindowStyleFlag()|wxSTAY_ON_TOP|wxMINIMIZE_BOX );

// TODO (death#1#): Search icon	//wxIcon search_ICON (?_xpm);
	//progress_gauge.SetIcon(search_ICON);

	unsigned options = 0;
	options |= m_searchtype->GetSelection() == 0 ? SEARCH_TEXT : SEARCH_HEX;
	options |= chkWrapAround->GetValue() ? SEARCH_WRAPAROUND : 0;
	options |= chkSearchBackwards->GetValue() ? SEARCH_BACKWARDS : 0;

	int mode = 0;
	wxString search_string;
	wxMemoryBuffer search_binary;
	if(options & SEARCH_TEXT) {
		mode = SEARCH_TEXT;
		options |= chkUTF8->GetValue() ? SEARCH_UTF8 : 0;
		options |= chkMatchCase->GetValue() ? SEARCH_MATCHCASE : 0;
		search_size = m_comboBoxSearch->GetValue().Len();
		search_string = m_comboBoxSearch->GetValue();
		parent->Goto(0);
		}

	else {
		mode = SEARCH_HEX;
		wxString hexval = m_comboBoxSearch->GetValue();
		parent->Goto(0);
		for( unsigned i = 0 ; i < hexval.Len() ; i++ )
			if( !isxdigit( hexval[i] ) or hexval == ' ' ) { //Not hexadecimal!
				wxMessageBox(_("Search value is not hexadecimal!"), _("Format Error!"), wxOK, this );
				wxBell();
				}
		//Remove all space chars and update the Search value
		while( hexval.find(' ') != -1 )
			hexval.Remove( hexval.find(' '),1);
		if( hexval.Len() % 2 )//there is odd hex value, must be even for byte search!
			hexval = wxChar('0')+hexval;
		m_comboBoxSearch->SetValue(hexval.Upper());
		search_binary = wxHexCtrl::HexToBin( m_comboBoxSearch->GetValue());
		search_size = search_binary.GetDataLen();
		}

	//Merged Search loop!
	for(int64_t i=0; i+search_size <parent->FileLength(); i=i+search_size) {
		wxYield();
		if( ! progress_gauge.Update(i*search_size*100/parent->FileLength()))		// update progress and break on abort
			break;

		uint64_t temp=found;
		if( mode == SEARCH_TEXT )
			found = FindText( search_string, i+1, options );
		else  //mode == SEARCH_HEX
			found = FindBinary( search_binary, i+1 ,options );

#ifdef _DEBUG_
		wxString mystring = wxString::Format(wxT("%d"),found);//Debug
#else
		wxString mystring = wxEmptyString;
#endif

		if(found!=-1&& temp!=found) {
			TagElement *mytag=new TagElement(found,found+search_size-1,mystring,*wxBLACK, wxColour(255,255,0,0) );
			parent->HighlightArray.Add(mytag);
			}
		}

	progress_gauge.Hide();

	if(parent->HighlightArray.GetCount()<0) {
		wxMessageBox(_("Search value not found"), _("Nothing found!"), wxOK, this );
		}
	else {
		parent->Select(found,found+search_size);
		wxMessageBox(wxString::Format(_("Found %d matches."),parent->HighlightArray.GetCount()), _("Find All Done!"), wxOK, this );
		}
	}

// TODO (death#9#): Implement better search algorithm. (Like one using OpenCL and one using OpenMP) :)
//WARNING! THIS FUNCTION WILL CHANGE BFR and/or SEARCH strings if SEARCH_MATCHCASE not selected as an option!
int FindDialog::SearchAtBuffer( char *bfr, int bfr_size, char* search, int search_size, unsigned options ){	// Dummy search algorithm\ Yes yes I know there are better ones but I think this enought for now.
	if( bfr_size < search_size )
		return -1;

	if( options & SEARCH_HEX or (options & SEARCH_TEXT and options & SEARCH_MATCHCASE) ){
		for(int i=0 ; i < bfr_size - search_size + 1 ; i++ )
			if(! memcmp( bfr+i, search, search_size ))
				return i;
		}
	else // if( options & SEARCH_TEXT ) and not ( options & SEARCH_MATCHCASE )
		{
		for( int i = 0 ; i < bfr_size; i++)
			bfr[i]=tolower(bfr[i]);
		for( int i = 0 ; i < search_size; i++)
			search[i]=tolower(search[i]);
		for(int i=0 ; i < bfr_size - search_size + 1 ; i++ )
			if(! memcmp( bfr+i, search, search_size ))
				return i;
		}
	return -1;
	}

ReplaceDialog::ReplaceDialog( wxWindow* parent, FAL *find_file, wxString title ):FindDialog( parent, find_file, title ){
	m_comboBoxReplace->Show();
	m_static_replace->Show();
	btnReplace->Show();
	btnReplaceAll->Show();
	Fit();
	}

int ReplaceDialog::OnReplace( bool internal ){
	if( parent->select->IsState( parent->select->SELECT_FALSE ) ) {
		if( OnFind( internal ) == false )
			return 0;
		else
			return -1;
		}

	else{
		if( m_searchtype->GetSelection() == 0 ){//text search
			if( parent->select->GetSize() == m_comboBoxReplace->GetValue().Len() ){
				parent->FileAddDiff( parent->CursorOffset(), m_comboBoxReplace->GetValue().ToAscii(), m_comboBoxReplace->GetValue().Len());
				parent->select->SetState( parent->select->SELECT_FALSE );
				parent->Reload();
				return 1;
				}
			else{
				wxMessageBox(_("Search and Replace sizes are not equal!\nReplacing with differnet sizez are avoided."), _("Error!"), wxOK, this);
				return 0;
				}
			}
		else{ //hex search
			wxMemoryBuffer search_binary = wxHexCtrl::HexToBin( m_comboBoxReplace->GetValue());
			if( parent->select->GetSize() == search_binary.GetDataLen() ){
				parent->FileAddDiff( parent->CursorOffset(), static_cast<char*>(search_binary.GetData()) ,search_binary.GetDataLen() );
				parent->select->IsState( parent->select->SELECT_FALSE );
				parent->Reload();
				return 1;
				}
			else{
				wxMessageBox(_("Search and Replace sizes are not equal!\nReplacing with differnet sizez are avoided."), _("Error!"), wxOK, this);
				return 0;
				}
			}
		}
	}

int ReplaceDialog::OnReplaceAll( void ){
	int counter=0;
	int x;
	while( true ){
		x = OnReplace( true );
		if( x == 1 )
			counter++;
		if( x == 0 )
			break;
		if( x == -1 )
			continue;
		}
	if( counter > 0)
		wxMessageBox(wxString::Format(_("%d records changed."), counter ), _("Info!"), wxOK, this);
	return counter;
	}

void ReplaceDialog::EventHandler( wxCommandEvent& event ){
	int id = event.GetId();
	if( id == btnFind->GetId() )
		OnFind();
	else if( id == btnFindAll->GetId() )
		OnFindAll();
//	else if( id == btnFindPrev->GetId() )
//		OnFindPrev();
	else if( id == btnReplace->GetId() )
		OnReplace();
	else if( id == btnReplaceAll->GetId() )
		OnReplaceAll();
	else
		wxBell();
	}
