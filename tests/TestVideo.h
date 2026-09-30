#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <gtest/gtest.h>

// Creates a temporary directory for a test and removes it afterwards
class TempDir
{
public:
	TempDir()
	{
		const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
		path_ = std::filesystem::temp_directory_path() / ("camystat_tests_" + std::string(info->test_suite_name()) + "_" + info->name());
		std::filesystem::remove_all(path_);
		std::filesystem::create_directories(path_);
	}

	~TempDir()
	{
		std::error_code ec;
		std::filesystem::remove_all(path_, ec);
	}

	const std::filesystem::path& path() const { return path_; }

private:
	std::filesystem::path path_;
};

// Writes a lossless-enough (MJPG) video whose frames are uniformly filled with the given grey levels
inline void writeGreyVideo(const std::filesystem::path& path, const std::vector<int>& greyLevels, cv::Size size = cv::Size(32, 32))
{
	cv::VideoWriter writer(path.string(), cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, size, true);
	ASSERT_TRUE(writer.isOpened()) << "could not create test video " << path;

	for (int grey : greyLevels) {
		writer.write(cv::Mat(size, CV_8UC3, cv::Scalar(grey, grey, grey)));
	}
	writer.release();
}
