#include "FsUtils.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>

void FsUtils::RemoveFilesAndFolder(const fs::path& folderPath) {
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

bool FsUtils::RemoveDirectoryRecursively(const wxString& dirPath)
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

std::wstring FsUtils::StringToWString(const std::string& str) {
	size_t len = str.length();
	std::wstring wstr(len, L'\0');
	std::mbstowcs(&wstr[0], str.c_str(), len);
	return wstr;
}

bool FsUtils::FolderExists(const fs::path& folderPath) {
	std::error_code ec;
	return fs::exists(folderPath, ec) && fs::is_directory(folderPath, ec);
}

void FsUtils::CreateDirectoryWithCheck(const fs::path& dirPath) {
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

std::filesystem::path FsUtils::RuntimeResourcePath(const std::string& filename) {
#ifdef __APPLE__
	const wxString path = wxFileName(
		wxStandardPaths::Get().GetResourcesDir(), filename).GetFullPath();
#else
	const wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	const wxString path = wxFileName(wxFileName(exePath).GetPath(), filename).GetFullPath();
#endif
	return std::filesystem::path(path.ToUTF8().data());
}

bool FsUtils::LoadAppIcon(wxIcon& icon) {
	const std::filesystem::path iconIco = RuntimeResourcePath("icon.ico");
	if (icon.LoadFile(iconIco.string(), wxBITMAP_TYPE_ICO)) {
		return true;
	}
	const std::filesystem::path iconPng = RuntimeResourcePath("ikona.png");
	if (std::filesystem::exists(iconPng) && icon.LoadFile(iconPng.string(), wxBITMAP_TYPE_PNG)) {
		return true;
	}
	return false;
}
