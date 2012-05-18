/*	BOSS

	A "one-click" program for users that quickly optimises and avoids 
	detrimental conflicts in their TES IV: Oblivion, Nehrim - At Fate's Edge, 
	TES V: Skyrim, Fallout 3 and Fallout: New Vegas mod load orders.

    Copyright (C) 2009-2012    BOSS Development Team.

	This file is part of BOSS.

    BOSS is free software: you can redistribute 
	it and/or modify it under the terms of the GNU General Public License 
	as published by the Free Software Foundation, either version 3 of 
	the License, or (at your option) any later version.

    BOSS is distributed in the hope that it will 
	be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BOSS.  If not, see 
	<http://www.gnu.org/licenses/>.

	$Revision: 2188 $, $Date: 2011-01-20 10:05:16 +0000 (Thu, 20 Jan 2011) $
*/

//We want to ensure that the GUI-specific code in BOSS-Common is included.

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/unordered_set.hpp>
#include <boost/regex.hpp>

#include <clocale>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>

#include <wx/aboutdlg.h>
#include <wx/snglinst.h>

#include "GUI/MainWindow.h"
#include "GUI/SettingsWindow.h"
#include "GUI/UserRuleEditor.h"
#include "GUI/ElementIDs.h"

wxDEFINE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxThreadEvent);

BEGIN_EVENT_TABLE ( MainFrame, wxFrame )
	EVT_CLOSE (MainFrame::OnClose )
	EVT_MENU ( MENU_Quit, MainFrame::OnQuit )
	EVT_MENU ( OPTION_EditUserRules, MainFrame::OnEditUserRules )
	EVT_MENU ( OPTION_OpenBOSSlog, MainFrame::OnOpenFile )
	EVT_MENU ( OPTION_Run, MainFrame::OnRunBOSS )
	EVT_MENU ( MENU_OpenMainReadMe, MainFrame::OnOpenFile )
	EVT_MENU ( MENU_OpenUserRulesReadMe, MainFrame::OnOpenFile )
	EVT_MENU ( MENU_OpenMasterlistReadMe, MainFrame::OnOpenFile )
	EVT_MENU ( MENU_OpenAPIReadMe, MainFrame::OnOpenFile )
	EVT_MENU ( MENU_OpenLicenses, MainFrame::OnOpenFile )
	EVT_MENU ( OPTION_CheckForUpdates, MainFrame::OnUpdateCheck )
	EVT_MENU ( MENU_ShowAbout, MainFrame::OnAbout )
	EVT_MENU ( MENU_ShowSettings, MainFrame::OnOpenSettings )
	EVT_MENU ( MENU_Oblivion, MainFrame::OnGameChange )
	EVT_MENU ( MENU_Nehrim, MainFrame::OnGameChange )
	EVT_MENU ( MENU_Skyrim, MainFrame::OnGameChange )
	EVT_MENU ( MENU_Fallout3, MainFrame::OnGameChange )
	EVT_MENU ( MENU_FalloutNewVegas, MainFrame::OnGameChange )
	EVT_MENU ( MENU_Morrowind, MainFrame::OnGameChange )
	EVT_BUTTON ( OPTION_Run, MainFrame::OnRunBOSS )
	EVT_BUTTON ( OPTION_EditUserRules, MainFrame::OnEditUserRules )
	EVT_BUTTON ( OPTION_OpenBOSSlog, MainFrame::OnOpenFile )
	EVT_BUTTON ( OPTION_CheckForUpdates, MainFrame::OnUpdateCheck )
	EVT_CHOICE ( DROPDOWN_LogFormat, MainFrame::OnFormatChange )
	EVT_CHOICE ( DROPDOWN_Game, MainFrame::OnGameChange )
	EVT_CHOICE ( DROPDOWN_Revert, MainFrame::OnRevertChange )
	EVT_CHECKBOX ( CHECKBOX_ShowBOSSlog, MainFrame::OnLogDisplayChange )
	EVT_CHECKBOX ( CHECKBOX_Update, MainFrame::OnUpdateChange )
	EVT_CHECKBOX ( CHECKBOX_EnableCRCs, MainFrame::OnCRCDisplayChange )
	EVT_CHECKBOX ( CHECKBOX_TrialRun, MainFrame::OnTrialRunChange )
	EVT_RADIOBUTTON ( RADIOBUTTON_SortOption, MainFrame::OnRunTypeChange )
	EVT_RADIOBUTTON ( RADIOBUTTON_UpdateOption, MainFrame::OnRunTypeChange )
	EVT_RADIOBUTTON ( RADIOBUTTON_UndoOption, MainFrame::OnRunTypeChange )
	EVT_THREAD( wxEVT_COMMAND_MYTHREAD_UPDATE, MainFrame::OnThreadUpdate )
END_EVENT_TABLE()

IMPLEMENT_APP(BossGUI)

using namespace boss;
using namespace std;

//Draws the main window when program starts.
bool BossGUI::OnInit() {
	//Set locale.
	setlocale(LC_CTYPE, "");
	locale global_loc = locale();
	locale loc(global_loc, new boost::filesystem::detail::utf8_codecvt_facet());
	boost::filesystem::path::imbue(loc);

	//Check if GUI is already running.
	wxSingleInstanceChecker *checker = new wxSingleInstanceChecker;

	if (checker->IsAnotherRunning()) {
		wxMessageBox(wxString::Format(
				wxT("Error: The BOSS GUI is already running. This instance will now quit.")
			),
			wxT("BOSS: Error"),
			wxOK | wxICON_ERROR,
			NULL);

		delete checker; // OnExit() won't be called if we return false
		checker = NULL;

		return false;
	}

	Settings ini;
	//Set up variable defaults.
	if (fs::exists(ini_path)) {
		try {
			ini.Load(ini_path);
		} catch (boss_error &e) {
			LOG_ERROR("Error: %s", e.getString().c_str());
			wxMessageBox(wxString::Format(
					wxT("Error: " + e.getString() + " Details: " + Outputter(PLAINTEXT, ini.ErrorBuffer()).AsString())
				),
				wxT("BOSS: Error"),
				wxOK | wxICON_ERROR,
				NULL);
		}
	}

	// set alternative output stream for logger and whether to track log statement origins
	if (gl_log_debug_output)
		g_logger.setStream(debug_log_path.string().c_str());
	g_logger.setOriginTracking(gl_debug_with_source);
	// it's ok if this number is too high.  setVerbosity will handle it
	g_logger.setVerbosity(static_cast<LogVerbosity>(LV_WARN + gl_debug_verbosity));

	MainFrame *frame = new MainFrame(wxT("BOSS"));

	LOG_DEBUG("Detecting game...");
	
	Game game;
	std::vector<uint32_t> detected;
	try {
		vector<uint32_t> undetected;
		uint32_t detectedGame = DetectGame(detected, undetected);
		if (detectedGame == AUTODETECT) {
			wxArrayString choices;

			for (size_t i=0, max = detected.size(); i < max; i++)
				choices.Add(Game(detected[i], "", true).Name());
			for (size_t i=0, max = undetected.size(); i < max; i++)
				choices.Add(Game(undetected[i], "", true).Name() + " (not detected)");

			size_t ans;

			wxSingleChoiceDialog* choiceDia = new wxSingleChoiceDialog(frame, wxT("Please pick which game to run BOSS for:"),
				wxT("BOSS: Select Game"), choices);
			choiceDia->SetIcon(wxIconLocation("BOSS GUI.exe"));

			if (choiceDia->ShowModal() != wxID_OK)
				throw boss_error(BOSS_ERROR_NO_GAME_DETECTED);

			ans = choiceDia->GetSelection();
			choiceDia->Close(true);

			if (ans < detected.size())
				detectedGame = detected[ans];
			else if (ans < detected.size() + undetected.size())
				detectedGame = undetected[ans - detected.size()];
			else
				throw boss_error(BOSS_ERROR_NO_GAME_DETECTED);
		}
		game = Game(detectedGame);
		LOG_INFO("Game detected: %s", game.Name().c_str());
	} catch (boss_error &e) {
		return false;
	}
	frame->SetGames(game, detected);

	frame->SetIcon(wxIconLocation("BOSS GUI.exe"));
	frame->Show(TRUE);
	SetTopWindow(frame);

	//Now check for updates.
	if (gl_do_startup_update_check)
		frame->CheckForUpdates();
	return true;
}

MainFrame::MainFrame(const wxChar *title) : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxMINIMIZE_BOX|wxSYSTEM_MENU|wxCAPTION|wxCLOSE_BOX|wxCLIP_CHILDREN) {

	//Some variable setup.
	isStartup = true;

	wxString BOSSlogFormat[] = {
        wxT("HTML"),
        wxT("Plain Text")
    };

	wxString UndoLevel[] = {
		wxT("Last Run"),
		wxT("2nd Last Run")
	};

	//Set up menu bar first.
    MenuBar = new wxMenuBar();
    // File Menu
    FileMenu = new wxMenu();
	FileMenu->Append(OPTION_OpenBOSSlog, wxT("&View BOSS Log"), wxT("Opens your BOSSlog."));
    FileMenu->Append(OPTION_Run, wxT("&Run BOSS"), wxT("Runs BOSS with the options you have chosen."));
    FileMenu->AppendSeparator();
    FileMenu->Append(MENU_Quit, wxT("&Quit"), wxT("Quit BOSS."));
    MenuBar->Append(FileMenu, wxT("&File"));
	//Edit Menu
	EditMenu = new wxMenu();
	EditMenu->Append(OPTION_EditUserRules, wxT("&User Rules..."), wxT("Opens your userlist in your default text editor."));
	EditMenu->Append(MENU_ShowSettings, wxT("&Settings..."), wxT("Opens the Settings window."));
	MenuBar->Append(EditMenu, wxT("&Edit"));
	//Game menu
	GameMenu = new wxMenu();
	GameMenu->AppendRadioItem(MENU_None, wxT("&None"), wxT("If selected, BOSS was unable to detect or run for a game."));
	GameMenu->AppendRadioItem(MENU_Oblivion, wxT("&Oblivion"), wxT("Switch to running BOSS for Oblivion."));
	GameMenu->AppendRadioItem(MENU_Nehrim, wxT("&Nehrim"), wxT("Switch to running BOSS for Nehrim."));
	GameMenu->AppendRadioItem(MENU_Skyrim, wxT("&Skyrim"), wxT("Switch to running BOSS for Skyrim."));
	GameMenu->AppendRadioItem(MENU_Fallout3, wxT("&Fallout 3"), wxT("Switch to running BOSS for Fallout 3."));
	GameMenu->AppendRadioItem(MENU_FalloutNewVegas, wxT("&Fallout: New Vegas"), wxT("Switch to running BOSS for Fallout: New Vegas."));
	GameMenu->AppendRadioItem(MENU_Morrowind, wxT("&Morrowind"), wxT("Switch to running BOSS for Morrowind."));
	MenuBar->Append(GameMenu, wxT("&Active Game"));
    // About menu
    HelpMenu = new wxMenu();
	HelpMenu->Append(MENU_OpenMainReadMe, wxT("Open &Main ReadMe"), wxT("Opens the main BOSS ReadMe in your default web browser."));
	HelpMenu->Append(MENU_OpenUserRulesReadMe, wxT("Open &User Rules ReadMe"), wxT("Opens the User Rules ReadMe in your default web browser."));
	HelpMenu->Append(MENU_OpenMasterlistReadMe, wxT("Open &Masterlist &ReadMe"), wxT("Opens the BOSS Masterlist Syntax ReadMe in your default web browser."));
	//HelpMenu->Append(MENU_OpenAPIReadMe, wxT("&Open API ReadMe"), wxT("Opens the BOSS API ReadMe in your default web browser."));
	HelpMenu->Append(MENU_OpenLicenses, wxT("View &Copyright Licenses"), wxT("View the GNU General Public License v3.0 and GNU Free Documentation License v1.3."));
	HelpMenu->AppendSeparator();
	HelpMenu->Append(OPTION_CheckForUpdates, wxT("&Check For Updates..."), wxT("Checks for updates to BOSS."));
	HelpMenu->Append(MENU_ShowAbout, wxT("&About BOSS..."), wxT("Shows information about BOSS."));
    MenuBar->Append(HelpMenu, wxT("&Help"));
    SetMenuBar(MenuBar);

	//Set up stuff in the frame.
	SetBackgroundColour(wxColour(255,255,255));

	//Contents in one big resizing box.
	wxBoxSizer *bigBox = new wxBoxSizer(wxHORIZONTAL);

	//Create first column box and add the output options to it.
	wxBoxSizer *columnBox = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer *outputOptionsBox = new wxStaticBoxSizer(wxVERTICAL, this, "Output Options");
	wxBoxSizer *formatBox = new wxBoxSizer(wxHORIZONTAL);
	
	//Add stuff to output options sizer.
	outputOptionsBox->Add(ShowLogBox = new wxCheckBox(this,CHECKBOX_ShowBOSSlog, wxT("Show BOSS Log On Completion")), 0, wxALL, 5);
	outputOptionsBox->Add(CRCBox = new wxCheckBox(this,CHECKBOX_EnableCRCs, wxT("Display File CRCs")), 0, wxLEFT | wxBOTTOM, 5);
	formatBox->Add(new wxStaticText(this, wxID_ANY, wxT("BOSS Log Format: ")), 1, wxLEFT | wxBOTTOM, 5);
	formatBox->Add(FormatChoice = new wxChoice(this, DROPDOWN_LogFormat, wxPoint(110,60), wxDefaultSize, 2, BOSSlogFormat, wxCB_READONLY), 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 5);
	//Add the verbosityBox to its parent now to preserve layout.
	outputOptionsBox->Add(formatBox, 0, wxEXPAND, 0);
	columnBox->Add(outputOptionsBox, 0, wxBOTTOM, 20);

	//Now add the main buttons to the first column.
	wxBoxSizer *buttonBox = new wxBoxSizer(wxVERTICAL);
	buttonBox->Add(EditUserRulesButton = new wxButton(this,OPTION_EditUserRules, wxT("Edit User Rules"), wxDefaultPosition, wxSize(120,30)), 0, wxBOTTOM, 5);
	buttonBox->Add(RunBOSSButton = new wxButton(this,OPTION_Run, wxT("Run BOSS"), wxDefaultPosition, wxSize(120,30)));
	buttonBox->Add(OpenBOSSlogButton = new wxButton(this,OPTION_OpenBOSSlog, wxT("View BOSS Log"), wxDefaultPosition, wxSize(120,30)), 0, wxTOP, 5);
	columnBox->Add(buttonBox, 0, wxALIGN_CENTER, 20);

	//Add the first column to the big box.
	bigBox->Add(columnBox, 0, wxALL, 20);

	//The second column has a border.
	wxStaticBoxSizer *runOptionsBox = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Run Options"));
	wxBoxSizer *sortBox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *undoBox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *revertBox = new wxBoxSizer(wxHORIZONTAL);

	//Run Options
	runOptionsBox->Add(SortOption = new wxRadioButton(this, RADIOBUTTON_SortOption, wxT("Sort Plugins")), 0, wxALL, 5);
	
	//Sort option stuff.
	sortBox->Add(UpdateBox = new wxCheckBox(this,CHECKBOX_Update, wxT("Update Masterlist")), 0, wxBOTTOM, 5);
	sortBox->Add(TrialRunBox = new wxCheckBox(this,CHECKBOX_TrialRun, wxT("Perform Trial Run")));
	runOptionsBox->Add(sortBox, 0, wxLEFT | wxRIGHT, 20);
	runOptionsBox->AddSpacer(10);
	
	//Update only stuff.
	runOptionsBox->Add(UpdateOption = new wxRadioButton(this, RADIOBUTTON_UpdateOption, wxT("Update Masterlist Only")), 0, wxALL, 5);
	runOptionsBox->AddSpacer(10);
	
	//Undo option stuff.
	runOptionsBox->Add(UndoOption = new wxRadioButton(this, RADIOBUTTON_UndoOption, wxT("Undo Changes")), 0, wxALL, 5);
	revertBox->Add(RevertText = new wxStaticText(this, wxID_ANY, wxT("Undo Level: ")));
	revertBox->Add(RevertChoice = new wxChoice(this, DROPDOWN_Revert, wxDefaultPosition, wxDefaultSize, 2, UndoLevel));
	runOptionsBox->Add(revertBox, 0, wxLEFT | wxRIGHT, 20);
	runOptionsBox->AddSpacer(5);

	bigBox->Add(runOptionsBox, 0, wxTOP | wxRIGHT | wxBOTTOM, 20);


	//Tooltips
	FormatChoice->SetToolTip(wxT("This decides both the format of BOSSlog generated when you click the \"Run BOSS\" button and the BOSSlog format opened when you click the \"View BOSSlog\" button."));
	OpenBOSSlogButton->SetToolTip(wxT("The format of BOSSlog this opens is decided by the setting of the \"BOSSlog Format\" Output Option above."));
	TrialRunBox->SetToolTip(wxT("Runs BOSS, simulating its changes to your load order, but doesn't actually reorder your mods."));

	//Set option values based on initialised variable values.
	RunBOSSButton->SetDefault();

	//Disable the None option from the Active Game menu. It should stay disabled.
	GameMenu->FindItem(MENU_None)->Enable(false);

	if (!gl_silent)
		ShowLogBox->SetValue(true);

	if (gl_log_format == HTML)
		FormatChoice->SetSelection(0);
	else
		FormatChoice->SetSelection(1);

	if (gl_show_CRCs)
		CRCBox->SetValue(true);

	if (gl_update)
		UpdateBox->SetValue(true);

	if (gl_trial_run)
		TrialRunBox->SetValue(true);

	if (gl_revert < 2)
		RevertChoice->SetSelection(0);
	else
		RevertChoice->SetSelection(1);

	if (gl_revert == 0 && !gl_update_only) {
		SortOption->SetValue(true);
		UpdateBox->Enable(true);
		TrialRunBox->Enable(true);

		RevertText->Enable(false);
		RevertChoice->Enable(false);
	} else if (gl_update_only) {
		UpdateOption->SetValue(true);

		UpdateBox->Enable(false);
		TrialRunBox->Enable(false);
		RevertText->Enable(false);
		RevertChoice->Enable(false);
	} else if (gl_revert > 0) {
		UndoOption->SetValue(true);
		RevertText->Enable(true);
		RevertChoice->Enable(true);

		UpdateBox->Enable(false);
		TrialRunBox->Enable(false);
	}

	//Now set up the status bar.
	CreateStatusBar(1);
    SetStatusText(wxT("Ready"));

	//Now set the layout and sizes.
	SetSizerAndFit(bigBox);
}

//Called when program exits.
void MainFrame::OnQuit( wxCommandEvent& event ) {
	Close(true); // Tells the OS to quit running this process
}

void MainFrame::OnClose(wxCloseEvent& event) {
	Settings ini;
	//Save settings to BOSS.ini before quitting.
	try {
		ini.Save(ini_path, game.Id());
	} catch (boss_error &e) {
			wxMessageBox(wxString::Format(
				wxT("Error: " + e.getString())
			),
			wxT("BOSS: Error"),
			wxOK | wxICON_ERROR,
			NULL);
	}

	// important: before terminating, we _must_ wait for our joinable
    // thread to end, if it's running; in fact it uses variables of this
    // instance and posts events to *this event handler

    if (GetThread() && GetThread()->IsRunning())
        GetThread()->Wait();

    Destroy();  // you may also do:  event.Skip();
                // since the default event handler does call Destroy(), too
}

void MainFrame::OnRunBOSS( wxCommandEvent& event ) {
	BossLog bosslog(gl_log_format);				//BOSSlog contents.
	fs::path sortfile;						//Modlist/masterlist to sort plugins using.

	//Tell the user that stuff is happenining.
	wxProgressDialog *progDia = new wxProgressDialog(wxT("BOSS: Working..."),wxT("BOSS working..."), 1000, this, wxPD_APP_MODAL|wxPD_AUTO_HIDE|wxPD_ELAPSED_TIME|wxPD_CAN_ABORT);

	LOG_INFO("BOSS starting...");

	//Clear modlist, masterlist and userlist in case they were filled by the User Rules Manager but the user has made changes to their game.
	game.modlist.Clear();
	game.masterlist.Clear();
	game.userlist.Clear();


	/////////////////////////////////////////////////////////
	// Update Masterlist
	/////////////////////////////////////////////////////////
	
	if (gl_revert<1 && (gl_update || gl_update_only)) {
		//First check for internet connection, then update masterlist if connection present.
		GUIMlistUpdater mUpdater;
		try {
			if (mUpdater.IsInternetReachable()) {
				progDia->Update(0,wxT("Updating to the latest masterlist from the Google Code repository..."));
				LOG_DEBUG("Updating masterlist...");
				try {
					string localDate, remoteDate;
					uint32_t localRevision, remoteRevision;
					mUpdater.progDialog = progDia;
					mUpdater.Update(game.Id(), game.Masterlist(), localRevision, localDate, remoteRevision, remoteDate);
					if (localRevision == remoteRevision) {
						bosslog.updaterOutput << LIST_ITEM_CLASS_SUCCESS << "Your masterlist is already at the latest revision (r" << localRevision << "; " << localDate << "). No update necessary.";
						progDia->Pulse(wxT("Masterlist already up-to-date."));
						LOG_DEBUG("Masterlist update unnecessary.");
					} else {
						bosslog.updaterOutput << LIST_ITEM_CLASS_SUCCESS << "Your masterlist has been updated to revision " << remoteRevision << " (" << remoteDate << ").";
						progDia->Pulse(wxT("Masterlist updated successfully."));
						LOG_DEBUG("Masterlist updated successfully.");
					}
				} catch (boss_error &e) {
					bosslog.updaterOutput << LIST_ITEM_CLASS_ERROR << "Error: masterlist update failed." << LINE_BREAK
						<< "Details: " << e.getString() << LINE_BREAK
						<< "Check the Troubleshooting section of the ReadMe for more information and possible solutions.";
					LOG_ERROR("Error: Masterlist update failed. Details: %s", e.getString().c_str());
				}
			} else {
				bosslog.updaterOutput << LIST_ITEM_CLASS_WARN << "No internet connection detected. Masterlist auto-updater could not check for updates.";
			}
		} catch (boss_error &e) {
			bosslog.updaterOutput << LIST_ITEM_CLASS_ERROR << "Error: masterlist update failed." << LINE_BREAK
				<< "Details: " << e.getString() << LINE_BREAK
				<< "Check the Troubleshooting section of the ReadMe for more information and possible solutions.";
			LOG_ERROR("Error: Masterlist update failed. Details: %s", e.getString().c_str());
		}
	}

	//If true, exit BOSS now. Flush earlyBOSSlogBuffer to the bosslog and exit.
	if (gl_update_only == true) {
		try {
			bosslog.Save(game.Log(gl_log_format), true);
		} catch (boss_error &e) {
			LOG_ERROR("Critical Error: %s", e.getString().c_str());
		}
		if ( !gl_silent ) 
			wxLaunchDefaultApplication(game.Log(gl_log_format).string());	//Displays the BOSSlog.
		progDia->Destroy();
		return;
	}

	progDia->Pulse(wxT("BOSS working..."));
	if (progDia->WasCancelled()) {
		progDia->Destroy();
		return;
	}

	///////////////////////////////////
	// Resume Error Condition Checks
	///////////////////////////////////

	//Build and save modlist.
	try {
		game.modlist.Load(game, game.DataFolder());
		if (gl_revert<1)
			game.modlist.Save(game.Modlist(), game.OldModlist());
	} catch (boss_error &e) {
		LOG_ERROR("Failed to load/save modlist, error was: %s", e.getString().c_str());
		game.bosslog.criticalError << LIST_ITEM_CLASS_ERROR << "Critical Error: " << e.getString() << LINE_BREAK
			<< "Check the Troubleshooting section of the ReadMe for more information and possible solutions." << LINE_BREAK
			<< "Utility will end now.";
		try {
			game.bosslog.Save(game.Log(gl_log_format), true);
		} catch (boss_error &e) {
			LOG_ERROR("Critical Error: %s", e.getString().c_str());
		}
		if ( !gl_silent ) 
			wxLaunchDefaultApplication(game.Log(gl_log_format).string());	//Displays the BOSSlog.txt.
		progDia->Destroy();
		return; //fail in screaming heap.
	}

	progDia->Pulse();
	if (progDia->WasCancelled()) {
		progDia->Destroy();
		return;
	}

	/////////////////////////////////
	// Parse Master- and Userlists
	/////////////////////////////////
	//Masterlist parse errors are critical, ini and userlist parse errors are not.

	
	//Set masterlist path to be used.
	if (gl_revert==1)
		sortfile = game.Modlist();	
	else if (gl_revert==2) 
		sortfile = game.OldModlist();
	else 
		sortfile = game.Masterlist();
	LOG_INFO("Using sorting file: %s", sortfile.string().c_str());

	//Parse masterlist/modlist backup into data structure.
	try {
		LOG_INFO("Starting to parse sorting file: %s", sortfile.string().c_str());
		game.masterlist.Load(game, sortfile);
		LOG_INFO("Starting to parse conditionals from sorting file: %s", sortfile.string().c_str());
		game.masterlist.EvalConditions(game);
		game.masterlist.EvalRegex(game);
		game.bosslog.globalMessages = game.masterlist.GlobalMessageBuffer();
		game.bosslog.parsingErrors.push_back(game.masterlist.ErrorBuffer());
	} catch (boss_error &e) {
		LOG_ERROR("Critical Error: %s", e.getString().c_str());
        if (e.getCode() == BOSS_ERROR_FILE_PARSE_FAIL)
			bosslog.criticalError << game.masterlist.ErrorBuffer();
		else if (e.getCode() == BOSS_ERROR_CONDITION_EVAL_FAIL)
			bosslog.criticalError << LIST_ITEM_CLASS_ERROR << e.getString();
		else
			bosslog.criticalError << LIST_ITEM_CLASS_ERROR << "Critical Error: " << e.getString() << LINE_BREAK
				<< "Check the Troubleshooting section of the ReadMe for more information and possible solutions." << LINE_BREAK
				<< "Utility will end now.";
		try {
			game.bosslog.Save(game.Log(gl_log_format), true);
		} catch (boss_error &e) {
			LOG_ERROR("Critical Error: %s", e.getString().c_str());
		}
        if ( !gl_silent ) 
			wxLaunchDefaultApplication(game.Log(gl_log_format).string());  //Displays the BOSSlog.txt.
		progDia->Destroy();
        return; //fail in screaming heap.
	}

	LOG_INFO("Starting to parse userlist.");
	try {
		game.userlist.Load(game, game.Userlist());
		vector<ParsingError> errs = game.userlist.ErrorBuffer();
		game.bosslog.parsingErrors.insert(game.bosslog.parsingErrors.end(), errs.begin(), errs.end());
	} catch (boss_error &e) {
		vector<ParsingError> errs = game.userlist.ErrorBuffer();
		game.bosslog.parsingErrors.insert(game.bosslog.parsingErrors.end(), errs.begin(), errs.end());
		game.userlist.Clear();  //If userlist has parsing errors, empty it so no rules are applied.
		LOG_ERROR("Error: %s", e.getString().c_str());
	}

	progDia->Pulse();
	if (progDia->WasCancelled()) {
		progDia->Destroy();
		return;
	}

	/////////////////////////////////////////////////
	// Perform Sorting Functionality
	/////////////////////////////////////////////////

	try {
		game.ApplyMasterlist();
		LOG_INFO("masterlist now filled with ordered mods and modlist filled with unknowns.");
		game.ApplyUserlist();
		LOG_INFO("userlist sorting process finished.");
		game.ScanSEPlugins();
		game.SortPlugins();
		game.bosslog.Save(game.Log(gl_log_format), true);
	} catch (boss_error &e) {
		LOG_ERROR("Critical Error: %s", e.getString().c_str());
		game.bosslog.criticalError << LIST_ITEM_CLASS_ERROR << "Critical Error: " << e.getString() << LINE_BREAK
			<< "Check the Troubleshooting section of the ReadMe for more information and possible solutions." << LINE_BREAK
			<< "Utility will end now.";
		try {
			game.bosslog.Save(game.Log(gl_log_format), true);
		} catch (boss_error &e) {
			LOG_ERROR("Critical Error: %s", e.getString().c_str());
		}
		if ( !gl_silent ) 
                wxLaunchDefaultApplication(game.Log(gl_log_format).string());  //Displays the BOSSlog.txt.
        exit (1); //fail in screaming heap.
	}

	LOG_INFO("Launching boss log in browser.");
	if ( !gl_silent ) 
		wxLaunchDefaultApplication(game.Log(gl_log_format).string());	//Displays the BOSSlog.txt.
	LOG_INFO("BOSS finished.");
	progDia->Destroy();
	return;
}

void MainFrame::OnEditUserRules( wxCommandEvent& event ) {
	if (gl_use_user_rules_manager) {
		UserRulesEditorFrame *editor = new UserRulesEditorFrame(wxT("BOSS: User Rules Manager"), this, game);
		editor->SetIcon(wxIconLocation("BOSS GUI.exe"));
		editor->Show();
		return;
	} else {
		if (fs::exists(game.Userlist()))
			wxLaunchDefaultApplication(game.Userlist().string());
		else {
			try {
				RuleList userlist;
				userlist.Save(game.Userlist());
				wxLaunchDefaultApplication(game.Userlist().string());
			} catch (boss_error &e) {
				wxMessageBox(wxString::Format(
					wxT("Error: " + e.getString())
				),
				wxT("BOSS: Error"),
				wxOK | wxICON_ERROR,
				this);
			}
		}
	}
}

//Call when a file is opened. Either readmes or BOSS Logs.
void MainFrame::OnOpenFile( wxCommandEvent& event ) {
	if (event.GetId() == OPTION_OpenBOSSlog) {
		if (fs::exists(game.Log(gl_log_format)))
			wxLaunchDefaultApplication(game.Log(gl_log_format).string());
		else
			wxMessageBox(wxString::Format(
				wxT("Error: \"" + game.Log(gl_log_format).string() + "\" cannot be found!")
			),
			wxT("BOSS: Error"),
			wxOK | wxICON_ERROR,
			this);
	} else {
		//Readme files.
		string file;
		if (event.GetId() == MENU_OpenMainReadMe)
			file = readme_path.string();
		else if (event.GetId() == MENU_OpenUserRulesReadMe)
			file = rules_readme_path.string();
		else if (event.GetId() == MENU_OpenMasterlistReadMe)
			file = masterlist_doc_path.string();
		else if (event.GetId() == MENU_OpenAPIReadMe)
			file = api_doc_path.string();
		else if (event.GetId() == MENU_OpenLicenses)
			file = licenses_path.string();
		//Look for file.
		if (fs::exists(file)) {
			wxLaunchDefaultApplication(file);
		} else  //No ReadMe exists, show a pop-up message saying so.
			wxMessageBox(wxString::Format(
				wxT("Error: \"" + file + "\" cannot be found!"),
				file
			),
			wxT("BOSS: Error"),
			wxOK | wxICON_ERROR,
			this);

	}
}

void MainFrame::OnAbout(wxCommandEvent& event) {
	wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("BOSS");
    aboutInfo.SetVersion(IntToString(BOSS_VERSION_MAJOR)+"."+IntToString(BOSS_VERSION_MINOR)+"."+IntToString(BOSS_VERSION_PATCH));
    aboutInfo.SetDescription(wxT("A \"one-click\" program for users that quickly optimises and avoids detrimental conflicts in their\nTES IV: Oblivion, Nehrim - At Fate's Edge, TES V: Skyrim, Fallout 3 and Fallout: New Vegas mod load orders."));
    aboutInfo.SetCopyright("Copyright (C) 2009-2012 BOSS Development Team.");
    aboutInfo.SetWebSite("http://code.google.com/p/better-oblivion-sorting-software/");
	aboutInfo.SetLicence("This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n"
	"\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
	"\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program.  If not, see <http://www.gnu.org/licenses/>.");
	aboutInfo.SetIcon(wxIconLocation("BOSS GUI.exe"));
    wxAboutBox(aboutInfo);
}

void MainFrame::OnLogDisplayChange(wxCommandEvent& event) {
	gl_silent = (!event.IsChecked());
}

void MainFrame::OnFormatChange(wxCommandEvent& event) {
	if (event.GetInt() == 0)
		gl_log_format = HTML;
	else
		gl_log_format = PLAINTEXT;
}

void MainFrame::OnCRCDisplayChange(wxCommandEvent& event) {
	gl_show_CRCs = event.IsChecked();
}

void MainFrame::OnUpdateChange(wxCommandEvent& event) {
	gl_update = event.IsChecked();
}

void MainFrame::OnTrialRunChange(wxCommandEvent& event) {
	gl_trial_run = event.IsChecked();
}

void MainFrame::OnGameChange(wxCommandEvent& event) {
	switch (event.GetId()) {
	case MENU_Oblivion:
		game = Game(OBLIVION);
		break;
	case MENU_Nehrim:
		game = Game(NEHRIM);
		break;
	case MENU_Skyrim:
		game = Game(SKYRIM);
		break;
	case MENU_Fallout3:
		game = Game(FALLOUT3);
		break;
	case MENU_FalloutNewVegas:
		game = Game(FALLOUTNV);
		break;
	case MENU_Morrowind:
		game = Game(MORROWIND);
		break;
	}
	SetTitle(wxT("BOSS - " + game.Name()));
}

void MainFrame::OnRevertChange(wxCommandEvent& event) {
	gl_revert = event.GetInt() + 1;
}

void MainFrame::OnRunTypeChange(wxCommandEvent& event) {
	if (event.GetId() == RADIOBUTTON_SortOption) {
		gl_revert = 0;
		gl_update_only = false;

		UpdateBox->Enable(true);
		TrialRunBox->Enable(true);

		RevertText->Enable(false);
		RevertChoice->Enable(false);
	} else if (event.GetId() == RADIOBUTTON_UpdateOption) {
		gl_revert = 0;
		gl_update_only = true;

		UpdateBox->Enable(false);
		TrialRunBox->Enable(false);
		RevertText->Enable(false);
		RevertChoice->Enable(false);
	} else {
		gl_revert = RevertChoice->GetSelection() + 1;
		gl_update_only = false;

		RevertText->Enable(true);
		RevertChoice->Enable(true);

		UpdateBox->Enable(false);
		TrialRunBox->Enable(false);
	}
	DisableUndetectedGames();  //Doesn't actually disable games if (gl_update_only).
}

//This is called when the menu "Check For Updates" option is selected.
void MainFrame::OnUpdateCheck(wxCommandEvent& event) {
	isStartup = false;
	CheckForUpdates();
}

void MainFrame::DisableUndetectedGames() {
	//Also disable the options for undetected games.
	bool enabled;
	if (gl_update_only)
		enabled = true;
	else
		enabled = false;
	GameMenu->FindItem(MENU_Oblivion)->Enable(enabled);
	GameMenu->FindItem(MENU_Nehrim)->Enable(enabled);
	GameMenu->FindItem(MENU_Skyrim)->Enable(enabled);
	GameMenu->FindItem(MENU_Fallout3)->Enable(enabled);
	GameMenu->FindItem(MENU_FalloutNewVegas)->Enable(enabled);
	GameMenu->FindItem(MENU_Morrowind)->Enable(enabled);
	for (size_t i=0; i < detectedGames.size(); i++) {
		if (detectedGames[i] == OBLIVION)
			GameMenu->FindItem(MENU_Oblivion)->Enable();
		else if (detectedGames[i] == NEHRIM)
			GameMenu->FindItem(MENU_Nehrim)->Enable();
		else if (detectedGames[i] == SKYRIM)
			GameMenu->FindItem(MENU_Skyrim)->Enable();
		else if (detectedGames[i] == FALLOUT3)
			GameMenu->FindItem(MENU_Fallout3)->Enable();
		else if (detectedGames[i] == FALLOUTNV)
			GameMenu->FindItem(MENU_FalloutNewVegas)->Enable();
		else if (detectedGames[i] == MORROWIND)
			GameMenu->FindItem(MENU_Morrowind)->Enable();
	}

	//Swapping from gl_update_only to !gl_update_only with undetected game active: need to change game to a detected game.
	if ((GameMenu->FindItem(MENU_Oblivion)->IsChecked() && !GameMenu->FindItem(MENU_Oblivion)->IsEnabled())
		|| (GameMenu->FindItem(MENU_Nehrim)->IsChecked() && !GameMenu->FindItem(MENU_Nehrim)->IsEnabled())
		|| (GameMenu->FindItem(MENU_Skyrim)->IsChecked() && !GameMenu->FindItem(MENU_Skyrim)->IsEnabled())
		|| (GameMenu->FindItem(MENU_Fallout3)->IsChecked() && !GameMenu->FindItem(MENU_Fallout3)->IsEnabled())
		|| (GameMenu->FindItem(MENU_FalloutNewVegas)->IsChecked() && !GameMenu->FindItem(MENU_FalloutNewVegas)->IsEnabled())
		|| (GameMenu->FindItem(MENU_Morrowind)->IsChecked() && !GameMenu->FindItem(MENU_Morrowind)->IsEnabled())) {
			if (!detectedGames.empty()) {
				if (detectedGames.front() == OBLIVION) {
					game = Game(OBLIVION);
					GameMenu->FindItem(MENU_Oblivion)->Check();
				} else if (detectedGames.front() == NEHRIM) {
					game = Game(NEHRIM);
					GameMenu->FindItem(MENU_Nehrim)->Check();
				} else if (detectedGames.front() == SKYRIM) {
					game = Game(SKYRIM);
					GameMenu->FindItem(MENU_Skyrim)->Check();
				} else if (detectedGames.front() == FALLOUT3) {
					game = Game(FALLOUT3);
					GameMenu->FindItem(MENU_Fallout3)->Check();
				} else if (detectedGames.front() == FALLOUTNV) {
					game = Game(FALLOUTNV);
					GameMenu->FindItem(MENU_FalloutNewVegas)->Check();
				} else if (detectedGames.front() == MORROWIND) {
					game = Game(MORROWIND);
					GameMenu->FindItem(MENU_Morrowind)->Check();
				}
			}
	}
	SetTitle(wxT("BOSS - " + game.Name()));
}

void MainFrame::SetGames(const Game& inGame, const vector<uint32_t> inGames) {
	game = inGame;
	detectedGames = inGames;
	if (game.Id() == OBLIVION)
		GameMenu->FindItem(MENU_Oblivion)->Check();
	else if (game.Id() == NEHRIM)
		GameMenu->FindItem(MENU_Nehrim)->Check();
	else if (game.Id() == SKYRIM)
		GameMenu->FindItem(MENU_Skyrim)->Check();
	else if (game.Id() == FALLOUT3)
		GameMenu->FindItem(MENU_Fallout3)->Check();
	else if (game.Id() == FALLOUTNV)
		GameMenu->FindItem(MENU_FalloutNewVegas)->Check();
	else if (game.Id() == MORROWIND)
		GameMenu->FindItem(MENU_Morrowind)->Check();
	else {  //No game detected. Only valid option is to force a game and update masterlist only, so set options to that and disable selecting of other run types.
		GameMenu->FindItem(MENU_None)->Check();
		gl_update_only = true;
		SortOption->Enable(false);
		UpdateOption->SetValue(true);
		UndoOption->Enable(false);
	}
	size_t i=0;
	for (i=0; i < detectedGames.size(); i++) {
		if (detectedGames[i] == game.Id())
			break;
	}
	if (i == detectedGames.size()) {  //The current game wasn't in the list of detected games. Run in update only mode.
		gl_update_only = true;
		UpdateBox->Enable(false);
		TrialRunBox->Enable(false);
		SortOption->Enable(false);
		UpdateOption->SetValue(true);
		UndoOption->Enable(false);
	}
	DisableUndetectedGames();
}

void MainFrame::Update(string updateVersion) {
	wxString message = wxT("The automatic updater will download the installer for the new version to this BOSS folder.\n\n");
	message += wxT("It will then launch the installer before exiting. Complete the installer to complete the update.");
		
	wxMessageDialog *dlg = new wxMessageDialog(this,message, wxT("BOSS: Automatic Updater"), wxOK | wxCANCEL);
	if (dlg->ShowModal() != wxID_OK) {  //User has chosen to cancel. Quit now.
		wxMessageBox(wxT("Automatic updater cancelled."), wxT("BOSS: Automatic Updater"), wxOK | wxICON_EXCLAMATION, this);
		return;
	}

	wxProgressDialog *progDia = new wxProgressDialog(wxT("BOSS: Automatic Updater"),wxT("Initialising download..."), 1000, this, wxPD_APP_MODAL|wxPD_AUTO_HIDE|wxPD_ELAPSED_TIME|wxPD_CAN_ABORT);
	string file = "BOSS Installer.exe";
	GUIBOSSUpdater bUpdater;
	try {
		bUpdater.progDialog = progDia;
		bUpdater.GetUpdate(fs::path(file), updateVersion);

		//Remind the user to run the installer.
		wxMessageBox(wxT("New installer successfully downloaded! When you click 'OK', BOSS will launch the downloaded installer and exit. Complete the installer to complete the update."), wxT("BOSS: Automatic Updater"), wxOK | wxICON_INFORMATION, this);
		if (fs::exists(file))
			wxLaunchDefaultApplication(file);
	} catch (boss_error &e) {
		progDia->Destroy();
		try {
			bUpdater.CleanUp();
			if (e.getCode() == BOSS_ERROR_CURL_USER_CANCEL)
				wxMessageBox(wxT("Update cancelled."), wxT("BOSS: Automatic Updater"), wxOK | wxICON_INFORMATION, this);
			else
				wxMessageBox("Update failed. Details: " + e.getString() + "\n\nUpdate cancelled.", wxT("BOSS: Automatic Updater"), wxOK | wxICON_ERROR, this);
		} catch (boss_error &ee) {
			if (e.getCode() != BOSS_ERROR_CURL_USER_CANCEL)
				LOG_ERROR("Update failed. Details: '%s'", e.getString().c_str());
			LOG_ERROR("Update clean up failed. Details: '%s'", ee.getString().c_str());
			wxMessageBox("Update failed. Details: " + e.getString() + "; " + ee.getString() + "\n\nUpdate cancelled.", wxT("BOSS: Automatic Updater"), wxOK | wxICON_ERROR, this);
		}
	}
	this->Close();
}

void MainFrame::OnOpenSettings(wxCommandEvent& event) {
	//Tell the user that stuff is happenining.
	SettingsFrame *settings = new SettingsFrame(wxT("BOSS: Settings"),this);
	settings->SetIcon(wxIconLocation("BOSS GUI.exe"));
	settings->Show();
}

void MainFrame::CheckForUpdates() {
	// we want to start a long task, but we don't want our GUI to block
    // while it's executed, so we use a thread to do it.
    if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)
    {
        LOG_ERROR("Could not create the worker thread!");
        return;
    }

    // go!
    if (GetThread()->Run() != wxTHREAD_NO_ERROR)
    {
        LOG_ERROR("Could not run the worker thread!");
        return;
    }
}

wxThread::ExitCode MainFrame::Entry() {
    // IMPORTANT:
    // this function gets executed in the secondary thread context!

	GUIBOSSUpdater bUpdater;
	string updateText, updateVersion;
	try {
		if (bUpdater.IsInternetReachable()) {
			try {
				updateVersion = bUpdater.IsUpdateAvailable();
				if (updateVersion.empty()) {
					wxCriticalSectionLocker lock(updateData);
					updateCheckCode = 1;
					updateCheckString = "You are already using the latest version of BOSS.";
					wxQueueEvent(this, new wxThreadEvent(wxEVT_THREAD, wxEVT_COMMAND_MYTHREAD_UPDATE));
				} else {
					wxCriticalSectionLocker lock(updateData);
					updateCheckCode = 0;
					updateCheckString = updateVersion;
					wxQueueEvent(this, new wxThreadEvent(wxEVT_THREAD, wxEVT_COMMAND_MYTHREAD_UPDATE));
				}
			} catch (boss_error &e) {
				wxCriticalSectionLocker lock(updateData);
				updateCheckCode = 2;
				updateCheckString = "Update check failed. Details: " + e.getString();
				wxQueueEvent(this, new wxThreadEvent(wxEVT_THREAD, wxEVT_COMMAND_MYTHREAD_UPDATE));
			}
		} else {
			wxCriticalSectionLocker lock(updateData);
			updateCheckCode = 1;
			updateCheckString = "Update check failed. No Internet connection detected.";
			wxQueueEvent(this, new wxThreadEvent(wxEVT_THREAD, wxEVT_COMMAND_MYTHREAD_UPDATE));
		}
	} catch (boss_error &e) {
		wxCriticalSectionLocker lock(updateData);
		updateCheckCode = 2;
		updateCheckString = "Update check failed. Details: " + e.getString();
		wxQueueEvent(this, new wxThreadEvent(wxEVT_THREAD, wxEVT_COMMAND_MYTHREAD_UPDATE));
	}
	return (wxThread::ExitCode)0;
}

void MainFrame::OnThreadUpdate(wxThreadEvent& evt) {
    wxCriticalSectionLocker lock(updateData);
	if (updateCheckCode == 2 && !isStartup)
		wxMessageBox(updateCheckString, wxT("BOSS: Check For Updates"), wxOK | wxICON_ERROR, this);
	else if (updateCheckCode == 1 && !isStartup)
		wxMessageBox(updateCheckString, wxT("BOSS: Check For Updates"), wxOK | wxICON_INFORMATION, this);
	else if (updateCheckCode == 0) {
		wxMessageDialog *dlg;
		if (!RegKeyExists("HKEY_LOCAL_MACHINE", "Software\\BOSS", "Installed Path"))  //Manual.
			dlg = new wxMessageDialog(this,
				"Update available! New version: " + updateCheckString + "\nThe update may be downloaded from any of the locations listed in the BOSS Log."
				, wxT("BOSS: Check For Updates"), wxOK);
		else {
			GUIBOSSUpdater bUpdater;
			string notes;
			//Display release notes.
			try {
				notes = bUpdater.FetchReleaseNotes(updateCheckString);
			} catch (boss_error &e) {
				wxMessageBox("Failed to get release notes. Details: " + e.getString(), wxT("BOSS: Automatic Updater"), wxOK | wxICON_ERROR, this);
			}
			notes = "Update available! New version: " + updateCheckString + "\nRelease notes:\n\n" + notes + "\n\nDo you want to download and install the update?";
			dlg = new wxMessageDialog(this, notes, wxT("BOSS: Check For Updates"), wxYES_NO);

			if (dlg->ShowModal() == wxID_YES)
				this->Update(updateCheckString);
		}
	}
}

int GUIMlistUpdater::progress(boss::Updater * updater, double dlFraction, double dlTotal) {
	int currentProgress = (int)floor(dlFraction * 10);
	if (currentProgress == 1000)
		--currentProgress; //Stop the progress bar from closing in case of multiple downloads.
	wxProgressDialog* progress = (wxProgressDialog*)updater->progDialog;
	bool cont = progress->Update(currentProgress, "Downloading: " + updater->targetFile);
	if (!cont) {  //the user decided to cancel. Slightly temperamental, the progDia seems to hang a little sometimes and keypresses don't get registered. Can't do much about that.
		uint32_t ans = wxMessageBox(wxT("Are you sure you want to cancel?"), wxT("BOSS: Updater"), wxYES_NO | wxICON_EXCLAMATION, progress);
		if (ans == wxYES)
			return 1;
		progress->Resume();
	}
	return 0;
}

int GUIBOSSUpdater::progress(boss::Updater * updater, double dlFraction, double dlTotal) {
	int currentProgress = (int)floor(dlFraction * 10);
	if (currentProgress == 1000)
		--currentProgress; //Stop the progress bar from closing in case of multiple downloads.
	wxProgressDialog* progress = (wxProgressDialog*)updater->progDialog;
	bool cont = progress->Update(currentProgress, "Downloading: " + updater->targetFile);
	if (!cont) {  //the user decided to cancel. Slightly temperamental, the progDia seems to hang a little sometimes and keypresses don't get registered. Can't do much about that.
		uint32_t ans = wxMessageBox(wxT("Are you sure you want to cancel?"), wxT("BOSS: Updater"), wxYES_NO | wxICON_EXCLAMATION, progress);
		if (ans == wxYES)
			return 1;
		progress->Resume();
	}
	return 0;
}