#include "MainFrame.h"
#include <wx/wx.h>
#include "V3.h"
#include "wx/setup.h"
#include "Utils.h"
#include <shlobj.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "Savgol.h"
#include <locale>
#include <codecvt>
#include <string>
#include <tchar.h>
#include <wx/app.h>
#include "resource.h"

#include "Utils.h"

//UNCOMMENT BELOW LINE WITH DEFINE TO INTRODUCE DEBUG MODE
//IN DEBUG MODE EVERY STEP IS BEING LOGGED
//WHICH CAN BE QUITE ANNOYING
// 
//#DEFINE DEBUG 1

namespace fs = std::filesystem;

void RemoveFilesAndFolder(const fs::path& folderPath) {
	// Check if the folder exists
	if (fs::exists(folderPath) && fs::is_directory(folderPath)) {
		// Iterate over the files in the directory and remove them
		for (const auto& entry : fs::directory_iterator(folderPath)) {
			fs::remove(entry);
		}

		// Remove the folder itself
		fs::remove(folderPath);

		std::cout << "Folder and its contents have been removed successfully.\n";
	}
	else {
		std::cerr << "The folder does not exist or is not a directory.\n";
	}
}

bool RemoveDirectoryRecursively(const wxString& dirPath)
{
	wxDir dir(dirPath);
	if (!dir.IsOpened())
	{
		return false;
	}

	wxString filename;
	bool cont = dir.GetFirst(&filename);

	while (cont)
	{
		wxString filePath = dirPath + wxFILE_SEP_PATH + filename;

		if (wxFileName::DirExists(filePath))
		{
			if (!RemoveDirectoryRecursively(filePath))
			{
				return false;
			}
		}
		else
		{
			if (!wxRemoveFile(filePath))
			{
				return false;
			}
		}

		cont = dir.GetNext(&filename);
	} 

	return wxFileName::Rmdir(dirPath);
}

std::wstring StringToWString(const std::string& str) {
	size_t len = str.length();
	std::wstring wstr(len, L'\0');
	std::mbstowcs(&wstr[0], str.c_str(), len);
	return wstr;
}

bool FolderExists(const std::wstring& folderPath) {
	DWORD fileAttributes = GetFileAttributes(folderPath.c_str());

	if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
		// The folder does not exist if GetFileAttributes returns INVALID_FILE_ATTRIBUTES
		return false;
	}

	// Check if the path is a directory
	return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::string getCurrentDateTime() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
	return ss.str();
}

void createDirectoryWithCheck(const fs::path& dirPath) {
	if (fs::create_directory(dirPath)) {
		std::cout << "Created folder: " << dirPath << std::endl;
	}
	else if (fs::exists(dirPath)) {
		std::cout << "Folder already exists: " << dirPath << std::endl;
	}
	else {
		std::cerr << "Failed to create folder: " << dirPath << std::endl;
	}
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
	ID_AUTO_DETECT_EVENTS,
	ID_AUTOMATIC_BINARIZATION_THRESHOLD,
	ID_FOCUS_FIELD
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_CHECKBOX(ID_FPS_AUTO_DETECT_ANALYSIS, MainFrame::wxFPSAutoDetectAnalysisToggle)
EVT_CHECKBOX(ID_FOCUS_FIELD, MainFrame::wxAutomaticRecognitionAnalysis)
EVT_CHECKBOX(ID_SAVITZKY_GOLAY_FILTER, MainFrame::wxSavitzkyGolayFilter)
EVT_CHECKBOX(ID_MOVING_AVERAGE, MainFrame::wxMovingAverage)
EVT_CHECKBOX(ID_MERGE_EVENTS, MainFrame::wxMergeEvents)
EVT_CHECKBOX(ID_AUTO_SELECT_EVENTS, MainFrame::wxAutoSelectEvents)
EVT_TIMER(ID_Timer, MainFrame::OnTimer)
EVT_CHAR(MainFrame::OnChar)
EVT_KILL_FOCUS(MainFrame::OnKillFocus)
EVT_TEXT_PASTE(wxID_ANY, MainFrame::OnPaste)
EVT_CHECKBOX(ID_AUTO_DETECT_EVENTS, MainFrame::wxCBAutoMDetectEventsToggle)
EVT_MENU(wxID_ANY, MainFrame::OnCiteMe)
EVT_THREAD(wxEVT_CREATE_NEW_WINDOW, MainFrame::OnCreateNewWindow)
wxEND_EVENT_TABLE()

void MainFrame::syncAutomaticRecognitionAnalysisFieldStates() {
	isAnyAutomaticAnalysisOptionActive = wxCBSavitzkyGolayFilter->IsEnabled() || wxCBMovingAverage->IsEnabled() || wxCBAutoMDetectEvents->IsEnabled();

	wxTCFirstFrame->Enable(isAnyAutomaticAnalysisOptionActive);
	wxTCLastFrame->Enable(isAnyAutomaticAnalysisOptionActive);
}

MainFrame::MainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title) {

	numberOfFiles = 0;
	panel = new wxPanel(this);

	//Left side of the GUI

	wxSTListOfFiles = new wxStaticText(panel, wxID_ANY, "List of files .mov for analysis", wxPoint(20, 20), wxSize(280, 20));
	{
		wxFont font = wxSTListOfFiles->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);

		int newSize = font.GetPointSize() + 2;
		font.SetPointSize(newSize);

		wxSTListOfFiles->SetFont(font);
		wxSTListOfFiles->Refresh();
	}

	wxCTFileList = new wxTextCtrl(panel, wxID_ANY, "", wxPoint(20, 45), wxSize(280, 80), wxTE_MULTILINE);
	wxCTFileList->SetEditable(false);
	wxCTFileList->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnMouseClick, this);
	wxCTFileList->Bind(wxEVT_SET_FOCUS, &MainFrame::OnFocus, this);

	wxBChooseVideo = new wxButton(panel, wxID_ANY, "Choose video for analysis (.mov)", wxPoint(20, 135), wxSize(280, 20));

	wxSTNumOfChosenFiles = new wxStaticText(panel, wxID_ANY, "Number of chosen files: 0", wxPoint(20, 165), wxSize(280, 20));

	wxBAnalyze = new wxButton(panel, wxID_ANY, "Analyze", wxPoint(20, 190), wxSize(280, 20));

	wxSTStatus = new wxStaticText(panel, wxID_ANY, "Status: Waiting for input", wxPoint(20, 220), wxSize(280, 20));
	wxSTStatus->Hide();

	wxSTStatusDisplayed = new wxStaticText(panel, wxID_ANY, "Status: Waiting for input", wxPoint(20, 220), wxSize(280, 20));

	wxSTStatusVideo = new wxStaticText(panel, wxID_ANY, "Video: Awaiting", wxPoint(20, 245), wxSize(280, 20));

	//wxGProgress = new wxGauge(panel, wxID_ANY, 100, wxPoint(20, 245), wxSize(280, 20));
	//wxGProgress->SetRange(100);
	//wxGProgress->SetValue(0);

	timer = new wxTimer(this);
	this->Bind(wxEVT_TIMER, &MainFrame::OnTimer, this);

	wxBOutputPath = new wxButton(panel, wxID_ANY, "Output path", wxPoint(20, 270), wxSize(280, 20));

	PWSTR path = NULL;

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);

	if (SUCCEEDED(hr)) {
		std::wcout << L"Documents folder: " << path << std::endl;
		wxCTOutputPath = new wxTextCtrl(panel, wxID_ANY, path, wxPoint(20, 295), wxSize(280, 60), wxTE_MULTILINE);
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

	wxSTOutputOptions = new wxStaticText(panel, wxID_ANY, "Output options", wxPoint(20, 360), wxSize(280, 20));
	{
		wxFont font = wxSTOutputOptions->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);

		int newSize = font.GetPointSize() + 2;
		font.SetPointSize(newSize);

		wxSTOutputOptions->SetFont(font);
		wxSTOutputOptions->Refresh();
	}

	wxCBCVSStats = new wxCheckBox(panel, wxID_ANY, "CSV with stats", wxPoint(20, 385));
	wxCBCVSStats->SetValue(true);
	wxCBCVSRaw = new wxCheckBox(panel, wxID_ANY, "CSV with raw data", wxPoint(20, 410));
	wxCBCVSRaw->SetValue(true);
	wxCBLineChart = new wxCheckBox(panel, wxID_ANY, "Generate line chart", wxPoint(20, 435));
	wxCBLineChart->SetValue(true);
	wxCBNormalizedChart = new wxCheckBox(panel, wxID_ANY, "Generate normalized chart", wxPoint(20, 460));
	wxCBNormalizedChart->SetValue(true);

	//Right side of the GUI

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
			wxTCBinarizationTreshold->Enable(!event.IsChecked());
		});


	rightSideCoordY += 25;

	wxSTBinarizationTreshold = new wxStaticText(panel, wxID_ANY, "Binarization threshold", wxPoint(320, rightSideCoordY), wxSize(150, 20));
	wxTCBinarizationTreshold = new wxTextCtrl(panel, wxID_ANY, "100", wxPoint(510, rightSideCoordY), wxSize(40, 20));
	wxTCBinarizationTreshold->Enable(false);
	wxTCBinarizationTreshold->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCBinarizationTreshold->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCBinarizationTreshold->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);

	wxSTBinarizationTresholdRange = new wxStaticText(panel, wxID_ANY, "0-255", wxPoint(560, rightSideCoordY), wxSize(40, 20));

	rightSideCoordY += 25;

	wxCBFocusField = new wxCheckBox(panel, ID_FOCUS_FIELD, "Focus field", wxPoint(320, rightSideCoordY));
	wxCBFocusField->SetValue(true);

	rightSideCoordY += 25;

	wxCBFocusCoordinatesAnalysis = new wxStaticText(panel, wxID_ANY, "Focus coordinates analysis:", wxPoint(320, rightSideCoordY), wxSize(280, 20));

	rightSideCoordY += 25;

	wxTCSizeOfFocusField = new wxTextCtrl(panel, wxID_ANY, "25", wxPoint(320, rightSideCoordY), wxSize(40, 20));
	wxTCSizeOfFocusField->Bind(wxEVT_CHAR, &MainFrame::OnCharNoDot, this);
	wxTCSizeOfFocusField->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCSizeOfFocusField->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTSizeOfFocusField = new wxStaticText(panel, wxID_ANY, "Size of the focus field", wxPoint(370, rightSideCoordY), wxSize(80, 30));

	wxTCPercentileOfTheHighestValues = new wxTextCtrl(panel, wxID_ANY, "90", wxPoint(460, rightSideCoordY), wxSize(40, 20));
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
	wxCBSavitzkyGolayFilter->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &event) { syncAutomaticRecognitionAnalysisFieldStates(); });

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
	wxCBMovingAverage->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &event) { syncAutomaticRecognitionAnalysisFieldStates(); });

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

	wxCBAutoMDetectEvents = new wxCheckBox(panel, ID_AUTO_DETECT_EVENTS, "Auto detect events", wxPoint(320, rightSideCoordY));
	wxCBAutoMDetectEvents->SetValue(true);

	rightSideCoordY += 25;

	wxSTAutoMovementTresholdStatic = new wxStaticText(panel, wxID_ANY, "Movement treshold", wxPoint(345, rightSideCoordY));

	wxTCAutoMovementTreshold = new wxTextCtrl(panel, wxID_ANY, "0.45", wxPoint(510, rightSideCoordY), wxSize(40, 20));
	wxTCAutoMovementTreshold->Bind(wxEVT_CHAR, &MainFrame::OnChar, this);
	wxTCAutoMovementTreshold->Bind(wxEVT_KILL_FOCUS, &MainFrame::OnKillFocus, this);
	wxTCAutoMovementTreshold->Bind(wxEVT_TEXT_PASTE, &MainFrame::OnPaste, this);
	wxSTAutoMovementTreshold = new wxStaticText(panel, wxID_ANY, "units", wxPoint(560, rightSideCoordY), wxSize(40, 20));

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

	wxMenuBar* menuBar = new wxMenuBar;
	wxMenu* fileMenu = new wxMenu;
	citeMeMenuItem = new wxMenuItem(fileMenu, wxID_ANY, "About the authors");
	fileMenu->Append(citeMeMenuItem);
	menuBar->Append(fileMenu, "Cite me");
	SetMenuBar(menuBar);
	Bind(wxEVT_MENU, &MainFrame::OnCiteMe, this, citeMeMenuItem->GetId());
	Bind(wxEVT_CREATE_NEW_WINDOW, &MainFrame::OnCreateNewWindow, this);
	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

	SetSize(640, rightSideCoordY + 90);

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

	wxBAnalyze->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event)
		{
			timer->Start(1300);
			DisableUI();

			std::thread([this]() {
				RunAnalysis();

				UpdateUIAfterProcessing();
				wxBAnalyze->Enable(true);
				timer->Stop();
				wxSTStatusDisplayed->SetLabel(wxSTStatus->GetLabel());
				m_dotCount = 0;

				wxSTStatus->SetLabel("Status: Processing complete!");
				}).detach();
		});

	wxCBAutoMDetectEvents->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event)
		{
			syncAutomaticRecognitionAnalysisFieldStates();

			if (event.IsChecked()) {
				wxSTAutoMovementTresholdStatic->Enable(true);
				wxTCAutoMovementTreshold->Enable(true);
				wxSTAutoMovementTreshold->Enable(true);
				wxCBAutoMergeEvents->Enable(true);
				wxTCAutoMergeEvents->Enable(true);
				wxSTAutoMergeEvents->Enable(true);
				wxCBAutoSelectEvents->Enable(true);
				wxTCAutoSelectEvents->Enable(true);
				wxSTAutoSelectEvents->Enable(true);
			}
			else {
				wxSTAutoMovementTresholdStatic->Enable(false);
				wxTCAutoMovementTreshold->Enable(false);
				wxSTAutoMovementTreshold->Enable(false);
				wxCBAutoMergeEvents->Enable(false);
				wxTCAutoMergeEvents->Enable(false);
				wxSTAutoMergeEvents->Enable(false);
				wxCBAutoSelectEvents->Enable(false);
				wxTCAutoSelectEvents->Enable(false);
				wxSTAutoSelectEvents->Enable(false);
			}
		});

	allInteractiveControls.push_back(wxCTFileList);
	allInteractiveControls.push_back(wxBChooseVideo);
	allInteractiveControls.push_back(wxBAnalyze);
	allInteractiveControls.push_back(wxBOutputPath);
	allInteractiveControls.push_back(wxCTOutputPath);
	allInteractiveControls.push_back(wxSTBinarizationTreshold);
	allInteractiveControls.push_back(wxCBCVSStats);
	allInteractiveControls.push_back(wxCBCVSRaw);
	allInteractiveControls.push_back(wxCBLineChart);
	allInteractiveControls.push_back(wxCBNormalizedChart);
	allInteractiveControls.push_back(wxTCFPS);
	allInteractiveControls.push_back(wxTCBinarizationTreshold);
	allInteractiveControls.push_back(wxTCFirstFrame);
	allInteractiveControls.push_back(wxTCLastFrame);
	allInteractiveControls.push_back(wxTCSizeOfFocusField);
	allInteractiveControls.push_back(wxTCPercentileOfTheHighestValues);
	allInteractiveControls.push_back(wxCBSavitzkyGolayFilter);
	allInteractiveControls.push_back(wxTCWindowLengthSGF);
	allInteractiveControls.push_back(wxTCPolyorder);
	allInteractiveControls.push_back(wxCBMovingAverage);
	allInteractiveControls.push_back(wxTCWindowLengthMA);
	allInteractiveControls.push_back(wxTCNumberOfRepetitions);
	allInteractiveControls.push_back(wxTCAutoMovementTreshold);
	allInteractiveControls.push_back(wxTCLeftTrim);
	allInteractiveControls.push_back(wxTCRightTrim);
	allInteractiveControls.push_back(wxCBAutoMergeEvents);
	allInteractiveControls.push_back(wxTCAutoMergeEvents);
	allInteractiveControls.push_back(wxCBAutoSelectEvents);
	allInteractiveControls.push_back(wxTCAutoSelectEvents);
	allInteractiveControls.push_back(wxSTFPS);
	allInteractiveControls.push_back(wxSTBinarizationTresholdRange);
	allInteractiveControls.push_back(wxSTFirstFrame);
	allInteractiveControls.push_back(wxSTLastFrame);
	allInteractiveControls.push_back(wxCBFocusCoordinatesAnalysis);
	allInteractiveControls.push_back(wxSTSizeOfFocusField);
	allInteractiveControls.push_back(wxSTPercentileOfTheHighestValues);
	allInteractiveControls.push_back(wxSTWindowLengthSGF);
	allInteractiveControls.push_back(wxSTPolyorder);
	allInteractiveControls.push_back(wxSTWindowLengthMA);
	allInteractiveControls.push_back(wxCBAutoMDetectEvents);
	allInteractiveControls.push_back(wxSTNumberOfRepetitions);
	allInteractiveControls.push_back(wxSTAutoMovementTresholdStatic);
	allInteractiveControls.push_back(wxSTAutoMovementTreshold);
	allInteractiveControls.push_back(wxSTTrimList);
	allInteractiveControls.push_back(wxSTLeftTrim);
	allInteractiveControls.push_back(wxSTRightTrim);
	allInteractiveControls.push_back(wxSTAutoMergeEvents);
	allInteractiveControls.push_back(wxSTAutoSelectEvents);
	allInteractiveControls.push_back(wxCBAutomaticBinarizationThreshold);
	allInteractiveControls.push_back(wxCBFocusField);

	syncAutomaticRecognitionAnalysisFieldStates();
}

void MainFrame::DisableUI()
{
	for (wxControl*& ctrl : allInteractiveControls) {
		ctrl->Enable(false);
	}
}

void MainFrame::UpdateUIAfterProcessing()
{
	for (wxControl*& ctrl : allInteractiveControls) {
		ctrl->Enable(true);
	}
}

void MainFrame::wxFPSAutoDetectAnalysisToggle(wxCommandEvent& evt) {
	if (evt.IsChecked()) {
		wxTCFPS->Disable();
	}
	else {
		wxTCFPS->Enable();
	}
}

void MainFrame::wxCBAutoMDetectEventsToggle(wxCommandEvent& evt) {
	if (evt.IsChecked()) {
		wxSTAutoMovementTresholdStatic->Enable(true);
		wxTCAutoMovementTreshold->Enable(true);
		wxSTAutoMovementTreshold->Enable(true);
		wxCBAutoMergeEvents->Enable(true);
		wxTCAutoMergeEvents->Enable(true);
		wxSTAutoMergeEvents->Enable(true);
		wxCBAutoSelectEvents->Enable(true);
		wxTCAutoSelectEvents->Enable(true);
		wxSTAutoSelectEvents->Enable(true);
	}
	else {
		wxSTAutoMovementTresholdStatic->Enable(false);
		wxTCAutoMovementTreshold->Enable(false);
		wxSTAutoMovementTreshold->Enable(false);
		wxCBAutoMergeEvents->Enable(false);
		wxTCAutoMergeEvents->Enable(false);
		wxSTAutoMergeEvents->Enable(false);
		wxCBAutoSelectEvents->Enable(false);
		wxTCAutoSelectEvents->Enable(false);
		wxSTAutoSelectEvents->Enable(false);
	}
}

void MainFrame::wxAutomaticRecognitionAnalysis(wxCommandEvent & evt) {
	if (evt.IsChecked()) {
		wxTCFirstFrame->Enable();
		wxTCLastFrame->Enable();
		wxTCSizeOfFocusField->Enable();
		wxTCPercentileOfTheHighestValues->Enable();
	}
	else {
		wxTCFirstFrame->Disable();
		wxTCLastFrame->Disable();
		wxTCSizeOfFocusField->Disable();
		wxTCPercentileOfTheHighestValues->Disable();
	}
}

void MainFrame::wxSavitzkyGolayFilter(wxCommandEvent& evt) {
	if (evt.IsChecked()) {
		wxTCWindowLengthSGF->Enable();
		wxTCPolyorder->Enable();
	}
	else {
		wxTCWindowLengthSGF->Disable();
		wxTCPolyorder->Disable();
	}
}

void MainFrame::wxMovingAverage(wxCommandEvent& evt) {
	if (evt.IsChecked()) {
		wxTCWindowLengthMA->Enable();
		wxTCNumberOfRepetitions->Enable();
	}
	else {
		wxTCWindowLengthMA->Disable();
		wxTCNumberOfRepetitions->Disable();
	}
}

void MainFrame::wxMergeEvents(wxCommandEvent& evt) {
	if (evt.IsChecked()) {
		wxTCAutoMergeEvents->Enable();
	}
	else {
		wxTCAutoMergeEvents->Disable();
	}
}

void MainFrame::wxAutoSelectEvents(wxCommandEvent& evt) {
	if (evt.IsChecked()) {
		wxTCAutoSelectEvents->Enable();
	}
	else {
		wxTCAutoSelectEvents->Disable();
	}
}

void MainFrame::SetTaskBarIcon()
{
	HWND hwnd = (HWND)GetHWND();

	HICON hIcon = (HICON)LoadImage(NULL, L"./icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}

void MainFrame::RunAnalysis()
{
	// Place the entire block of calculation code here.
	// Make sure to remove or adjust any UI interaction code that doesn't make sense in a background thread.

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
#ifdef DEBUG
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
#ifdef DEBUG
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
#ifdef DEBUG
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

	strTemp = wxTCBinarizationTreshold->GetValue();
	int threshold;
	if (strTemp.ToLong(&longTemp)) {
		threshold = static_cast<int>(longTemp);
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Binarization treshold not specified, it is assumed to be 158", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of treshold.\n";
	}

	if (!(threshold >= 0 && threshold <= 255)) {
		errorMessage += "Treshold value should be (O-255).\n";
	}

	double squarePercent;
	double topPercent;

	// Find max sum square coordinates
	strTemp = wxTCSizeOfFocusField->GetValue();
	if (strTemp.ToDouble(&squarePercent)) {
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Focus field not specified, it is assumed to be 25", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of size of focus field.\n";
	}

	if (!(squarePercent >= 1 && squarePercent <= 100)) {
		errorMessage += "Square percent value should be (1-100).\n";
	}

	strTemp = wxTCPercentileOfTheHighestValues->GetValue();
	if (strTemp.ToDouble(&topPercent)) {
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Percentile of the highest values, it is assumed to be 90", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value percentile of the highest values is wrong.\n";
	}

	if (!(topPercent > 0 && topPercent <= 100)) {
		errorMessage += "Top percent value should be (0-100) excluding zero.\n";
	}

	strTemp = wxTCWindowLengthSGF->GetLabel();
	int windowLength;
	if (strTemp.ToLong(&longTemp)) {
		windowLength = static_cast<int>(longTemp);
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Window length (savitzky-golay filter) not specified, it is assumed to be 7", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value of window length (savitzky-golay filter).\n";
	}

	if (windowLength % 2 == 0) {
		errorMessage += "Window length value should be odd.\n";
	}

	strTemp = wxTCPolyorder->GetLabel();
	int polyorder;
	if (strTemp.ToLong(&longTemp)) {
		polyorder = static_cast<int>(longTemp);
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Polyorder (savitzky-golay filter) not specified, it is assumed to be 5", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in polyorder field.\n";
	}

	if (polyorder >= windowLength) {
		errorMessage += "Polyorder value should be lower than window length.\n";
	}

	strTemp = wxTCWindowLengthMA->GetLabel();
	int windowLength2;
	if (strTemp.ToLong(&longTemp)) {
		windowLength2 = static_cast<int>(longTemp);
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Window length (Moving Average) not specified, it is assumed to be FPS times 0.1", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal()
#endif
		errorMessage += "Empty or wrong format value in window length (moving average) field.\n";
	}

	if (windowLength2 < 2) {
		errorMessage += "Window length (moving average) should be higher than two.\n";
	}

	strTemp = wxTCNumberOfRepetitions->GetLabel();
	int numberOfRepetitions;
	if (strTemp.ToLong(&longTemp)) {
		numberOfRepetitions = static_cast<int>(longTemp);
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Number of repetitions (Moving Average) not specified, it is assumed to be 10", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in number of repetitions field.\n";
	}

	if (numberOfRepetitions < 0) {
		errorMessage += "Number of repetitions value should be higher than zero.\n";
	}

	strTemp = wxTCAutoMovementTreshold->GetValue();
	double movementTreshold;
	if (strTemp.ToDouble(&movementTreshold)) {
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Auto movement treshold not specified, it is assumed to be 0.45", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong value format in movement treshold field.\n";
	}

	if (movementTreshold <= 0.0) {
		errorMessage += "Movement treshold should be higher than zero.\n";
	}

	strTemp = wxTCLeftTrim->GetLabel();
	int leftTrim;
	if (strTemp.ToLong(&longTemp)) {
		leftTrim = static_cast<int>(longTemp);
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Left trim not specified, it is assumed to be 0", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in left trim field.\n";
	}

	if (leftTrim < 0) {
		errorMessage += "Left trim value should be non-negative.\n";
	}

	strTemp = wxTCRightTrim->GetLabel();
	int rightTrim;
	if (strTemp.ToLong(&longTemp)) {
		rightTrim = static_cast<int>(longTemp);
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Right trim not specified, it is assumed to be 0", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty of wrong format value in right trim field.\n";
		return;
	}

	if (rightTrim < 0) {
		errorMessage += "Right trim value should be non-negative.\n";
	}

	strTemp = wxTCAutoMergeEvents->GetValue();
	double autoMergedEvents;
	if (strTemp.ToDouble(&autoMergedEvents)) {
	}
	else {
#ifdef DEBUG
		wxMessageDialog dialog(NULL, "WARNING: Auto merged events not specified, it is assumed to be 0", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
#endif
		errorMessage += "Empty or wrong format value in auto merged events field.\n";
	}

	if (autoMergedEvents < 0.0) {
		errorMessage += "Right trim value should be non-negative.\n";
	}

	strTemp = wxTCAutoSelectEvents->GetValue();
	double autoSelectEvents;
	if (strTemp.ToDouble(&autoSelectEvents)) {
	}
	else {
#ifdef DEBUG
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
		return;
	}

	wxCommandEvent dummyEvent;
	OnCiteMe(dummyEvent);

	std::string dateTime = getCurrentDateTime();
	std::string outputFolderName = "camystat_output_" + dateTime;
	fs::path outputFolderPath = fs::path(outputPath) / outputFolderName;

	createDirectoryWithCheck(outputFolderPath);

	for (wxString strTemp : directories) {
		std::string inputPath = std::string(strTemp.mb_str());

		if (inputPath == "") {
			wxSTStatus->SetLabel("Status: List of files .mov shouldn't be empty.");
			wxMessageDialog dialog(NULL, "ERROR: List of files .mov shouldn't be empty.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
			return;
		}

		fs::path pathObj(inputPath);

		wxSTStatus->SetLabel("Status: Initializing...");

		std::string fileName = pathObj.stem().string();

		strTemp = wxCTOutputPath->GetValue();
		std::string outputPath = std::string(strTemp.mb_str());

		if (!fs::exists(outputPath)) {
			return;
		}

		std::string outputFolderName2 = "camystat_output_" + fileName;
		wxSTStatusVideo->SetLabel("Video: " + fileName);
		fs::path outputFolderPath = fs::path(outputPath) / outputFolderName / outputFolderName2;

		bool cvsstats = wxCBCVSStats->IsChecked();
		bool cVSRaw = wxCBCVSRaw->IsChecked();
		bool lineChart = wxCBLineChart->IsChecked();
		bool normalizedChart = wxCBNormalizedChart->IsChecked();

		wxSTStatus->SetLabel("Status: Preparing output folders");

		// Create the main output folder
		createDirectoryWithCheck(outputFolderPath);

		// Create the subfolders
		if (isAnyAutomaticAnalysisOptionActive) {
			createDirectoryWithCheck(outputFolderPath / "activity_heatmap");
			createDirectoryWithCheck(outputFolderPath / "heatmap_coordinates");
		}
		if (cvsstats) {
			createDirectoryWithCheck(outputFolderPath / "csv_stats");
		}
		if (cVSRaw) {
			createDirectoryWithCheck(outputFolderPath / "csv_raw");
		}
		if (lineChart) {
			createDirectoryWithCheck(outputFolderPath / "raw_chart");
		}
		if (normalizedChart) {
			createDirectoryWithCheck(outputFolderPath / "normalized_chart");
		}

		TCHAR tempPath[MAX_PATH];

		if (GetTempPath(MAX_PATH, tempPath) != 0) {
		}
		else {
			wxSTStatus->SetLabel("Status: ERROR: Could not locate appdata folder!");
			wxMessageDialog dialog(NULL, "ERROR: Could not locate the appdata folder.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
			return;
		}

		std::wstring wstr(tempPath);

		std::string str(wstr.begin(), wstr.end());
		{
			wxSTStatus->SetLabel("Status: Detected appdata folder");
		}

		std::wstring wstr2 = StringToWString(str + "\\Cammystat");
		{
			wxSTStatus->SetLabel("Status: Cammystat folder located");
		}
		if (!FolderExists(wstr2)) {
			createDirectoryWithCheck(str + "\\Cammystat");
			wxSTStatus->SetLabel("Status: Cammystat appdata folder has been created");
		}

		std::string heatmapPath = outputFolderPath.string() + "\\activity_heatmap\\" + fileName + ".png";

		if (FolderExists(wstr2)) {
			std::string tempPath2 = str + "\\Cammystat\\" + fileName;
			createDirectoryWithCheck(tempPath2);
		}

		//wxString number = wxSTFileList->GetValue();
		//double scaleFactor = 2.0; //assumed
		//if (!number.ToDouble(&scaleFactor)) { /* error! */ }

		//V3::Compression::resizeVideo(inputPath, outputPath, scaleFactor);

		wxSTStatus->SetLabel("Status: Reading form's parameters...");

		double scaleFactor = 2.0;

		std::vector<std::string> inputPaths;
		std::stringstream ss(inputPath);
		std::string line;

		// Use std::getline to extract each line separated by '\n'
		while (std::getline(ss, line)) {
			inputPaths.push_back(line);
		}

		// Create heatmap
		std::vector<std::pair<int, int>> maxSumCoords12;
		if (isAnyAutomaticAnalysisOptionActive) {
			wxSTStatus->SetLabel("Status: Creating heatmaps");
			cv::Mat heatmap11 = V3::Preprocessing::createHeatmap(inputPath, startFrame, endFrame, threshold, heatmapPath);

			std::string heatmapCoordinatesPath = outputFolderPath.string() + "\\heatmap_coordinates\\" + fileName + ".png";

			wxSTStatus->SetLabel("Status: Finding max sum squares");
			maxSumCoords12 = V3::Preprocessing::findMaxSumSquareCoordinatesWithPercent(
				heatmap11, squarePercent, topPercent, heatmapCoordinatesPath, heatmapPath
			);

		}

		std::string rawCSVPath = outputFolderPath.string() + "\\csv_raw\\" + fileName + ".csv";

		// Count ones in XOR at coordinates
		std::vector<double> passedDoubleVector;
		if (isAnyAutomaticAnalysisOptionActive) {
			wxSTStatus->SetLabel("Status: Couting ones in xor");
			passedDoubleVector = V3::Preprocessing::countOnesInXorAtCoordinates(inputPath, maxSumCoords12, threshold, rawCSVPath);
#ifdef DEBUG
			{
				wxMessageDialog dialog(NULL, "LOG: XOR (with coords) calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif
		}
		else {
			std::vector<std::pair<int, int>> coordinates;
			wxSTStatus->SetLabel("Status: Couting ones in xor without coords");
			passedDoubleVector = V3::Preprocessing::countOnesInXorAtCoordinates(inputPath, coordinates, threshold, rawCSVPath);
#ifdef DEBUG
			{
				wxMessageDialog dialog(NULL, "LOG: no coordinates XOR calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
				Utils::plotVector(passedDoubleVector, "XOR", 1000, 1500);
			}
#endif
		}

		std::string valuesB4XORPath = str + "\\Cammystat\\" + fileName + "\\valuesB4XOR" + fileName + ".csv";
		wxSTStatus->SetLabel("Status: Saving vectors before xor");
		Utils::writeVectorToFile(valuesB4XORPath, passedDoubleVector);

		if (wxCBSavitzkyGolayFilter->IsChecked()) {
			wxSTStatus->SetLabel("Status: Filtering (Savgol)");
			passedDoubleVector = Savgol::savgol_filter(passedDoubleVector, windowLength, polyorder);
#ifdef DEBUG
			{
				wxMessageDialog dialog(NULL, "LOG: Savgol (Savitzky-Golay) filter has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
				Utils::plotVector(passedDoubleVector, "Savgol filter", 1000, 1500);
			}
#endif
		}

		if (wxCBMovingAverage->IsChecked()) {
			wxSTStatus->SetLabel("Status: Modifying means");
			passedDoubleVector = V3::Smoothing::modify_means(passedDoubleVector, windowLength2, numberOfRepetitions);
#ifdef DEBUG
			{
				wxMessageDialog dialog(NULL, "LOG: Moving Average calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
				Utils::plotVector(passedDoubleVector, "Moving average", 1000, 1500);
			}
#endif
		}

		wxSTStatus->SetLabel("Status: Normalizing values");
		passedDoubleVector = V3::Smoothing::clone_normalized_values(passedDoubleVector);
#ifdef DEBUG
		{
			Utils::plotVector(passedDoubleVector, "normalize_values", 1000, 1500);
			wxMessageDialog dialog(NULL, "LOG: Normalize values calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		std::string valuesPath = str + "\\Cammystat\\" + fileName + "\\values" + fileName + ".csv";
		wxSTStatus->SetLabel("Status: Saving vectors");
		Utils::writeVectorToFile(valuesPath, passedDoubleVector);

		std::string rawChartPath = outputFolderPath.string() + "\\csv_stats\\" + fileName + ".csv";
		wxSTStatus->SetLabel("Status: Replacing zeros");
		passedDoubleVector = V3::Smoothing::clone_replace_zeros_values_below_threshold(passedDoubleVector, movementTreshold, rawChartPath);
#ifdef DEBUG
		Utils::plotVector(passedDoubleVector, "replace_zeros_values_below_threshold", 1000, 1500);
		{
			wxMessageDialog dialog(NULL, "LOG: Replace zeros values below threshold calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		wxSTStatus->SetLabel("Status: Trimming");
		passedDoubleVector = V3::Detection::trim_list(passedDoubleVector, leftTrim, rightTrim);
#ifdef DEBUG
		{
			Utils::plotVector(passedDoubleVector, "trim_list", 1000, 1500);
			wxMessageDialog dialog(NULL, "LOG: Trim_list calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif
		wxSTStatus->SetLabel("Status: Adding zeros");
		passedDoubleVector = V3::Detection::clone_padded_with_zeros(passedDoubleVector);
		wxSTStatus->SetLabel("Status: Zeros has been added to a list.");
#ifdef DEBUG
		{
			Utils::plotVector(passedDoubleVector, "add_zeros_to_list", 1000, 1500);
			wxMessageDialog dialog(NULL, "LOG: add_zeros_to_list calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		std::vector<std::vector<double>> events;
		if (wxCBAutoMDetectEvents->GetValue()) {
			wxSTStatus->SetLabel("Status: Calculating integrals");
			events = V3::Detection::calculate_integrals_with_reference_points(passedDoubleVector);
#ifdef DEBUG
			{
				wxMessageDialog dialog(NULL, "LOG: calculate_integrals_with_reference_points calculation has been finished successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif

			wxSTStatus->SetLabel("Status: Merging events");
			events = V3::Detection::merge_events(events, autoMergedEvents);
#ifdef DEBUG
			{
				wxMessageDialog dialog(NULL, "LOG: merge_events calculation has been finished successfully", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_WARNING | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
#endif

			wxSTStatus->SetLabel("Status: Removing events");
			events = V3::Detection::remove_events(events, autoSelectEvents);
#ifdef DEBUG
			{
				wxMessageDialog dialog(NULL, "LOG: remove_events calculation has been finished successfully", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
			Utils::plot_events(passedDoubleVector, events);
#endif

			wxSTStatus->SetLabel("Status: Normalizing second column");
			events = Utils::normalizeSecondColumnInCopy(events);
		}
		std::string normalizedChartsPath = outputFolderPath.string() + "\\normalized_chart";
		wxSTStatus->SetLabel("Status: Reading current path");
		TCHAR buffer[MAX_PATH];
		DWORD length = GetCurrentDirectory(MAX_PATH, buffer);
		std::string plotPath = Utils::TCHARToString(buffer);
		plotPath = plotPath.substr(0, plotPath.size() - 1);
		wxSTStatus->SetLabel("Status: Calculating path to plot");
		plotPath = plotPath + "\\plot.exe";
#ifdef DEBUG
		{
			wxMessageDialog dialog(NULL, "LOG: a path to the plot.exe: " + plotPath, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
			wxMessageDialog dialog(NULL, plotPath, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif
#ifdef DEBUG
		{
			wxMessageDialog dialog(NULL, fileName + " " + normalizedChartsPath + " " + std::to_string(fps), wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
#endif

		std::string eventsPath = str + "\\Cammystat\\" + fileName + "\\events" + fileName + ".csv";

		wxSTStatus->SetLabel("Status: Saving vector of vectors");
		Utils::writeVectorOfVectorsToFile(eventsPath, events);

		std::string pathToRemove = str + "\\Cammystat\\" + fileName;

		if(lineChart || normalizedChart) wxSTStatus->SetLabel("Status: Saving plots");

		if (lineChart) {
			Utils::callPlotEvents(plotPath, valuesB4XORPath, "", fileName, outputFolderPath.string() + "\\raw_chart", fps, pathToRemove);
		}

		if (normalizedChart) {
			Utils::callPlotEvents(plotPath, valuesPath, eventsPath, fileName, normalizedChartsPath, fps, pathToRemove);
		}

		wxSTStatus->SetLabel("Status: Finished");
	}
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

		//if (textCtrl == m_numberInput || textCtrl == m_decimalInput) {
			// Prevent leading zeros (e.g., 00 or 000)
		if (value.StartsWith("0") && value.Length() > 1 && value[1] != '.') {
			value.Trim(false);  // Remove leading zeroes
			textCtrl->SetValue(value);
		}
		//}
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

	if (GetTempPath(MAX_PATH, tempPath) != 0) {
	}
	else {
		wxMessageDialog dialog(NULL, "ERROR: Could not locate the appdata folder.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
		return;
	}

	std::wstring wstr(tempPath);

	std::string str(wstr.begin(), wstr.end());
	std::string pathToRemove = str + "\\Cammystat\\";

	if (wxFileName::DirExists(pathToRemove))
	{
		RemoveDirectoryRecursively(pathToRemove);
	}

	event.Skip();
}