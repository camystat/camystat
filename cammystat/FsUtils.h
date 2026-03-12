#pragma once
#include <iostream>
#include <string>
#include <filesystem>

#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/filesys.h>

namespace FsUtils {
	namespace fs = std::filesystem;

	void RemoveFilesAndFolder(const fs::path& folderPath);
	bool RemoveDirectoryRecursively(const wxString& dirPath);
	std::wstring StringToWString(const std::string& str);
	bool FolderExists(const fs::path& folderPath);
	void CreateDirectoryWithCheck(const fs::path& dirPath);
}
