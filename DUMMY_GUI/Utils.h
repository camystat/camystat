#pragma once
#include <vector>
#include <iostream>
#include "V3.h"
#include "MainFrame.h"
#include <wx/wx.h>
#include "wx/setup.h"
#include "Utils.h"
#include <shlobj.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "Savgol.h"
#include <Windows.h>
#include <string>

namespace fs = std::filesystem;

class Utils
{
public:
	static std::vector<double> clone_trimmed_list(const std::vector<double>& lst, size_t n, size_t x);
	static std::vector<double> aggregate(const std::string& videoPath, const std::string& compressedPath, const std::string& heatmapPath, const std::string& coordPath, const std::string& resultPath);
	static void write_vector_to_file(const std::vector<std::vector<double>>& data, const std::string& filename);
	static std::string TCHARToString(const TCHAR* tcharStr);

	static std::wstring s2ws(const std::string& s) {
		std::wstring ws(s.begin(), s.end());
		return ws;
	}

	// Helper function to convert std::string to std::wstring
	static std::wstring stringToWString(const std::string& str) {
		return std::wstring(str.begin(), str.end());
	}

	static void callPlotEvents(const std::string& exePath,
		const std::string& valuesPath,
		const std::string& eventsPath,
		const std::string& videoName,
		const std::string& savePath,
		int fps,
		const std::string& pathToRemove) {

		std::string command = "\"" + exePath + "\" \"" + valuesPath + "\" \"" + eventsPath + "\" \"" + videoName + "\" \"" + savePath + "\" " + std::to_string(fps) + " \"" + pathToRemove + "\"";
		std::cout << "Command: " << command << std::endl;

		// Display command using wxMessageDialog
		//wxMessageDialog dialog(NULL, command, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		//dialog.ShowModal();

		// Convert std::string to std::wstring for WinAPI functions
		std::wstring commandW = stringToWString(command);

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

		// Save command to a file
		std::string filePath = "command.txt";
		std::ofstream outFile(filePath);
		if (outFile.is_open()) {
			outFile << command;
			outFile.close();
		}
	}

	static void writeVectorToFile(const std::string& filePath, const std::vector<double>& vec) {
		std::ofstream outFile(filePath);
		if (outFile.is_open()) {
			for (const auto& value : vec) {
				outFile << value << ",\n";
			}
			outFile.close();
		}
		else {
			std::cerr << "Failed to open file for writing: " << filePath << std::endl;
		}
	}

	static void writeVectorOfVectorsToFile(const std::string& filePath, const std::vector<std::vector<double>>& vecOfVecs) {
		std::ofstream outFile(filePath);
		if (outFile.is_open()) {
			for (const auto& vec : vecOfVecs) {
				for (size_t i = 0; i < vec.size(); ++i) {
					outFile << vec[i];
					if (i < vec.size() - 1) {
						outFile << ",";
					}
				}
				outFile << "\n";
			}
			outFile.close();
		}
		else {
			std::cerr << "Failed to open file for writing: " << filePath << std::endl;
		}
	}

	static std::vector<std::vector<double>> normalizeSecondColumnInCopy(const std::vector<std::vector<double>>& results) {
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
};
