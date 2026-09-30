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
