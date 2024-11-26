#include "V3.h"

/// <summary>
/// Video conversion functionality - DEPRECATED
/// THE CONVERSION HAS BEEN MOVED TO A DEDICATED FILE
/// </summary>
/// <param name="inputPath">The path to the input video</param>
/// <param name="outputPath">The path to the output video</param>
/// <param name="scaleFactor">The scale factor for resizing</param>
void V3::Compression::resizeVideo(const std::string inputPath, std::string outputPath, double scaleFactor) {
	cv::VideoCapture cap(inputPath);

	// Check whether the video was loaded
	if (!cap.isOpened()) {
		wxMessageDialog dialog(NULL, "resizeVideo (preprocessing): Could not open video. Check whether the path has been specified.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
		return;
	}

	// Load video parameters
	int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

	// Calculate new dimensions
	int new_width = static_cast<int>(frame_width / scaleFactor);
	int new_height = static_cast<int>(frame_height / scaleFactor);

	// Create object for saving new data into .mp4v
	cv::VideoWriter out(outputPath, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 30, cv::Size(new_width, new_height));

	// Check whether the object could been created
	if (!out.isOpened()) {
		wxMessageDialog dialog(NULL, "resizeVideo (preprocessing): nie udalo sie otworzyc obiektu do zapisywania filmu. Sprawdz czy sciezka zostala podana.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		dialog.ShowModal();
		std::cerr << "resizeVideo (preprocessing): nie udalo sie otworzyc obiektu do zapisywania filmu." << std::endl;
		return;
	}

	// Load next image frames
	cv::Mat frame;
	while (true) {
		cap >> frame;

		if (frame.empty()) {
			break;
		}

		// Change the frames dimensions
		cv::Mat resized_frame;
		cv::resize(frame, resized_frame, cv::Size(new_width, new_height), 0, 0, cv::INTER_LINEAR);

		// Save frames to buffer
		out.write(resized_frame);
	}

	// Release the resources
	cap.release();
	out.release();
}

long double calcU8MatAvgBrightness(const cv::Mat& mat, std::map<int, long double>& frameAvgBrightnessCache, int frameIndex){
	auto cacheIt = frameAvgBrightnessCache.find(frameIndex);
	if (cacheIt != frameAvgBrightnessCache.end()) {
		return cacheIt->second;
	}

	long int sum = 0;

	for(int i = 0; i < mat.rows; i++){
		for(int j = 0; j < mat.cols; j++){
			sum += mat.at<uint8_t>(i, j);
		}
	}

	long double average = sum / (long double)(mat.rows * mat.cols);

	frameAvgBrightnessCache.insert({ frameIndex, average });

	return average;
}

/// <summary>
/// Calculates a binarization threshold as per Marcin's algorithm design
/// </summary>
/// <param name="videoPath">The path to the video file</param>
/// <param name="startFrame">The starting frame</param>
/// <param name="endFrame">The ending frame</param>
/// <param name="progressCallback">Callback invoked when progress changes.</param>
/// <param name="abortFlag">Flag that indicates whether to abort processing</param>
/// <returns>The threshold & XOR scores vector for all tested threshold values (0-255).</returns>
std::pair<int, std::vector<int>> V3::Preprocessing::calculateBinarizationThreshold(const std::string& videoPath, const int startFrame, const int endFrame, const V3::Preprocessing::BinarizationThresholdCalcProgressCallback& progressCallback, const std::atomic<bool>& abortFlag) {
	progressCallback(V3::Preprocessing::BinarizationThresholdCalcProgress::STARTING, std::nullopt, std::nullopt);

	// For handling edge case when XOR operation returns 0s for the selected given pair of frames to retry with a next-in-turn pair of frames
	std::set<int> retryFrameIndicesBlacklist;

	// Open the video
	cv::VideoCapture cap(videoPath);

	// Check whether the video has been loaded correctly
	if (!cap.isOpened()) {
		wxMessageDialog dialog1(NULL, "ERROR: (calculateBinarizationThreshold) Could not open a file for calculating binarization threshold ", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
		dialog1.ShowModal();
		throw std::runtime_error("calculateBinarizationThreshold: Could not open a file.");
	}

	// Load the video's dimentions
	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

	cv::Mat currentFrame(height, width, CV_8UC1);
	cv::Mat currentFrameGray(height, width, CV_8UC1);
	cv::Mat prevFrameGray(height, width, CV_8UC1);

	int frameIndex;
	std::optional<int> maxAvgBrightnessDiffStartFrameIdx;
	long double maxAvgBrightnessDiff = -1;
	std::vector<int> xorScores; // stored in a vector for debug & visualization purposes
	xorScores.reserve(256);
	int maxXorThreshold;

	std::optional<int> maybeRetryNumber = std::nullopt;

	std::map<int, long double> frameAvgBrightnessCache;

	cv::Mat frame1Gray(height, width, CV_8UC1), frame2Gray(height, width, CV_8UC1);
	cv::Mat frame1Binary(height, width, CV_8UC1), frame2Binary(height, width, CV_8UC1);
	cv::Mat tempFrame(height, width, CV_8UC1);
	cv::Mat xorResult(height, width, CV_8UC1);

	try
	{
		// Below: retry (including first try) loop
		while (true)
		{
			if (abortFlag) {
				throw V3::ProcessingAbortedException();
			}

			maxAvgBrightnessDiffStartFrameIdx = std::nullopt;
			maxAvgBrightnessDiff = -1;
			xorScores.clear();
			maxXorThreshold = 0;

			cap.set(cv::CAP_PROP_POS_FRAMES, startFrame);

			if (!cap.read(currentFrame)) {
				wxMessageDialog dialog1(NULL, "ERROR: (calculateBinarizationThreshold) Could not open a frame", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
				dialog1.ShowModal();
				throw std::runtime_error("calculateBinarizationThreshold: Could not open a frame" + std::to_string(startFrame));
			}
			cv::cvtColor(currentFrame, prevFrameGray, cv::COLOR_BGR2GRAY);

			long double prevAvgBrightness = calcU8MatAvgBrightness(prevFrameGray, frameAvgBrightnessCache, startFrame);

			frameIndex = startFrame + 1; // since the frame at index 0 had already been read

			// Move through the next frames and find pair of consecutive frames that has the max avg. brightness diff
			while (true)
			{
				if (abortFlag) {
					throw V3::ProcessingAbortedException();
				}

				// Load next frame
				if (!cap.read(currentFrame) || (endFrame != -1 && frameIndex > endFrame)) {
					break;
				}

				progressCallback(V3::Preprocessing::BinarizationThresholdCalcProgress::FINDING_MAX_BRIGHTNESS_DIFF_FRAMES, (double)frameIndex / (double)endFrame, maybeRetryNumber);

				cv::cvtColor(currentFrame, currentFrameGray, cv::COLOR_BGR2GRAY);

				// Calculate current avg. brightness
				long double currentAvgBrightness = calcU8MatAvgBrightness(currentFrameGray, frameAvgBrightnessCache, frameIndex);

				// Store the result if applicable
				long double diff = abs(currentAvgBrightness - prevAvgBrightness);
				if (diff > maxAvgBrightnessDiff && retryFrameIndicesBlacklist.find(frameIndex - 1) == retryFrameIndicesBlacklist.end())
				{
					std::cout << "calculateBinarizationThreshold: Candidate frame pair " << frameIndex - 1 << " & " << frameIndex << " with diff " << diff << std::endl;
					maxAvgBrightnessDiff = diff;
					maxAvgBrightnessDiffStartFrameIdx.emplace(frameIndex - 1);
				}

				// Before advancing iteration, assign 'current' values to 'previous' vars
				prevFrameGray = currentFrameGray;
				prevAvgBrightness = currentAvgBrightness;

				frameIndex++;
			}
			
			if (!maxAvgBrightnessDiffStartFrameIdx.has_value()){
				throw std::runtime_error("calculateBinarizationThreshold: Could not find any consecutive frames with a non-zero brightness difference. Please input the binarization threshold manually.");
			}

			cap.set(cv::CAP_PROP_POS_FRAMES, maxAvgBrightnessDiffStartFrameIdx.value());

			cap.read(tempFrame);
			cv::cvtColor(tempFrame, frame1Gray, cv::COLOR_BGR2GRAY);
			cap.read(tempFrame);
			cv::cvtColor(tempFrame, frame2Gray, cv::COLOR_BGR2GRAY);

			// Find binarization threshold that maximizes the XOR score
			for (int t = 0; t <= 255; t++)
			{
				if (abortFlag) {
					throw V3::ProcessingAbortedException();
				}

				progressCallback(V3::Preprocessing::BinarizationThresholdCalcProgress::CALCULATING_XOR_SCORES, (double)t / 255.0, std::nullopt);
				
				cv::threshold(frame1Gray, frame1Binary, t, 255, cv::THRESH_BINARY);
				cv::threshold(frame2Gray, frame2Binary, t, 255, cv::THRESH_BINARY);
				
				cv::bitwise_xor(frame1Binary, frame2Binary, xorResult);
				
				xorScores.push_back(cv::sum(xorResult)[0]);
			}

			std::vector<int>::iterator maxXorScore = std::max_element(xorScores.begin(), xorScores.end());
			// Since thresholds range from 0-255, the index of the max value is the threshold itself
			maxXorThreshold = std::distance(xorScores.begin(), maxXorScore);

			// Edge case: all binarized frame per pair were identical & all XOR scores are thus 0 -> retry with other frames, blacklist this pair
			if (maxXorThreshold == 0)
			{
				int frameIndex1 = maxAvgBrightnessDiffStartFrameIdx.value(), frameIndex2 = frameIndex1 + 1;

				retryFrameIndicesBlacklist.insert(frameIndex1);

				std::cout << "calculateBinarizationThreshold: (WARNING) calculated binarization threshold using frames " << frameIndex1 << " & " << frameIndex2 << " is 0, retrying with blacklisted start frame " << frameIndex1 << std::endl;
				
				maybeRetryNumber.emplace(maybeRetryNumber.value_or(0) + 1);

				continue;
			}
			else
			{
				// Release resources
				cap.release();

				std::cout << "calculateBinarizationThreshold: calculated binarization threshold is " << maxXorThreshold << std::endl;

				return { maxXorThreshold, xorScores };
			}
		}
	}
	catch (const V3::ProcessingAbortedException& e) {
		cap.release(); // release resources
		throw e; // re-throw the exception
	}
}

/// <summary>
/// 1.1 Create heatmap of activity
/// </summary>
/// <param name="videoPath">The path to the video file</param>
/// <param name="startFrame">The starting frame</param>
/// <param name="endFrame">The ending frame</param>
/// <param name="threshold">The threshold value for binarization</param>
/// <param name="resultPath">The path to the file where the result will be saved</param>
/// <param name="abortFlag">The flag that indicates whether to abort processing</param>
/// <returns>The matrix with pixel counts</returns>
cv::Mat V3::Preprocessing::createHeatmap(const std::string& videoPath, const int startFrame, const int endFrame, const int threshold, const std::string& resultPath, const std::atomic<bool>& abortFlag) {
	// Open the video
	cv::VideoCapture cap(videoPath);

	// Check whether the video has been loaded correctly
	if (!cap.isOpened()) {
		wxMessageDialog dialog1(NULL, "ERROR: (createHeatmap) Could not open a file for heatmap ", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
		dialog1.ShowModal();
		throw std::runtime_error("createHeatmap: Could not open a file.");
	}

	// Load the video's dimentions
	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

	// Initialize matrix
	cv::Mat pixelCount = cv::Mat::zeros(height, width, CV_32SC1);

	// Move on to the next frame
	cap.set(cv::CAP_PROP_POS_FRAMES, startFrame);

	// Load the first frame
	cv::Mat prevFrame, prevFrameGray, prevBinary;
	if (!cap.read(prevFrame)) {
		wxMessageDialog dialog1(NULL, "ERROR: (createHeatmap) Could not open a frame", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR | wxDIALOG_NO_PARENT);
		dialog1.ShowModal();
		throw std::runtime_error("createHeatmap: Could not open a frame" + std::to_string(startFrame));
	}

	// Convert a frame to a grayscale
	cv::cvtColor(prevFrame, prevFrameGray, cv::COLOR_BGR2GRAY);
	cv::threshold(prevFrameGray, prevBinary, threshold, 1, cv::THRESH_BINARY);

	int frameIndex = startFrame + 1;

	// Move through the next frames
	try
	{
		while (true) {
			if (abortFlag) {
				throw V3::ProcessingAbortedException();
			}

			// Load next frames
			cv::Mat currentFrame, currentFrameGray, currentBinary;
			if (!cap.read(currentFrame) || (endFrame != -1 && frameIndex > endFrame)) {
				break;
			}

			// Convert a given frame
			cv::cvtColor(currentFrame, currentFrameGray, cv::COLOR_BGR2GRAY);
			cv::threshold(currentFrameGray, currentBinary, threshold, 1, cv::THRESH_BINARY);

			// Do the XOR operation over the frames
			cv::Mat xorResult;
			cv::bitwise_xor(prevBinary, currentBinary, xorResult);

			// Update the pixel matrix
			pixelCount += xorResult;

			// Save the pixel matrix before new iteration starts
			prevBinary = currentBinary;

			frameIndex++;
		}
	}
	catch (const V3::ProcessingAbortedException& e) {
		cap.release(); // release resources
		throw e; // re-throw the exception
	}

	// Release resources
	cap.release();

	if (!pixelCount.empty()) {
		// Normalize the heatmap
		cv::normalize(pixelCount, pixelCount, 0, 255, cv::NORM_MINMAX);

		cv::Mat pixelCountU8;

		pixelCount.convertTo(pixelCountU8, CV_8UC1);

		if (abortFlag) {
			throw V3::ProcessingAbortedException();
		}

		// Apply color palette to the heatmap
		cv::Mat heatmapColor;
		cv::applyColorMap(pixelCountU8, heatmapColor, cv::COLORMAP_JET);

		// Save the heatmap
		cv::imwrite(resultPath, heatmapColor);
	}

	return pixelCount;
}

/// <summary>
/// 1.2 Find the region of interest
/// </summary>
/// <param name="pixel_count_array">The matrix with pixel counts</param>
/// <param name="square_percent">The percentage of the shorter edge to use for the square size</param>
/// <param name="top_percent">The percentage of the top cells to choose</param>
/// <param name="resultPath">The path to the file where the result will be saved</param>
/// <param name="imagePath">The path to the image file</param>
/// <param name="abortFlag">The flag that indicates whether to abort processing</param>
/// <returns>The coordinates of the selected cells</returns>
std::vector<std::pair<int, int>> V3::Preprocessing::findMaxSumSquareCoordinatesWithPercent(
	const cv::Mat& pixel_count_array,
	const double square_percent,
	const double top_percent,
	const std::string& resultPath,
	const std::string& imagePath,
	const std::atomic<bool>& abortFlag
) {
	// Return an empty vector if the matrix is empty
	if (pixel_count_array.empty()) {
		return {};
	}

	int rows = pixel_count_array.rows;
	int cols = pixel_count_array.cols;
	int shorter_edge = std::min(rows, cols);
	int square_size = static_cast<int>(shorter_edge * (square_percent / 100.0));

	int max_sum = std::numeric_limits<int>::min();
	cv::Point max_sum_coords = { -1, -1 };

	// Find coordinates with the maximum sum
	for (int i = 0; i <= rows - square_size; ++i) {
		for (int j = 0; j <= cols - square_size; ++j) {
			if (abortFlag) {
				throw V3::ProcessingAbortedException();
			}

			int current_sum = cv::sum(pixel_count_array(cv::Rect(j, i, square_size, square_size)))[0];

			if (current_sum > max_sum) {
				max_sum = current_sum;
				max_sum_coords = { j, i };
			}
		}
	}

	// Return an empty vector if the coordinates were not found
	if (max_sum_coords.x == -1 || max_sum_coords.y == -1) {
		return {};
	}

	int start_i = max_sum_coords.y;
	int start_j = max_sum_coords.x;
	int total_cells = square_size * square_size;
	int num_cells_to_choose = static_cast<int>(top_percent / 100.0 * total_cells);

	std::vector<std::pair<int, int>> selected_coordinates;
	std::vector<std::pair<int, int>> values_inside_square;

	// Find values and their coordinates in the found square
	for (int i = start_i; i < start_i + square_size; ++i) {
		for (int j = start_j; j < start_j + square_size; ++j) {
 			values_inside_square.push_back({ pixel_count_array.at<int>(i, j), i * cols + j });
		}
	}

	if (abortFlag) {
		throw V3::ProcessingAbortedException();
	}

	// Sort by values
	std::sort(values_inside_square.begin(), values_inside_square.end(),
		[](const std::pair<int, int>& a, const std::pair<int, int>& b) {
			return a.first > b.first;
		});

	if (abortFlag) {
		throw V3::ProcessingAbortedException();
	}

	// Find the most important cells
	for (int i = 0; i < num_cells_to_choose; ++i) {
		int index = values_inside_square[i].second;
		int y = index / cols;
		int x = index % cols;
		selected_coordinates.push_back({ x, y });
	}

	if (abortFlag) {
		throw V3::ProcessingAbortedException();
	}

	// Read the existing PNG image
	cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);

	if (image.empty()) {
		std::cerr << "Nie udalo sie wczytac obrazu: " << imagePath << std::endl;
		return {};
	}

	// Draw points on the image
	for (const auto& coord : selected_coordinates) {
		image.at<cv::Vec3b>(cv::Point(coord.first, coord.second)) = cv::Vec3b(0, 0, 255); // Red single-pixel marker
	}

	// Save the image as PNG with the points overlaid
	cv::imwrite(resultPath, image);

	return selected_coordinates;
}

/// <summary>
/// 1.3 Analysis of activity in the recording
/// </summary>
/// <param name="videoPath">The path to the video file</param>
/// <param name="coordinates">The coordinates of the cells to analyze</param>
/// <param name="threshold">The threshold value for binarization</param>
/// <param name="resultPath">The path to the file where the results will be saved</param>
/// <param name="abortFlag">The flag that indicates whether to abort processing</param>
/// <returns>The percentage of ones in the XOR matrix over time</returns>
std::vector<double> V3::Preprocessing::countOnesInXorAtCoordinates(
	const std::string& videoPath,
	const std::vector<std::pair<int, int>>& coordinates,
	const int threshold,
	const std::string& resultPath,
	const std::atomic<bool>& abortFlag
) {
	// Open the video file
	std::cout << "countOnesInXorAtCoordinates: started" << std::endl;

	cv::VideoCapture cap(videoPath);

	// Check whether the video has been loaded correctly
	if (!cap.isOpened()) {
		throw std::runtime_error("countOnesInXorAtCoordinates: error, could not open file.");
	}

	// Initialize a list for saving XOR comparisons to
	std::vector<double> ones_count_over_time;

	// Read the first frame
	cv::Mat prev_frame;
	bool ret = cap.read(prev_frame);

	if (!ret) {
		throw std::runtime_error("countOnesInXorAtCoordinates: error, could not read frame.");
	}

	// Convert the first frame to grayscale
	cv::Mat prev_frame_gray, prev_binary;
	cv::cvtColor(prev_frame, prev_frame_gray, cv::COLOR_BGR2GRAY);
	cv::threshold(prev_frame_gray, prev_binary, threshold, 1, cv::THRESH_BINARY);

	int frame_index = 1;

	// Calculate the exact number of cells in the matrix
	int total_cells;
	if (coordinates.empty()) {
		total_cells = prev_binary.total();
	}
	else {
		total_cells = coordinates.size();
	}

	// Load the video's dimentions
	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

	cv::Mat current_frame(height, width, CV_8UC1);
	cv::Mat current_frame_gray(height, width, CV_8UC1), current_binary(height, width, CV_8UC1);
	cv::Mat xor_result(height, width, CV_8UC1);

	try
	{
		// Loop through all frames
		while (true) {
			if (abortFlag) {
				throw V3::ProcessingAbortedException();
			}

			// Read the current frame
			ret = cap.read(current_frame);

			// Break the loop if end of video is reached
			if (!ret) {
				break;
			}

			// Convert the current frame to grayscale
			cv::cvtColor(current_frame, current_frame_gray, cv::COLOR_BGR2GRAY);
			cv::threshold(current_frame_gray, current_binary, threshold, 1, cv::THRESH_BINARY);

			// Calculate the XOR difference between frames
			cv::bitwise_xor(prev_binary, current_binary, xor_result);

			if (abortFlag) {
				throw V3::ProcessingAbortedException();
			}

			int ones_count = 0;
			if (coordinates.empty()) {

				// If the coordinates are empty, analyze the XOR matrix
				ones_count = cv::countNonZero(xor_result);
			}
			else {
				// Calculate the number of ones in the XOR matrix
				for (const auto& coord : coordinates) {
					ones_count += xor_result.at<uchar>(coord.second, coord.first);
				}
			}

			// Calculate the percentage based on the number of cells
			double percentage_count = (static_cast<double>(ones_count) / total_cells) * 100.0;

			ones_count_over_time.push_back(percentage_count);

			// Update the current frame for the next iteration
			prev_binary = current_binary;

			frame_index++;
		}
	}
	catch (const V3::ProcessingAbortedException& e) {
		cap.release(); // release resources
		throw e; // re-throw the exception
	}

	// Release resources
	cap.release();

	// Save the result to a file
	std::ofstream resultFile(resultPath);
	for (const double& count : ones_count_over_time) {
		resultFile << count << "\n";
	}
	resultFile.close();

	return ones_count_over_time;
}

/// <summary>
/// 
/// </summary>
/// <param name="input_list"></param>
/// <param name="n"></param>
/// <param name="x"></param>
/// <param name="abortFlag">Flag that indicates whether to abort processing</param>
/// <returns></returns>
std::vector<double> V3::Smoothing::modifyMeans(const std::vector<double>& input_list, size_t n, size_t x, const std::atomic<bool>& abortFlag) {
	if (n <= 0 || x <= 0) {
		return input_list;
	}

	std::vector<double> current_list = input_list;

	for (int iter = 0; iter < x; ++iter) {		
		if (abortFlag) {
			throw V3::ProcessingAbortedException();
		}

		std::vector<double> modified_list;

		for (size_t i = 0; i < current_list.size(); ++i) {
			if (abortFlag) {
				throw V3::ProcessingAbortedException();
			}

			double mean_value = 0.0;

			if (i < n / 2) {
				mean_value = std::accumulate(current_list.begin() + i, current_list.begin() + std::min(i + n, current_list.size()), 0.0) / n;
			}
			else if (i >= current_list.size() - n / 2) {
				mean_value = std::accumulate(current_list.begin() + std::max(i - n + 1, size_t(0)), current_list.begin() + i + 1, 0.0) / n;
			}
			else {
				mean_value = std::accumulate(current_list.begin() + i - n / 2, current_list.begin() + i + n / 2 + 1, 0.0) / n;
			}

			modified_list.push_back(mean_value);
		}

		current_list = modified_list;
	}
	return current_list;
}

/// <summary>
/// Creates a copy of the given vector of values <paramref name="values"/> with values normalized using the min-max feature scaling formula
/// </summary>
/// <param name="values">The vector of values to create a copy of with normalized values from</param>
/// <returns>Vector of normalized values (new object)</returns>
std::vector<double> V3::Smoothing::cloneNormalizedValues(const std::vector<double>& values) {
	if (values.empty()) {
		return {};
	}

	double min_value = *std::min_element(values.begin(), values.end());
	double max_value = *std::max_element(values.begin(), values.end());

	std::vector<double> normalized_values;
	normalized_values.reserve(values.size());

	for (const auto& x : values) {
		normalized_values.push_back((x - min_value) / (max_value - min_value));
	}

	return normalized_values;
}

/// <summary>
/// Replaces values below the specified threshold with zeros in the given list and saves the result to a file
/// </summary>
/// <param name="lst">The list of values to be modified</param>
/// <param name="threshold">The threshold value</param>
/// <param name="resultPath">The path to the file where the result will be saved</param>
/// <returns>The modified list of values (new object)</returns>
std::vector<double> V3::Smoothing::cloneReplaceZerosValuesBelowThreshold(const std::vector<double>& lst, double threshold, std::string resultPath) {
	std::vector<double> modified_values;
	modified_values.reserve(lst.size());

	for (const auto& value : lst) {
		if (value < threshold) {
			modified_values.push_back(0);
		}
		else {
			modified_values.push_back(value);
		}
	}

	// Save the result to a file
	std::ofstream resultFile(resultPath);
	for (const double& count : modified_values) {
		resultFile << count << "\n";
	}
	resultFile.close();

	return modified_values;
}

/// <summary>
/// Pads the list with zeros at the beginning and end; if the list is empty, a single zero is appended
/// </summary>
/// <param name="input_list"></param>
/// <returns></returns>
std::vector<double> V3::Detection::clone_padded_with_zeros(const std::vector<double>& input_list) {
	if (input_list.empty()) {
		return { 0 };
	}

	std::vector<double> modified_list;
	modified_list.reserve(input_list.size() + 2);

	modified_list.push_back(0);
	modified_list.insert(modified_list.end(), input_list.begin(), input_list.end());
	modified_list.push_back(0);

	return modified_list;
}

/// <summary>
/// Calculates the integrals of the given values with reference points at zero values
/// </summary>
/// <param name="values">The list of values to calculate the integrals for</param>
/// <param name="abortFlag">The flag that indicates whether to abort processing</param>
/// <returns>The list of integrals with reference points (new object)</returns>
std::vector<std::vector<double>> V3::Detection::calculate_integrals_with_reference_points(const std::vector<double>& values, const std::atomic<bool>& abortFlag) {
	std::vector<double> integrals; // List to store calculated integrals
	std::vector<std::vector<double>> results; // List to store results in the specified format

	int start_index = 0; // Index of the initial reference point for the integral

	// Function to calculate the integral using the trapezoidal rule
	auto trapezoidal_rule = [](const std::vector<double>& vals, int start, int end) -> double {
		double integral_value = 0.0;
		for (int i = start; i < end - 1; ++i) {
			integral_value += (vals[i] + vals[i + 1]) * 0.5; // Trapezoidal rule
		}
		return integral_value;
		};

	for (size_t i = 0; i < values.size(); ++i) {
		if (values[i] == 0) {
			if (i > start_index) {
				if (abortFlag) {
					throw V3::ProcessingAbortedException();
				}

				// If zero occurred after non-zero values, calculate the integral between them
				double integral_value = trapezoidal_rule(values, start_index, i);
				integrals.push_back(integral_value);
				results.push_back({ static_cast<double>(integrals.size()), integral_value, static_cast<double>(start_index), static_cast<double>(i) });
			}
			start_index = i + 1; // Move the starting point of the next integral
		}
	}

	return results;
}

/// <summary>
/// Merges events that are close to each other into a single event
/// </summary>
/// <param name="event_list">The list of events to be merged</param>
/// <param name="distance_threshold">The threshold distance for merging events</param>
/// <returns>The merged list of events (new object)</returns>
std::vector<std::vector<double>> V3::Detection::merge_events(const std::vector<std::vector<double>>& event_list, double distance_threshold) {
	std::vector<std::vector<double>> merged_list;
	size_t i = 0;

	while (i < event_list.size()) {
		if (i == event_list.size() - 1) {
			merged_list.push_back(event_list[i]);
			i += 1;
		}
		else {
			const std::vector<double>& current_event = event_list[i];
			const std::vector<double>& next_event = event_list[i + 1];

			double distance = next_event[2] - current_event[3];

			if (distance <= distance_threshold) {
				std::vector<double> merged_event = { current_event[0], current_event[1] + next_event[1], current_event[2], next_event[3] };
				merged_list.push_back(merged_event);
				i += 2;
			}
			else {
				merged_list.push_back(current_event);
				i += 1;
			}
		}
	}

	for (size_t idx = 0; idx < merged_list.size(); ++idx) {
		merged_list[idx][0] = idx + 1;
	}

	return merged_list;
}

/// <summary>
/// Removes events with values below the specified threshold
/// </summary>
/// <param name="event_list">The list of events</param>
/// <param name="threshold_value">The threshold value for removing events</param>
/// <returns>The updated list of events (new object)</returns>
std::vector<std::vector<double>> V3::Detection::remove_events(const std::vector<std::vector<double>>& event_list, double threshold_value) {
	std::vector<std::vector<double>> updated_list;

	for (const auto& event : event_list) {
		if (event[1] >= threshold_value) {
			updated_list.push_back(event);
		}
	}

	for (size_t idx = 0; idx < updated_list.size(); ++idx) {
		updated_list[idx][0] = idx + 1;
	}

	return updated_list;
}

std::vector<V3::Detection::Phase> V3::Detection::locate_contractions_and_relaxations(const std::vector<double>& values, const std::vector<std::vector<double>>& integrals_results, const std::atomic<bool>& abortFlag)
{
	std::vector<Phase> results;

	for (const auto& contraction : integrals_results) {
		if (abortFlag) {
			throw V3::ProcessingAbortedException();
		}

		int start_index = contraction[2], end_index = contraction[3];

		if (end_index - start_index <= 2) {
			continue; // Not enough points to find a local minimum
		}

		// Extract segment of values for the current contraction
		std::vector<double> segment(values.begin() + start_index, values.begin() + end_index);
		std::vector<double> inner_segment(segment.begin() + 1, segment.end() - 1);

		// Find indices of local minima in the inner segment
		std::vector<int> local_minima;
		for (size_t i = 1; i < inner_segment.size() - 1; ++i) {
			if (inner_segment[i - 1] > inner_segment[i] && inner_segment[i] < inner_segment[i + 1]) {
				local_minima.push_back(i);
			}
		}

		// Skip if there's not exactly one local minimum
		if (local_minima.size() != 1) {
			continue;
		}

		// Calculate absolute index of the minimum
		int min_index_relative = local_minima[0];
		int min_index_absolute = start_index + 1 + min_index_relative;

		// Split the segment into contraction and relaxation phases
		std::vector<double> contraction_segment(values.begin() + start_index, values.begin() + min_index_absolute + 1);
		std::vector<double> relaxation_segment(values.begin() + min_index_absolute, values.begin() + end_index);

		// Calculate integrals (approximated using trapezoidal rule)
		auto trapezoidal_integral = [](const std::vector<double>& data) {
			double integral = 0.0;
			for (size_t i = 0; i < data.size() - 1; ++i) {
				integral += 0.5 * (data[i] + data[i + 1]);
			}
			return integral;
			};

		double contraction_value = trapezoidal_integral(contraction_segment);
		double relaxation_value = trapezoidal_integral(relaxation_segment);

		// Append results
		results.push_back({ "contraction", static_cast<int>(results.size()) + 1, contraction_value, start_index, min_index_absolute });
		results.push_back({ "relaxation", static_cast<int>(results.size()) + 1, relaxation_value, min_index_absolute, end_index });
	}

	return results;
}
