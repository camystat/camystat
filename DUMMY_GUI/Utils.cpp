#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <matplotlibcpp.h>
#include <chrono>
#include <numeric>

namespace plt = matplotlibcpp;

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
/// Plots the given vector of values with the specified name.
/// </summary>
/// <param name="values">The vector of values to plot</param>
/// <param name="name">The name of the plot</param>
/// <param name="min">The minimum index to plot</param>
/// <param name="max">The maximum index to plot</param>
void Utils::plotVector(const std::vector<double>& values, const std::string& name, size_t min, size_t max) {
	if (max == 0 || max > values.size()) {
		max = values.size();
	}

	if (min >= max) {
		std::cerr << "Invalid range: min should be < max and within the bounds of the vector size." << std::endl;
		return;
	}

	std::vector<size_t> indices(max - min);
	std::vector<double> subset_values(max - min);

	for (size_t i = min; i < max; ++i) {
		indices[i - min] = i;
		subset_values[i - min] = values[i];
	}

	plt::plot(indices, subset_values);
	plt::title(name);
	plt::show();
	// plt::savefig(name);
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
	cv::Mat heatmap = V3::Preprocessing::createHeatmap(videoPath, 0, -1, 4, heatmapPath);

	std::string imagePath = ""; //mock

	// Find max sum square coordinates
	double squarePercent = 30.0;  // Example percentage
	double topPercent = 30.0;  // Example percentage
	std::vector<std::pair<int, int>> maxSumCoords = V3::Preprocessing::findMaxSumSquareCoordinatesWithPercent(
		heatmap, squarePercent, topPercent, coordPath, imagePath
	);

	// Count ones in XOR at coordinates
	std::vector<double> onesCountOverTime = V3::Preprocessing::countOnesInXorAtCoordinates(videoPath, maxSumCoords, 100, resultPath);
	return onesCountOverTime;
	//plotVector(onesCountOverTime, "");
}

/// <summary>
/// Plots events on a line plot with the given <paramref name="values"/> and <paramref name="events"/>
/// </summary>
/// <param name="values">The values to be plotted</param>
/// <param name="events">The events to be plotted</param>
void Utils::plot_events(const std::vector<double>& values, const std::vector<std::vector<double>>& events) {
	std::vector<double> normalized_values = V3::Smoothing::clone_normalized_values(values);

	plt::figure();

	std::vector<int> x_values(normalized_values.size());
	std::iota(x_values.begin(), x_values.end(), 0);

	plt::plot(x_values, normalized_values, "b-");
	plt::named_plot("Values", x_values, normalized_values);

	std::vector<double> event_values;
	for (const auto& event : events) {
		event_values.push_back(event[1]);
	}
	std::vector<double> normalized_event_values = V3::Smoothing::clone_normalized_values(event_values);

	for (size_t idx = 0; idx < events.size(); ++idx) {
		const auto& event = events[idx];
		double ordinal_number = event[0];
		double value1 = event[1];
		double time1 = event[2];
		double time2 = event[3];
		double event_time = (time1 + time2) / 2;

		plt::plot(std::vector<double>{event_time}, std::vector<double>{normalized_event_values[idx]}, "ro");

		std::vector<double> x_shade = { time1, time1, time2, time2 };
		std::vector<double> y_shade = { 0, 1, 1, 0 };
		plt::fill(x_shade, y_shade, { {"color", "red"}, {"alpha", "0.3"} });
	}

	plt::title("Line Plot with Events");
	plt::xlabel("Time");
	plt::ylabel("Normalized Values");
	plt::legend();

	plt::show();
}

/// <summary>
/// Writes the given vector of vectors <paramref name="data"/> to a file with the specified <paramref name="filename"/>
/// </summary>
/// <param name="data">The data to be written to the file</param>
/// <param name="filename">The name of the file to write the data to</param>
void Utils::write_vector_to_file(const std::vector<std::vector<double>>& data, const std::string& filename) {
	std::ofstream outfile(filename);

	if (!outfile.is_open()) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		return;
	}

	outfile << "[";

	for (size_t i = 0; i < data.size(); ++i) {
		outfile << "[";
		for (size_t j = 0; j < data[i].size(); ++j) {
			outfile << data[i][j];
			if (j < data[i].size() - 1) {
				outfile << ", ";
			}
		}
		outfile << "]";
		if (i < data.size() - 1) {
			outfile << ",\n ";
		}
	}

	outfile << "]";
	outfile.close();
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
