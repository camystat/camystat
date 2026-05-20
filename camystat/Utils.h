#pragma once
#include <vector>
#include <iostream>
#include <wx/wx.h>
#include "wx/setup.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <string>
#include "Savgol.h"
#include "Utils.h"
#include "Camystat.h"
#include "MainFrame.h"

#ifdef _WIN32
  #include <Windows.h>
  #include <tchar.h>
#endif

namespace fs = std::filesystem;

class Utils
{
public:
	static std::vector<double> clone_trimmed_list(const std::vector<double>& lst, size_t n, size_t x);
	static std::vector<double> aggregate(const std::string& videoPath, const std::string& compressedPath, const std::string& heatmapPath, const std::string& coordPath, const std::string& resultPath);
#ifdef _WIN32
	static std::string TCHARToString(const TCHAR* tcharStr);
#endif

	static std::wstring s2ws(const std::string& s) {
		return std::wstring(s.begin(), s.end());
	}

	// Helper function to convert std::string to std::wstring
	static std::wstring stringToWString(const std::string& str) {
		return std::wstring(str.begin(), str.end());
	}

	static void callPlotExe(const std::string& exePath, const std::string& subCommand, const std::string& flags);

	static std::vector<std::vector<double>> normalizeSecondColumnInCopy(const std::vector<std::vector<double>>& results);

	template<typename T> void static writeVectorToFile(const std::string& filePath, const std::vector<T>& vec) {
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

	template<typename T> void static writeVectorOfVectorsToFile(const std::string& filePath, const std::vector<std::vector<T>>& vecOfVecs) {
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
};
