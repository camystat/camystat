#include "Application.h"
#include <filesystem>
#include "MainFrame.h"
#include "resource.h"

wxIMPLEMENT_APP(Application);

Application::Application() {
}

bool Application::OnInit() {
	MainFrame* mainFrame = new MainFrame("CamMyStat");
	mainFrame->SetClientSize(640, 520);
	mainFrame->SetSizeHints(640, 600, 640, 600);

	mainFrame->SetTitle("CamMyStat");
	wxIcon icon(wxICON(IDI_APP_ICON));
	mainFrame->SetIcon(icon);
	mainFrame->Center();
	mainFrame->SetTaskBarIcon();
	mainFrame->Show();
	return true;
}
