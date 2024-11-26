#include "MainFrame.h"

//UNCOMMENT BELOW LINE WITH DEFINE TO INTRODUCE DEBUG MODE
//IN DEBUG MODE EVERY STEP IS BEING LOGGED
//WHICH CAN BE QUITE ANNOYING
// 
//#DEFINE SHOW_DEBUG_DIALOGS 1

namespace fs = std::filesystem;
using namespace std::string_literals;

template <typename... Arguments>
std::string JoinCommandLineArguments(Arguments... arguments)
{
	std::ostringstream oss;
	((oss << "\"" << arguments << "\" "), ...); // Fold all arguments using sstream << operator

	std::string result = oss.str();
	if (!result.empty()) {
		result.pop_back(); // Trim trailing space
	}

	return result;
}

wxDEFINE_EVENT(wxEVT_CREATE_NEW_WINDOW, wxThreadEvent);

class NewWindowThread : public wxThread {
public:
	NewWindowThread() : wxThread(wxTHREAD_DETACHED) {}

protected:
	virtual ExitCode Entry() override {
		wxThreadEvent* event = new wxThreadEvent(wxEVT_CREATE_NEW_WINDOW);
		wxQueueEvent(wxTheApp->GetTopWindow(), event);

		return (wxThread::ExitCode)0;
	}
};

enum IDs {
	ID_FPS_AUTO_DETECT_ANALYSIS = 1,
	ID_SAVITZKY_GOLAY_FILTER,
	ID_MOVING_AVERAGE,
	ID_MERGE_EVENTS,
	ID_AUTO_SELECT_EVENTS,
	ID_Timer,
	ID_AUTOMATIC_BINARIZATION_THRESHOLD
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_CHECKBOX(ID_FPS_AUTO_DETECT_ANALYSIS, MainFrame::wxFPSAutoDetectAnalysisToggle)
EVT_CHECKBOX(ID_SAVITZKY_GOLAY_FILTER, MainFrame::wxSavitzkyGolayFilter)
EVT_CHECKBOX(ID_MOVING_AVERAGE, MainFrame::wxMovingAverage)
EVT_CHECKBOX(ID_MERGE_EVENTS, MainFrame::wxMergeEvents)
EVT_CHECKBOX(ID_AUTO_SELECT_EVENTS, MainFrame::wxAutoSelectEvents)
EVT_TIMER(ID_Timer, MainFrame::OnTimer)
EVT_CHAR(MainFrame::OnChar)
EVT_KILL_FOCUS(MainFrame::OnKillFocus)
EVT_TEXT_PASTE(wxID_ANY, MainFrame::OnPaste)
EVT_MENU(wxID_ANY, MainFrame::OnCiteMe)
EVT_THREAD(wxEVT_CREATE_NEW_WINDOW, MainFrame::OnCreateNewWindow)
wxEND_EVENT_TABLE()

void MainFrame::ShowConsole()
{
	HWND debugConsoleWindowHwnd = GetConsoleWindow();

    ShowWindow(debugConsoleWindowHwnd, SW_SHOW);
	SetActiveWindow(debugConsoleWindowHwnd);

	this->SyncToggleDebugWindowMenuItemLabel();
}

void MainFrame::HideConsole()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	this->SyncToggleDebugWindowMenuItemLabel();
}

bool MainFrame::IsConsoleShown() {
	return IsWindowVisible(GetConsoleWindow());
}

void MainFrame::SyncToggleDebugWindowMenuItemLabel() {
	toggleDebugWindowMenuItem->SetItemLabel(MainFrame::IsConsoleShown() ? "Hide debug console" : "Show debug console");
}

void MainFrame::syncAutomaticRecognitionAnalysisFieldStates() {
	isAnyAutomaticAnalysisOptionActive = wxCBFocusField->IsChecked() || wxCBAutomaticBinarizationThreshold->IsChecked();

	controlEnabledState[wxTCFirstFrame] = isAnyAutomaticAnalysisOptionActive;
	controlEnabledState[wxTCLastFrame] = isAnyAutomaticAnalysisOptionActive;
}

std::string MainFrame::getAnalysisResultLabel(const AnalysisResult& result) {
	switch (result) {
	case MainFrame::AnalysisResult::FINISHED:
		return "completed";

	case MainFrame::AnalysisResult::ABORTED:
		return "aborted";

	case MainFrame::AnalysisResult::INVALID_PARAMETERS:
		return "invalid parameters";

	case MainFrame::AnalysisResult::FAILED:
		return "failed";

	default:
		return "unknown";
	}
}

MainFrame::MainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~wxMAXIMIZE_BOX) {

	numberOfFiles = 0;
	panel = new wxPanel(this);

	// Left side of the GUI

	uint leftSideCoordY = 20;

	wxSTListOfFiles = new wxStaticText(panel, wxID_ANY, "List of .mov files for analysis", wxPoint(20, leftSideCoordY), wxSize(280, 20));
	{
		wxFont font = wxSTListOfFiles->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);

		int newSize = font.GetPointSize() + 2;
		font.SetPointSize(newSize);

		wxSTListOfFiles->SetFont(font);
		wxSTListOfFiles->Refresh();
	}

	leftSideCoordY += 25;

	wxCTFileList = new wxTextCtrl(panel, wxID_ANY, "", wxPoint(20, leftSideCoordY), wxSize(280, 80), wxTE_MULTILINE);
	wxCTFileList->SetEditable(false);
	wxCTFileList->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnMouseClick, this);
	wxCTFileList->Bind(wxEVT_SET_FOCUS, &MainFrame::OnFocus, this);

	leftSideCoordY += 90;

	wxBChooseVideo = new wxButton(panel, wxID_ANY, "Choose video for analysis (.mov)", wxPoint(20, leftSideCoordY), wxSize(280, 20));

	leftSideCoordY += 30;

	wxSTNumOfChosenFiles = new wxStaticText(panel, wxID_ANY, "Number of chosen files: 0", wxPoint(20, leftSideCoordY), wxSize(280, 20));

	leftSideCoordY += 25;

	wxBAnalyse = new wxButton(panel, wxID_ANY, "Analyse", wxPoint(20, leftSideCoordY), wxSize(280, 20));
	wxBAnalyse->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event)
		{
			timer->Start(1300);

			std::thread([this]() {
				AnalysisResult result = RunAnalysis();

				timer->Stop();
				wxSTStatusDisplayed->SetLabel(wxSTStatus->GetLabel());
				m_dotCount = 0;

				stopAnalysisThreadFlag = false;

				processingRunning = false;
				UpdateUI(result);
			}).detach();
		});

	leftSideCoordY += 25;

	wxBAbortAnalysis = new wxButton(panel, wxID_ANY, "Abort analysis", wxPoint(20, leftSideCoordY), wxSize(280, 20));
	wxBAbortAnalysis->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) {
		std::cout << "Aborting analysis on user request." << std::endl;

		stopAnalysisThreadFlag = true;
		UpdateUI();
	});

	leftSideCoordY += 30;

	wxSTStatus = new wxStaticText(panel, wxID_ANY, STRING_STATUS_WAITING_FOR_INPUT, wxPoint(20, leftSideCoordY), wxSize(280, 20));
	wxSTStatus->Hide();

	wxSTStatusDisplayed = new wxStaticText(panel, wxID_ANY, STRING_STATUS_WAITING_FOR_INPUT, wxPoint(20, leftSideCoordY), wxSize(280, 20));

	leftSideCoordY += 25;

	wxSTStatusVideo = new wxStaticText(panel, wxID_ANY, "Video: Awaiting", wxPoint(20, leftSideCoordY), wxSize(280, 20));

	leftSideCoordY += 25;

	timer = new wxTimer(this);
	this->Bind(wxEVT_TIMER, &MainFrame::OnTimer, this);

	wxBOutputPath = new wxButton(panel, wxID_ANY, "Output path", wxPoint(20, leftSideCoordY), wxSize(280, 20));

	leftSideCoordY += 25;

	PWSTR path = NULL;

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);

	if (SUCCEEDED(hr)) {
		std::wcout << L"Documents folder: " << path << std::endl;
		wxCTOutputPath = new wxTextCtrl(panel, wxID_ANY, path, wxPoint(20, leftSideCoordY), wxSize(280, 60), wxTE_MULTILINE);
		wxCTOutputPath->SetEditable(false);
		wxCTOutputPath->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnMouseClick, this);
		wxCTOutputPath->Bind(wxEVT_SET_FOCUS, &MainFrame::OnFocus, this);
	}
	else {
		wxSTStatus->SetLabel("Status: ERROR: Failed to get the path to the Documents folder.");
		wxMessageDialog dialog(NULL, "Status: ERROR: Failed to get the path to the Documents folder.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
	}

	// Free the memory allocated by SHGetKnownFolderPath
	if (path) {
		CoTaskMemFree(path);
	}

	leftSideCoordY += 65;

	wxSTOutputOptions = new wxStaticText(panel, wxID_ANY, "Output options", wxPoint(20, leftSideCoordY), wxSize(280, 20));
	{
		wxFont font = wxSTOutputOptions->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);

		int newSize = font.GetPointSize() + 2;
		font.SetPointSize(newSize);

		wxSTOutputOptions->SetFont(font);
		wxSTOutputOptions->Refresh();
	}

	leftSideCoordY += 25;

	wxCBCsvStats = new wxCheckBox(panel, wxID_ANY, "CSV with stats", wxPoint(20, leftSideCoordY));
	wxCBCsvStats->SetValue(true);

	leftSideCoordY += 25;

	wxCBCsvRaw = new wxCheckBox(panel, wxID_ANY, "CSV with raw data", wxPoint(20, leftSideCoordY));
	wxCBCsvRaw->SetValue(true);

	leftSideCoordY += 25;

	wxCBLineChart = new wxCheckBox(panel, wxID_ANY, "Generate line chart", wxPoint(20, leftSideCoordY));
	wxCBLineChart->SetValue(true);

	leftSideCoordY += 25;

	wxCBEventChart = new wxCheckBox(panel, wxID_ANY, "Generate event chart", wxPoint(20, leftSideCoordY));
	wxCBEventChart->SetValue(true);

	leftSideCoordY += 25;

	// Right side of the GUI

	uint rightSideCoordY = 20;

	wxSTOptionsForAnalysis = new wxStaticText(panel, wxID_ANY, "Options of automatic analysis:", wxPoint(320, rightSideCoordY), wxSize(280, rightSideCoordY));
	{
		wxFont font = wxSTOptionsForAnalysis->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);

		int newSize = font.GetPointSize() + 2;
		font.SetPointSize(newSize);

		wxSTOptionsForAnalysis->SetFont(font);
		wxSTOptionsForAnalysis->Refresh();
	}

	rightSideCoordY += 25;

	wxCBAutomaticBinarizationThreshold = new wxCheckBox(panel, ID_AUTOMATIC_BINARIZATION_THRESHOLD, "Auto binarization threshold", wxPoint(320, rightSideCoordY));
	wxCBAutomaticBinarizationThreshold->SetValue(true);
	wxCBAutomaticBinarizationThreshold->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event)
		{
			controlEnabledState[wxTCBinarizationThreshold] = !event.IsChecked();
			
			syncAutomaticRecognitionAnalysisFieldStates();

			UpdateUI();
		});


	rightSideCoordY += 25;

	wxSTBinarizationThreshold = new wxStaticText(panel, wxID_ANY, "Binarization threshold", wxPoint(320, rightSideCoordY), wxSize(150, 20));
	wxTCBinarizationThreshold = new wxTextCtrl(panel, wxID_ANY, "100", wxPoint(510, rightSideCoordY), wxSize(40, 20));
	controlEnabledState[wxTCBinarizationThreshold] = false;
	wxTCBinarizationThreshold->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCBinarizationThreshold->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCBinarizationThreshold->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);

	wxSTBinarizationThresholdRange = new wxStaticText(panel, wxID_ANY, "0-255", wxPoint(560, rightSideCoordY), wxSize(40, 20));

	rightSideCoordY += 25;

	wxCBFocusField = new wxCheckBox(panel, wxID_ANY, "Focus field", wxPoint(320, rightSideCoordY));
	wxCBFocusField->SetValue(true);
	wxCBFocusField->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
		bool isCBFocusFieldEnabled = event.IsChecked();
		
		controlEnabledState[wxTCSizeOfFocusField] = isCBFocusFieldEnabled;
		controlEnabledState[wxTCPercentileOfTheHighestValues] = isCBFocusFieldEnabled;

		syncAutomaticRecognitionAnalysisFieldStates();

		UpdateUI();
	});

	rightSideCoordY += 25;

	wxCBFocusCoordinatesAnalysis = new wxStaticText(panel, wxID_ANY, "Focus coordinates analysis:", wxPoint(320, rightSideCoordY), wxSize(280, 20));

	rightSideCoordY += 25;

	wxTCSizeOfFocusField = new wxTextCtrl(panel, wxID_ANY, std::to_string(DEFAULT_SIZE_OF_FOCUS_FIELD), wxPoint(320, rightSideCoordY), wxSize(40, 20));
	wxTCSizeOfFocusField->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCSizeOfFocusField->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCSizeOfFocusField->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTSizeOfFocusField = new wxStaticText(panel, wxID_ANY, "Size of the focus field", wxPoint(370, rightSideCoordY), wxSize(80, 30));
	
	wxTCPercentileOfTheHighestValues = new wxTextCtrl(panel, wxID_ANY, std::to_string(DEFAULT_PERCENTILE_OF_HIGHEST_VALUES), wxPoint(460, rightSideCoordY), wxSize(40, 20));
	wxTCPercentileOfTheHighestValues->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCPercentileOfTheHighestValues->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCPercentileOfTheHighestValues->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTPercentileOfTheHighestValues = new wxStaticText(panel, wxID_ANY, "Percentile of the highest values", wxPoint(510, rightSideCoordY), wxSize(80, 50));

	rightSideCoordY += 50;

	wxSTAutomaticRecognitionAnalysis = new wxStaticText(panel, wxID_ANY, "Determine recognition analysis depth", wxPoint(320, rightSideCoordY));

	rightSideCoordY += 25;

	wxTCFirstFrame = new wxTextCtrl(panel, wxID_ANY, "0", wxPoint(320, rightSideCoordY), wxSize(40, 20));
	wxTCFirstFrame->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCFirstFrame->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCFirstFrame->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTFirstFrame = new wxStaticText(panel, wxID_ANY, "First frame", wxPoint(370, rightSideCoordY), wxSize(80, 20));

	wxTCLastFrame = new wxTextCtrl(panel, wxID_ANY, "90", wxPoint(460, rightSideCoordY), wxSize(40, 20));
	wxTCLastFrame->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCLastFrame->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCLastFrame->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTLastFrame = new wxStaticText(panel, wxID_ANY, "Last frame", wxPoint(510, rightSideCoordY), wxSize(80, 30));

	rightSideCoordY += 25;

	wxSTOptionsOfEventDetection = new wxStaticText(panel, wxID_ANY, "Options of event detection:", wxPoint(320, rightSideCoordY), wxSize(280, 30));
	{
		wxFont font = wxSTOptionsOfEventDetection->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);

		int newSize = font.GetPointSize() + 2;
		font.SetPointSize(newSize);

		wxSTOptionsOfEventDetection->SetFont(font);
		wxSTOptionsOfEventDetection->Refresh();
	}

	rightSideCoordY += 25;

	wxTCFPS = new wxTextCtrl(panel, wxID_ANY, "30", wxPoint(320, rightSideCoordY), wxSize(40, 20));
	wxTCFPS->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCFPS->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCFPS->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);

	wxSTFPS = new wxStaticText(panel, wxID_ANY, "FPS", wxPoint(370, rightSideCoordY), wxSize(40, 20));

	rightSideCoordY += 25;

	wxCBSavitzkyGolayFilter = new wxCheckBox(panel, ID_SAVITZKY_GOLAY_FILTER, "Savitzky-Golay filter", wxPoint(320, rightSideCoordY));
	wxCBSavitzkyGolayFilter->SetValue(true);

	rightSideCoordY += 25;

	wxTCWindowLengthSGF = new wxTextCtrl(panel, wxID_ANY, "7", wxPoint(320, rightSideCoordY), wxSize(40, 20));
	wxTCWindowLengthSGF->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCWindowLengthSGF->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCWindowLengthSGF->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTWindowLengthSGF = new wxStaticText(panel, wxID_ANY, "Window length", wxPoint(370, rightSideCoordY), wxSize(80, 40));

	wxTCPolyorder = new wxTextCtrl(panel, wxID_ANY, "5", wxPoint(460, rightSideCoordY), wxSize(40, 20));
	wxTCPolyorder->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCPolyorder->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCPolyorder->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTPolyorder = new wxStaticText(panel, wxID_ANY, "Polyorder", wxPoint(510, rightSideCoordY), wxSize(80, 20));

	rightSideCoordY += 40;

	wxCBMovingAverage = new wxCheckBox(panel, ID_MOVING_AVERAGE, "Moving average", wxPoint(320, rightSideCoordY));
	wxCBMovingAverage->SetValue(true);

	rightSideCoordY += 25;

	wxTCWindowLengthMA = new wxTextCtrl(panel, wxID_ANY, "2", wxPoint(320, rightSideCoordY), wxSize(40, 20));
	wxTCWindowLengthMA->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCWindowLengthMA->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCWindowLengthMA->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTWindowLengthMA = new wxStaticText(panel, wxID_ANY, "Window length", wxPoint(370, rightSideCoordY), wxSize(80, 40));

	wxTCNumberOfRepetitions = new wxTextCtrl(panel, wxID_ANY, "10", wxPoint(460, rightSideCoordY), wxSize(40, 20));
	wxTCNumberOfRepetitions->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCNumberOfRepetitions->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCNumberOfRepetitions->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTNumberOfRepetitions = new wxStaticText(panel, wxID_ANY, "Number of repetitions", wxPoint(510, rightSideCoordY), wxSize(80, 40));

	rightSideCoordY += 45;

	wxSTTrimList = new wxStaticText(panel, wxID_ANY, "Trim list:", wxPoint(320, rightSideCoordY), wxSize(280, 20));

	rightSideCoordY += 25;

	wxTCLeftTrim = new wxTextCtrl(panel, wxID_ANY, "0", wxPoint(320, rightSideCoordY), wxSize(40, 20));
	wxTCLeftTrim->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCLeftTrim->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCLeftTrim->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTLeftTrim = new wxStaticText(panel, wxID_ANY, "Left trim", wxPoint(370, rightSideCoordY), wxSize(80, 30));

	wxTCRightTrim = new wxTextCtrl(panel, wxID_ANY, "0", wxPoint(460, rightSideCoordY), wxSize(40, 20));
	wxTCRightTrim->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCRightTrim->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCRightTrim->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTRightTrim = new wxStaticText(panel, wxID_ANY, "Right trim", wxPoint(510, rightSideCoordY), wxSize(80, 30));

	rightSideCoordY += 30;

	wxCBAutoDetectEvents = new wxCheckBox(panel, wxID_ANY, "Auto detect events", wxPoint(320, rightSideCoordY));
	wxCBAutoDetectEvents->SetValue(true);

	rightSideCoordY += 25;

	wxSTAutoMovementThresholdStatic = new wxStaticText(panel, wxID_ANY, "Movement threshold", wxPoint(345, rightSideCoordY));

	wxTCAutoMovementThreshold = new wxTextCtrl(panel, wxID_ANY, "0.45", wxPoint(510, rightSideCoordY), wxSize(40, 20));
	wxTCAutoMovementThreshold->Bind(wxEVT_CHAR, &MainFrame::OnChar, this);
	wxTCAutoMovementThreshold->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCAutoMovementThreshold->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTAutoMovementThreshold = new wxStaticText(panel, wxID_ANY, "units", wxPoint(560, rightSideCoordY), wxSize(40, 20));

	rightSideCoordY += 25;

	wxCBAutoMergeEvents = new wxCheckBox(panel, ID_MERGE_EVENTS, "Auto merge events", wxPoint(345, rightSideCoordY));
	wxCBAutoMergeEvents->SetValue(true);

	wxTCAutoMergeEvents = new wxTextCtrl(panel, wxID_ANY, "0", wxPoint(510, rightSideCoordY), wxSize(40, 20));
	wxTCAutoMergeEvents->Bind(wxEVT_CHAR, &MainFrame::OnChar, this);
	wxTCAutoMergeEvents->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCAutoMergeEvents->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTAutoMergeEvents = new wxStaticText(panel, wxID_ANY, "frames", wxPoint(560, rightSideCoordY), wxSize(40, 20));

	rightSideCoordY += 25;

	wxCBAutoSelectEvents = new wxCheckBox(panel, ID_AUTO_SELECT_EVENTS, "Auto select events", wxPoint(345, rightSideCoordY));
	wxCBAutoSelectEvents->SetValue(true);

	wxTCAutoSelectEvents = new wxTextCtrl(panel, wxID_ANY, "5", wxPoint(510, rightSideCoordY), wxSize(40, 20));
	wxTCAutoSelectEvents->Bind(wxEVT_CHAR, &MainFrame::OnChar, this);
	wxTCAutoSelectEvents->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCAutoSelectEvents->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTAutoSelectEvents = new wxStaticText(panel, wxID_ANY, "units", wxPoint(560, rightSideCoordY), wxSize(40, 20));

	rightSideCoordY += 25;

	wxCBContractionRelaxationChart = new wxCheckBox(panel, wxID_ANY, "Auto contraction-relaxation analysis", wxPoint(345, rightSideCoordY));
	wxCBContractionRelaxationChart->SetValue(true);

	wxMenuBar* menuBar = new wxMenuBar;

	wxMenu* debugWindowMenu = new wxMenu;
	toggleDebugWindowMenuItem = new wxMenuItem(debugWindowMenu, wxID_ANY, "Show debug console");
	debugWindowMenu->Append(toggleDebugWindowMenuItem);
	menuBar->Append(debugWindowMenu, "Debug console");

	wxMenu* aboutAuthorsMenu = new wxMenu;
	citeMeMenuItem = new wxMenuItem(aboutAuthorsMenu, wxID_ANY, "About the authors");
	aboutAuthorsMenu->Append(citeMeMenuItem);
	menuBar->Append(aboutAuthorsMenu, "Cite me");

	wxMenu* licensesMenu = new wxMenu;
	openLicensesFolderMenuItem = new wxMenuItem(licensesMenu, wxID_ANY, "Open licenses folder");
	licensesMenu->Append(openLicensesFolderMenuItem);
	menuBar->Append(licensesMenu, "Licenses");

	SetMenuBar(menuBar);

	Bind(wxEVT_MENU, &MainFrame::OnCiteMe, this, citeMeMenuItem->GetId());
	Bind(wxEVT_MENU, &MainFrame::OpenLicensesFolder, this, openLicensesFolderMenuItem->GetId());
	Bind(wxEVT_MENU, &MainFrame::ToggleConsole, this, toggleDebugWindowMenuItem->GetId());
	Bind(wxEVT_CREATE_NEW_WINDOW, &MainFrame::OnCreateNewWindow, this);
	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

	{
		int windowHeight = rightSideCoordY + 90;
		SetSizeHints(MAIN_WINDOW_WIDTH, windowHeight, MAIN_WINDOW_WIDTH, windowHeight);
	}

	wxBOutputPath->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event)
		{
			wxDirDialog dialog(this, "Choose a directory!", wxGetCwd(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

			if (dialog.ShowModal() == wxID_OK)
			{
				wxString path = dialog.GetPath();
				wxCTOutputPath->SetEditable(true);
				wxCTOutputPath->ChangeValue("");
				wxCTOutputPath->AppendText(path);
				wxCTOutputPath->SetLabel(path);
				wxCTOutputPath->SetEditable(false);
			}
		});

	wxBChooseVideo->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event)
		{
			wxFileDialog dialog(this, "Choose a file!", wxGetCwd(), "out.mov", "Mov files (*.mov)|*.mov", wxFD_OPEN | wxFD_MULTIPLE);
			if (dialog.ShowModal() == wxID_OK)
			{
				wxArrayString filePaths;
				dialog.GetPaths(filePaths);

				wxCTFileList->Clear();

				directories.clear();

				for (wxString path : filePaths) {
					directories.push_back(path);
				}

				for (wxString directory : directories) {
					wxFileName fileName(directory);
					wxCTFileList->AppendText(fileName.GetFullName() + "\n");
				}

				numberOfFiles = filePaths.size();

				wxString str = wxString::Format("Number of chosen files: %d", numberOfFiles);
				wxSTNumOfChosenFiles->SetLabel(str);
				wxCTFileList->SetEditable(false);
			}
		});

	wxCBAutoDetectEvents->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event)
		{
			for (wxControl* ctrl : std::initializer_list<wxControl*>{
				wxSTAutoMovementThresholdStatic,
				wxTCAutoMovementThreshold,
				wxSTAutoMovementThreshold,
				wxCBAutoMergeEvents,
				wxTCAutoMergeEvents,
				wxSTAutoMergeEvents,
				wxCBAutoSelectEvents,
				wxTCAutoSelectEvents,
				wxSTAutoSelectEvents,
				wxCBContractionRelaxationChart
			}) {
				controlEnabledState[ctrl] = event.IsChecked();
			}

			UpdateUI();
		});

	allInteractiveControls.insert(allInteractiveControls.end(), {
		wxCTFileList, wxBChooseVideo, wxBAnalyse, wxBOutputPath, wxCTOutputPath, wxSTBinarizationThreshold, wxCBCsvStats,
		wxCBCsvRaw, wxCBLineChart, wxCBEventChart, wxTCFPS, wxTCBinarizationThreshold, wxTCFirstFrame, wxTCLastFrame,
		wxTCSizeOfFocusField, wxTCPercentileOfTheHighestValues, wxCBSavitzkyGolayFilter, wxTCWindowLengthSGF, wxTCPolyorder,
		wxCBMovingAverage, wxTCWindowLengthMA, wxTCNumberOfRepetitions, wxTCAutoMovementThreshold, wxTCLeftTrim, wxTCRightTrim,
		wxCBAutoMergeEvents, wxTCAutoMergeEvents, wxCBAutoSelectEvents, wxTCAutoSelectEvents, wxSTFPS, wxSTBinarizationThresholdRange,
		wxSTFirstFrame, wxSTLastFrame, wxCBFocusCoordinatesAnalysis, wxSTSizeOfFocusField, wxSTPercentileOfTheHighestValues, wxSTWindowLengthSGF,
		wxSTPolyorder, wxSTWindowLengthMA, wxCBAutoDetectEvents, wxSTNumberOfRepetitions, wxSTAutoMovementThresholdStatic,
		wxSTAutoMovementThreshold, wxSTTrimList, wxSTLeftTrim, wxSTRightTrim, wxSTAutoMergeEvents, wxSTAutoSelectEvents,
		wxCBAutomaticBinarizationThreshold, wxCBFocusField, wxCBContractionRelaxationChart
	});

	syncAutomaticRecognitionAnalysisFieldStates();
	UpdateUI();

	if (IsDebuggerPresent())
	{
		// if a debugger is attached, the user may want the debug window to be shown by default
		ShowConsole();
	}
	else
	{
		// if this is a release session, hide the debug window by default
		HideConsole();
	}
}

void MainFrame::UpdateUI(const std::optional<const MainFrame::AnalysisResult>& analysisResult)
{
	for (wxControl*& ctrl : allInteractiveControls) {
		bool isNormallyEnabled = !processingRunning; // default value

		if (!processingRunning) {
			auto isNormallyEnabledIt = controlEnabledState.find(ctrl);

			if (isNormallyEnabledIt != controlEnabledState.end()) {
				isNormallyEnabled = isNormallyEnabledIt->second;
			}
		}

		ctrl->Enable(isNormallyEnabled);
	}

	// exception: wxBAbortAnalysis is controlled separately & is not on allInteractiveControls
	// list since its state is inverted w.r.t. analysis running state
	wxBAbortAnalysis->Enable(processingRunning && !stopAnalysisThreadFlag);

	if (analysisResult.has_value())
	{
		std::string label = "Status: Processing "s + getAnalysisResultLabel(analysisResult.value()) + "!";
		wxSTStatus->SetLabel(label);
		wxSTStatusDisplayed->SetLabel(label);
	}
}

void MainFrame::wxFPSAutoDetectAnalysisToggle(wxCommandEvent& evt) {
	controlEnabledState[wxTCFPS] = !evt.IsChecked();
	UpdateUI();
}

void MainFrame::wxSavitzkyGolayFilter(wxCommandEvent& evt) {
	for (wxControl* ctrl : std::initializer_list<wxControl*>{
		wxTCWindowLengthSGF,
		wxTCPolyorder
	}) {
		controlEnabledState[ctrl] = evt.IsChecked();
	}

	UpdateUI();
}

void MainFrame::wxMovingAverage(wxCommandEvent& evt) {
	for (wxControl* ctrl : std::initializer_list<wxControl*>{
		wxTCWindowLengthMA,
		wxTCNumberOfRepetitions
	}) {
		controlEnabledState[ctrl] = evt.IsChecked();
	}

	UpdateUI();
}

void MainFrame::wxMergeEvents(wxCommandEvent& evt) {
	controlEnabledState[wxTCAutoMergeEvents] = evt.IsChecked();

	UpdateUI();
}

void MainFrame::wxAutoSelectEvents(wxCommandEvent& evt) {
	controlEnabledState[wxTCAutoSelectEvents] = evt.IsChecked();

	UpdateUI();
}

void MainFrame::SetTaskBarIcon()
{
	HWND hwnd = (HWND)GetHWND();

	HICON hIcon = (HICON)LoadImage(NULL, L"./icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}

MainFrame::AnalysisResult MainFrame::RunAnalysis()
{
	// Place the entire block of calculation code here.
	// Make sure to remove or adjust any UI interaction code that doesn't make sense in a background thread.

	stopAnalysisThreadFlag = false;

	///PREPROCESSING

	//RESIZING - DO PRZETESTOWANIA

	//BLEDNY INPUT - ZABEZPIECZONE

	int fps;
	wxString strTemp;

	std::string errorMessage = "";

	strTemp = wxTCFPS->GetValue();
	long longTemp;
	if (strTemp.ToLong(&longTemp)) {
		// Conversion successful, int_value now contains the integer
		fps = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: FPS has not been specified by the user, assumed to be 30.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT | wxICON_WARNING);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of FPS.\n";
	}

	if (fps <= 0) {
		errorMessage += "FPS should be more than zero.\n";
	}

	strTemp = wxTCFirstFrame->GetValue();
	int startFrame;
	if (strTemp.ToLong(&longTemp)) {
		startFrame = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Index of the start frame was not specified by the user, it is assumed to be FPS times two", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of start frame.\n";
	}

	if (startFrame < 0) {
		errorMessage += "Start frame should be positive.\n";
	}

	strTemp = wxTCLastFrame->GetValue();
	int endFrame;
	if (strTemp.ToLong(&longTemp)) {
		endFrame = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Index of the end frame was not specified, it is assumed to be FPS times twelve", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of end frame.\n";
	}

	if (endFrame < 0) {
		errorMessage += "End frame should be positive.\n";
	}

	if (endFrame - startFrame < 2) {
		errorMessage += "Start and end frame should min. 2 frames apart.\n";
	}

	double squarePercent;
	double topPercent;

	// Find max sum square coordinates
	strTemp = wxTCSizeOfFocusField->IsEnabled() ? wxTCSizeOfFocusField->GetValue() : std::to_string(DEFAULT_SIZE_OF_FOCUS_FIELD);
	if (!strTemp.ToDouble(&squarePercent)) {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Focus field not specified, it is assumed to be 25", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of size of focus field.\n";
	}

	if (!(squarePercent >= 1 && squarePercent <= 100)) {
		errorMessage += "Square percent value should be (1-100).\n";
	}

	strTemp = wxTCPercentileOfTheHighestValues->IsEnabled() ? wxTCPercentileOfTheHighestValues->GetValue() : std::to_string(DEFAULT_PERCENTILE_OF_HIGHEST_VALUES);
	if (!strTemp.ToDouble(&topPercent)) {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Percentile of the highest values, it is assumed to be 90", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value percentile of the highest values is wrong.\n";
	}

	if (!(topPercent > 0 && topPercent <= 100)) {
		errorMessage += "Top percent value should be (0-100) excluding zero.\n";
	}

	strTemp = wxTCWindowLengthSGF->GetValue();
	int savitzkyGolayWindowLength;
	if (strTemp.ToLong(&longTemp)) {
		savitzkyGolayWindowLength = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Window length (savitzky-golay filter) not specified, it is assumed to be 7", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of window length (savitzky-golay filter).\n";
	}

	if (savitzkyGolayWindowLength % 2 == 0) {
		errorMessage += "Window length value should be odd.\n";
	}

	strTemp = wxTCPolyorder->GetValue();
	int polyorder;
	if (strTemp.ToLong(&longTemp)) {
		polyorder = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Polyorder (savitzky-golay filter) not specified, it is assumed to be 5", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in polyorder field.\n";
	}

	if (polyorder >= savitzkyGolayWindowLength) {
		errorMessage += "Polyorder value should be lower than window length.\n";
	}

	strTemp = wxTCWindowLengthMA->GetValue();
	int movingAverageWindowLength;
	if (strTemp.ToLong(&longTemp)) {
		movingAverageWindowLength = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Window length (Moving Average) not specified, it is assumed to be FPS times 0.1", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in window length (moving average) field.\n";
	}

	if (movingAverageWindowLength < 2) {
		errorMessage += "Window length (moving average) should be higher than two.\n";
	}

	strTemp = wxTCNumberOfRepetitions->GetValue();
	int numberOfRepetitions;
	if (strTemp.ToLong(&longTemp)) {
		numberOfRepetitions = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Number of repetitions (Moving Average) not specified, it is assumed to be 10", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in number of repetitions field.\n";
	}

	if (numberOfRepetitions < 0) {
		errorMessage += "Number of repetitions value should be higher than zero.\n";
	}

	strTemp = wxTCAutoMovementThreshold->GetValue();
	double movementThreshold;
	if (!strTemp.ToDouble(&movementThreshold)) {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Auto movement threshold not specified, it is assumed to be 0.45", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong value format in movement threshold field.\n";
	}

	if (movementThreshold <= 0.0) {
		errorMessage += "Movement threshold should be higher than zero.\n";
	}

	strTemp = wxTCLeftTrim->GetValue();
	int leftTrim;
	if (strTemp.ToLong(&longTemp)) {
		leftTrim = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Left trim not specified, it is assumed to be 0", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in left trim field.\n";
	}

	if (leftTrim < 0) {
		errorMessage += "Left trim value should be non-negative.\n";
	}

	strTemp = wxTCRightTrim->GetValue();
	int rightTrim;
	if (strTemp.ToLong(&longTemp)) {
		rightTrim = static_cast<int>(longTemp);
	}
	else {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Right trim not specified, it is assumed to be 0", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty of wrong format value in right trim field.\n";
		return MainFrame::AnalysisResult::INVALID_PARAMETERS;
	}

	if (rightTrim < 0) {
		errorMessage += "Right trim value should be non-negative.\n";
	}

	strTemp = wxTCAutoMergeEvents->GetValue();
	double autoMergeEvents;
	if (!strTemp.ToDouble(&autoMergeEvents)) {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Auto merged events not specified, it is assumed to be 0", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in auto merged events field.\n";
	}

	if (autoMergeEvents < 0.0) {
		errorMessage += "Right trim value should be non-negative.\n";
	}

	strTemp = wxTCAutoSelectEvents->GetValue();
	double autoSelectEvents;
	if (!strTemp.ToDouble(&autoSelectEvents)) {
#if SHOW_DEBUG_DIALOGS
		wxMessageDialog dialog(NULL, "WARNING: Auto merged events not specified, it is assumed to be 0", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in auto select events field.\n";
	}

	if (autoSelectEvents < 0.0) {
		errorMessage += "Auto select events value should be non-negative.\n";
	}

	strTemp = wxCTOutputPath->GetValue();
	std::string outputPath = std::string(strTemp.mb_str());

	if (!fs::exists(outputPath)) {
		errorMessage += "Output path has to be specified.\n";
	}

	if (directories.empty()) {
		errorMessage += "Video paths have to be specified.\n";
	}

	if (errorMessage != "") {
		wxMessageDialog dialog(NULL, "Following inputs have to be corrected:\n\n" + errorMessage, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
		return MainFrame::AnalysisResult::INVALID_PARAMETERS;
	}

	processingRunning = true;
	UpdateUI();

	wxCommandEvent citeMeEvent;
	OnCiteMe(citeMeEvent);

	std::string dateTime = MiscUtils::GetCurrentDateTime();
	std::string outputFolderName = "camystat_output_" + dateTime;
	fs::path outputFolderPath = fs::path(outputPath) / outputFolderName;

	FsUtils::CreateDirectoryWithCheck(outputFolderPath);

	const bool outputCsvStats = wxCBCsvStats->IsChecked();
	const bool outputCsvRaw = wxCBCsvRaw->IsChecked();
	const bool outputLineChart = wxCBLineChart->IsChecked();
	const bool outputEventChart = wxCBEventChart->IsChecked();

	// write out parameters
	std::cout << std::endl << "---------------------------------------------------------" << std::endl;
	std::cout << "Program parameters:" << std::endl << std::endl;
	std::cout << "Auto binarization threshold: " << (wxCBAutomaticBinarizationThreshold->IsChecked() ? "true" : "false") << std::endl;
	
	if (isAnyAutomaticAnalysisOptionActive) {
		std::cout << "Focus field square percent: " << squarePercent << "%" << std::endl;
		std::cout << "Percentile of the highest values (top percent): " << topPercent << "%" << std::endl;
	}
	else {
		std::cout << "Automatic analysis (focus field or auto binarization threshold) disabled" << std::endl;
	}

	std::cout << "First frame: " << startFrame << std::endl;
	std::cout << "Last frame: " << endFrame << std::endl;
	std::cout << "Frames per second (fps): " << fps << std::endl;
	
	if (wxCBSavitzkyGolayFilter->IsChecked()) {
		std::cout << "Savitzky-Golay window length: " << savitzkyGolayWindowLength << std::endl;
		std::cout << "Savitzky-Golay polynomial order (polyorder): " << polyorder << std::endl;
	}
	else {
		std::cout << "Savitzky-Golay filter disabled" << std::endl;
	}

	if (wxCBMovingAverage->IsChecked()) {
		std::cout << "Moving Average window length: " << movingAverageWindowLength << std::endl;
		std::cout << "Moving Average number of repetitions: " << numberOfRepetitions << std::endl;
	}
	else {
		std::cout << "Moving Average filter disabled" << std::endl;
	}

	std::cout << "Left trim: " << leftTrim << std::endl;
	std::cout << "Right trim: " << rightTrim << std::endl;

	const bool analyseContractionRelaxationEvents = wxCBContractionRelaxationChart->IsChecked();

	if (wxCBAutoDetectEvents->IsChecked()) {
		std::cout << "Movement threshold: " << movementThreshold << std::endl;
		std::cout << "Automatically merge events: " << autoMergeEvents << " frames" << std::endl;
		std::cout << "Automatically select events: " << autoSelectEvents << " units" << std::endl;
		std::cout << "Analyse contraction-relaxation events: " << MiscUtils::BoolToStringDebug(analyseContractionRelaxationEvents) << std::endl;
	}

	std::cout << "Output path base: " << outputPath << std::endl;
	std::cout << "Output folder path: " << outputFolderPath << std::endl;
	std::cout << std::endl;
	std::cout << "Output - CSV with stats: " << MiscUtils::BoolToStringDebug(outputCsvStats) << std::endl;
	std::cout << "Output - CSV with raw data: " << MiscUtils::BoolToStringDebug(outputCsvRaw) << std::endl;
	std::cout << "Output - line chart: " << MiscUtils::BoolToStringDebug(outputLineChart) << std::endl;
	std::cout << "Output - event chart: " << MiscUtils::BoolToStringDebug(outputEventChart) << std::endl;
	std::cout << "---------------------------------------------------------" << std::endl << std::endl;

	for (wxString strTemp : directories) {
		if (stopAnalysisThreadFlag) return MainFrame::AnalysisResult::ABORTED;

		std::string inputPath = std::string(strTemp.mb_str());

		if (inputPath == "") {
			wxSTStatus->SetLabel("Status: List of files .mov shouldn't be empty.");
			wxMessageDialog dialog(NULL, "ERROR: List of files .mov shouldn't be empty.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
			return MainFrame::AnalysisResult::INVALID_PARAMETERS;
		}

		fs::path pathObj(inputPath);

		wxSTStatus->SetLabel("Status: Initializing...");

		std::string fileName = pathObj.stem().string();

		strTemp = wxCTOutputPath->GetValue();
		std::string outputPath = std::string(strTemp.mb_str());

		if (!fs::exists(outputPath)) {
			return MainFrame::AnalysisResult::FAILED;
		}

		std::string outputFolderName2 = "camystat_output_" + fileName;
		wxSTStatusVideo->SetLabel("Video: " + fileName);
		fs::path outputFolderPath = fs::path(outputPath) / outputFolderName / outputFolderName2;

		wxSTStatus->SetLabel("Status: Preparing output folders");

		// Create the main output folder
		FsUtils::CreateDirectoryWithCheck(outputFolderPath);

		// Create the subfolders
		if (isAnyAutomaticAnalysisOptionActive) {
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "activity_heatmap");
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "heatmap_coordinates");
		}
		if (outputCsvStats) {
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "csv_stats");
		}
		if (outputCsvRaw) {
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "csv_raw");
		}
		if (outputLineChart) {
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "raw_chart");
		}
		if (outputEventChart) {
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "normalized_chart");
		}
		if (analyseContractionRelaxationEvents) {
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "contraction_relaxation_chart");
		}
		if (wxCBAutomaticBinarizationThreshold->IsChecked()) {
			FsUtils::CreateDirectoryWithCheck(outputFolderPath / "auto_binarization_threshold");
		}

		wxSTStatus->SetLabel("Status: Reading current path");
		TCHAR buffer[MAX_PATH];
		DWORD length = GetCurrentDirectory(MAX_PATH, buffer);
		std::string plotPath = Utils::TCHARToString(buffer);
		plotPath = plotPath.substr(0, plotPath.size() - 1);
		wxSTStatus->SetLabel("Status: Calculating path to plot");
		plotPath = plotPath + "\\plot.exe";
#if SHOW_DEBUG_DIALOGS
		{
			wxMessageDialog dialog(NULL, "LOG: path to the plot.exe: " + plotPath, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
			wxMessageDialog dialog(NULL, plotPath, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		TCHAR tempPath[MAX_PATH];

		if (GetTempPath(MAX_PATH, tempPath) == 0) {
			wxSTStatus->SetLabel("Status: ERROR: Could not locate appdata folder!");
			wxMessageDialog dialog(NULL, "ERROR: Could not locate the appdata folder.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
			return MainFrame::AnalysisResult::FAILED;
		}

		std::wstring tempPathWstr(tempPath);

		std::string tempPathStr(tempPathWstr.begin(), tempPathWstr.end());
		{
			wxSTStatus->SetLabel("Status: Detected appdata folder");
		}

		std::wstring cammystatTempPathWstr = FsUtils::StringToWString(tempPathStr + "Cammystat");
		{
			wxSTStatus->SetLabel("Status: Cammystat folder located");
		}
		if (!FsUtils::FolderExists(cammystatTempPathWstr)) {
			FsUtils::CreateDirectoryWithCheck(cammystatTempPathWstr);
			wxSTStatus->SetLabel("Status: Cammystat appdata folder has been created");
		}

		std::string cammystatTempPathStr(cammystatTempPathWstr.begin(), cammystatTempPathWstr.end());

		std::string heatmapPath = outputFolderPath.string() + "\\activity_heatmap\\" + fileName + ".png";

		if (FsUtils::FolderExists(cammystatTempPathWstr)) {
			std::string tempPath2 = cammystatTempPathStr + "\\" + fileName;
			FsUtils::CreateDirectoryWithCheck(tempPath2);
		}

		std::string pathToRemove = cammystatTempPathStr + "\\" + fileName;

		const auto internalCleanup = [&pathToRemove]() {
			// clean up the files after analysis is completed or aborted
			if (wxFileName::DirExists(pathToRemove))
			{
				FsUtils::RemoveDirectoryRecursively(pathToRemove);
			}
		};

		wxSTStatus->SetLabel("Status: Reading form's parameters...");

		double scaleFactor = 2.0;

		std::vector<std::string> inputPaths;
		std::stringstream ss(inputPath);
		std::string line;

		// Use std::getline to extract each line separated by '\n'
		while (std::getline(ss, line)) {
			inputPaths.push_back(line);
		}

		int threshold;
		if (wxCBAutomaticBinarizationThreshold->IsChecked()) {
			// automatically calculate binarization threshold as per Marcin's algorithm design

			try {
				std::vector<int> xorScoresForThresholds;
				std::tie(threshold, xorScoresForThresholds) = V3::Preprocessing::calculateBinarizationThreshold(
					inputPath,
					startFrame,
					endFrame,
					[this](V3::Preprocessing::BinarizationThresholdCalcProgress stage, std::optional<double> maybeProgress, std::optional<int> maybeRetryNumber)
					{
						std::stringstream status;
						status << "Status: Calc. bin. thresh. ";

						switch(stage){
							case V3::Preprocessing::BinarizationThresholdCalcProgress::STARTING:
								status << "starting";
								break;

							case V3::Preprocessing::BinarizationThresholdCalcProgress::FINDING_MAX_BRIGHTNESS_DIFF_FRAMES:
								status << "max diff. frames";
								break;

							case V3::Preprocessing::BinarizationThresholdCalcProgress::CALCULATING_XOR_SCORES:
								status << "XOR scores";
								break;
						}

						if (maybeProgress.has_value()) {
							status << " " << (int) std::round(maybeProgress.value() * 100) << "%";
						}

						if (maybeRetryNumber.has_value()) {
							status << " (retry " << maybeRetryNumber.value() << ")";
						}

						wxSTStatus->SetLabel(status.str());
					},
					stopAnalysisThreadFlag
				);

				wxTCBinarizationThreshold->SetValue(std::to_string(threshold));

				std::string xorScoresForThresholdsPath = cammystatTempPathStr + "\\" + fileName + "\\autoBinarizationThresholdXOR_" + fileName + ".csv";
				wxSTStatus->SetLabel("Status: Saving threshold calculation results");
				Utils::writeVectorToFile(xorScoresForThresholdsPath, xorScoresForThresholds);

				std::string calculatedThresholdPath = cammystatTempPathStr + "\\" + fileName + "\\autoBinarizationThreshold_" + fileName + ".txt";
				std::ofstream outFile(calculatedThresholdPath);
				if (outFile.is_open()) {
					outFile << threshold;
					outFile.close();
				}
				else {
					std::cerr << "Failed to open file for writing: " << calculatedThresholdPath << std::endl;
				}

				std::string savePath = outputFolderPath.string() + "\\auto_binarization_threshold";
				Utils::callPlotExe(plotPath, "auto_binarization_thresh", "\"" + xorScoresForThresholdsPath + "\" \"" + calculatedThresholdPath + "\" \"" + savePath + "\"");
			}
			// Error handling
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
			catch (const std::exception& e) {
				wxMessageDialog dialog1(NULL, "ERROR: (calculateBinarizationThreshold) An error occurred during the binarization threshold calculation", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
				dialog1.ShowModal();

				wxSTStatus->SetLabel(STRING_STATUS_WAITING_FOR_INPUT);
				
				continue; // process the next image
			}
		}
		else {
			strTemp = wxTCBinarizationThreshold->GetValue();
			if (strTemp.ToLong(&longTemp)) {
				threshold = static_cast<int>(longTemp);
			}
			else {
#if SHOW_DEBUG_DIALOGS
				wxMessageDialog dialog(NULL, "WARNING: Binarization threshold not specified, it is assumed to be 158", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
#endif
				errorMessage += "Empty or wrong format value of threshold.\n";
			}
		}

		if (!(threshold >= 0 && threshold <= 255)) {
			errorMessage += "Threshold value should be (O-255).\n";
		}

		std::cout << "Using binarization threshold: " << threshold << std::endl;

		// Create heatmap
		std::vector<std::pair<int, int>> maxSumCoords12;
		if (isAnyAutomaticAnalysisOptionActive) {
			wxSTStatus->SetLabel("Status: Creating heatmaps");

			cv::Mat heatmap11;
			try
			{
				heatmap11 = V3::Preprocessing::createHeatmap(inputPath, startFrame, endFrame, threshold, heatmapPath, stopAnalysisThreadFlag);
			}
			// Error handling
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
			catch (const std::exception& e) {
				wxMessageDialog dialog1(NULL, "ERROR: (createHeatmap) An error occurred during heatmap creation", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
				dialog1.ShowModal();

				wxSTStatus->SetLabel(STRING_STATUS_WAITING_FOR_INPUT);
				
				continue; // process the next image
			}

			std::string heatmapCoordinatesPath = outputFolderPath.string() + "\\heatmap_coordinates\\" + fileName + ".png";

			wxSTStatus->SetLabel("Status: Finding max sum squares");
			try
			{
				maxSumCoords12 = V3::Preprocessing::findMaxSumSquareCoordinatesWithPercent(
					heatmap11, squarePercent, topPercent, heatmapCoordinatesPath, heatmapPath, stopAnalysisThreadFlag
				);
			}
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
		}

		std::string rawCSVPath = outputFolderPath.string() + "\\csv_raw\\" + fileName + ".csv";

		// Count ones in XOR at coordinates
		std::vector<double> passedDoubleVector;
		try
		{
			if (isAnyAutomaticAnalysisOptionActive) {
				wxSTStatus->SetLabel("Status: Couting ones in xor");
				try
				{
					passedDoubleVector = V3::Preprocessing::countOnesInXorAtCoordinates(inputPath, maxSumCoords12, threshold, rawCSVPath, stopAnalysisThreadFlag);
				}
				catch (const V3::ProcessingAbortedException& e) {
					internalCleanup();
					return MainFrame::AnalysisResult::ABORTED;
				}
#if SHOW_DEBUG_DIALOGS
				{
					wxMessageDialog dialog(NULL, "LOG: XOR (with coords) calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}
#endif
			}
			else {
				std::vector<std::pair<int, int>> coordinates;
				wxSTStatus->SetLabel("Status: Couting ones in xor without coords");
				passedDoubleVector = V3::Preprocessing::countOnesInXorAtCoordinates(inputPath, coordinates, threshold, rawCSVPath, stopAnalysisThreadFlag);
#if SHOW_DEBUG_DIALOGS
				{
					wxMessageDialog dialog(NULL, "LOG: no coordinates XOR calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}
#endif
			}
		}
		// Error handling
		catch (const std::exception& e) {
			wxMessageDialog dialog1(NULL, "ERROR: (countOnesInXorAtCoordinates) An error occurred during counting ones in XOR at coordinates", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
			dialog1.ShowModal();

			wxSTStatus->SetLabel(STRING_STATUS_WAITING_FOR_INPUT);
			
			continue; // process the next image
		}

		std::string valuesB4XORPath = cammystatTempPathStr + "\\" + fileName + "\\valuesB4XOR" + fileName + ".csv";
		wxSTStatus->SetLabel("Status: Saving vectors before xor");
		Utils::writeVectorToFile(valuesB4XORPath, passedDoubleVector);

		if (wxCBSavitzkyGolayFilter->IsChecked()) {
			wxSTStatus->SetLabel("Status: Filtering (Savgol)");
			try
			{
				passedDoubleVector = Savgol::savgol_filter(passedDoubleVector, savitzkyGolayWindowLength, polyorder);
			}
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
#if SHOW_DEBUG_DIALOGS
			{
				wxMessageDialog dialog(NULL, "LOG: Savgol (Savitzky-Golay) filter has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif
		}

		if (wxCBMovingAverage->IsChecked()) {
			wxSTStatus->SetLabel("Status: Modifying means");
			try
			{
				passedDoubleVector = V3::Smoothing::modifyMeans(passedDoubleVector, movingAverageWindowLength, numberOfRepetitions, stopAnalysisThreadFlag);
			}
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
#if SHOW_DEBUG_DIALOGS
			{
				wxMessageDialog dialog(NULL, "LOG: Moving Average calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif
		}

		wxSTStatus->SetLabel("Status: Normalizing values");
		passedDoubleVector = V3::Smoothing::cloneNormalizedValues(passedDoubleVector);
#if SHOW_DEBUG_DIALOGS
		{
			wxMessageDialog dialog(NULL, "LOG: Normalize values calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		wxSTStatus->SetLabel("Status: Trimming");
		passedDoubleVector = V3::Detection::trim_list(passedDoubleVector, leftTrim, rightTrim);
#if SHOW_DEBUG_DIALOGS
		{
			wxMessageDialog dialog(NULL, "LOG: Trim_list calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		std::string valuesPath = cammystatTempPathStr + "\\" + fileName + "\\values" + fileName + ".csv";
		wxSTStatus->SetLabel("Status: Saving vectors");
		Utils::writeVectorToFile(valuesPath, passedDoubleVector);

		std::string rawChartPath = outputFolderPath.string() + "\\csv_stats\\" + fileName + ".csv";
		wxSTStatus->SetLabel("Status: Replacing zeros");
		passedDoubleVector = V3::Smoothing::cloneReplaceZerosValuesBelowThreshold(passedDoubleVector, movementThreshold, rawChartPath);
#if SHOW_DEBUG_DIALOGS
		{
			wxMessageDialog dialog(NULL, "LOG: Replace zeros values below threshold calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		wxSTStatus->SetLabel("Status: Adding zeros");
		passedDoubleVector = V3::Detection::clone_padded_with_zeros(passedDoubleVector);
		wxSTStatus->SetLabel("Status: Zeros has been added to a list.");
#if SHOW_DEBUG_DIALOGS
		{
			wxMessageDialog dialog(NULL, "LOG: add_zeros_to_list calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		std::vector<std::vector<double>> events;
		if (wxCBAutoDetectEvents->GetValue()) {
			wxSTStatus->SetLabel("Status: Calculating integrals");
			try
			{
				events = V3::Detection::calculate_integrals_with_reference_points(passedDoubleVector, stopAnalysisThreadFlag);
			}
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
#if SHOW_DEBUG_DIALOGS
			{
				wxMessageDialog dialog(NULL, "LOG: calculate_integrals_with_reference_points calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif

			wxSTStatus->SetLabel("Status: Merging events");
			events = V3::Detection::merge_events(events, autoMergeEvents);
#if SHOW_DEBUG_DIALOGS
			{
				wxMessageDialog dialog(NULL, "LOG: merge_events calculation has been finished successfully", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif

			wxSTStatus->SetLabel("Status: Removing events");
			events = V3::Detection::remove_events(events, autoSelectEvents);
#if SHOW_DEBUG_DIALOGS
			{
				wxMessageDialog dialog(NULL, "LOG: remove_events calculation has been finished successfully", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif

			wxSTStatus->SetLabel("Status: Normalizing second column");
			try
			{
				events = Utils::normalizeSecondColumnInCopy(events);
			}
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
		}
		std::string normalizedChartsPath = outputFolderPath.string() + "\\normalized_chart";
#if SHOW_DEBUG_DIALOGS
		{
			wxMessageDialog dialog(NULL, fileName + " " + normalizedChartsPath + " " + std::to_string(fps), wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif
		
		std::string contractionRelaxationPhasesPath = cammystatTempPathStr + "\\" + fileName + "\\contractionRelaxationPhases" + fileName + ".csv";
		
		if (analyseContractionRelaxationEvents) {
			wxSTStatus->SetLabel("Status: Contraction-relaxation analysis");
			try
			{
				std::vector<V3::Detection::Phase> contractionRelaxationPhases = V3::Detection::locate_contractions_and_relaxations(passedDoubleVector, events, stopAnalysisThreadFlag);
				Utils::writeVectorToFile(contractionRelaxationPhasesPath, contractionRelaxationPhases);
			}
			catch (const V3::ProcessingAbortedException& e) {
				internalCleanup();
				return MainFrame::AnalysisResult::ABORTED;
			}
		}

		wxSTStatus->SetLabel("Status: Saving vector of vectors");
		std::string eventsPath = cammystatTempPathStr + "\\" + fileName + "\\events" + fileName + ".csv";
		Utils::writeVectorOfVectorsToFile(eventsPath, events);

		if(outputLineChart || outputEventChart) wxSTStatus->SetLabel("Status: Saving plots");

		if (outputLineChart) {
			Utils::callPlotExe(plotPath, "video_events", JoinCommandLineArguments(valuesB4XORPath, "", fileName, outputFolderPath.string() + "\\raw_chart", fps));
		}

		if (stopAnalysisThreadFlag) {
			internalCleanup();
			return MainFrame::AnalysisResult::ABORTED;
		}
		
		if (outputEventChart) {
			Utils::callPlotExe(plotPath, "video_events", JoinCommandLineArguments(valuesPath, eventsPath, fileName, normalizedChartsPath, fps));
		}

		if (stopAnalysisThreadFlag) {
			internalCleanup();
			return MainFrame::AnalysisResult::ABORTED;
		}

		if (analyseContractionRelaxationEvents) {
			std::string contractionRelaxationPath = outputFolderPath.string() + "\\contraction_relaxation_chart";
			Utils::callPlotExe(plotPath, "contraction_relaxation_analysis", JoinCommandLineArguments(valuesPath, contractionRelaxationPhasesPath, fileName, contractionRelaxationPath));
		}

		wxSTStatus->SetLabel("Status: Finished");
	}

	return MainFrame::AnalysisResult::FINISHED;
}

void MainFrame::OnTimer(wxTimerEvent& event)
{
	wxString dots = wxString('.', m_dotCount % 4);
	wxSTStatusDisplayed->SetLabel(wxSTStatus->GetLabel() + dots);
	m_dotCount++;
	m_dotCount %= 4;
}

void MainFrame::OnCharNoDot(wxKeyEvent& event) {
	wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
	if (textCtrl) {
		int keyCode = event.GetKeyCode();

		// Allow control keys (like backspace, delete, etc.)
		if (keyCode == WXK_DELETE || keyCode == WXK_BACK) {
			event.Skip();
			return;
		}

		// Handle different input restrictions based on the specific field
		//if (textCtrl == m_numberInput) {
			// Allow only digits
		if (wxIsdigit(keyCode)) {
			event.Skip();
		}
		//}
		//else if (textCtrl == m_decimalInput) {
			// Allow digits and one decimal point
		wxString value = textCtrl->GetValue();
		if (wxIsdigit(keyCode)) {
			event.Skip();
		}
		//}
	}
}

void MainFrame::OnChar(wxKeyEvent& event) {
	wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
	if (textCtrl) {
		int keyCode = event.GetKeyCode();

		// Allow control keys (like backspace, delete, etc.)
		if (keyCode == WXK_DELETE || keyCode == WXK_BACK) {
			event.Skip();
			return;
		}

		// Handle different input restrictions based on the specific field
		//if (textCtrl == m_numberInput) {
			// Allow only digits
		if (wxIsdigit(keyCode)) {
			event.Skip();
		}
		//}
		//else if (textCtrl == m_decimalInput) {
			// Allow digits and one decimal point
		wxString value = textCtrl->GetValue();
		if (wxIsdigit(keyCode) || (keyCode == '.' && !value.Contains('.'))) {
			event.Skip();
		}
		//}
	}
}

void MainFrame::OnKillFocus(wxFocusEvent& event) {
	wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
	if (textCtrl) {
		wxString value = textCtrl->GetValue();

		if (value.StartsWith("0") && value.Length() > 1 && value[1] != '.') {
			value.Trim(false);  // Remove leading zeroes
			textCtrl->SetValue(value);
		}
	}
	event.Skip();  // Make sure other event handlers still get this event
}

void MainFrame::OnPaste(wxClipboardTextEvent& event) {
}

void MainFrame::OnMouseClick(wxMouseEvent& event) {
}

void MainFrame::OnFocus(wxFocusEvent& event) {
}

void MainFrame::OnCiteMe(wxCommandEvent& WXUNUSED(event)) {
	NewWindowThread* thread = new NewWindowThread();
	if (thread->Run() != wxTHREAD_NO_ERROR) {
		wxLogError("Could not start thread!");
		delete thread;
	}
}

void MainFrame::OpenLicensesFolder(wxCommandEvent& WXUNUSED(event)) {
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxFileName exeFilePath(exePath);

	wxString exeDirPath = exeFilePath.GetPath();

	wxString command = wxString::Format("explorer \"%s\"", exeDirPath);
	system(command.c_str());
}

void MainFrame::ToggleConsole(wxCommandEvent& WXUNUSED(event)) {
	bool oldIsDebugWindowOpen = this->IsConsoleShown();

	if (oldIsDebugWindowOpen) {
		this->HideConsole();
	}
	else {
		this->ShowConsole();
	}

	this->SyncToggleDebugWindowMenuItemLabel();
}

void MainFrame::OnCreateNewWindow(wxThreadEvent& event) {
	wxFrame* aboutFrame = new wxFrame(this, wxID_ANY, "About the Authors", wxDefaultPosition, wxSize(500, 600));
	wxIcon icon;
	if (icon.LoadFile("icon.ico", wxBITMAP_TYPE_ICO)) {
		aboutFrame->SetIcon(icon);
	}
	else {
		wxLogError("Failed to load icon file.");
	}

	wxPanel* panel = new wxPanel(aboutFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	panel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

	std::ifstream file("citeme.txt");
	std::stringstream buffer;
	if (file.is_open()) {
		buffer << file.rdbuf();
		file.close();
	}
	else {
		buffer << "Failed to load citeme.txt";
	}

	wxTextCtrl* textCtrl = new wxTextCtrl(panel, wxID_ANY, buffer.str(), wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL | wxVSCROLL);

	textCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	textCtrl->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(textCtrl, 1, wxALL | wxEXPAND, 10);
	panel->SetSizer(sizer);

	aboutFrame->Show(true);
}

void MainFrame::OnClose(wxCloseEvent& event)
{
	TCHAR tempPath[MAX_PATH];

	if (GetTempPath(MAX_PATH, tempPath) == 0) {
		wxMessageDialog dialog(NULL, "ERROR: Could not locate the appdata folder.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
		return;
	}

	std::wstring wstr(tempPath);

	std::string str(wstr.begin(), wstr.end());
	std::string pathToRemove = str + "\\Cammystat\\";

	if (wxFileName::DirExists(pathToRemove))
	{
		FsUtils::RemoveDirectoryRecursively(pathToRemove);
	}

	event.Skip();
}
