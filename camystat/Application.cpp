#include "Application.h"
#include <filesystem>
#include "MainFrame.h"
#include "resource.h"

wxIMPLEMENT_APP(Application);

Application::Application() {
#ifdef _WIN32
	AllocConsole();
	SetConsoleTitleA(("Debug console"));
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);

	bool consoleCloseDisabled = false;

	HWND consoleHwnd = GetConsoleWindow();
	if (consoleHwnd != NULL)
	{
		HMENU consoleHMenu = GetSystemMenu(consoleHwnd, FALSE);
		if (consoleHMenu != NULL) {
			DeleteMenu(consoleHMenu, SC_CLOSE, MF_BYCOMMAND);
			consoleCloseDisabled = true;
		}
	}

	if (!consoleCloseDisabled) {
		std::cerr << "Failed to disable the debug console window close button. Clicking it will close the whole application." << std::endl;
	}
#endif
}

bool Application::OnInit() {
	wxInitAllImageHandlers();

	MainFrame* mainFrame = new MainFrame("Camystat");

	mainFrame->SetTitle("Camystat");
#ifdef _WIN32
	wxIcon icon(wxICON(IDI_APP_ICON));
	mainFrame->SetIcon(icon);
#else
	// Best-effort: allow running from build dir where we copy icon.ico
	wxIcon icon;
	if (icon.LoadFile("icon.ico", wxBITMAP_TYPE_ICO)) {
		mainFrame->SetIcon(icon);
	}
#endif
	mainFrame->Center();
	mainFrame->SetTaskBarIcon();
	mainFrame->Show();
	return true;
}
