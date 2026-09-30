#include <gtest/gtest.h>
#include "Camystat.h"

using Camystat::Smoothing;

namespace
{
	void expectVectorNear(const std::vector<double>& actual, const std::vector<double>& expected, double tolerance = 1e-12)
	{
		ASSERT_EQ(actual.size(), expected.size());
		for (size_t i = 0; i < expected.size(); ++i) {
			EXPECT_NEAR(actual[i], expected[i], tolerance) << "at index " << i;
		}
	}
}

TEST(ModifyMeans, PreservesConstantSignal)
{
	const std::vector<double> flat(50, 5.0);

	for (size_t n : { 2, 3, 4, 5 }) {
		for (size_t repetitions : { 1, 10 }) {
			SCOPED_TRACE("n=" + std::to_string(n) + " x=" + std::to_string(repetitions));
			expectVectorNear(Smoothing::modifyMeans(flat, n, repetitions), flat);
		}
	}
}

TEST(ModifyMeans, EvenWindowAveragesCentredNeighbourhood)
{
	// n = 2 averages i-1..i+1 and divides by the number of samples summed
	expectVectorNear(Smoothing::modifyMeans({ 0, 0, 0, 9, 0, 0, 0 }, 2, 1), { 0, 0, 3, 3, 3, 0, 0 });
}

TEST(ModifyMeans, OddWindowAveragesCentredNeighbourhood)
{
	expectVectorNear(Smoothing::modifyMeans({ 0, 0, 0, 10, 0, 0, 0 }, 5, 1), { 0, 2.5, 2, 2, 2, 2.5, 0 });
}

TEST(ModifyMeans, TruncatesWindowAtEdges)
{
	expectVectorNear(Smoothing::modifyMeans({ 3, 6, 9 }, 2, 1), { 4.5, 6, 7.5 });
}

TEST(ModifyMeans, RepetitionsApplyFilterRepeatedly)
{
	const std::vector<double> input = { 1, 5, 2, 8, 3 };

	expectVectorNear(Smoothing::modifyMeans(input, 2, 2), Smoothing::modifyMeans(Smoothing::modifyMeans(input, 2, 1), 2, 1));
}

TEST(ModifyMeans, HandlesSignalsShorterThanWindow)
{
	expectVectorNear(Smoothing::modifyMeans({ 1 }, 5, 3), { 1 });
	expectVectorNear(Smoothing::modifyMeans({ 1, 2, 3 }, 10, 1), { 2, 2, 2 });
	expectVectorNear(Smoothing::modifyMeans({ 2, 4, 6, 8 }, 4, 1), { 4, 5, 5, 6 });
	EXPECT_TRUE(Smoothing::modifyMeans({}, 2, 10).empty());
}

TEST(ModifyMeans, ZeroWindowOrRepetitionsReturnsInput)
{
	const std::vector<double> input = { 1, 5, 2 };

	expectVectorNear(Smoothing::modifyMeans(input, 0, 3), input);
	expectVectorNear(Smoothing::modifyMeans(input, 3, 0), input);
}

TEST(ModifyMeans, ThrowsWhenAborted)
{
	std::atomic<bool> abort{ true };

	EXPECT_THROW(Smoothing::modifyMeans({ 1, 2, 3 }, 2, 1, abort), Camystat::ProcessingAbortedException);
}

TEST(CloneNormalizedValues, ScalesToUnitRange)
{
	expectVectorNear(Smoothing::cloneNormalizedValues({ 2, 4, 6, 3 }), { 0, 0.5, 1, 0.25 });
}

TEST(CloneNormalizedValues, ConstantSignalBecomesZeros)
{
	expectVectorNear(Smoothing::cloneNormalizedValues({ 2, 2, 2 }), { 0, 0, 0 });
}

TEST(CloneNormalizedValues, EmptyInput)
{
	EXPECT_TRUE(Smoothing::cloneNormalizedValues({}).empty());
}
