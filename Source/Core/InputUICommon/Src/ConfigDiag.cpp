// Copyright (C) 2010 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#include "ConfigDiag.h"

#define _connect_macro_(b, f, c, s)	(b)->Connect(wxID_ANY, (c), wxCommandEventHandler( f ), (wxObject*)0, (wxEvtHandler*)s)
#define WXSTR_FROM_STR(s)	(wxString::FromAscii((s).c_str()))
// ToAscii was causing probs with some extended ascii characters, To8BitData seems to work
#define STR_FROM_WXSTR(w)	(std::string((w).To8BitData()))


void GamepadPage::ConfigExtension( wxCommandEvent& event )
{
	ControllerEmu::Extension* const ex = ((ExtensionButton*)event.GetEventObject())->extension;

	// show config diag, if "none" isn't selected
	if ( ex->switch_extension )
	{
		wxDialog* const dlg = new wxDialog( this, -1, wxString::FromAscii(ex->attachments[ex->switch_extension]->GetName().c_str()), wxDefaultPosition );
		wxPanel* const pnl = new wxPanel( dlg, -1, wxDefaultPosition );
		wxBoxSizer* const pnl_szr = new wxBoxSizer( wxHORIZONTAL );

		const std::size_t orig_size = control_groups.size();

		ControlGroupsSizer* const szr = new ControlGroupsSizer( ex->attachments[ex->switch_extension], pnl, this, &control_groups );
		pnl->SetSizerAndFit( szr );	// needed
		pnl_szr->Add( pnl, 0, wxLEFT, 5 );
		dlg->SetSizerAndFit( pnl_szr );	// needed

		dlg->Center();

		dlg->ShowModal();
		dlg->Destroy();

		// remove the new groups that were just added, now that the window closed
		control_groups.resize( orig_size );
	}
}

PadSettingExtension::PadSettingExtension( wxWindow* const parent, ControllerEmu::Extension* const ext )
	: wxChoice( parent, -1 )
	, extension(ext)
{

	std::vector<ControllerEmu*>::const_iterator
		i = extension->attachments.begin(),
		e = extension->attachments.end();

	for ( ; i!=e; ++i )
		Append( wxString::FromAscii( (*i)->GetName().c_str() ) );

	UpdateGUI();
}

void PadSettingExtension::UpdateGUI()
{
	Select( extension->switch_extension );
}

void PadSettingExtension::UpdateValue()
{
	extension->switch_extension = GetSelection();
}

PadSettingCheckBox::PadSettingCheckBox( wxWindow* const parent, ControlState& _value, const char* const label )
	: wxCheckBox( parent, -1, wxString::FromAscii( label ), wxDefaultPosition )
	, value(_value)
{
	UpdateGUI();
}

void PadSettingCheckBox::UpdateGUI()
{
	SetValue( value > 0 );
}

void PadSettingCheckBox::UpdateValue()
{
	// 0.01 so its saved to the ini file as just 1. :(
	value = 0.01 * GetValue();
}

void PadSettingChoice::UpdateGUI()
{
	SetValue(value * 100);
}

void PadSettingChoice::UpdateValue()
{
	value = float(GetValue()) / 100;
}

ControlDialog::ControlDialog(GamepadPage* const parent, InputPlugin& plugin, ControllerInterface::ControlReference* const ref)
	: wxDialog(parent, -1, wxT("Configure Control"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, control_reference(ref)
	, m_plugin(plugin)
	, m_parent(parent)
{
	m_devq = m_parent->controller->default_device;

	// GetStrings() sounds slow :/
	//device_cbox = new wxComboBox(this, -1, WXSTR_FROM_STR(ref->device_qualifier.ToString()), wxDefaultPosition, wxSize(256,-1), parent->device_cbox->GetStrings(), wxTE_PROCESS_ENTER);
	device_cbox = new wxComboBox(this, -1, WXSTR_FROM_STR(m_devq.ToString()), wxDefaultPosition, wxSize(256,-1), parent->device_cbox->GetStrings(), wxTE_PROCESS_ENTER);

	_connect_macro_( device_cbox, ControlDialog::SetDevice, wxEVT_COMMAND_COMBOBOX_SELECTED, this );
	_connect_macro_( device_cbox, ControlDialog::SetDevice, wxEVT_COMMAND_TEXT_ENTER, this );

	wxStaticBoxSizer* const control_chooser = CreateControlChooser( this, parent );

	wxStaticBoxSizer* const d_szr = new wxStaticBoxSizer( wxVERTICAL, this, wxT("Device") );
	d_szr->Add( device_cbox, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* const szr = new wxBoxSizer( wxVERTICAL );
	szr->Add( d_szr, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	szr->Add( control_chooser, 1, wxEXPAND|wxALL, 5 );

	SetSizerAndFit( szr );	// needed

	UpdateGUI();
}

ControlButton::ControlButton( wxWindow* const parent, ControllerInterface::ControlReference* const _ref, const unsigned int width, const std::string& label )
: wxButton( parent, -1, wxT(""), wxDefaultPosition, wxSize( width,20) )
, control_reference( _ref )
{
	if ( label.empty() )
		SetLabel(WXSTR_FROM_STR(_ref->expression));
	else
		SetLabel(WXSTR_FROM_STR(label));
}

void InputConfigDialog::UpdateProfileComboBox()
{
	std::string pname( File::GetUserPath(D_CONFIG_IDX) );
	pname += PROFILES_PATH;
	pname += m_plugin.profile_name;

	CFileSearch::XStringVector exts;
	exts.push_back("*.ini");
	CFileSearch::XStringVector dirs;
	dirs.push_back( pname );
	CFileSearch cfs( exts, dirs );
	const CFileSearch::XStringVector& sv = cfs.GetFileNames();

	wxArrayString strs;
	CFileSearch::XStringVector::const_iterator si = sv.begin(),
		se = sv.end();
	for ( ; si!=se; ++si )
	{
		std::string str( si->begin() + si->find_last_of('/') + 1 , si->end() - 4 ) ;
		strs.push_back( wxString::FromAscii( str.c_str() ) );
	}

	std::vector< GamepadPage* >::iterator i = m_padpages.begin(),
		e = m_padpages.end();
	for ( ; i != e; ++i  )
	{
		(*i)->profile_cbox->Clear();
		(*i)->profile_cbox->Append(strs);
	}
}

void InputConfigDialog::UpdateControlReferences()
{
	std::vector< GamepadPage* >::iterator i = m_padpages.begin(),
		e = m_padpages.end();
	for ( ; i != e; ++i  )
		(*i)->controller->UpdateReferences( m_plugin.controller_interface );
}

void InputConfigDialog::ClickSave( wxCommandEvent& event )
{
	m_plugin.SaveConfig();
	Close();
}

void ControlDialog::UpdateListContents()
{
	control_lbox->Clear();

	std::vector<ControllerInterface::Device*>::const_iterator di = 
		FindDevice(m_plugin.controller_interface.Devices(), m_devq);	

	if (m_plugin.controller_interface.Devices().end() != di)
	{
		if (control_reference->is_input)
		{
			// for inputs
			std::vector<ControllerInterface::Device::Input*>::const_iterator
			   	i = (*di)->Inputs().begin(),
				e = (*di)->Inputs().end();
			for (; i!=e; ++i)
				control_lbox->Append(WXSTR_FROM_STR((*i)->GetName()));
		}
		else
		{
			// for outputs
			std::vector<ControllerInterface::Device::Output*>::const_iterator
			   	i = (*di)->Outputs().begin(),
				e = (*di)->Outputs().end();
			for (; i!=e; ++i)
				control_lbox->Append(WXSTR_FROM_STR((*i)->GetName()));
		}
	}
}

void ControlDialog::SelectControl(const std::string& name)
{
	//UpdateGUI();

	const int f = control_lbox->FindString(WXSTR_FROM_STR(name));
	if (f >= 0)
		control_lbox->Select(f);
}

void ControlDialog::UpdateGUI()
{
	// update textbox
	textctrl->SetValue(WXSTR_FROM_STR(control_reference->expression));

	// updates the "bound controls:" label
	size_t bound = control_reference->BoundCount();
	std::ostringstream ss;
	ss << "Bound Controls: ";
	if ( bound ) ss << bound; else ss << "None";
	m_bound_label->SetLabel( wxString::FromAscii(ss.str().c_str()) );
};

void GamepadPage::UpdateGUI()
{
	device_cbox->SetValue(WXSTR_FROM_STR(controller->default_device.ToString()));

	std::vector< ControlGroupBox* >::const_iterator g = control_groups.begin(),
		ge = control_groups.end();
	for ( ; g!=ge; ++g )
	{
		// buttons
		std::vector<ControlButton*>::const_iterator i = (*g)->control_buttons.begin()
			, e = (*g)->control_buttons.end();
		for ( ; i!=e; ++i )
			//if (std::string::npos == (*i)->control_reference->expression.find_first_of("`|&!#"))
				(*i)->SetLabel(WXSTR_FROM_STR((*i)->control_reference->expression));
			//else
				//(*i)->SetLabel(wxT("..."));

		// cboxes
		std::vector<PadSetting*>::const_iterator si = (*g)->options.begin()
			, se = (*g)->options.end();
		for ( ; si!=se; ++si )
			(*si)->UpdateGUI();
	}
}

void GamepadPage::ClearAll( wxCommandEvent& event )
{
	m_plugin.controls_crit.Enter();		// enter

	// just load an empty ini section to clear everything :P
	IniFile::Section section;
	controller->LoadConfig(&section);

	// no point in using the real ControllerInterface i guess
	ControllerInterface face;
	controller->UpdateReferences(face);

	UpdateGUI();

	m_plugin.controls_crit.Leave();		// leave
}

void ControlDialog::SetControl( wxCommandEvent& event )
{
	control_reference->expression = STR_FROM_WXSTR(textctrl->GetValue());

	m_plugin.controls_crit.Enter();		// enter
	m_plugin.controller_interface.UpdateReference(control_reference, m_parent->controller->default_device);
	m_plugin.controls_crit.Leave();		// leave

	UpdateGUI();
}

void GamepadPage::SetDevice( wxCommandEvent& event )
{
	controller->default_device.FromString(STR_FROM_WXSTR(device_cbox->GetValue()));
	
	// show user what it was validated as
	device_cbox->SetValue(WXSTR_FROM_STR(controller->default_device.ToString()));

	// this will set all the controls to this default device
	controller->UpdateDefaultDevice();

	// update references
	m_plugin.controls_crit.Enter();		// enter
	controller->UpdateReferences( m_plugin.controller_interface );
	m_plugin.controls_crit.Leave();		// leave
}

void ControlDialog::SetDevice( wxCommandEvent& event )
{
	m_devq.FromString(STR_FROM_WXSTR(device_cbox->GetValue()));
	
	// show user what it was validated as
	device_cbox->SetValue(WXSTR_FROM_STR(m_devq.ToString()));

	// update gui
	UpdateListContents();
}

void ControlDialog::ClearControl(wxCommandEvent& event)
{
	control_reference->expression.clear();

	m_plugin.controls_crit.Leave();		// enter
	m_plugin.controller_interface.UpdateReference(control_reference, m_parent->controller->default_device);
	m_plugin.controls_crit.Leave();		// leave

	UpdateGUI();
}

void ControlDialog::AppendControl(wxCommandEvent& event)
{
	const int num = control_lbox->GetSelection();

	if (num < 0)
		return;

	// o boy!, hax
	const wxString lbl = ((wxButton*)event.GetEventObject())->GetLabel();

	wxString expr = textctrl->GetLabel();

	// append the operator to the expression
	if (wxT('!') == lbl[0] || false == expr.empty())
		expr += lbl[0];

	// non-default device
	if (false == (m_devq == m_parent->controller->default_device))
		expr.append(wxT('`')).append(WXSTR_FROM_STR(m_devq.ToString())).append(wxT('`'));

	// append the control name
	expr += control_lbox->GetString(num);

	control_reference->expression = STR_FROM_WXSTR(expr);

	m_plugin.controls_crit.Leave();		// enter
	m_plugin.controller_interface.UpdateReference(control_reference, m_parent->controller->default_device);
	m_plugin.controls_crit.Leave();		// leave

	UpdateGUI();
}

void GamepadPage::AdjustSetting( wxCommandEvent& event )
{
	m_plugin.controls_crit.Enter();		// enter

	// updates the setting value from the GUI control
	(dynamic_cast<PadSetting*>(event.GetEventObject()))->UpdateValue();

	m_plugin.controls_crit.Leave();		// leave
}

void GamepadPage::AdjustControlOption( wxCommandEvent& event )
{
	m_plugin.controls_crit.Enter();		// enter / prolly fine not being here

	m_control_dialog->control_reference->range = (ControlState)(m_control_dialog->range_slider->GetValue()) / SLIDER_TICK_COUNT;

	m_plugin.controls_crit.Leave();		// leave
}

void GamepadPage::ConfigControl( wxCommandEvent& event )
{
	m_control_dialog = new ControlDialog(this, m_plugin, ((ControlButton*)event.GetEventObject())->control_reference);
	m_control_dialog->ShowModal();
	m_control_dialog->Destroy();

	// update changes that were made in the dialog
	UpdateGUI();
}

void GamepadPage::ClearControl( wxCommandEvent& event )
{
	ControlButton* const btn = (ControlButton*)event.GetEventObject();
	btn->control_reference->expression.clear();
	//btn->control_reference->device_qualifier = controller->default_device;

	m_plugin.controls_crit.Enter();
	controller->UpdateReferences( m_plugin.controller_interface );
	m_plugin.controls_crit.Leave();

	// update changes
	UpdateGUI();
}

void ControlDialog::DetectControl(wxCommandEvent& event)
{
	wxButton* const btn = (wxButton*)event.GetEventObject();
	const wxString lbl = btn->GetLabel();

	std::vector<ControllerInterface::Device*>::const_iterator di = 
		FindDevice(m_plugin.controller_interface.Devices(), m_devq);

	if (m_plugin.controller_interface.Devices().end() != di)
	{
		btn->SetLabel(wxT("[ waiting ]"));

		m_plugin.controls_crit.Enter();		// enter
		ControllerInterface::Device::Control* const ctrl = control_reference->Detect(DETECT_WAIT_TIME, *di);

		// if we got input, select it in the list
		if (ctrl)
			SelectControl(ctrl->GetName());

		m_plugin.controls_crit.Leave();		// leave

		btn->SetLabel(lbl);
	}
}

void GamepadPage::DetectControl( wxCommandEvent& event )
{
	ControlButton* btn = (ControlButton*)event.GetEventObject();

	// find device :/
	const std::vector<ControllerInterface::Device*>::const_iterator di =
		FindDevice(m_plugin.controller_interface.Devices(), controller->default_device);

	if (m_plugin.controller_interface.Devices().end() != di)
	{
		btn->SetLabel(wxT("[ waiting ]"));

		m_plugin.controls_crit.Enter();		// enter
		ControllerInterface::Device::Control* const ctrl = btn->control_reference->Detect(DETECT_WAIT_TIME, *di);

		// if we got input, update expression and reference
		if (ctrl)
		{
			btn->control_reference->expression = ctrl->GetName();
			m_plugin.controller_interface.UpdateReference(btn->control_reference, controller->default_device);
		}

		m_plugin.controls_crit.Leave();		// leave

		btn->SetLabel(WXSTR_FROM_STR(btn->control_reference->expression));
	}
}

wxStaticBoxSizer* ControlDialog::CreateControlChooser( wxWindow* const parent, wxWindow* const eventsink )
{
	wxStaticBoxSizer* const main_szr = new wxStaticBoxSizer(wxVERTICAL, parent, control_reference->is_input ? wxT("Input") : wxT("Output"));

	textctrl = new wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, wxSize(-1, 48), wxTE_PROCESS_ENTER | wxTE_MULTILINE);
	_connect_macro_(textctrl, ControlDialog::SetControl, wxEVT_COMMAND_TEXT_ENTER, parent);

	wxButton* const detect_button = new wxButton( parent, -1, control_reference->is_input ? wxT("Detect") : wxT("Test") );
	wxButton* const clear_button = new  wxButton( parent, -1, wxT("Clear"), wxDefaultPosition );
	wxButton* const set_button = new wxButton( parent, -1, wxT("Set")/*, wxDefaultPosition, wxSize( 32, -1 )*/ );

	wxButton* const or_button = new  wxButton( parent, -1, wxT("| OR"), wxDefaultPosition );
	// TODO: check if && is good on other OS
	wxButton* const and_button = new  wxButton( parent, -1, wxT("&& AND"), wxDefaultPosition );
	wxButton* const not_button = new  wxButton( parent, -1, wxT("! NOT"), wxDefaultPosition );
	wxButton* const add_button = new  wxButton( parent, -1, wxT("# ADD"), wxDefaultPosition );

	_connect_macro_(or_button, ControlDialog::AppendControl, wxEVT_COMMAND_BUTTON_CLICKED, parent);
	_connect_macro_(and_button, ControlDialog::AppendControl, wxEVT_COMMAND_BUTTON_CLICKED, parent);
	_connect_macro_(not_button, ControlDialog::AppendControl, wxEVT_COMMAND_BUTTON_CLICKED, parent);
	_connect_macro_(add_button, ControlDialog::AppendControl, wxEVT_COMMAND_BUTTON_CLICKED, parent);

	control_lbox = new wxListBox(parent, -1, wxDefaultPosition, wxSize(128, 128));

	wxBoxSizer* const button_sizer = new wxBoxSizer(wxVERTICAL);
	button_sizer->Add(detect_button, 1, 0, 5);
	button_sizer->Add(clear_button, 1, 0, 5);
	button_sizer->Add(or_button, 1, 0, 5);
	button_sizer->Add(and_button, 1, 0, 5);
	button_sizer->Add(not_button, 1, 0, 5);
	button_sizer->Add(add_button, 1, 0, 5);
	button_sizer->Add(set_button, 1, 0, 5);

	range_slider = new wxSlider( parent, -1, SLIDER_TICK_COUNT, 0, SLIDER_TICK_COUNT * 2, wxDefaultPosition, wxDefaultSize, wxSL_TOP | wxSL_LABELS /*| wxSL_AUTOTICKS*/ );

	range_slider->SetValue( control_reference->range * SLIDER_TICK_COUNT );

	_connect_macro_( detect_button, ControlDialog::DetectControl, wxEVT_COMMAND_BUTTON_CLICKED, parent);
	_connect_macro_( clear_button, ControlDialog::ClearControl, wxEVT_COMMAND_BUTTON_CLICKED, parent);
	_connect_macro_( set_button, ControlDialog::SetControl, wxEVT_COMMAND_BUTTON_CLICKED, parent);

	_connect_macro_( range_slider, GamepadPage::AdjustControlOption, wxEVT_SCROLL_CHANGED, eventsink);
	wxStaticText* const range_label = new wxStaticText( parent, -1, wxT("Range"));
	m_bound_label = new wxStaticText( parent, -1, wxT("") );

	wxBoxSizer* const range_sizer = new wxBoxSizer( wxHORIZONTAL );
	range_sizer->Add( range_label, 0, wxCENTER|wxLEFT, 5 );
	range_sizer->Add( range_slider, 1, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* const ctrls_sizer = new wxBoxSizer(wxHORIZONTAL);
	ctrls_sizer->Add(control_lbox, 1, wxEXPAND, 0);
	ctrls_sizer->Add(button_sizer, 0, wxEXPAND, 0);

	main_szr->Add(range_sizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
	main_szr->Add(ctrls_sizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5);
	main_szr->Add(textctrl, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5);
	main_szr->Add(m_bound_label, 0, wxCENTER, 0);

	UpdateListContents();

	return main_szr;
}

void GamepadPage::GetProfilePath(std::string& path)
{
	const wxString& name = profile_cbox->GetValue();
	if (false == name.empty())
	{
		// TODO: check for dumb characters maybe

		path = File::GetUserPath(D_CONFIG_IDX);
		path += PROFILES_PATH;
		path += m_plugin.profile_name;
		path += '/';
		path += STR_FROM_WXSTR(profile_cbox->GetValue());
		path += ".ini";
	}
}

void GamepadPage::LoadProfile( wxCommandEvent& event )
{
	std::string fname;
	GamepadPage::GetProfilePath(fname);

	if (false == File::Exists(fname.c_str()))
		return;

	IniFile inifile;
	inifile.Load(fname);

	m_plugin.controls_crit.Enter();
	controller->LoadConfig(inifile.GetOrCreateSection("Profile"));
	controller->UpdateReferences(m_plugin.controller_interface);
	m_plugin.controls_crit.Leave();

	UpdateGUI();
}

void GamepadPage::SaveProfile( wxCommandEvent& event )
{
	std::string fname;
	GamepadPage::GetProfilePath(fname);

	if (false == fname.empty())
	{
		IniFile inifile;
		controller->SaveConfig(inifile.GetOrCreateSection("Profile"));
		inifile.Save(fname);
	}

	m_config_dialog->UpdateProfileComboBox();
}

void GamepadPage::DeleteProfile( wxCommandEvent& event )
{
	std::string fname;
	GamepadPage::GetProfilePath(fname);

	const char* const fnamecstr = fname.c_str();

	if (File::Exists(fnamecstr))
		File::Delete(fnamecstr);

	m_config_dialog->UpdateProfileComboBox();
}

void InputConfigDialog::UpdateDeviceComboBox()
{
	std::vector< GamepadPage* >::iterator i = m_padpages.begin(),
		e = m_padpages.end();
	ControllerInterface::DeviceQualifier dq;
	for ( ; i != e; ++i  )
	{
		(*i)->device_cbox->Clear();
		std::vector<ControllerInterface::Device*>::const_iterator di = m_plugin.controller_interface.Devices().begin(),
			de = m_plugin.controller_interface.Devices().end();
		for ( ; di!=de; ++di )
		{
			dq.FromDevice( *di );
			(*i)->device_cbox->Append(WXSTR_FROM_STR(dq.ToString()));
		}
		(*i)->device_cbox->SetValue(WXSTR_FROM_STR((*i)->controller->default_device.ToString()));
	}
}

void GamepadPage::RefreshDevices( wxCommandEvent& event )
{
	m_plugin.controls_crit.Enter();		// enter

	// refresh devices
	// TODO: remove hackery of not deinting SDL
	m_plugin.controller_interface.DeInit(true);
	m_plugin.controller_interface.Init();

	// update all control references
	m_config_dialog->UpdateControlReferences();

	// update device cbox
	m_config_dialog->UpdateDeviceComboBox();

	m_plugin.controls_crit.Leave();		// leave
}

ControlGroupBox::ControlGroupBox( ControllerEmu::ControlGroup* const group, wxWindow* const parent, wxWindow* const eventsink )
	: wxStaticBoxSizer( wxVERTICAL, parent, wxString::FromAscii( group->name ) )
{

	control_group = group;
	static_bitmap = NULL;

	wxFont m_SmallFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	std::vector<ControllerEmu::ControlGroup::Control*>::iterator
		ci = group->controls.begin(),
		ce = group->controls.end();
	for ( ; ci != ce; ++ci)
	{

		wxStaticText* const label = new wxStaticText(parent, -1, wxString::FromAscii((*ci)->name)/*.append(wxT(" :"))*/ );
		
		ControlButton* const control_button = new ControlButton(parent, (*ci)->control_ref, 80);
		control_button->SetFont(m_SmallFont);

		controls.push_back(control_button);
		control_buttons.push_back(control_button);

		if ((*ci)->control_ref->is_input)
		{
			control_button->SetToolTip(wxT("Left-click to detect input.\nMiddle-click to clear.\nRight-click for more options."));
			_connect_macro_( control_button, GamepadPage::DetectControl, wxEVT_COMMAND_BUTTON_CLICKED, eventsink );
		}
		else
		{
			control_button->SetToolTip(wxT("Left/Right-click for more options.\nMiddle-click to clear."));
			_connect_macro_( control_button, GamepadPage::ConfigControl, wxEVT_COMMAND_BUTTON_CLICKED, eventsink );
		}

		_connect_macro_( control_button, GamepadPage::ClearControl, wxEVT_MIDDLE_DOWN, eventsink );
		_connect_macro_( control_button, GamepadPage::ConfigControl, wxEVT_RIGHT_UP, eventsink );

		wxBoxSizer* const control_sizer = new wxBoxSizer( wxHORIZONTAL );
		control_sizer->AddStretchSpacer( 1 );
		control_sizer->Add( label, 0, wxCENTER | wxRIGHT, 3 );
		control_sizer->Add( control_button, 0, 0, 0 );

		Add( control_sizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 3 );

	}

	wxMemoryDC dc;

	switch ( group->type )
	{
	case GROUP_TYPE_STICK :
	case GROUP_TYPE_TILT :
	case GROUP_TYPE_CURSOR :
	case GROUP_TYPE_FORCE :
		{
			wxBitmap bitmap(64, 64);
			dc.SelectObject(bitmap);
			dc.Clear();
			static_bitmap = new wxStaticBitmap( parent, -1, bitmap, wxDefaultPosition, wxDefaultSize, wxBITMAP_TYPE_BMP );

			std::vector< ControllerEmu::ControlGroup::Setting* >::const_iterator
				i = group->settings.begin(),
				e = group->settings.end();

			wxBoxSizer* const szr = new wxBoxSizer( wxVERTICAL );
			for ( ; i!=e; ++i )
			{
				PadSettingChoice* cbox = new PadSettingChoice( parent, *i );
				_connect_macro_( cbox, GamepadPage::AdjustSetting, wxEVT_COMMAND_SPINCTRL_UPDATED, eventsink );
				options.push_back( cbox );
				szr->Add( new wxStaticText( parent, -1, wxString::FromAscii( (*i)->name ) ) );
				szr->Add( cbox, 0, wxLEFT, 0 );
			}

			wxBoxSizer* const h_szr = new wxBoxSizer( wxHORIZONTAL );
			h_szr->Add( szr, 1, 0, 5 );
			h_szr->Add( static_bitmap, 0, wxALL|wxCENTER, 3 );

			Add( h_szr, 0, wxEXPAND|wxLEFT|wxCENTER|wxTOP, 3 );
		}
		break;
	case GROUP_TYPE_BUTTONS :
		{
			wxBitmap bitmap(int(12*group->controls.size()+1), 12);
			dc.SelectObject(bitmap);
			dc.Clear();
			static_bitmap = new wxStaticBitmap( parent, -1, bitmap, wxDefaultPosition, wxDefaultSize, wxBITMAP_TYPE_BMP );

			PadSettingChoice* const threshold_cbox = new PadSettingChoice( parent, group->settings[0] );
			_connect_macro_( threshold_cbox, GamepadPage::AdjustSetting, wxEVT_COMMAND_SPINCTRL_UPDATED, eventsink );

			threshold_cbox->SetToolTip(wxT("Adjust the analog control pressure required to activate buttons."));

			options.push_back( threshold_cbox );

			wxBoxSizer* const szr = new wxBoxSizer( wxHORIZONTAL );
			szr->Add( new wxStaticText( parent, -1, wxString::FromAscii( group->settings[0]->name ) ), 0, wxCENTER|wxRIGHT, 3 );
			szr->Add( threshold_cbox, 0, wxRIGHT, 3 );

			Add( szr, 0, wxALL|wxCENTER, 3 );
			Add( static_bitmap, 0, wxALL|wxCENTER, 3 );
		}
		break;
	case GROUP_TYPE_MIXED_TRIGGERS :
	case GROUP_TYPE_TRIGGERS :
		{
			int height = (int)(6 * group->controls.size());
			int width = 64+12+1;
			if (GROUP_TYPE_TRIGGERS == group->type)
			{
				height *= 2;
				width = 64;
			}
			wxBitmap bitmap(width, height+1);
			dc.SelectObject(bitmap);
			dc.Clear();
			static_bitmap = new wxStaticBitmap( parent, -1, bitmap, wxDefaultPosition, wxDefaultSize, wxBITMAP_TYPE_BMP );

			std::vector<ControllerEmu::ControlGroup::Setting*>::const_iterator
				i = group->settings.begin(),
				e = group->settings.end();
			for ( ; i!=e; ++i )
			{
				PadSettingChoice* cbox = new PadSettingChoice( parent, *i );
				_connect_macro_( cbox, GamepadPage::AdjustSetting, wxEVT_COMMAND_SPINCTRL_UPDATED, eventsink );
				options.push_back( cbox );
				wxBoxSizer* const szr = new wxBoxSizer( wxHORIZONTAL );
				szr->Add( new wxStaticText( parent, -1, wxString::FromAscii( (*i)->name ) ), 0, wxCENTER|wxRIGHT, 3 );
				szr->Add( cbox, 0, wxRIGHT, 3 );
				Add( szr, 0, wxALL|wxCENTER, 3 );
			}

			Add( static_bitmap, 0, wxALL|wxCENTER, 3 );
		}
		break;
	case GROUP_TYPE_EXTENSION :
		{
			PadSettingExtension* const attachments = new PadSettingExtension( parent, (ControllerEmu::Extension*)group );
			wxButton* const configure = new ExtensionButton( parent, (ControllerEmu::Extension*)group );

			options.push_back( attachments );

			_connect_macro_( attachments, GamepadPage::AdjustSetting, wxEVT_COMMAND_CHOICE_SELECTED, eventsink );
			_connect_macro_( configure, GamepadPage::ConfigExtension, wxEVT_COMMAND_BUTTON_CLICKED, eventsink );

			Add( attachments, 0, wxTOP|wxLEFT|wxRIGHT|wxEXPAND, 3 );
			Add( configure, 0, wxALL|wxEXPAND, 3 );
		}
		break;
	default :
		{
			//options

			std::vector<ControllerEmu::ControlGroup::Setting*>::const_iterator
				i = group->settings.begin(),
				e = group->settings.end();
			for ( ; i!=e; ++i )
			{
				PadSettingCheckBox* setting_cbox = new PadSettingCheckBox( parent, (*i)->value, (*i)->name );
				_connect_macro_( setting_cbox, GamepadPage::AdjustSetting, wxEVT_COMMAND_CHECKBOX_CLICKED, eventsink );
				options.push_back( setting_cbox );

				Add( setting_cbox, 0, wxALL|wxLEFT, 5 );

			}
		}
		break;
	}

	dc.SelectObject(wxNullBitmap);

	//AddStretchSpacer( 0 );
}

ControlGroupsSizer::ControlGroupsSizer( ControllerEmu* const controller, wxWindow* const parent, wxWindow* const eventsink, std::vector<ControlGroupBox*>* groups )
	: wxBoxSizer( wxHORIZONTAL )
{

	wxBoxSizer* stacked_groups = NULL;
	for ( unsigned int i = 0; i < controller->groups.size(); ++i )
	{
		ControlGroupBox* control_group = new ControlGroupBox( controller->groups[i], parent, eventsink );

		if ( control_group->control_buttons.size() > 1 )
		{
			if ( stacked_groups )
				Add( stacked_groups, 0, /*wxEXPAND|*/wxBOTTOM|wxRIGHT, 5 );

			stacked_groups = new wxBoxSizer( wxVERTICAL );
			stacked_groups->Add( control_group, 0, wxEXPAND );
		}
		else
			stacked_groups->Add( control_group, 0, wxEXPAND );

		if ( groups )
			groups->push_back( control_group );
	}

	if ( stacked_groups )
		Add( stacked_groups, 0, /*wxEXPAND|*/wxBOTTOM|wxRIGHT, 5 );

}

GamepadPage::GamepadPage( wxWindow* parent, InputPlugin& plugin, const unsigned int pad_num, InputConfigDialog* const config_dialog )
	: wxNotebookPage( parent, -1 , wxDefaultPosition, wxDefaultSize )
	,controller(plugin.controllers[pad_num])
	,m_config_dialog(config_dialog)
	,m_plugin(plugin)
{

	wxBoxSizer* control_group_sizer = new ControlGroupsSizer( m_plugin.controllers[pad_num], this, this, &control_groups );

	wxStaticBoxSizer* profile_sbox = new wxStaticBoxSizer( wxHORIZONTAL, this, wxT("Profile") );

	// device chooser

	wxStaticBoxSizer* const device_sbox = new wxStaticBoxSizer( wxHORIZONTAL, this, wxT("Device") );

	device_cbox = new wxComboBox( this, -1, wxT(""), wxDefaultPosition, wxSize(128,-1), 0, 0, wxTE_PROCESS_ENTER );

	wxButton* refresh_button = new wxButton( this, -1, wxT("Refresh"), wxDefaultPosition, wxSize(60,-1) );

	_connect_macro_( device_cbox, GamepadPage::SetDevice, wxEVT_COMMAND_COMBOBOX_SELECTED, this );
	_connect_macro_( device_cbox, GamepadPage::SetDevice, wxEVT_COMMAND_TEXT_ENTER, this );
	_connect_macro_( refresh_button, GamepadPage::RefreshDevices, wxEVT_COMMAND_BUTTON_CLICKED, this );

	device_sbox->Add( device_cbox, 1, wxLEFT|wxRIGHT, 3 );
	device_sbox->Add( refresh_button, 0, wxRIGHT|wxBOTTOM, 3 );

	wxStaticBoxSizer* const clear_sbox = new wxStaticBoxSizer( wxHORIZONTAL, this, wxT("Clear") );
	wxButton* all_button = new wxButton( this, -1, wxT("All"), wxDefaultPosition, wxSize(48,-1) );
	clear_sbox->Add( all_button, 1, wxLEFT|wxRIGHT, 3 );

	_connect_macro_(all_button, GamepadPage::ClearAll, wxEVT_COMMAND_BUTTON_CLICKED, this);

	profile_cbox = new wxComboBox( this, -1, wxT(""), wxDefaultPosition, wxSize(128,-1) );

	wxButton* const pload_btn = new wxButton( this, -1, wxT("Load"), wxDefaultPosition, wxSize(48,-1) );
	wxButton* const psave_btn = new wxButton( this, -1, wxT("Save"), wxDefaultPosition, wxSize(48,-1) );
	wxButton* const pdelete_btn = new wxButton( this, -1, wxT("Delete"), wxDefaultPosition, wxSize(60,-1) );

	_connect_macro_(pload_btn, GamepadPage::LoadProfile, wxEVT_COMMAND_BUTTON_CLICKED, this);
	_connect_macro_(psave_btn, GamepadPage::SaveProfile, wxEVT_COMMAND_BUTTON_CLICKED, this);
	_connect_macro_(pdelete_btn, GamepadPage::DeleteProfile, wxEVT_COMMAND_BUTTON_CLICKED, this);

	profile_sbox->Add( profile_cbox, 1, wxLEFT, 3 );
	profile_sbox->Add( pload_btn, 0, wxLEFT, 3 );
	profile_sbox->Add( psave_btn, 0, 0, 3 );
	profile_sbox->Add( pdelete_btn, 0, wxRIGHT|wxBOTTOM, 3 );

	wxBoxSizer* const dio = new wxBoxSizer( wxHORIZONTAL );
	dio->Add( device_sbox, 1, wxEXPAND|wxRIGHT, 5 );
	dio->Add( clear_sbox, 0, wxEXPAND|wxRIGHT, 5 );
	dio->Add( profile_sbox, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* const mapping = new wxBoxSizer( wxVERTICAL );

	mapping->Add( dio, 1, wxEXPAND|wxLEFT|wxTOP|wxBOTTOM, 5 );
	mapping->Add( control_group_sizer, 0, wxLEFT|wxEXPAND, 5 );

	UpdateGUI();

	SetSizerAndFit( mapping );	// needed
	Layout();
};


InputConfigDialog::InputConfigDialog(wxWindow* const parent, InputPlugin& plugin, const std::string& name)
	: wxDialog( parent, wxID_ANY, wxString::FromAscii(name.c_str()), wxPoint(128,-1), wxDefaultSize )
	, m_plugin(plugin)
{
	m_pad_notebook = new wxNotebook( this, -1, wxDefaultPosition, wxDefaultSize, wxNB_DEFAULT );
	for ( unsigned int i = 0; i < plugin.controllers.size(); ++i )
	{
		GamepadPage* gp = new GamepadPage( m_pad_notebook, m_plugin, i, this );
		m_padpages.push_back( gp );
		m_pad_notebook->AddPage( gp, wxString::FromAscii( m_plugin.gui_name ) + wxT(' ') + wxChar('1'+i) );
	}

	UpdateDeviceComboBox();
	UpdateProfileComboBox();

	wxButton* const close_button = new wxButton( this, -1, wxT("Save"));
	_connect_macro_(close_button, InputConfigDialog::ClickSave, wxEVT_COMMAND_BUTTON_CLICKED, this);
	_connect_macro_(close_button, InputConfigDialog::ClickSave, wxEVT_COMMAND_BUTTON_CLICKED, this);

	wxBoxSizer* btns = new wxBoxSizer( wxHORIZONTAL );
	//btns->Add( new wxStaticText( this, -1, wxString::FromAscii(ver.c_str())), 0, wxLEFT|wxTOP, 5 );
	btns->AddStretchSpacer();
	btns->Add( close_button, 0, 0, 0 );

	wxBoxSizer* const szr = new wxBoxSizer( wxVERTICAL );
	szr->Add( m_pad_notebook, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5 );
	szr->Add( btns, 0, wxEXPAND|wxALL, 5 );

	SetSizerAndFit( szr );	// needed

	// not needed here it seems, but it cant hurt
	//Layout();

	Center();

	// live preview update timer
	m_update_timer = new wxTimer( this, -1 );
	Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( InputConfigDialog::UpdateBitmaps ), (wxObject*)0, this );
	m_update_timer->Start( PREVIEW_UPDATE_TIME, wxTIMER_CONTINUOUS);


}

bool InputConfigDialog::Destroy()
{
	m_update_timer->Stop();
	return true;
}
