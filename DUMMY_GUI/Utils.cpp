#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <matplotlibcpp.h>
#include <chrono>
#include <numeric>

namespace plt = matplotlibcpp;

std::vector<double> Utils::trim_list(const std::vector<double> lst, int n, int x) {
    if (n < 0 || x < 0) {
        throw std::invalid_argument("n and x must be non-negative integers.");
    }

    if (n + x > static_cast<int>(lst.size())) {
        throw std::invalid_argument("n and x must not exceed the list size.");
    }

    std::vector<double> trimmed_list(lst.begin() + n, lst.end() - x);
    return trimmed_list;
}

void Utils::plotVector(const std::vector<double> values, const std::string name, int min, int max) {
    if (max == 0 || max > values.size()) {
        max = values.size();
    }

    if (min < 0) {
        min = 0;
    }

    if (min >= max) {
        std::cerr << "Invalid range: min should be less than max and within the bounds of the vector size." << std::endl;
        return;
    }

    std::vector<int> indices(max - min);
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

std::vector<double> Utils::aggregate(std::string videoPath, std::string compressedPath, std::string heatmapPath, std::string coordPath, std::string resultPath) {
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

std::vector<double> Utils::normalize_values(const std::vector<double>& values) {
    double min_value = *std::min_element(values.begin(), values.end());
    double max_value = *std::max_element(values.begin(), values.end());

    std::vector<double> normalized_values(values.size());
    std::transform(values.begin(), values.end(), normalized_values.begin(),
        [min_value, max_value](double x) {
            return (x - min_value) / (max_value - min_value);
        });

    return normalized_values;
}

void Utils::plot_events(const std::vector<double>& values, const std::vector<std::vector<double>>& events) {
    std::vector<double> normalized_values = normalize_values(values);

    plt::figure();

    std::vector<int> x_values(normalized_values.size());
    std::iota(x_values.begin(), x_values.end(), 0);

    plt::plot(x_values, normalized_values, "b-");
    plt::named_plot("Values", x_values, normalized_values);

    std::vector<double> event_values;
    for (const auto& event : events) {
        event_values.push_back(event[1]);
    }
    std::vector<double> normalized_event_values = normalize_values(event_values);

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
