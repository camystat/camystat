#include <gtest/gtest.h>
#include "Camystat.h"

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
