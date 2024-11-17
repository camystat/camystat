#pragma once
#include <wx/wx.h>
#include <wx/filesys.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/dir.h>

#include <wx/listctrl.h>
#include <wx/gbsizer.h>
#include <wx/progdlg.h>
#include <string>

#include <vector>
#include <map>

#define STRING_STATUS_WAITING_FOR_INPUT "Status: Waiting for input"
#define MAIN_WINDOW_WIDTH 640

#define DEFAULT_PERCENTILE_OF_HIGHEST_VALUES 90
#define DEFAULT_SIZE_OF_FOCUS_FIELD 25

class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString &title);
	void wxFPSAutoDetectAnalysisToggle(wxCommandEvent& evt);
	void wxSavitzkyGolayFilter(wxCommandEvent& evt);
	void wxMovingAverage(wxCommandEvent& evt);
	void wxMergeEvents(wxCommandEvent& evt);
	void wxAutoSelectEvents(wxCommandEvent& evt);
	void SetTaskBarIcon();
	void RunAnalysis();
	void UpdateUI();
	void OnTimer(wxTimerEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnKillFocus(wxFocusEvent& event);
	void OnPaste(wxClipboardTextEvent& event);
	void OnMouseClick(wxMouseEvent& event);
	void OnFocus(wxFocusEvent& event);
	void OnCiteMe(wxCommandEvent& event);
	void OpenLicensesFolder(wxCommandEvent& event);
	void OnCreateNewWindow(wxThreadEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnCharNoDot(wxKeyEvent& event);

	void syncAutomaticRecognitionAnalysisFieldStates();
	bool isAnyAutomaticAnalysisOptionActive;

	std::vector<wxControl*> allInteractiveControls;
	std::map<wxControl*, bool> controlEnabledState;
	bool processingRunning = false;

	wxDECLARE_EVENT_TABLE();

	wxPanel* panel;
	wxStaticText* wxSTListOfFiles;
	wxTextCtrl* wxCTFileList;
	wxButton* wxBChooseVideo;
	wxStaticText* wxSTNumOfChosenFiles;
	wxButton* wxBAnalyze;
	wxStaticText* wxSTStatus;
	wxStaticText* wxSTStatusDisplayed;
	wxStaticText* wxSTStatusVideo;
	wxGauge* wxGProgress;
	wxTimer* timer;
	int direction = 1; // 1 for forward, -1 for backward
	int position = 0; // Current position of the progress part

	wxButton* wxBOutputPath;
	wxTextCtrl* wxCTOutputPath;

	wxStaticText* wxSTOutputOptions;
	wxCheckBox* wxCBCsvStats;
	wxCheckBox* wxCBCsvRaw;
	wxCheckBox* wxCBLineChart;
	wxCheckBox* wxCBNormalizedChart;

	//Right Side of the GUI

	wxStaticText* wxSTOptionsForAnalysis;

	wxTextCtrl* wxTCFPS;
	wxStaticText* wxSTFPS;

	wxStaticText* wxSTBinarizationThreshold;
	wxTextCtrl* wxTCBinarizationThreshold;
	wxStaticText* wxSTBinarizationThresholdRange;

	wxStaticText* wxSTAutomaticRecognitionAnalysis;

	wxTextCtrl* wxTCFirstFrame;
	wxStaticText* wxSTFirstFrame;

	wxTextCtrl* wxTCLastFrame;
	wxStaticText* wxSTLastFrame;

	wxStaticText* wxCBFocusCoordinatesAnalysis;

	wxTextCtrl* wxTCSizeOfFocusField;
	wxStaticText* wxSTSizeOfFocusField;

	wxTextCtrl* wxTCPercentileOfTheHighestValues;
	wxStaticText* wxSTPercentileOfTheHighestValues;

	wxStaticText* wxSTOptionsOfEventDetection;
	wxCheckBox* wxCBSavitzkyGolayFilter;

	wxCheckBox* wxCBAutomaticBinarizationThreshold;
	wxCheckBox* wxCBFocusField;

	wxTextCtrl* wxTCWindowLengthSGF;
	wxStaticText* wxSTWindowLengthSGF;

	wxTextCtrl* wxTCPolyorder;
	wxStaticText* wxSTPolyorder;

	wxCheckBox* wxCBMovingAverage;

	wxTextCtrl* wxTCWindowLengthMA;
	wxStaticText* wxSTWindowLengthMA;

	wxTextCtrl* wxTCNumberOfRepetitions;
	wxStaticText* wxSTNumberOfRepetitions;

	wxStaticText* wxSTAutoMovementThresholdStatic;

	wxTextCtrl* wxTCAutoMovementThreshold;
	wxStaticText* wxSTAutoMovementThreshold;

	wxCheckBox* wxCBAutoDetectEvents;

	wxCheckBox* wxCBAutoMergeEvents;

	wxTextCtrl* wxTCAutoMergeEvents;
	wxStaticText* wxSTAutoMergeEvents;

	wxCheckBox* wxCBAutoSelectEvents;

	wxTextCtrl* wxTCAutoSelectEvents;
	wxStaticText* wxSTAutoSelectEvents;

	wxStaticText* wxSTTrimList;

	wxTextCtrl* wxTCLeftTrim;
	wxStaticText* wxSTLeftTrim;

	wxTextCtrl* wxTCRightTrim;
	wxStaticText* wxSTRightTrim;

	wxMenuBar* menuBar;
	wxMenu* fileMenu;
	wxMenuItem* citeMeMenuItem;
	wxMenuItem* openLicensesFolderMenuItem;

	std::vector<wxString> directories;
	int numberOfFiles;


	int m_dotCount = 0;

	std::string strStatus;
};