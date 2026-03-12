#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <numeric>
#include <wx/utils.h>

/// <summary>
/// Creates a trimmed copy of the list <paramref name="lst"/>, by filtering out the first <paramref name="n"/> elements and the last <paramref name="x"/> elements from the input list
/// </summary>
/// <param name="lst">The source list to create a trimmed copy from</param>
/// <param name="n">Number of leading items to be dropped</param>
/// <param name="x">Number of trailing items to be dropped</param>
/// <returns>Trimmed list (new object)</returns>
std::vector<double> Utils::clone_trimmed_list(const std::vector<double>& lst, size_t n, size_t x) {
	if (n + x > lst.size()) {
		throw std::invalid_argument("n + x must be <= list size.");
	}

	return std::vector<double>(lst.begin() + n, lst.end() - x);
}

/// <summary>
/// Calculates the count of ones in the XOR of the video frames at the specified coordinates
/// </summary>
/// <param name="videoPath">Path to video</param>
/// <param name="compressedPath">Path to compressed video</param>
/// <param name="heatmapPath>Path to heatmap</param>
/// <param name="coordPath">Path to coordinates</param>
/// <param name="resultPath">Path to result</param>
/// <returns>Vector of counts over time</returns>
std::vector<double> Utils::aggregate(const std::string& videoPath, const std::string& compressedPath, const std::string& heatmapPath, const std::string& coordPath, const std::string& resultPath) {
	double scaleFactor = 2.0;

	//V2::Compression::resizeVideo(videoPath, compressedPath, scaleFactor);

	// Create heatmap
	cv::Mat heatmap = Cammystat::Preprocessing::createHeatmap(videoPath, 0, -1, 4, heatmapPath);

	std::string imagePath = ""; //mock

	// Find max sum square coordinates
	double squarePercent = 30.0;  // Example percentage
	double topPercent = 30.0;  // Example percentage
	std::vector<std::pair<int, int>> maxSumCoords = Cammystat::Preprocessing::findMaxSumSquareCoordinatesWithPercent(
		heatmap, squarePercent, topPercent, coordPath, imagePath
	);

	// Count ones in XOR at coordinates
	std::vector<double> onesCountOverTime = Cammystat::Preprocessing::countOnesInXorAtCoordinates(videoPath, maxSumCoords, 100, resultPath);
	return onesCountOverTime;
}

#ifdef _WIN32
/// <summary>
/// Converts a TCHAR string to a std::string
/// </summary>
/// <param name="tcharStr">The TCHAR</param>
/// <returns>converted std::string</returns>
std::string Utils::TCHARToString(const TCHAR* tcharStr) {
#ifdef UNICODE
	// Convert from wchar_t to std::string (UTF-8)
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, &strTo[0], size_needed, NULL, NULL);
	return strTo;
#else
	// Convert from char to std::string (no conversion needed)
	return std::string(tcharStr);
#endif
}
#endif

void Utils::callPlotExe(
	const std::string& exePath,
	const std::string& subCommand,
	const std::string& flags
)
{
	std::string command = "\"" + exePath + "\" \"" + subCommand + "\" " + flags;
	std::cout << "Command: " << command << std::endl;

	// Display command using wxMessageDialog
	//wxMessageDialog dialog(NULL, command, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
	//dialog.ShowModal();

#ifdef _WIN32
	// Convert std::string to std::wstring for WinAPI functions
	std::wstring commandW = Utils::stringToWString(command);

	// Method: Using CreateProcess with hidden window
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESHOWWINDOW;  // Use the wShowWindow member
	si.wShowWindow = SW_HIDE;            // Set wShowWindow to hide the window

	ZeroMemory(&pi, sizeof(pi));
	// Prepare the command as a writable wide char array
	wchar_t commandWCStr[2048];
	wcscpy_s(commandWCStr, commandW.c_str());
	bool resultCreateProcess = CreateProcess(NULL,           // No module name (use command line)
		commandWCStr,   // Command line (wide char)
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		CREATE_NO_WINDOW, // Hide the window
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi);           // Pointer to PROCESS_INFORMATION structure

	if (resultCreateProcess) {
		// Wait for the process to finish
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exitCode;
		GetExitCodeProcess(pi.hProcess, &exitCode);

		// Log CreateProcess result code
		std::cout << "CreateProcess Exit Code: " << exitCode << std::endl;

		if (exitCode != 0) {
			wxMessageDialog dialog(NULL, "Plotting script finished with a non-zero exit code!.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}

		// Close process and thread handles
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		// Log CreateProcess error
		DWORD error = GetLastError();
		std::cout << "CreateProcess Failed with Error Code: " << error << std::endl;

		wxMessageDialog dialog(NULL, "Failed to execute plotting script using CreateProcess.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
	}
#else
	long exitCode = wxExecute(command, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
	if (exitCode != 0) {
		wxMessageDialog dialog(NULL, "Plotting script finished with a non-zero exit code!.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
	}
#endif

	// Save command to a file
	std::string filePath = "command.txt";
	std::ofstream outFile(filePath);
	if (outFile.is_open()) {
		outFile << command;
		outFile.close();
	}
}

std::vector<std::vector<double>> Utils::normalizeSecondColumnInCopy(const std::vector<std::vector<double>>& results) {
	std::vector<std::vector<double>> modifiedResults = results;

	if (results.empty()) return modifiedResults; // Return empty if no results

	// Extract the second column values from the original data
	std::vector<double> secondColumn;
	for (const auto& row : results) {
		if (row.size() >= 2) { // Ensure the row has at least two elements
			secondColumn.push_back(row[1]);
		}
	}

	if (secondColumn.empty()) return modifiedResults; // Return empty if second column is empty

	// Find the minimum and maximum values in the second column
	double minVal = *std::min_element(secondColumn.begin(), secondColumn.end());
	double maxVal = *std::max_element(secondColumn.begin(), secondColumn.end());

	if (minVal == maxVal) return modifiedResults; // Handle case where all values are the same

	// Normalize the second column values in the copy of the data
	for (auto& row : modifiedResults) {
		if (row.size() >= 2) { // Ensure the row has at least two elements
			row[1] = (row[1] - minVal) / (maxVal - minVal);
		}
	}

	return modifiedResults;
}
