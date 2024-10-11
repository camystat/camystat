#include "V3.h"
#include <wx/wx.h>
#include "wx/setup.h"
#define _CRT_SECURE_NO_WARNINGS
#include <Eigen/Dense>

// Video conversion functionality - DEPRECATED
// THE CONVERSION HAS BEEN MOVED TO A DEDICATED FILE
void V3::Compression::resizeVideo(std::string inputPath, std::string outputPath, double scaleFactor) {
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

    //load next image frames

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

// 1.1 Create heatmap of activity
cv::Mat V3::Preprocessing::createHeatmap(std::string videoPath, int startFrame, int endFrame, int threshold, std::string resultPath) {
    try {
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

        while (true) {

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

        // Zwolnij zasoby

        cap.release();

        if (!pixelCount.empty()) {

            // Znormalizuj mape ciepla

            cv::normalize(pixelCount, pixelCount, 0, 255, cv::NORM_MINMAX);

            pixelCount.convertTo(pixelCount, CV_8UC1);

            // Zaaplikuj palete kolorow do mapy ciepla

            cv::Mat heatmapColor;
            cv::applyColorMap(pixelCount, heatmapColor, cv::COLORMAP_JET);

            // Zapisz mape ciepla

            cv::imwrite(resultPath, heatmapColor);
        }

        return pixelCount;

    }

    // Obsluga bledow

    catch (const std::exception& e) {
        return cv::Mat();
    }
}

// 1.2 Wyznaczanie obszaru zainteresowania

std::vector<std::pair<int, int>> V3::Preprocessing::findMaxSumSquareCoordinatesWithPercent(
    const cv::Mat& pixel_count_array,
    double square_percent,
    double top_percent,
    std::string resultPath,
    std::string imagePath
) {

    // Zwroc pusty wektor jezeli macierz jest pusta
    if (pixel_count_array.empty()) {
        return {};
    }

    int rows = pixel_count_array.rows;
    int cols = pixel_count_array.cols;
    int shorter_edge = std::min(rows, cols);
    int square_size = static_cast<int>(shorter_edge * (square_percent / 100.0));

    int max_sum = std::numeric_limits<int>::min();
    cv::Point max_sum_coords = { -1, -1 };

    // Znajdz koordynaty o maksymalnej sumie

    for (int i = 0; i <= rows - square_size; ++i) {
        for (int j = 0; j <= cols - square_size; ++j) {
            int current_sum = cv::sum(pixel_count_array(cv::Rect(j, i, square_size, square_size)))[0];

            if (current_sum > max_sum) {
                max_sum = current_sum;
                max_sum_coords = { j, i };
            }
        }
    }

    // Zwroc pusty wektor jezeli koordynaty nie zostaly znalezione

    if (max_sum_coords.x == -1 || max_sum_coords.y == -1) {
        return {};
    }

    int start_i = max_sum_coords.y;
    int start_j = max_sum_coords.x;
    int total_cells = square_size * square_size;
    int num_cells_to_choose = static_cast<int>(top_percent / 100.0 * total_cells);

    std::vector<std::pair<int, int>> selected_coordinates;
    std::vector<std::pair<int, int>> values_inside_square;

    // Znajdz wartosci i ich koordynaty w znalezionym kwadracie

    for (int i = start_i; i < start_i + square_size; ++i) {
        for (int j = start_j; j < start_j + square_size; ++j) {
            values_inside_square.push_back({ pixel_count_array.at<int>(i, j), i * cols + j });
        }
    }

    // Posortuj pod katem wartosci

    std::sort(values_inside_square.begin(), values_inside_square.end(),
        [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
            return a.first > b.first;
        });

    // Znajdz najwazniejsze komorki

    for (int i = 0; i < num_cells_to_choose; ++i) {
        int index = values_inside_square[i].second;
        int y = index / cols;
        int x = index % cols;
        selected_coordinates.push_back({ x, y });
    }

    // Wczytaj istniej¹cy obraz PNG
    cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);

    if (image.empty()) {
        std::cerr << "Nie udalo sie wczytac obrazu: " << imagePath << std::endl;
        return {};
    }

    // Rysowanie punktów na obrazie
    for (const auto& coord : selected_coordinates) {
        cv::circle(image, cv::Point(coord.first, coord.second), 3, cv::Scalar(0, 255, 0), -1); // Zielony punkt
    }

    // Zapisz obraz jako PNG z na³o¿onymi punktami
    cv::imwrite(resultPath, image);

    return selected_coordinates;
}

// 1.3 Analiza aktywnoœci na nagraniu

std::vector<double> V3::Preprocessing::countOnesInXorAtCoordinates(
    std::string videoPath,
    std::vector<std::pair<int, int>> coordinates,
    int threshold,
    std::string resultPath
) {
    try {

        // Otworz plik wideo
        std::cout << "countOnesInXorAtCoordinates: rozpoczeto" << std::endl;

        cv::VideoCapture cap(videoPath);

        // Sprawdz czy plik zostal otwarty 

        if (!cap.isOpened()) {
            throw std::runtime_error("countOnesInXorAtCoordinates: blad, nie udalo sie otworzyc pliku.");
        }

        // Zainicjalizuj liste do ktorej beda zapisywane porownania XOR

        std::vector<double> ones_count_over_time;

        // Przeczytaj pierwsza klatke

        cv::Mat prev_frame;
        bool ret = cap.read(prev_frame);

        if (!ret) {
            throw std::runtime_error("countOnesInXorAtCoordinates: blad, nie udalo sie otworzyc ramki.");
        }

        // Przekonwertuj pierwsza klatke do odcieni szarosci

        cv::Mat prev_frame_gray, prev_binary;
        cv::cvtColor(prev_frame, prev_frame_gray, cv::COLOR_BGR2GRAY);
        cv::threshold(prev_frame_gray, prev_binary, threshold, 1, cv::THRESH_BINARY);

        int frame_index = 1;

        // Oblicz konkretna liczbe komorek w macierzy

        int total_cells;
        if (coordinates.empty()) {
            total_cells = prev_binary.total();
        }
        else {
            total_cells = coordinates.size();
        }

        // Przejdz po wszystkich klatkach

        while (true) {
            // Przeczytaj biezaca klatke
            cv::Mat current_frame;
            ret = cap.read(current_frame);

            // Zakoncz petle jezeli dojdziesz do konca filmu

            if (!ret) {
                break;
            }

            // Przekonwertuj biezaca klatke do skali szarosci

            cv::Mat current_frame_gray, current_binary;
            cv::cvtColor(current_frame, current_frame_gray, cv::COLOR_BGR2GRAY);
            cv::threshold(current_frame_gray, current_binary, threshold, 1, cv::THRESH_BINARY);

            // Oblicz roznice XOR pomiedzy klatkami

            cv::Mat xor_result;
            cv::bitwise_xor(prev_binary, current_binary, xor_result);

            int ones_count = 0;
            if (coordinates.empty()) {

                // Jezeli koordynaty sa puste to przeanalizuj tablice XOR

                ones_count = cv::countNonZero(xor_result);
            }
            else {

                // Oblicz liczbe jedynek w tablicy XOR

                for (const auto& coord : coordinates) {
                    ones_count += xor_result.at<uchar>(coord.second, coord.first);
                }
            }

            // Oblicz procent w oparciu o ilosc komorek

            double percentage_count = (static_cast<double>(ones_count) / total_cells) * 100.0;

            ones_count_over_time.push_back(percentage_count);

            // Zaktualizuj biazaca ramke do kolejnej iteracji

            prev_binary = current_binary;

            frame_index++;
        }

        // Zwolnij zasoby

        cap.release();

        // Save the result to a file
        std::ofstream resultFile(resultPath);
        for (const double& count : ones_count_over_time) {
            resultFile << count << "\n";
        }
        resultFile.close();

        return ones_count_over_time;

    }
    // Obsluga bledu

    catch (const std::exception& e) {
        return {};
    }
}

std::vector<double> savgolFilter(std::vector<double> data, int window_length, int polyorder) {
    // Ensure window length is odd and greater than polyorder
    if (window_length % 2 == 0 || window_length <= polyorder) {
        throw std::invalid_argument("Window length must be odd and greater than the polynomial order.");
    }

    int half_window = (window_length - 1) / 2;

    // Create the Vandermonde matrix
    Eigen::MatrixXd A(window_length, polyorder + 1);
    for (int i = -half_window; i <= half_window; ++i) {
        for (int j = 0; j <= polyorder; ++j) {
            A(i + half_window, j) = std::pow(i, j);
        }
    }

    // Compute the pseudoinverse of A
    Eigen::MatrixXd AtA = A.transpose() * A;
    Eigen::MatrixXd AtA_inv = AtA.inverse();
    Eigen::MatrixXd pseudoInv = AtA_inv * A.transpose();

    // Compute the filter coefficients (middle row of the pseudo-inverse for smoothing)
    Eigen::VectorXd coeffs = pseudoInv.row(half_window);

    // Normalize the coefficients so their sum is 1
    coeffs /= coeffs.sum();

    std::vector<double> result(data.size(), 0.0);

    // Apply the filter to the data
    for (size_t i = 0; i < data.size(); ++i) {
        double filtered_value = 0.0;

        // Handle the boundaries by adjusting the window
        for (int j = -half_window; j <= half_window; ++j) {
            int idx = std::clamp<int>(i + j, 0, data.size() - 1);
            filtered_value += coeffs(j + half_window) * data[idx];
        }

        result[i] = filtered_value;
    }

    return result;
}

std::vector<double> V3::Smoothing::applySavgolFilter(std::vector<double> data, int window_length, int polyorder) {
    if (data.size() < window_length) {
        {
            wxMessageDialog dialog(NULL, "Savgol: Window length must be less than or equal to the length of the data.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
            dialog.ShowModal();
        }
        throw std::invalid_argument("Window length must be less than or equal to the length of the data.");
    }
    if (window_length % 2 == 0) {
        {
            wxMessageDialog dialog(NULL, "Savgol: Window length must be odd.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
            dialog.ShowModal();
        }
        throw std::invalid_argument("Window length must be odd.");
    }
    if (polyorder >= window_length) {
        {
            wxMessageDialog dialog(NULL, "Savgol: Polynomial order must be less than the window length.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
            dialog.ShowModal();
        }
        throw std::invalid_argument("Polynomial order must be less than the window length.");
    }

    std::vector<double> filtered_data = savgolFilter(data, window_length, polyorder);

    return filtered_data;
}

std::vector<double> V3::Smoothing::smoothValues(std::vector<double> values, float percentile) {
    if (values.size() < 3) {
        std::cout << "smoothValues: zbyt malo wartosci do wygladzania" << std::endl;
        return values;
    }

    std::vector<double> differences(values.size() - 1);
    for (size_t i = 0; i < values.size() - 1; ++i) {
        differences[i] = std::abs(values[i + 1] - values[i]);
    }

    std::sort(differences.begin(), differences.end());
    size_t index = static_cast<size_t>(differences.size() * percentile / 100.0);
    double threshold = differences[index];

    std::vector<double> smoothedValues = values;
    const size_t maxIterations = 1000;
    size_t iteration = 0;

    while (iteration < maxIterations) {
        bool smoothed = false;

        for (size_t i = 1; i < smoothedValues.size() - 1; ++i) {
            double currentValue = smoothedValues[i];
            double prevValue = smoothedValues[i - 1];
            double nextValue = smoothedValues[i + 1];

            double diff = std::abs(currentValue - prevValue);

            if (diff > threshold) {
                double average = (prevValue + nextValue) / 2.0;
                smoothedValues[i] = average;
                smoothed = true;
            }
        }

        if (!smoothed) {
            break;
        }
        ++iteration;
    }

    return smoothedValues;
}

std::vector<double> V3::Smoothing::modify_means(std::vector<double> input_list, int n, int x) {
    if (n <= 0 || x <= 0) {
        return input_list;
    }

    std::vector<double> current_list = input_list;

    for (int iter = 0; iter < x; ++iter) {
        std::vector<double> modified_list;

        for (size_t i = 0; i < current_list.size(); ++i) {
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

std::vector<double> V3::Smoothing::normalize_values(std::vector<double> values) {
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

std::vector<double> V3::Smoothing::replace_zeros_values_below_threshold(const std::vector<double>& lst, double threshold, std::string resultPath) {
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

std::vector<double> V3::Detection::add_zeros_to_list(const std::vector<double> input_list) {
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

std::vector<std::vector<double>> V3::Detection::calculate_integrals_with_reference_points(std::vector<double> values) {
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
                // If zero occurred after non-zero values, calculate the integral between them
                double integral_value = trapezoidal_rule(values, start_index, i);
                integrals.push_back(integral_value);
                results.push_back({static_cast<double>(integrals.size()), integral_value, static_cast<double>(start_index), static_cast<double>(i)});
            }
            start_index = i + 1; // Move the starting point of the next integral
        }
    }

    return results;
}

std::vector<std::vector<double>> V3::Detection::merge_events(std::vector<std::vector<double>> event_list, double distance_threshold) {
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

std::vector<std::vector<double>> V3::Detection::remove_events(std::vector<std::vector<double>> event_list, double threshold_value) {
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