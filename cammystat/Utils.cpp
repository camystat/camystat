#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <numeric>

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
