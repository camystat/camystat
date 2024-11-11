#include "Application.h"
#include <filesystem>
#include "MainFrame.h"
#include "resource.h"

wxIMPLEMENT_APP(Application);

Application::Application() {
#if GUI_SHOW_CONSOLE
	AllocConsole();
	freopen("conin$","r",stdin);
	freopen("conout$","w",stdout);
	freopen("conout$","w",stderr);
#endif
}

bool Application::OnInit() {
	MainFrame* mainFrame = new MainFrame("CamMyStat");

	mainFrame->SetTitle("CamMyStat");
	wxIcon icon(wxICON(IDI_APP_ICON));
	mainFrame->SetIcon(icon);
	mainFrame->Center();
	mainFrame->SetTaskBarIcon();
	mainFrame->Show();
	return true;
}
