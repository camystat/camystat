#include "FsUtils.h"

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

bool FsUtils::FolderExists(const std::wstring& folderPath) {
	DWORD fileAttributes = GetFileAttributes(folderPath.c_str());

	if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
		// The folder does not exist if GetFileAttributes returns INVALID_FILE_ATTRIBUTES
		return false;
	}

	// Check if the path is a directory
	return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
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
