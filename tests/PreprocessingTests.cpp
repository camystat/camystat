#include <gtest/gtest.h>
#include "Camystat.h"
#include "TestVideo.h"

using Camystat::Preprocessing;

TEST(ParseBinarizationThreshold, AcceptsIntegersInRange)
{
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("0"), 0);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("158"), 158);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("255"), 255);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("  42 "), 42);
}

TEST(ParseBinarizationThreshold, RejectsEmptyInput)
{
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold(""), std::nullopt);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("   "), std::nullopt);
}

TEST(ParseBinarizationThreshold, RejectsOutOfRange)
{
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("256"), std::nullopt);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("999"), std::nullopt);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("-1"), std::nullopt);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("99999999999"), std::nullopt);
}

TEST(ParseBinarizationThreshold, RejectsNonIntegers)
{
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("12a"), std::nullopt);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("1.5"), std::nullopt);
	EXPECT_EQ(Preprocessing::parseBinarizationThreshold("abc"), std::nullopt);
}

TEST(CountOnesInXor, CountsChangedPixelsBetweenFrames)
{
	TempDir dir;
	const auto video = dir.path() / "video.avi";
	writeGreyVideo(video, { 0, 255, 255, 0 });

	auto ones = Preprocessing::countOnesInXorAtCoordinates(video, {}, 128, dir.path() / "result.csv");

	ASSERT_EQ(ones.size(), 3u);
	EXPECT_DOUBLE_EQ(ones[0], 100.0);
	EXPECT_DOUBLE_EQ(ones[1], 0.0);
	EXPECT_DOUBLE_EQ(ones[2], 100.0);
}

TEST(CountOnesInXor, ThrowsWhenAborted)
{
	TempDir dir;
	const auto video = dir.path() / "video.avi";
	writeGreyVideo(video, { 0, 255, 0, 255 });

	std::atomic<bool> abort{ true };

	EXPECT_THROW(Preprocessing::countOnesInXorAtCoordinates(video, {}, 128, dir.path() / "result.csv", abort), Camystat::ProcessingAbortedException);
}

namespace
{
	std::vector<std::pair<int, int>> findFocusCells(const cv::Mat& heatmap, double squarePercent, double topPercent, const TempDir& dir)
	{
		const auto imagePath = dir.path() / "heatmap.png";
		cv::imwrite(imagePath.string(), cv::Mat(heatmap.size(), CV_8UC3, cv::Scalar(0, 0, 0)));

		return Preprocessing::findMaxSumSquareCoordinatesWithPercent(heatmap, squarePercent, topPercent, dir.path() / "overlay.png", imagePath);
	}
}

TEST(FindMaxSumSquare, SelectsTopCellsOfHottestSquare)
{
	TempDir dir;
	cv::Mat heatmap = cv::Mat::zeros(10, 10, CV_32SC1);
	heatmap.at<int>(6, 7) = 100;
	heatmap.at<int>(6, 8) = 90;
	heatmap.at<int>(7, 7) = 80;
	heatmap.at<int>(1, 1) = 50;

	// 20% of 10 px -> 2x2 square; 50% of its 4 cells -> 2 cells, as (x, y)
	auto cells = findFocusCells(heatmap, 20, 50, dir);

	EXPECT_EQ(cells, (std::vector<std::pair<int, int>>{ { 7, 6 }, { 8, 6 } }));
}

TEST(FindMaxSumSquare, TinySquareStillSelectsACell)
{
	TempDir dir;
	cv::Mat heatmap = cv::Mat::zeros(50, 80, CV_32SC1);
	heatmap.at<int>(20, 30) = 255;

	// 1% of 50 px rounds down to a 0 px square
	auto cells = findFocusCells(heatmap, 1, 90, dir);

	EXPECT_EQ(cells, (std::vector<std::pair<int, int>>{ { 30, 20 } }));
}

TEST(FindMaxSumSquare, LowTopPercentStillSelectsACell)
{
	TempDir dir;
	cv::Mat heatmap = cv::Mat::zeros(10, 10, CV_32SC1);
	heatmap.at<int>(4, 5) = 255;

	// 10% of a 2x2 square rounds down to 0 cells
	auto cells = findFocusCells(heatmap, 20, 10, dir);

	ASSERT_EQ(cells.size(), 1u);
	EXPECT_EQ(cells[0], (std::pair<int, int>{ 5, 4 }));
}

namespace
{
	std::vector<std::optional<double>> collectBrightnessDiffProgress(const std::filesystem::path& video, int startFrame, int endFrame)
	{
		std::vector<std::optional<double>> progress;
		Preprocessing::calculateBinarizationThreshold(video, startFrame, endFrame,
			[&progress](Preprocessing::BinarizationThresholdCalcProgress stage, std::optional<double> value, std::optional<int>) {
				if (stage == Preprocessing::BinarizationThresholdCalcProgress::FINDING_MAX_BRIGHTNESS_DIFF_FRAMES) {
					progress.push_back(value);
				}
			},
			Camystat::kNoAbort);
		return progress;
	}
}

TEST(CalculateBinarizationThreshold, ProgressIsRelativeToFrameRange)
{
	TempDir dir;
	const auto video = dir.path() / "video.avi";
	writeGreyVideo(video, { 10, 20, 30, 200, 40, 50, 60, 70 });

	auto progress = collectBrightnessDiffProgress(video, 2, 6);

	ASSERT_EQ(progress.size(), 4u);
	for (size_t i = 0; i < progress.size(); ++i) {
		ASSERT_TRUE(progress[i].has_value());
		EXPECT_DOUBLE_EQ(progress[i].value(), (i + 1) / 4.0);
	}
}

TEST(CalculateBinarizationThreshold, NoProgressFractionWithoutEndFrame)
{
	TempDir dir;
	const auto video = dir.path() / "video.avi";
	writeGreyVideo(video, { 10, 20, 200, 40 });

	auto progress = collectBrightnessDiffProgress(video, 0, -1);

	ASSERT_EQ(progress.size(), 3u);
	for (const auto& value : progress) {
		EXPECT_FALSE(value.has_value());
	}
}
